#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../include/daisy-dpt-host.h"

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

daisy::dpt::DPT dpt;
struct sig_daisy_Host* dptHost;

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals = {
    .items = (void**) &listStorage,
    .capacity = MAX_NUM_SIGNALS,
    .length = 0
};

struct sig_dsp_SignalListEvaluator* evaluator;
struct sig_daisy_GateIn* clockInput;
struct sig_dsp_ClockFreqDetector* clockFreq;
struct sig_daisy_CVIn* lfoAmpValue;
struct sig_dsp_BinaryOp* lfoGain;
struct sig_daisy_CVIn* lfoClockScaleValue;
struct sig_dsp_BinaryOp* lfoClockScale;
struct sig_dsp_Oscillator* lfo;
struct sig_daisy_CVOut* cv1Out;
struct sig_daisy_GateOut* gate1Out;

void InitCVInputs(struct sig_SignalContext* context,
    struct sig_Status* status) {
    clockInput = sig_daisy_GateIn_new(&alloc, context, dptHost);
    clockInput->parameters.control = sig_daisy_DPT_GATE_IN_1;
    sig_List_append(&signals, clockInput, status);

    lfoClockScaleValue = sig_daisy_CVIn_new(&alloc, context, dptHost);
    lfoClockScaleValue->parameters.control = sig_daisy_DPT_CV_IN_1;
    lfoClockScaleValue->parameters.scale = 9.9f;
    lfoClockScaleValue->parameters.offset = 0.1f;
    sig_List_append(&signals, lfoClockScaleValue, status);

    lfoAmpValue = sig_daisy_CVIn_new(&alloc, context, dptHost);
    lfoAmpValue->parameters.control = sig_daisy_DPT_CV_IN_2;
    sig_List_append(&signals, lfoAmpValue, status);
}

void InitClock(struct sig_SignalContext* context, struct sig_Status* status) {
    clockFreq = sig_dsp_ClockFreqDetector_new(&alloc, context);
    clockFreq->inputs.source = clockInput->outputs.main;
    sig_List_append(&signals, clockFreq, status);
}

void InitLFO(struct sig_SignalContext* context, struct sig_Status* status) {
    lfoClockScale = sig_dsp_Mul_new(&alloc, context);
    lfoClockScale->inputs.left = clockFreq->outputs.main;
    lfoClockScale->inputs.right = lfoClockScaleValue->outputs.main;
    sig_List_append(&signals, lfoClockScale, status);

    lfo = sig_dsp_LFTriangle_new(&alloc, context);
    lfo->inputs.freq = lfoClockScale->outputs.main;
    lfo->inputs.mul = sig_AudioBlock_newWithValue(&alloc,
        context->audioSettings, 1.0f);
    sig_List_append(&signals, lfo, status);

    lfoGain = sig_dsp_Mul_new(&alloc, context);
    lfoGain->inputs.left = lfo->outputs.main;
    lfoGain->inputs.right = lfoAmpValue->outputs.main;
    sig_List_append(&signals, lfoGain, status);
}

void InitCVOutputs(struct sig_SignalContext* context,
    struct sig_Status* status) {
    cv1Out = sig_daisy_CVOut_new(&alloc, context, dptHost);
    cv1Out->parameters.control = sig_daisy_DPT_CV_OUT_1;
    cv1Out->inputs.source = lfoGain->outputs.main;
    sig_List_append(&signals, cv1Out, status);

    // TODO: My DPT seems to output -4.67V to 7.96V,
    // this was tuned by hand with the VCV Rack oscilloscope.
    cv1Out->parameters.scale = 0.68;
    cv1Out->parameters.offset = -0.32;

    gate1Out = sig_daisy_GateOut_new(&alloc, context, dptHost);
    gate1Out->parameters.control = sig_daisy_DPT_GATE_OUT_1;
    gate1Out->inputs.source = clockInput->outputs.main;
    sig_List_append(&signals, gate1Out, status);
}

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    InitCVInputs(context, status);
    InitClock(context, status);
    InitLFO(context, status);
    InitCVOutputs(context, status);
}

int main(void) {
    struct sig_AudioSettings audioSettings = {
        .sampleRate = 48000,
        .numChannels = 2,
        .blockSize = 1
    };

    struct sig_Status status;
    sig_Status_init(&status);
    alloc.impl->init(&alloc);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);
    evaluator = sig_dsp_SignalListEvaluator_new(&alloc, &signals);

    dptHost = sig_daisy_DPTHost_new(&alloc,
        &audioSettings, &dpt, (struct sig_dsp_SignalEvaluator*) evaluator);
    sig_daisy_Host_registerGlobalHost(dptHost);

    struct sig_SignalContext* context = sig_SignalContext_new(
        &alloc, &audioSettings);
    buildSignalGraph(context, &status);
    dptHost->impl->start(dptHost);

    while (1) {

    }
}
