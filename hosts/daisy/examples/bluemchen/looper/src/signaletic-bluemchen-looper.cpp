#include "../../../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libsignaletic.h>
#include "../../../../include/looper-view.h"
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter startKnob;
Parameter endKnob;
Parameter speedCV;
Parameter skewCV;
Parameter recordGateCV;

#define SIGNAL_HEAP_SIZE 1024 * 384
char signalMemory[SIGNAL_HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = SIGNAL_HEAP_SIZE,
    .memory = (void*) signalMemory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

#define LOOP_TIME_SECS 60
#define LOOP_LENGTH 48000 * LOOP_TIME_SECS
#define LONG_ENCODER_PRESS 2.0f

float DSY_SDRAM_BSS leftSamples[LOOP_LENGTH];
struct sig_Buffer leftBuffer = {
    .length = LOOP_LENGTH,
    .samples = leftSamples
};
float DSY_SDRAM_BSS rightSamples[LOOP_LENGTH];
struct sig_Buffer rightBuffer = {
    .length = LOOP_LENGTH,
    .samples = rightSamples
};

struct sig_dsp_ConstantValue* smoothCoefficient;
struct sig_dsp_Value* start;
struct sig_dsp_OnePole* startSmoother;
struct sig_dsp_Value* end;
struct sig_dsp_OnePole* endSmoother;
struct sig_dsp_Value* speedIncrement;
struct sig_dsp_Accumulate* speedControl;
struct sig_dsp_Value* speedMod;
struct sig_dsp_OnePole* speedModSmoother;
struct sig_dsp_BinaryOp* speedAdder;
struct sig_dsp_Value* speedSkew;
struct sig_dsp_OnePole* speedSkewSmoother;
struct sig_dsp_Invert* leftSpeedSkewInverter;
struct sig_dsp_BinaryOp* leftSpeedAdder;
struct sig_dsp_BinaryOp* rightSpeedAdder;
struct sig_dsp_Value* encoderButton;
struct sig_dsp_TimedTriggerCounter* encoderTap;
struct sig_dsp_GatedTimer* encoderLongPress;
struct sig_dsp_ToggleGate* recordGate;
struct sig_dsp_Looper* leftLooper;
struct sig_dsp_Looper* rightLooper;
struct sig_dsp_BinaryOp* leftGain;
struct sig_dsp_BinaryOp* rightGain;

struct sig_ui_daisy_LooperView looperView;

void UpdateOled() {
    bool foregroundOn = !recordGate->isGateOpen;
    bluemchen.display.Fill(!foregroundOn);

    sig_ui_daisy_LooperView_render(&looperView,
        startSmoother->previousSample,
        endSmoother->previousSample,
        leftLooper->playbackPos,
        foregroundOn);

    bluemchen.display.Update();
}

void UpdateControls() {
    bluemchen.encoder.Debounce();
    startKnob.Process();
    endKnob.Process();
    speedCV.Process();
    skewCV.Process();
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    UpdateControls();

    float encoderPressed = bluemchen.encoder.Pressed() ? 1.0f : 0.0f;

    // Bind control values to Signals.
    // TODO: These should be handled by Host-provided Signals
    // for knob inputs, CV, and the encoder's various parameters.
    // https://github.com/continuing-creativity/signaletic/issues/22
    start->parameters.value = startKnob.Value();
    end->parameters.value = endKnob.Value();
    speedIncrement->parameters.value = bluemchen.encoder.Increment() *
        0.01;
    encoderButton->parameters.value = encoderPressed;
    speedMod->parameters.value = speedCV.Value();
    speedSkew->parameters.value = skewCV.Value();

    start->signal.generate(start);
    startSmoother->signal.generate(startSmoother);
    end->signal.generate(end);
    endSmoother->signal.generate(endSmoother);
    speedIncrement->signal.generate(speedIncrement);
    speedControl->signal.generate(speedControl);
    speedMod->signal.generate(speedMod);
    speedModSmoother->signal.generate(speedModSmoother);
    speedAdder->signal.generate(speedAdder);
    speedSkew->signal.generate(speedSkew);
    speedSkewSmoother->signal.generate(speedSkewSmoother);
    encoderButton->signal.generate(encoderButton);
    encoderTap->signal.generate(encoderTap);
    encoderLongPress->signal.generate(encoderLongPress);
    recordGate->signal.generate(recordGate);

    // TODO: Need a host-provided Signal to do this.
    // https://github.com/continuing-creativity/signaletic/issues/22
    for (size_t i = 0; i < size; i++) {
        leftLooper->inputs.source[i] = in[0][i];
        rightLooper->inputs.source[i] = in[1][i];
    }

    leftSpeedSkewInverter->signal.generate(leftSpeedSkewInverter);
    leftSpeedAdder->signal.generate(leftSpeedAdder);
    leftLooper->signal.generate(leftLooper);
    leftGain->signal.generate(leftGain);

    rightSpeedAdder->signal.generate(rightSpeedAdder);
    rightLooper->signal.generate(rightLooper);
    rightGain->signal.generate(rightGain);

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = leftGain->outputs.main[i];
        out[1][i] = rightGain->outputs.main[i];
    }

    // Note: this is required until we have something that
    // offers some way to get the current value from a Signal.
    // See https://github.com/continuing-creativity/signaletic/issues/19
    size_t lastSamp = leftSpeedAdder->signal.audioSettings->blockSize - 1;
    looperView.leftSpeed = leftSpeedAdder->outputs.main[lastSamp];
    looperView.rightSpeed = rightSpeedAdder->outputs.main[lastSamp];
}

