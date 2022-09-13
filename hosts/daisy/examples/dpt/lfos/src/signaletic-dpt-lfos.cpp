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

struct sig_dsp_Value* clockInput;
struct sig_dsp_ClockFreqDetector* clockFreq;
struct sig_dsp_Oscillator* lfo;
struct sig_dsp_Value* lfoAmpValue;
struct sig_dsp_BinaryOp* lfoGain;
struct sig_dsp_Value* lfoClockScaleValue;
struct sig_dsp_BinaryOp* lfoClockScale;

void dac7554callback(void *data) {

}

void AudioCallback(AudioHandle::InputBuffer in,
    AudioHandle::OutputBuffer out, size_t size) {
    patch.ProcessAllControls();

    // Gates are inverted on the Daisy Patch SM.
    float gate1Value = patch.gate_in_1.State() ? 1.0f : 0.0f;
    clockInput->parameters.value = gate1Value;

    float cvIn1Value = patch.controls[CV_1].Value();
    lfoAmpValue->parameters.value = cvIn1Value;

    float cvIn2Value = patch.controls[CV_2].Value() * 9.9 + 0.1;
    lfoClockScaleValue->parameters.value = cvIn2Value;

    clockInput->signal.generate(clockInput);
    clockFreq->signal.generate(clockFreq);
    lfoClockScaleValue->signal.generate(lfoClockScaleValue);
    lfoClockScale->signal.generate(lfoClockScale);
    lfo->signal.generate(lfo);
    lfoAmpValue->signal.generate(lfoAmpValue);
    lfoGain->signal.generate(lfoGain);

    uint16_t lfoValue = sig_bipolarToInvUint12(lfoGain->signal.output[0]);
    patch.WriteCvOut(1, lfoValue, true);

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

void InitClock(struct sig_AudioSettings* audioSettings) {
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

void InitLFO(struct sig_AudioSettings* audioSettings) {
    lfoClockScaleValue = sig_dsp_Value_new(&alloc, audioSettings);

    struct sig_dsp_BinaryOp_Inputs* lfoClockScaleInputs =
        (struct sig_dsp_BinaryOp_Inputs*) alloc.impl->malloc(
        &alloc,
        sizeof(struct sig_dsp_BinaryOp_Inputs));
    lfoClockScaleInputs->left = clockFreq->signal.output;
    lfoClockScaleInputs->right = lfoClockScaleValue->signal.output;

    lfoClockScale = sig_dsp_Mul_new(&alloc, audioSettings,
        lfoClockScaleInputs);

    struct sig_dsp_Oscillator_Inputs* lfoInputs =
        (struct sig_dsp_Oscillator_Inputs*)
            alloc.impl->malloc(
                &alloc,
                sizeof(struct sig_dsp_Oscillator_Inputs));
    lfoInputs->freq = lfoClockScale->signal.output;
    lfoInputs->phaseOffset = sig_AudioBlock_newWithValue(&alloc,
        audioSettings, 0.0f);
    lfoInputs->mul = sig_AudioBlock_newWithValue(&alloc,
        audioSettings, 1.0f);
    lfoInputs->add = sig_AudioBlock_newWithValue(&alloc,
        audioSettings, 0.0f);

    lfo = sig_dsp_LFTriangle_new(&alloc, audioSettings, lfoInputs);

    lfoAmpValue = sig_dsp_Value_new(&alloc, audioSettings);

    struct sig_dsp_BinaryOp_Inputs* mulInputs =
        (struct sig_dsp_BinaryOp_Inputs*) alloc.impl->malloc(
            &alloc,
            sizeof(struct sig_dsp_BinaryOp_Inputs));
    mulInputs->left = lfo->signal.output;
    mulInputs->right = lfoAmpValue->signal.output;

    lfoGain = sig_dsp_Mul_new(&alloc, audioSettings, mulInputs);
}

void InitAudioGraph(struct sig_AudioSettings* audioSettings) {
    InitClock(audioSettings);
    InitLFO(audioSettings);
}

int main(void) {
    alloc.impl->init(&alloc);

    patch.Init();

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 48000,
        .numChannels = 2,
        .blockSize = 1
    };

    patch.SetAudioSampleRate(audioSettings.sampleRate);
    patch.SetAudioBlockSize(audioSettings.blockSize);

    InitAudioGraph(&audioSettings);

    patch.StartAudio(AudioCallback);
    patch.InitTimer(dac7554callback, nullptr);

    while (1) {

    }
}
