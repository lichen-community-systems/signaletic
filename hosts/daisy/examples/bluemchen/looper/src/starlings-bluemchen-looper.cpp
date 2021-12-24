#include "../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libstar.h>
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter startKnob;
Parameter endKnob;
Parameter speedCV;
Parameter gateCV;

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
struct star_sig_Value* end;
struct star_sig_OnePole* endSmoother;
struct star_sig_Value* speed;
struct star_sig_OnePole* speedSmoother;
struct star_sig_Value* gate;
struct star_sig_OnePole* gateSmoother;
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
    displayStr.Append(gate->parameters.value > 0.0 ?
        "Recording" : "Playing");
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    bluemchen.display.Update();
    displayStr.Clear();
}

void UpdateControls() {
    startKnob.Process();
    endKnob.Process();

    speedCV.Process();
    gateCV.Process();
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    UpdateControls();

    // Bind control values to Signals.
    // TODO: This should be handled by a Host-provided Signal
    start->parameters.value = startKnob.Value();
    end->parameters.value = endKnob.Value();

    speed->parameters.value = speedCV.Value();
    gate->parameters.value = gateCV.Value();

    // Evaluate the signal graph.
    smoothCoeff->signal.generate(smoothCoeff);
    start->signal.generate(start);
    startSmoother->signal.generate(startSmoother);
    end->signal.generate(end);
    endSmoother->signal.generate(endSmoother);
    speed->signal.generate(speed);
    speedSmoother->signal.generate(speedSmoother);
    gate->signal.generate(gate);
    gateSmoother->signal.generate(gateSmoother);

    // TODO: Need a host-provided Signal to do this.
    for (size_t i = 0; i < size; i++) {
        looper->inputs->source[i] = in[0][i];
    }

    looper->signal.generate(looper);
    gainValue->signal.generate(gainValue);
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
    endKnob.Init(bluemchen.controls[bluemchen.CTRL_2],
        0.0f, 1.0f, Parameter::LINEAR);
    speedCV.Init(bluemchen.controls[bluemchen.CTRL_3],
        -1.5f, 1.5f, Parameter::LINEAR);
    // TODO: We need smoothing on this, but for now we'll
    // just require a larger gate value.
    gateCV.Init(bluemchen.controls[bluemchen.CTRL_4],
        -1.0f, 1.0f, Parameter::LINEAR);
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

    start = star_sig_Value_new(&allocator, &audioSettings);
    start->parameters.value = 0.0f;
    struct star_sig_OnePole_Inputs startSmootherInputs = {
        .source = start->signal.output,
        .coefficient = smoothCoeff->signal.output
    };
    startSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &startSmootherInputs);


    end = star_sig_Value_new(&allocator, &audioSettings);
    end->parameters.value = 1.0f;
    struct star_sig_OnePole_Inputs endSmootherInputs = {
        .source = end->signal.output,
        .coefficient = smoothCoeff->signal.output
    };
    endSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &endSmootherInputs);


    speed = star_sig_Value_new(&allocator, &audioSettings);
    speed->parameters.value = 1.0f;
    struct star_sig_OnePole_Inputs speedSmootherInputs = {
        .source = speed->signal.output,
        .coefficient = smoothCoeff->signal.output
    };
    speedSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &speedSmootherInputs);

    gate = star_sig_Value_new(&allocator, &audioSettings);
    gate->parameters.value = 0.0f;
    struct star_sig_OnePole_Inputs gateSmootherInputs = {
        .source = gate->signal.output,
        .coefficient = smoothCoeff->signal.output
    };
    gateSmoother = star_sig_OnePole_new(&allocator,
        &audioSettings, &gateSmootherInputs);


    struct star_sig_Looper_Inputs inputs = {
        // TODO: Need a Daisy Host-provided Signal
        // for reading audio input.
        // For now, just use an empty block we'll copy into.
        .source = star_AudioBlock_newWithValue(0.0f,
            &allocator, &audioSettings),
        .start = startSmoother->signal.output,
        .end = endSmoother->signal.output,
        .speed = speedSmoother->signal.output,
        .recordGate = gateSmoother->signal.output
    };

    star_fillWithSilence(looperSamples, LOOP_LENGTH);
    looper = star_sig_Looper_new(&allocator, &audioSettings, &inputs);
    looper->buffer = &looperBuffer;

    // Bluemchen's output circuit clips as it approaches full gain,
    // so 0.85 seems to be around the practical maximum value.
    gainValue = star_sig_Value_new(&allocator, &audioSettings);
    gainValue->parameters.value = 0.85f;

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
