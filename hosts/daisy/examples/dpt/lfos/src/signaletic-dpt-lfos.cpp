#include "daisy.h"
#include "../../vendor/dpt/lib/daisy_dpt.h"
#include <libsignaletic.h>
#include "../../../include/signaletic-daisy-host.h"

daisy::dpt::DPT patch;

#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 128

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator alloc = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

struct sig_daisy_DPTState dptState = {
    .dpt = &patch,
    .dacCVOuts = {4095, 4095, 4095, 4095}
};

struct sig_daisy_Host host = {
    .impl = &sig_daisy_DPTHostImpl,
    .state = (void *) &dptState
};

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals = {
    .items = (void**) &listStorage,
    .capacity = MAX_NUM_SIGNALS,
    .length = 0
};

struct sig_daisy_GateIn* clockInput;
struct sig_dsp_ClockFreqDetector* clockFreq;
struct sig_daisy_CVIn* lfoAmpValue;
struct sig_dsp_BinaryOp* lfoGain;
struct sig_daisy_CVIn* lfoClockScaleValue;
struct sig_dsp_BinaryOp* lfoClockScale;
struct sig_dsp_Oscillator* lfo;
struct sig_daisy_CVOut* cv1Out;

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    patch.ProcessAllControls();

    sig_dsp_generateSignals(&signals);

    for (size_t i = 0; i < size; i++) {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

void InitCVInputs(struct sig_AudioSettings* audioSettings,
    struct sig_Status* status) {
    clockInput = sig_daisy_GateIn_new(&alloc, audioSettings, &host,
        sig_daisy_GATEIN_1);
    sig_List_append(&signals, clockInput, status);

    lfoClockScaleValue = sig_daisy_CVIn_new(&alloc, audioSettings,
        &host, daisy::dpt::CV_1);
    lfoClockScaleValue->parameters.scale = 9.9f;
    lfoClockScaleValue->parameters.offset = 0.1f;
    sig_List_append(&signals, lfoClockScaleValue, status);

    lfoAmpValue = sig_daisy_CVIn_new(&alloc, audioSettings,
        &host, daisy::dpt::CV_2);
    sig_List_append(&signals, lfoAmpValue, status);
}

void InitClock(struct sig_AudioSettings* audioSettings,
    struct sig_Status* status) {
    struct sig_dsp_ClockFreqDetector_Inputs* clockFreqInputs =
        (struct sig_dsp_ClockFreqDetector_Inputs*)
            alloc.impl->malloc(
                &alloc,
                sizeof(struct sig_dsp_ClockFreqDetector_Inputs));
    clockFreqInputs->source = clockInput->signal.output;

    clockFreq = sig_dsp_ClockFreqDetector_new(&alloc, audioSettings,
        clockFreqInputs);
    sig_List_append(&signals, clockFreq, status);
}

void InitLFO(struct sig_AudioSettings* audioSettings,
    struct sig_Status* status) {
    struct sig_dsp_BinaryOp_Inputs* lfoClockScaleInputs =
        (struct sig_dsp_BinaryOp_Inputs*) alloc.impl->malloc(
        &alloc,
        sizeof(struct sig_dsp_BinaryOp_Inputs));
    lfoClockScaleInputs->left = clockFreq->signal.output;
    lfoClockScaleInputs->right = lfoClockScaleValue->signal.output;

    lfoClockScale = sig_dsp_Mul_new(&alloc, audioSettings,
        lfoClockScaleInputs);
    sig_List_append(&signals, lfoClockScale, status);

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
    sig_List_append(&signals, lfo, status);


    struct sig_dsp_BinaryOp_Inputs* mulInputs =
        (struct sig_dsp_BinaryOp_Inputs*) alloc.impl->malloc(
            &alloc,
            sizeof(struct sig_dsp_BinaryOp_Inputs));
    mulInputs->left = lfo->signal.output;
    mulInputs->right = lfoAmpValue->signal.output;

    lfoGain = sig_dsp_Mul_new(&alloc, audioSettings, mulInputs);
    sig_List_append(&signals, lfoGain, status);
}

void InitCVOutputs(struct sig_AudioSettings* audioSettings,
    struct sig_Status* status) {
    struct sig_daisy_CVOut_Inputs* cv1OutInputs =
        (struct sig_daisy_CVOut_Inputs*)
            alloc.impl->malloc(
                &alloc, sizeof(struct sig_daisy_CVOut_Inputs));
    cv1OutInputs->source = lfoGain->signal.output;

    cv1Out = sig_daisy_CVOut_new(&alloc, audioSettings, cv1OutInputs,
        &host, sig_daisy_DPT_CVOUT_1);
    sig_List_append(&signals, cv1Out, status);

    // TODO: My DPT seems to output -4.67V to 7.96V,
    // this was tuned by hand with the VCV Rack oscilloscope.
    cv1Out->parameters.scale = 0.68;
    cv1Out->parameters.offset = -0.32;
}

void InitAudioGraph(struct sig_AudioSettings* audioSettings,
    struct sig_Status* status) {
    InitCVInputs(audioSettings, status);
    InitClock(audioSettings, status);
    InitLFO(audioSettings, status);
    InitCVOutputs(audioSettings, status);
}

int main(void) {
    alloc.impl->init(&alloc);

    patch.Init();

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 48000,
        .numChannels = 2,
        .blockSize = 1
    };

    struct sig_Status status;
    sig_Status_init(&status);

    patch.SetAudioSampleRate(audioSettings.sampleRate);
    patch.SetAudioBlockSize(audioSettings.blockSize);

    InitAudioGraph(&audioSettings, &status);

    patch.StartAudio(AudioCallback);
    patch.InitTimer(&sig_daisy_DPT_dacWriterCallback, &dptState);

    while (1) {

    }
}
