#include "../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libstar.h>
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter densityKnob;
Parameter durationKnob;

FixedCapStr<20> displayStr;

#define SIGNAL_HEAP_SIZE 1024 * 384
char signalHeap[SIGNAL_HEAP_SIZE];
struct star_Allocator allocator = {
    .heapSize = SIGNAL_HEAP_SIZE,
    .heap = (void*) signalHeap
};

struct star_sig_Value* density;
struct star_sig_Value* duration;
struct star_sig_BinaryOp* reciprocalDensity;
struct star_sig_BinaryOp* densityDurationMultiplier;
struct star_sig_Dust* dust;
struct star_sig_TimedGate* gate;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Dust Dust");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Den: ");
    displayStr.AppendFloat(density->signal.output[0], 3);
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Dur: ");
    displayStr.AppendFloat(densityDurationMultiplier->signal.output[0], 3);
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    bluemchen.display.Update();
}

void UpdateControls() {
    bluemchen.encoder.Debounce();
    densityKnob.Process();
    durationKnob.Process();
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    UpdateControls();

    density->parameters.value = densityKnob.Value();
    duration->parameters.value = durationKnob.Value();
    reciprocalDensity->signal.generate(reciprocalDensity);
    densityDurationMultiplier->signal.generate(densityDurationMultiplier);
    density->signal.generate(density);
    duration->signal.generate(duration);
    dust->signal.generate(dust);
    gate->signal.generate(gate);

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = gate->signal.output[i];
        out[1][i] = gate->signal.output[i];
    }
}

void initControls() {
    densityKnob.Init(bluemchen.controls[bluemchen.CTRL_1],
        1.0f/60.0f, 20.0f, Parameter::LINEAR);
    durationKnob.Init(bluemchen.controls[bluemchen.CTRL_2],
        0, 1, Parameter::LINEAR);
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

    density = star_sig_Value_new(&allocator, &audioSettings);
    density->parameters.value = 1.0f;
    duration = star_sig_Value_new(&allocator, &audioSettings);
    duration->parameters.value = 1.0f;

    struct star_sig_BinaryOp_Inputs reciprocalDensityInputs = {
        .left = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 1.0f),
        .right = density->signal.output
    };

    reciprocalDensity = star_sig_Div_new(&allocator,
        &audioSettings, &reciprocalDensityInputs);

    struct star_sig_BinaryOp_Inputs muliplierInputs = {
        .left = reciprocalDensity->signal.output,
        .right = duration->signal.output
    };

    densityDurationMultiplier = star_sig_Mul_new(&allocator,
        &audioSettings, &muliplierInputs);

    struct star_sig_Dust_Inputs dustInputs = {
        .density = density->signal.output
    };
    dust = star_sig_Dust_new(&allocator, &audioSettings, &dustInputs);
    dust->parameters.bipolar = 0.0f;

    struct star_sig_TimedGate_Inputs gateInputs = {
        .trigger = dust->signal.output,
        .duration = densityDurationMultiplier->signal.output
    };
    gate = star_sig_TimedGate_new(&allocator, &audioSettings, &gateInputs);

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
