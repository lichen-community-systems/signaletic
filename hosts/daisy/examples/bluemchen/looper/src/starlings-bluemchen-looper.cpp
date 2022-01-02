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

#define SIGNAL_HEAP_SIZE 1024 * 384
char signalHeap[SIGNAL_HEAP_SIZE];
struct star_Allocator allocator = {
    .heapSize = SIGNAL_HEAP_SIZE,
    .heap = (void*) signalHeap
};

#define LOOP_TIME_SECS 15
#define LOOP_LENGTH 48000 * LOOP_TIME_SECS

float DSY_SDRAM_BSS looperSamples[LOOP_LENGTH];
struct star_Buffer looperBuffer = {
    .length = LOOP_LENGTH,
    .samples = looperSamples
};

struct star_sig_Value* smoothCoeff;
struct star_sig_Value* start;
struct star_sig_OnePole* startSmoother;
struct star_sig_Value* length;
struct star_sig_OnePole* lengthSmoother;
struct star_sig_Value* speed;
struct star_sig_OnePole* speedSmoother;
struct star_sig_Value* recordGate;
struct star_sig_OnePole* recordGateSmoother;
struct star_sig_Value* clearTrigger;
struct star_sig_Looper* looper;
struct star_sig_Value* gainValue;
struct star_sig_Gain* gain;

void UpdateOled() {
    bluemchen.display.Fill(false);
    FixedCapStr<20> displayStr;

    displayStr.Append("STARLOOP");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append(recordGate->parameters.value > 0.0 ?
        "Recording" : "Playing");
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Spd ");
    displayStr.AppendFloat(speed->parameters.value, 2);
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    bluemchen.display.Update();
}

void UpdateControls() {
    bluemchen.encoder.Debounce();
    startKnob.Process();
    lengthKnob.Process();
    speedCV.Process();
    recordGateCV.Process();
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    UpdateControls();

    // Bind control values to Signals.
    // TODO: These should be handled by Host-provided Signals
    // for knob inputs, CV, and the encoder's various parameters.
    start->parameters.value = startKnob.Value();
    length->parameters.value = lengthKnob.Value();
    speed->parameters.value = speedCV.Value();
    recordGate->parameters.value = recordGateCV.Value();
    clearTrigger->parameters.value = bluemchen.encoder.Pressed() ?
        1.0f : 0.0f;

    // Evaluate the signal graph.
    // We don't evaluate the smoothCoeff or gainValue
    // signals because they are constant.
    // TODO: Implement a constValue Signal.
    start->signal.generate(start);
    startSmoother->signal.generate(startSmoother);
    length->signal.generate(length);
    lengthSmoother->signal.generate(lengthSmoother);
    speed->signal.generate(speed);
    speedSmoother->signal.generate(speedSmoother);
    recordGate->signal.generate(recordGate);
    recordGateSmoother->signal.generate(recordGateSmoother);
    clearTrigger->signal.generate(clearTrigger);

    // TODO: Need a host-provided Signal to do this.
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
}

void initControls() {
    startKnob.Init(bluemchen.controls[bluemchen.CTRL_1],
        0.0f, 1.0f, Parameter::LINEAR);
    lengthKnob.Init(bluemchen.controls[bluemchen.CTRL_2],
        0.0f, 1.0f, Parameter::LINEAR);

    // TODO: Adjust speed scaling so regular playback speed is at 0.0
    speedCV.Init(bluemchen.controls[bluemchen.CTRL_3],
        -1.5f, 1.5f, Parameter::LINEAR);
    recordGateCV.Init(bluemchen.controls[bluemchen.CTRL_4],
        -2.0f, 1.0f, Parameter::LINEAR);
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

    smoothCoeff = star_sig_Value_new(&allocator, &audioSettings);
    smoothCoeff->parameters.value = 0.01f;
    smoothCoeff->signal.generate(smoothCoeff);

    start = star_sig_Value_new(&allocator, &audioSettings);
    start->parameters.value = 0.0f;
    struct star_sig_OnePole_Inputs startSmootherInputs = {
        .source = start->signal.output,
        .coefficient = smoothCoeff->signal.output
    };
    startSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &startSmootherInputs);


    length = star_sig_Value_new(&allocator, &audioSettings);
    length->parameters.value = 1.0f;
    struct star_sig_OnePole_Inputs lengthSmootherInputs = {
        .source = length->signal.output,
        .coefficient = smoothCoeff->signal.output
    };
    lengthSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &lengthSmootherInputs);


    speed = star_sig_Value_new(&allocator, &audioSettings);
    speed->parameters.value = 1.0f;
    struct star_sig_OnePole_Inputs speedSmootherInputs = {
        .source = speed->signal.output,
        .coefficient = smoothCoeff->signal.output
    };
    speedSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &speedSmootherInputs);

    recordGate = star_sig_Value_new(&allocator, &audioSettings);
    recordGate->parameters.value = 0.0f;
    struct star_sig_OnePole_Inputs recordGateSmootherInputs = {
        .source = recordGate->signal.output,
        .coefficient = smoothCoeff->signal.output
    };
    recordGateSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &recordGateSmootherInputs);


    clearTrigger = star_sig_Value_new(&allocator, &audioSettings);
    clearTrigger->parameters.value = 0.0f;

    struct star_sig_Looper_Inputs inputs = {
        // TODO: Need a Daisy Host-provided Signal
        // for reading audio input.
        // For now, just use an empty block we'll copy into.
        .source = star_AudioBlock_newWithValue(0.0f,
            &allocator, &audioSettings),
        .start = startSmoother->signal.output,
        .length = lengthSmoother->signal.output,
        .speed = speedSmoother->signal.output,
        .record = recordGateSmoother->signal.output,
        .clear = clearTrigger->signal.output
    };

    star_fillWithSilence(looperSamples, LOOP_LENGTH);
    looper = star_sig_Looper_new(&allocator, &audioSettings, &inputs);
    looper->buffer = &looperBuffer;

    // Bluemchen's output circuit clips as it approaches full gain,
    // so 0.85 seems to be around the practical maximum value.
    gainValue = star_sig_Value_new(&allocator, &audioSettings);
    gainValue->parameters.value = 0.85f;
    gainValue->signal.generate(gainValue);

    struct star_sig_Gain_Inputs gainInputs = {
        .gain = gainValue->signal.output,
        .source = looper->signal.output
    };
    gain = star_sig_Gain_new(&allocator, &audioSettings,
        &gainInputs);

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
