#include "../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libstar.h>
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter densityKnob;

FixedCapStr<20> displayStr;

#define SIGNAL_HEAP_SIZE 1024 * 384
char signalHeap[SIGNAL_HEAP_SIZE];
struct star_Allocator allocator = {
    .heapSize = SIGNAL_HEAP_SIZE,
    .heap = (void*) signalHeap
};

struct star_sig_Value* density;
struct star_sig_Dust* dust;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Dust Dust");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);
    bluemchen.display.Update();
}

void UpdateControls() {
    bluemchen.encoder.Debounce();
    densityKnob.Process();
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    UpdateControls();

    density->parameters.value = densityKnob.Value();
    density->signal.generate(density);
    dust->signal.generate(dust);

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = dust->signal.output[i];
        out[1][i] = dust->signal.output[i];
    }
}

void initControls() {
    densityKnob.Init(bluemchen.controls[bluemchen.CTRL_1],
        1.0f/120.0f, 1000.0f, Parameter::LINEAR);
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

    struct star_sig_Dust_Inputs dustInputs = {
        .density = density->signal.output
    };

    dust = star_sig_Dust_new(&allocator, &audioSettings, &dustInputs);
    dust->parameters.bipolar = 0.0f;

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
