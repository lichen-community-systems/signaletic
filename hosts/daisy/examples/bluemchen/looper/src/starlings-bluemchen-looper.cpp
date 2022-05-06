#include "../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libstar.h>
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
char signalHeap[SIGNAL_HEAP_SIZE];
struct star_Allocator allocator = {
    .heapSize = SIGNAL_HEAP_SIZE,
    .heap = (void*) signalHeap
};

#define LOOP_TIME_SECS 60
#define LOOP_LENGTH 48000 * LOOP_TIME_SECS
#define LONG_ENCODER_PRESS 2.0f

float DSY_SDRAM_BSS leftSamples[LOOP_LENGTH];
struct star_Buffer leftBuffer = {
    .length = LOOP_LENGTH,
    .samples = leftSamples
};
float DSY_SDRAM_BSS rightSamples[LOOP_LENGTH];
struct star_Buffer rightBuffer = {
    .length = LOOP_LENGTH,
    .samples = rightSamples
};

struct star_sig_Value* start;
struct star_sig_OnePole* startSmoother;
struct star_sig_Value* end;
struct star_sig_OnePole* endSmoother;
struct star_sig_Value* speedIncrement;
struct star_sig_Accumulate* speedControl;
struct star_sig_Value* speedMod;
struct star_sig_OnePole* speedModSmoother;
struct star_sig_BinaryOp* speedAdder;
struct star_sig_Value* speedSkew;
struct star_sig_OnePole* speedSkewSmoother;
struct star_sig_Invert* leftSpeedSkewInverter;
struct star_sig_BinaryOp* leftSpeedAdder;
struct star_sig_BinaryOp* rightSpeedAdder;
struct star_sig_Value* encoderButton;
struct star_sig_TimedTriggerCounter* encoderTap;
struct star_sig_GatedTimer* encoderLongPress;
struct star_sig_ToggleGate* recordGate;
struct star_sig_Looper* leftLooper;
struct star_sig_Looper* rightLooper;
struct star_sig_BinaryOp* leftGain;
struct star_sig_BinaryOp* rightGain;

struct star_LooperView looperView;

