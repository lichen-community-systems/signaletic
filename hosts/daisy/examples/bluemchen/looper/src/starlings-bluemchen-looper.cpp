#include "../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libstar.h>
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter startKnob;
Parameter lengthKnob;
Parameter speedCV;
Parameter recordGateCV;
float speed = 0.0f;

#define SIGNAL_HEAP_SIZE 1024 * 384
char signalHeap[SIGNAL_HEAP_SIZE];
struct star_Allocator allocator = {
    .heapSize = SIGNAL_HEAP_SIZE,
    .heap = (void*) signalHeap
};

#define LOOP_TIME_SECS 30
#define LOOP_LENGTH 48000 * LOOP_TIME_SECS
#define LONG_ENCODER_PRESS 2.0f

float DSY_SDRAM_BSS looperSamples[LOOP_LENGTH];
struct star_Buffer looperBuffer = {
    .length = LOOP_LENGTH,
    .samples = looperSamples
};

struct star_sig_Value* start;
struct star_sig_OnePole* startSmoother;
struct star_sig_Value* length;
struct star_sig_OnePole* lengthSmoother;
struct star_sig_Value* speedIncrement;
struct star_sig_Accumulate* speedControl;
struct star_sig_Value* speedMod;
struct star_sig_OnePole* speedModSmoother;
struct star_sig_Add* speedAdder;
struct star_sig_Value* encoderButton;
struct star_sig_TimedTriggerCounter* encoderTap;
struct star_sig_GatedTimer* encoderLongPress;
struct star_sig_ToggleGate* recordGate;
struct star_sig_Looper* looper;
struct star_sig_Gain* gain;

void UpdateOled() {
    bluemchen.display.Fill(false);
    FixedCapStr<20> displayStr;

    displayStr.Append("STARLOOP");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append(recordGate->isGateOpen ? "Recording" : "Playing");
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Spd ");
    displayStr.AppendFloat(speed, 2);
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    bluemchen.display.Update();
}

void UpdateControls() {
    bluemchen.encoder.Debounce();
    startKnob.Process();
    lengthKnob.Process();
    speedCV.Process();
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    UpdateControls();

    float encoderPressed = bluemchen.encoder.Pressed() ? 1.0f : 0.0f;

    // Bind control values to Signals.
    // TODO: These should be handled by Host-provided Signals
    // for knob inputs, CV, and the encoder's various parameters.
    start->parameters.value = startKnob.Value();
    length->parameters.value = lengthKnob.Value();
    speedIncrement->parameters.value = bluemchen.encoder.Increment() *
        0.01;
    encoderButton->parameters.value = encoderPressed;
    speedMod->parameters.value = speedCV.Value();

    start->signal.generate(start);
    startSmoother->signal.generate(startSmoother);
    length->signal.generate(length);
    lengthSmoother->signal.generate(lengthSmoother);
    speedIncrement->signal.generate(speedIncrement);
    speedControl->signal.generate(speedControl);
    speedMod->signal.generate(speedMod);
    speedModSmoother->signal.generate(speedModSmoother);
    speedAdder->signal.generate(speedAdder);
    encoderButton->signal.generate(encoderButton);
    encoderTap->signal.generate(encoderTap);
    encoderLongPress->signal.generate(encoderLongPress);
    recordGate->signal.generate(recordGate);

    // TODO: Need a host-provided Signal to do this.
    // TODO: Create two Loopers, one for each channel,
    //       and potentially with some kind of "skew"
    //       between their values.
    for (size_t i = 0; i < size; i++) {
        looper->inputs->source[i] = in[0][i];
    }

    looper->signal.generate(looper);
    gain->signal.generate(gain);

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = gain->signal.output[i];
        out[1][i] = gain->signal.output[i];
    }

    // Note: this is required until we have something that
    // offers some way to get the current value from a Signal.
    // See https://github.com/continuing-creativity/starlings/issues/19
    speed = speedAdder->signal.output[
        speedAdder->signal.audioSettings->blockSize -1];
}

