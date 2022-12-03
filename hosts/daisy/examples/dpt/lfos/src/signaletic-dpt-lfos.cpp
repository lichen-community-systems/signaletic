#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../vendor/dpt/lib/daisy_dpt.h"
#include "../../../../include/signaletic-daisy-host.h"

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
    sig_dsp_generateSignals(&signals);

    for (size_t i = 0; i < size; i++) {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

void InitCVInputs(struct sig_SignalContext* context,
    struct sig_Status* status) {
    clockInput = sig_daisy_GateIn_new(&alloc, context, &host,
        sig_daisy_GATEIN_1);
    sig_List_append(&signals, clockInput, status);

    lfoClockScaleValue = sig_daisy_CVIn_new(&alloc, context,
        &host, daisy::dpt::CV_1);
    lfoClockScaleValue->parameters.scale = 9.9f;
    lfoClockScaleValue->parameters.offset = 0.1f;
    sig_List_append(&signals, lfoClockScaleValue, status);

    lfoAmpValue = sig_daisy_CVIn_new(&alloc, context,
        &host, daisy::dpt::CV_2);
    sig_List_append(&signals, lfoAmpValue, status);
}

void InitClock(struct sig_SignalContext* context, struct sig_Status* status) {
    clockFreq = sig_dsp_ClockFreqDetector_new(&alloc, context);
    clockFreq->inputs.source = clockInput->outputs.main;
    sig_List_append(&signals, clockFreq, status);
}

void InitLFO(struct sig_SignalContext* context, struct sig_Status* status) {
    lfoClockScale = sig_dsp_Mul_new(&alloc, context);
    sig_List_append(&signals, lfoClockScale, status);
    lfoClockScale->inputs.left = clockFreq->outputs.main;
    lfoClockScale->inputs.right = lfoClockScaleValue->outputs.main;

    lfo = sig_dsp_LFTriangle_new(&alloc, context);
    sig_List_append(&signals, lfo, status);
    lfo->inputs.freq = lfoClockScale->outputs.main;
    lfo->inputs.mul = sig_AudioBlock_newWithValue(&alloc,
        context->audioSettings, 1.0f);

    lfoGain = sig_dsp_Mul_new(&alloc, context);
    sig_List_append(&signals, lfoGain, status);
    lfoGain->inputs.left = lfo->outputs.main;
    lfoGain->inputs.right = lfoAmpValue->outputs.main;
}

void InitCVOutputs(struct sig_SignalContext* context,
    struct sig_Status* status) {
    cv1Out = sig_daisy_CVOut_new(&alloc, context, &host,
        sig_daisy_DPT_CVOUT_1);
    cv1Out->inputs.source = lfoGain->outputs.main;
    sig_List_append(&signals, cv1Out, status);

    // TODO: My DPT seems to output -4.67V to 7.96V,
    // this was tuned by hand with the VCV Rack oscilloscope.
    cv1Out->parameters.scale = 0.68;
    cv1Out->parameters.offset = -0.32;
}

void InitAudioGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    InitCVInputs(context, status);
    InitClock(context, status);
    InitLFO(context, status);
    InitCVOutputs(context, status);
}

int main(void) {
    alloc.impl->init(&alloc);

    patch.Init();

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 48000,
        .numChannels = 2,
        .blockSize = 1
    };

    struct sig_SignalContext* context = sig_SignalContext_new(
        &alloc, &audioSettings);

    struct sig_Status status;
    sig_Status_init(&status);

    patch.SetAudioSampleRate(audioSettings.sampleRate);
    patch.SetAudioBlockSize(audioSettings.blockSize);

    InitAudioGraph(context, &status);

    patch.StartAudio(AudioCallback);
    patch.InitTimer(&sig_daisy_DPT_dacWriterCallback, &dptState);

    while (1) {

    }
}
