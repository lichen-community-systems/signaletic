#include "../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libsignaletic.h>
#include "../../../../include/BufferView.h"
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

struct sig_LooperView looperView;

void UpdateOled() {
    bool foregroundOn = !recordGate->isGateOpen;
    bluemchen.display.Fill(!foregroundOn);

    sig_LooperView_render(&looperView,
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
        leftLooper->inputs->source[i] = in[0][i];
        rightLooper->inputs->source[i] = in[1][i];
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
        out[0][i] = leftGain->signal.output[i];
        out[1][i] = rightGain->signal.output[i];
    }

    // Note: this is required until we have something that
    // offers some way to get the current value from a Signal.
    // See https://github.com/continuing-creativity/signaletic/issues/19
    size_t lastSamp = leftSpeedAdder->signal.audioSettings->blockSize - 1;
    looperView.leftSpeed = leftSpeedAdder->signal.output[lastSamp];
    looperView.rightSpeed = rightSpeedAdder->signal.output[lastSamp];
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

    bluemchen.SetAudioBlockSize(audioSettings.blockSize);
    bluemchen.StartAdc();
    initControls();

    // TODO: Introduce a constant Signal type
    // https://github.com/continuing-creativity/signaletic/issues/23
    float* smoothCoefficient = sig_AudioBlock_newWithValue(
        &allocator, &audioSettings, 0.01f);

    start = sig_dsp_Value_new(&allocator, &audioSettings);
    start->parameters.value = 0.0f;
    struct sig_dsp_OnePole_Inputs startSmootherInputs = {
        .source = start->signal.output,
        .coefficient = smoothCoefficient
    };
    startSmoother = sig_dsp_OnePole_new(&allocator,
        &audioSettings, &startSmootherInputs);


    end = sig_dsp_Value_new(&allocator, &audioSettings);
    end->parameters.value = 1.0f;
    struct sig_dsp_OnePole_Inputs endSmootherInputs = {
        .source = end->signal.output,
        .coefficient = smoothCoefficient
    };
    endSmoother = sig_dsp_OnePole_new(&allocator,
        &audioSettings, &endSmootherInputs);


    speedIncrement = sig_dsp_Value_new(&allocator, &audioSettings);
    speedIncrement->parameters.value = 1.0f;

    struct sig_dsp_Accumulate_Inputs speedControlInputs = {
        .source = speedIncrement->signal.output,
        .reset = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f)
    };

    speedControl = sig_dsp_Accumulate_new(&allocator, &audioSettings,
        &speedControlInputs);
    speedControl->parameters.accumulatorStart = 1.0f;

    speedMod = sig_dsp_Value_new(&allocator, &audioSettings);
    speedMod->parameters.value = 0.0f;

    struct sig_dsp_OnePole_Inputs speedModSmootherInputs = {
        .source = speedMod->signal.output,
        .coefficient = smoothCoefficient
    };

    speedModSmoother = sig_dsp_OnePole_new(&allocator,
        &audioSettings, &speedModSmootherInputs);

    struct sig_dsp_BinaryOp_Inputs speedAdderInputs = {
        .left = speedControl->signal.output,
        .right = speedModSmoother->signal.output
    };

    speedAdder = sig_dsp_Add_new(&allocator, &audioSettings,
        &speedAdderInputs);

    speedSkew = sig_dsp_Value_new(&allocator, &audioSettings);
    speedSkew->parameters.value = 0.0f;

    struct sig_dsp_OnePole_Inputs speedSkewSmootherInputs = {
        .source = speedSkew->signal.output,
        .coefficient = smoothCoefficient
    };
    speedSkewSmoother = sig_dsp_OnePole_new(&allocator, &audioSettings,
        &speedSkewSmootherInputs);

    struct sig_dsp_Invert_Inputs leftSpeedSkewInverterInputs = {
        .source = speedSkewSmoother->signal.output
    };
    leftSpeedSkewInverter = sig_dsp_Invert_new(&allocator, &audioSettings,
        &leftSpeedSkewInverterInputs);

    struct sig_dsp_BinaryOp_Inputs leftSpeedAdderInputs = {
        .left = speedAdder->signal.output,
        .right = leftSpeedSkewInverter->signal.output
    };
    leftSpeedAdder = sig_dsp_Add_new(&allocator, &audioSettings,
        &leftSpeedAdderInputs);

    struct sig_dsp_BinaryOp_Inputs rightSpeedAdderInputs = {
        .left = speedAdder->signal.output,
        .right = speedSkewSmoother->signal.output
    };
    rightSpeedAdder = sig_dsp_Add_new(&allocator, &audioSettings,
        &rightSpeedAdderInputs);

    encoderButton = sig_dsp_Value_new(&allocator, &audioSettings);
    encoderButton->parameters.value = 0.0f;

    struct sig_dsp_TimedTriggerCounter_Inputs encoderClickInputs = {
        .source = encoderButton->signal.output,

        // TODO: Replace with constant value signal (gh-23).
        .duration = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.5f),

        // TODO: Replace with constant value signal (gh-23).
        .count = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 1.0f)
    };
    encoderTap = sig_dsp_TimedTriggerCounter_new(&allocator,
        &audioSettings, &encoderClickInputs);


    struct sig_dsp_ToggleGate_Inputs recordGateInputs = {
        .trigger = encoderTap->signal.output
    };
    recordGate = sig_dsp_ToggleGate_new(&allocator, &audioSettings,
        &recordGateInputs);

    struct sig_dsp_GatedTimer_Inputs encoderPressTimerInputs = {
        .gate = encoderButton->signal.output,

        // TODO: Replace with constant value signal (gh-23).
        .duration = sig_AudioBlock_newWithValue(
            &allocator, &audioSettings, LONG_ENCODER_PRESS),

        // TODO: Replace with constant value signal (gh-23).
        .loop = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f)
    };
    encoderLongPress = sig_dsp_GatedTimer_new(&allocator,
        &audioSettings, &encoderPressTimerInputs);


    struct sig_dsp_Looper_Inputs leftLooperInputs = {
        // TODO: Need a Daisy Host-provided Signal
        // for reading audio input (gh-22).
        // For now, just use an empty block that
        // is copied into manually in the audio callback.
        .source = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f),
        .start = startSmoother->signal.output,
        .end = endSmoother->signal.output,
        .speed = leftSpeedAdder->signal.output,
        .record = recordGate->signal.output,
        .clear = encoderLongPress->signal.output
    };

    // TODO: Should Buffers be automatically zeroed
    // when created?
    sig_fillWithSilence(leftSamples, LOOP_LENGTH);
    leftLooper = sig_dsp_Looper_new(&allocator, &audioSettings,
        &leftLooperInputs);
    leftLooper->buffer = &leftBuffer;

    struct sig_dsp_Looper_Inputs rightLooperInputs = {
        .source = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f),
        .start = leftLooperInputs.start,
        .end = leftLooperInputs.end,
        .speed = rightSpeedAdder->signal.output,
        .record = leftLooperInputs.record,
        .clear = leftLooperInputs.clear
    };
    sig_fillWithSilence(rightSamples, LOOP_LENGTH);
    rightLooper = sig_dsp_Looper_new(&allocator, &audioSettings,
        &rightLooperInputs);
    rightLooper->buffer = &rightBuffer;

    struct sig_dsp_BinaryOp_Inputs leftGainInputs = {
        // Bluemchen's output circuit clips as it approaches full gain,
        // so 0.85 seems to be around the practical maximum value.
        // TODO: Replace with constant value Signal (gh-23).
        .left = leftLooper->signal.output,
        .right = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.85f)
    };
    leftGain = sig_dsp_Mul_new(&allocator, &audioSettings,
        &leftGainInputs);

    struct sig_dsp_BinaryOp_Inputs rightGainInputs = {
        .left = rightLooper->signal.output,
        .right = leftGainInputs.left
    };
    rightGain = sig_dsp_Mul_new(&allocator, &audioSettings,
        &rightGainInputs);

    bluemchen.StartAudio(AudioCallback);

    struct sig_Rect looperViewRect = {
        .x = 0,
        .y = 8,
        .width = 64,
        .height = 24
    };

    struct sig_Canvas canvas = {
        .geometry = sig_Canvas_geometryFromRect(&looperViewRect),
        .display = &(bluemchen.display)
    };

    struct sig_BufferView bufferView = {
        .canvas = &canvas,
        .buffer = leftLooper->buffer
    };

    looperView.canvas = &canvas;
    looperView.bufferView = &bufferView;
    looperView.looper = leftLooper;
    looperView.leftSpeed = 0.0f;
    looperView.rightSpeed = 0.0f;
    looperView.positionLineThickness = 1;
    looperView.loopPointLineThickness = 2;

    while (1) {
        UpdateOled();
    }
}