void initControls() {
    startKnob.Init(bluemchen.controls[bluemchen.CTRL_1],
        0.0f, 1.0f, Parameter::LINEAR);
    lengthKnob.Init(bluemchen.controls[bluemchen.CTRL_2],
        0.0f, 1.0f, Parameter::LINEAR);
    speedCV.Init(bluemchen.controls[bluemchen.CTRL_3],
        -1.5f, 1.5f, Parameter::LINEAR);
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

    float* smoothCoefficient = star_AudioBlock_newWithValue(0.01f,
        &allocator, &audioSettings);

    start = star_sig_Value_new(&allocator, &audioSettings);
    start->parameters.value = 0.0f;
    struct star_sig_OnePole_Inputs startSmootherInputs = {
        .source = start->signal.output,
        // TODO: Introduce a constant Signal type
        .coefficient = smoothCoefficient
    };
    startSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &startSmootherInputs);


    length = star_sig_Value_new(&allocator, &audioSettings);
    length->parameters.value = 1.0f;
    struct star_sig_OnePole_Inputs lengthSmootherInputs = {
        .source = length->signal.output,
        .coefficient = smoothCoefficient
    };
    lengthSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &lengthSmootherInputs);


    speedIncrement = star_sig_Value_new(&allocator, &audioSettings);
    speedIncrement->parameters.value = 1.0f;

    struct star_sig_Accumulate_Inputs speedControlInputs = {
        .source = speedIncrement->signal.output,
        .reset = star_AudioBlock_newWithValue(0.0f, &allocator,
            &audioSettings)
    };

    struct star_sig_Accumulate_Parameters speedControlParams = {
        .accumulatorStart = 1.0f
    };
    speedControl = star_sig_Accumulate_new(&allocator, &audioSettings,
        &speedControlInputs, speedControlParams);

    speedMod = star_sig_Value_new(&allocator, &audioSettings);
    speedMod->parameters.value = 0.0f;

    struct star_sig_OnePole_Inputs speedModSmootherInputs = {
        .source = speedMod->signal.output,
        .coefficient = smoothCoefficient
    };

    speedModSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &speedModSmootherInputs);

    struct star_sig_Add_Inputs speedAdderInputs = {
        .left = speedControl->signal.output,
        .right = speedModSmoother->signal.output
    };

    speedAdder = star_sig_Add_new(&allocator, &audioSettings,
        &speedAdderInputs);

    encoderButton = star_sig_Value_new(&allocator, &audioSettings);
    encoderButton->parameters.value = 0.0f;

    struct star_sig_TimedTriggerCounter_Inputs encoderClickInputs = {
        .source = encoderButton->signal.output,

        // TODO: Replace with constant value signal
        .duration = star_AudioBlock_newWithValue(0.5f, &allocator,
            &audioSettings),

        // TODO: Replace with constant value signal
        .count = star_AudioBlock_newWithValue(1.0f, &allocator,
            &audioSettings)
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

        // TODO: Replace with constant value signal
        .duration = star_AudioBlock_newWithValue(LONG_ENCODER_PRESS,
            &allocator, &audioSettings),

        // TODO: Replace with constant value signal
        .loop = star_AudioBlock_newWithValue(0.0f, &allocator,
            &audioSettings)
    };
    encoderLongPress = star_sig_GatedTimer_new(&allocator,
        &audioSettings, &encoderPressTimerInputs);


    struct star_sig_Looper_Inputs inputs = {
        // TODO: Need a Daisy Host-provided Signal
        // for reading audio input.
        // For now, just use an empty block that
        // is copied into manually in the audio callback.
        .source = star_AudioBlock_newWithValue(0.0f,
            &allocator, &audioSettings),
        .start = startSmoother->signal.output,
        .length = lengthSmoother->signal.output,
        .speed = speedAdder->signal.output,
        .record = recordGate->signal.output,
        .clear = encoderLongPress->signal.output
    };

    star_fillWithSilence(looperSamples, LOOP_LENGTH);
    looper = star_sig_Looper_new(&allocator, &audioSettings, &inputs);
    looper->buffer = &looperBuffer;

    struct star_sig_Gain_Inputs gainInputs = {
        // Bluemchen's output circuit clips as it approaches full gain,
        // so 0.85 seems to be around the practical maximum value.
        // TODO: Replace with constant value Signal.
        .gain = star_AudioBlock_newWithValue(0.85f, &allocator,
            &audioSettings),
        .source = looper->signal.output
    };
    gain = star_sig_Gain_new(&allocator, &audioSettings,
        &gainInputs);

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
