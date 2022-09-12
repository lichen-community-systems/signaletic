#include "daisy.h"
#include "../../vendor/dpt/lib/daisy_dpt.h"
#include <libsignaletic.h>

using namespace daisy;
using namespace dpt;

DPT patch;

#define HEAP_SIZE 1024 * 256 // 256KB
uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator alloc = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

struct sig_AudioSettings* audioSettings;
struct sig_dsp_Value* clockInput;
struct sig_dsp_ClockFreqDetector* clockFreq;
struct sig_dsp_LFTri* lfo;

void dac7554callback(void *data) {

}

void AudioCallback(AudioHandle::InputBuffer in,
    AudioHandle::OutputBuffer out, size_t size) {
    patch.ProcessAllControls();

    clockInput->parameters.value = patch.gate_in_1.State() ? 1.0f : 0.0f;
    clockInput->signal.generate(&clockInput);
    clockFreq->signal.generate(&clockFreq);
    lfo->signal.generate(&lfo);

    patch.WriteCvOut(0, sig_bipolarToInvUint12(lfo->signal.output[0]),
        true);
    /*
        CV 1 - 8 accessed
            patch.controls[CV_1].Value()

        Gate ins acccessed

            patch.gate_in_1.State();
            patch.gate_in_2.State();

        Send data to gate outs

            dsy_gpio_write(&patch.gate_out_1, 1);
            dsy_gpio_write(&patch.gate_out_2, 2);

        Send data to CV 1 and 2 (0 is both, currently it seems 1 is CV2, 2 is CV1)
            patch.WriteCvOut(0, 5.0, false);
            patch.WriteCvOut(0, [0 - 4095], true); // last argument is 'raw', send raw 12-bit data

    */

    for (size_t i = 0; i < size; i++) {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

void InitClock() {
    clockInput = sig_dsp_Value_new(&alloc, audioSettings);

    struct sig_dsp_ClockFreqDetector_Inputs* clockFreqInputs =
        (struct sig_dsp_ClockFreqDetector_Inputs*)
            alloc.impl->malloc(
                &alloc,
                sizeof(struct sig_dsp_ClockFreqDetector_Inputs));
    clockFreqInputs->source = clockInput->signal.output;

    clockFreq = sig_dsp_ClockFreqDetector_new(&alloc, audioSettings,
        clockFreqInputs);
}

void InitLFO() {
    struct sig_dsp_Oscillator_Inputs* lfoInputs =
        (struct sig_dsp_Oscillator_Inputs*)
            alloc.impl->malloc(
                &alloc,
                sizeof(struct sig_dsp_Oscillator_Inputs));
    lfoInputs->freq = clockFreq->signal.output;
    lfoInputs->phaseOffset = sig_AudioBlock_newWithValue(&alloc,
        audioSettings, 0.0f);
    lfoInputs->mul = sig_AudioBlock_newWithValue(&alloc,
        audioSettings, 1.0f);
    lfoInputs->add = sig_AudioBlock_newWithValue(&alloc,
        audioSettings, 0.0f);

    lfo = sig_dsp_LFTri_new(&alloc, audioSettings, lfoInputs);
}

void InitAudioGraph() {
    audioSettings = sig_AudioSettings_new(&alloc);
    InitClock();
    InitLFO();
}

int main(void) {
    patch.Init();

    InitAudioGraph();

    patch.StartAudio(AudioCallback);
    patch.InitTimer(dac7554callback, nullptr);

    while (1) {

    }
}