void initControls() {
    startKnob.Init(bluemchen.controls[bluemchen.CTRL_1],
        0.0f, 1.0f, Parameter::LINEAR);
    endKnob.Init(bluemchen.controls[bluemchen.CTRL_2],
        0.0f, 1.0f, Parameter::LINEAR);
    speedCV.Init(bluemchen.controls[bluemchen.CTRL_3],
        -2.0f, 2.0f, Parameter::LINEAR);
    skewCV.Init(bluemchen.controls[bluemchen.CTRL_4],
        -0.5f, 0.5f, Parameter::LINEAR);
}

int main(void) {
    bluemchen.Init();
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = bluemchen.AudioSampleRate(),
        .numChannels = 1,
        .blockSize = 48
    };

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);

    bluemchen.SetAudioBlockSize(audioSettings.blockSize);
    bluemchen.StartAdc();
    initControls();

    smoothCoefficient = sig_dsp_ConstantValue_new(&allocator, context, 0.01f);

    start = sig_dsp_Value_new(&allocator, context);
    start->parameters.value = 0.0f;

    startSmoother = sig_dsp_OnePole_new(&allocator, context);
    startSmoother->inputs.source = start->outputs.main,
    startSmoother->inputs.coefficient = smoothCoefficient->outputs.main;

    end = sig_dsp_Value_new(&allocator, context);
    end->parameters.value = 1.0f;

    endSmoother = sig_dsp_OnePole_new(&allocator, context);
    endSmoother->inputs.source = end->outputs.main;
    endSmoother->inputs.coefficient = smoothCoefficient->outputs.main;

    speedIncrement = sig_dsp_Value_new(&allocator, context);
    speedIncrement->parameters.value = 1.0f;

    speedControl = sig_dsp_Accumulate_new(&allocator, context);
    speedControl->parameters.accumulatorStart = 1.0f;
    speedControl->inputs.source = speedIncrement->outputs.main;

    speedMod = sig_dsp_Value_new(&allocator, context);
    speedMod->parameters.value = 0.0f;

    speedModSmoother = sig_dsp_OnePole_new(&allocator, context);
    speedModSmoother->inputs.source = speedMod->outputs.main;
    speedModSmoother->inputs.coefficient = smoothCoefficient->outputs.main;

    speedAdder = sig_dsp_Add_new(&allocator, context);
    speedAdder->inputs.left = speedControl->outputs.main;
    speedAdder->inputs.right = speedModSmoother->outputs.main;

    speedSkew = sig_dsp_Value_new(&allocator, context);
    speedSkew->parameters.value = 0.0f;

    speedSkewSmoother = sig_dsp_OnePole_new(&allocator, context);
    speedSkewSmoother->inputs.source = speedSkew->outputs.main;
    speedSkewSmoother->inputs.coefficient = smoothCoefficient->outputs.main;

    leftSpeedSkewInverter = sig_dsp_Invert_new(&allocator, context);
    leftSpeedSkewInverter->inputs.source = speedSkewSmoother->outputs.main;

    leftSpeedAdder = sig_dsp_Add_new(&allocator, context);
    leftSpeedAdder->inputs.left = speedAdder->outputs.main;
    leftSpeedAdder->inputs.right = leftSpeedSkewInverter->outputs.main;

    rightSpeedAdder = sig_dsp_Add_new(&allocator, context);
    rightSpeedAdder->inputs.left = speedAdder->outputs.main;
    rightSpeedAdder->inputs.right = speedSkewSmoother->outputs.main;

    encoderButton = sig_dsp_Value_new(&allocator, context);
    encoderButton->parameters.value = 0.0f;

    encoderTap = sig_dsp_TimedTriggerCounter_new(&allocator, context);
    encoderTap->inputs.source = encoderButton->outputs.main;
    encoderTap->inputs.duration = sig_AudioBlock_newWithValue(&allocator,
        &audioSettings, 0.5f);
    encoderTap->inputs.count = sig_AudioBlock_newWithValue(&allocator,
        &audioSettings, 1.0f);

    recordGate = sig_dsp_ToggleGate_new(&allocator, context);
    recordGate->inputs.trigger = encoderTap->outputs.main;

    encoderLongPress = sig_dsp_GatedTimer_new(&allocator, context);
    encoderLongPress->inputs.gate = encoderButton->outputs.main;
    encoderLongPress->inputs.duration = sig_AudioBlock_newWithValue(
        &allocator, &audioSettings, LONG_ENCODER_PRESS);

    struct sig_dsp_Looper_Inputs leftLooperInputs = {
        // TODO: Need a Daisy Host-provided Signal
        // for reading audio input (gh-22).
        // For now, just use an empty block that
        // is copied into manually in the audio callback.
        .source = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f),
        .start = startSmoother->outputs.main,
        .end = endSmoother->outputs.main,
        .speed = leftSpeedAdder->outputs.main,
        .record = recordGate->outputs.main,
        .clear = encoderLongPress->outputs.main
    };

    sig_fillWithSilence(leftSamples, LOOP_LENGTH);
    leftLooper = sig_dsp_Looper_new(&allocator, context);
    leftLooper->inputs = leftLooperInputs;
    sig_dsp_Looper_setBuffer(leftLooper, &leftBuffer);

    struct sig_dsp_Looper_Inputs rightLooperInputs = {
        .source = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f),
        .start = leftLooperInputs.start,
        .end = leftLooperInputs.end,
        .speed = rightSpeedAdder->outputs.main,
        .record = leftLooperInputs.record,
        .clear = leftLooperInputs.clear
    };
    sig_fillWithSilence(rightSamples, LOOP_LENGTH);
    rightLooper = sig_dsp_Looper_new(&allocator, context);
    rightLooper->inputs = rightLooperInputs;
    sig_dsp_Looper_setBuffer(rightLooper, &rightBuffer);

    // Bluemchen's output circuit clips as it approaches full gain,
    // so 0.85 seems to be around the practical maximum value.
    struct sig_dsp_ConstantValue* gainAmount = sig_dsp_ConstantValue_new(
        &allocator, context, 0.70f);

    leftGain = sig_dsp_Mul_new(&allocator, context);
    leftGain->inputs.left = leftLooper->outputs.main;
    leftGain->inputs.right = gainAmount->outputs.main;

    rightGain = sig_dsp_Mul_new(&allocator, context);
    rightGain->inputs.left = rightLooper->outputs.main;
    rightGain->inputs.right = gainAmount->outputs.main;

    bluemchen.StartAudio(AudioCallback);

    struct sig_ui_Rect looperViewRect = {
        .x = 0,
        .y = 8,
        .width = 64,
        .height = 24
    };

    struct sig_ui_daisy_Canvas canvas = {
        .geometry = sig_ui_daisy_Canvas_geometryFromRect(&looperViewRect),
        .display = &(bluemchen.display)
    };

    struct sig_ui_daisy_LoopRenderer loopRenderer = {
        .canvas = &canvas,
        .loop = &leftLooper->loop
    };

    looperView.canvas = &canvas;
    looperView.loopRenderer = &loopRenderer;
    looperView.looper = leftLooper;
    looperView.leftSpeed = 0.0f;
    looperView.rightSpeed = 0.0f;
    looperView.positionLineThickness = 1;
    looperView.loopPointLineThickness = 2;

    while (1) {
        UpdateOled();
    }
}