void UpdateOled() {
    bool foregroundOn = !recordGate->isGateOpen;
    bluemchen.display.Fill(!foregroundOn);

    star_LooperView_render(&looperView,
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
    // See https://github.com/continuing-creativity/starlings/issues/19
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
    star_Allocator_init(&allocator);

    struct star_AudioSettings audioSettings = {
        .sampleRate = bluemchen.AudioSampleRate(),
        .numChannels = 1,
        .blockSize = 48
    };

    bluemchen.SetAudioBlockSize(audioSettings.blockSize);
    bluemchen.StartAdc();
    initControls();

    // TODO: Introduce a constant Signal type
    // https://github.com/continuing-creativity/signaletic/issues/23
    float* smoothCoefficient = star_AudioBlock_newWithValue(
        &allocator, &audioSettings, 0.01f);

    start = star_sig_Value_new(&allocator, &audioSettings);
    start->parameters.value = 0.0f;
    struct star_sig_OnePole_Inputs startSmootherInputs = {
        .source = start->signal.output,
        .coefficient = smoothCoefficient
    };
    startSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &startSmootherInputs);


    end = star_sig_Value_new(&allocator, &audioSettings);
    end->parameters.value = 1.0f;
    struct star_sig_OnePole_Inputs endSmootherInputs = {
        .source = end->signal.output,
        .coefficient = smoothCoefficient
    };
    endSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &endSmootherInputs);


    speedIncrement = star_sig_Value_new(&allocator, &audioSettings);
    speedIncrement->parameters.value = 1.0f;

    struct star_sig_Accumulate_Inputs speedControlInputs = {
        .source = speedIncrement->signal.output,
        .reset = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f)
    };

    speedControl = star_sig_Accumulate_new(&allocator, &audioSettings,
        &speedControlInputs);
    speedControl->parameters.accumulatorStart = 1.0f;

    speedMod = star_sig_Value_new(&allocator, &audioSettings);
    speedMod->parameters.value = 0.0f;

    struct star_sig_OnePole_Inputs speedModSmootherInputs = {
        .source = speedMod->signal.output,
        .coefficient = smoothCoefficient
    };

    speedModSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &speedModSmootherInputs);

    struct star_sig_BinaryOp_Inputs speedAdderInputs = {
        .left = speedControl->signal.output,
        .right = speedModSmoother->signal.output
    };

    speedAdder = star_sig_Add_new(&allocator, &audioSettings,
        &speedAdderInputs);

    speedSkew = star_sig_Value_new(&allocator, &audioSettings);
    speedSkew->parameters.value = 0.0f;

    struct star_sig_OnePole_Inputs speedSkewSmootherInputs = {
        .source = speedSkew->signal.output,
        .coefficient = smoothCoefficient
    };
    speedSkewSmoother = star_sig_OnePole_new(&allocator, &audioSettings,
        &speedSkewSmootherInputs);

    struct star_sig_Invert_Inputs leftSpeedSkewInverterInputs = {
        .source = speedSkewSmoother->signal.output
    };
    leftSpeedSkewInverter = star_sig_Invert_new(&allocator, &audioSettings,
        &leftSpeedSkewInverterInputs);

    struct star_sig_BinaryOp_Inputs leftSpeedAdderInputs = {
        .left = speedAdder->signal.output,
        .right = leftSpeedSkewInverter->signal.output
    };
    leftSpeedAdder = star_sig_Add_new(&allocator, &audioSettings,
        &leftSpeedAdderInputs);

    struct star_sig_BinaryOp_Inputs rightSpeedAdderInputs = {
        .left = speedAdder->signal.output,
        .right = speedSkewSmoother->signal.output
    };
    rightSpeedAdder = star_sig_Add_new(&allocator, &audioSettings,
        &rightSpeedAdderInputs);

    encoderButton = star_sig_Value_new(&allocator, &audioSettings);
    encoderButton->parameters.value = 0.0f;

    struct star_sig_TimedTriggerCounter_Inputs encoderClickInputs = {
        .source = encoderButton->signal.output,

        // TODO: Replace with constant value signal (gh-23).
        .duration = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.5f),

        // TODO: Replace with constant value signal (gh-23).
        .count = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 1.0f)
    };
    encoderTap = star_sig_TimedTriggerCounter_new(&allocator,
        &audioSettings, &encoderClickInputs);


    struct star_sig_ToggleGate_Inputs recordGateInputs = {
        .trigger = encoderTap->signal.output
    };
    recordGate = star_sig_ToggleGate_new(&allocator, &audioSettings,
        &recordGateInputs);

    struct star_sig_GatedTimer_Inputs encoderPressTimerInputs = {
        .gate = encoderButton->signal.output,

        // TODO: Replace with constant value signal (gh-23).
        .duration = star_AudioBlock_newWithValue(
            &allocator, &audioSettings, LONG_ENCODER_PRESS),

        // TODO: Replace with constant value signal (gh-23).
        .loop = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f)
    };
    encoderLongPress = star_sig_GatedTimer_new(&allocator,
        &audioSettings, &encoderPressTimerInputs);


    struct star_sig_Looper_Inputs leftLooperInputs = {
        // TODO: Need a Daisy Host-provided Signal
        // for reading audio input (gh-22).
        // For now, just use an empty block that
        // is copied into manually in the audio callback.
        .source = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f),
        .start = startSmoother->signal.output,
        .end = endSmoother->signal.output,
        .speed = leftSpeedAdder->signal.output,
        .record = recordGate->signal.output,
        .clear = encoderLongPress->signal.output
    };

    // TODO: Should Buffers be automatically zeroed
    // when created?
    star_fillWithSilence(leftSamples, LOOP_LENGTH);
    leftLooper = star_sig_Looper_new(&allocator, &audioSettings,
        &leftLooperInputs);
    leftLooper->buffer = &leftBuffer;

    struct star_sig_Looper_Inputs rightLooperInputs = {
        .source = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f),
        .start = leftLooperInputs.start,
        .end = leftLooperInputs.end,
        .speed = rightSpeedAdder->signal.output,
        .record = leftLooperInputs.record,
        .clear = leftLooperInputs.clear
    };
    star_fillWithSilence(rightSamples, LOOP_LENGTH);
    rightLooper = star_sig_Looper_new(&allocator, &audioSettings,
        &rightLooperInputs);
    rightLooper->buffer = &rightBuffer;

    struct star_sig_BinaryOp_Inputs leftGainInputs = {
        // Bluemchen's output circuit clips as it approaches full gain,
        // so 0.85 seems to be around the practical maximum value.
        // TODO: Replace with constant value Signal (gh-23).
        .left = leftLooper->signal.output,
        .right = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.85f)
    };
    leftGain = star_sig_Mul_new(&allocator, &audioSettings,
        &leftGainInputs);

    struct star_sig_BinaryOp_Inputs rightGainInputs = {
        .left = rightLooper->signal.output,
        .right = leftGainInputs.left
    };
    rightGain = star_sig_Mul_new(&allocator, &audioSettings,
        &rightGainInputs);

    bluemchen.StartAudio(AudioCallback);

    struct star_Rect looperViewRect = {
        .x = 0,
        .y = 8,
        .width = 64,
        .height = 24
    };

    struct star_Canvas canvas = {
        .geometry = star_Canvas_geometryFromRect(&looperViewRect),
        .display = &(bluemchen.display)
    };

    struct star_BufferView bufferView = {
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
