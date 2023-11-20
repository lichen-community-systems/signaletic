#include "daisy.h"
#include "../include/signaletic-dpt-lfos-signals.h"
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
struct sig_dsp_ClockDetector* clockFreq;
struct sig_daisy_ClockedLFO* lfo1;
struct sig_daisy_ClockedLFO* lfo2;
struct sig_daisy_ClockedLFO* lfo3;
struct sig_daisy_ClockedLFO* lfo4;
struct sig_dsp_BinaryOp* lfo1PlusLFO3;
struct sig_dsp_BinaryOp* lfo2PlusLFO4;
struct sig_daisy_CVOut* lfo1And3Out;
struct sig_daisy_CVOut* lfo2And4Out;
struct sig_daisy_GateOut* gate1Out;

void InitCVInputs(struct sig_SignalContext* context,
    struct sig_Status* status) {
    clockInput = sig_daisy_GateIn_new(&alloc, context, dptHost);
    clockInput->parameters.control = sig_daisy_DPT_GATE_IN_1;
    sig_List_append(&signals, clockInput, status);
}

void InitClock(struct sig_SignalContext* context, struct sig_Status* status) {
    clockFreq = sig_dsp_ClockDetector_new(&alloc, context);
    clockFreq->inputs.source = clockInput->outputs.main;
    sig_List_append(&signals, clockFreq, status);
}

void InitLFO(struct sig_SignalContext* context, struct sig_Status* status) {
    lfo1 = sig_daisy_ClockedLFO_new(&alloc, context, dptHost);
    sig_List_append(&signals, lfo1, status);
    lfo1->parameters.freqScaleCVInputControl = sig_daisy_DPT_CV_IN_1;
    lfo1->parameters.lfoGainCVInputControl = sig_daisy_DPT_CV_IN_2;
    lfo1->parameters.cvOutputControl = sig_daisy_DPT_CV_OUT_1;
    lfo1->inputs.clockFreq = clockFreq->outputs.main;

    lfo2 = sig_daisy_ClockedLFO_new(&alloc, context, dptHost);
    sig_List_append(&signals, lfo2, status);
    lfo2->parameters.freqScaleCVInputControl = sig_daisy_DPT_CV_IN_3;
    lfo2->parameters.lfoGainCVInputControl = sig_daisy_DPT_CV_IN_4;
    lfo2->parameters.cvOutputControl = sig_daisy_DPT_CV_OUT_2;
    lfo2->inputs.clockFreq = clockFreq->outputs.main;

    lfo3 = sig_daisy_ClockedLFO_new(&alloc, context, dptHost);
    sig_List_append(&signals, lfo3, status);
    lfo3->inputs.clockFreq = clockFreq->outputs.main;
    lfo3->parameters.freqScaleCVInputControl = sig_daisy_DPT_CV_IN_5;
    lfo3->parameters.lfoGainCVInputControl = sig_daisy_DPT_CV_IN_6;
    lfo3->parameters.cvOutputControl = sig_daisy_DPT_CV_OUT_3;

    lfo4 = sig_daisy_ClockedLFO_new(&alloc, context, dptHost);
    sig_List_append(&signals, lfo4, status);
    lfo4->inputs.clockFreq = clockFreq->outputs.main;
    lfo4->parameters.freqScaleCVInputControl = sig_daisy_DPT_CV_IN_7;
    lfo4->parameters.lfoGainCVInputControl = sig_daisy_DPT_CV_IN_8;
    lfo4->parameters.cvOutputControl = sig_daisy_DPT_CV_OUT_4;

    lfo1PlusLFO3 = sig_dsp_Add_new(&alloc, context);
    sig_List_append(&signals, lfo1PlusLFO3, status);
    lfo1PlusLFO3->inputs.left = lfo1->outputs.main;
    lfo1PlusLFO3->inputs.right = lfo3->outputs.main;

    lfo2PlusLFO4 = sig_dsp_Add_new(&alloc, context);
    sig_List_append(&signals, lfo2PlusLFO4, status);
    lfo2PlusLFO4->inputs.left = lfo2->outputs.main;
    lfo2PlusLFO4->inputs.right = lfo4->outputs.main;

    lfo1And3Out = sig_daisy_CVOut_new(&alloc, context, dptHost);
    sig_List_append(&signals, lfo1And3Out, status);
    lfo1And3Out->inputs.source = lfo1PlusLFO3->outputs.main;
    lfo1And3Out->parameters.control = sig_daisy_DPT_CV_OUT_5;
    lfo1And3Out->parameters.scale = 0.5f; // TODO: Replace with a wavefolder.

    lfo2And4Out = sig_daisy_CVOut_new(&alloc, context, dptHost);
    sig_List_append(&signals, lfo2And4Out, status);
    lfo2And4Out->inputs.source = lfo2PlusLFO4->outputs.main;
    lfo2And4Out->parameters.control = sig_daisy_DPT_CV_OUT_6;
    lfo2And4Out->parameters.scale = 0.5f;
}

void InitCVOutputs(struct sig_SignalContext* context,
    struct sig_Status* status) {
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
        .blockSize = 48
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
