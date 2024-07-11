#include "../include/signaletic-dpt-lfos-signals.h"
#include "../../../../include/dspcoffee-dpt-device.hpp"

#define SAMPLERATE 48000
#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 128

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

DaisyHost<dspcoffee::dpt::DPTDevice> host;

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals = {
    .items = (void**) &listStorage,
    .capacity = MAX_NUM_SIGNALS,
    .length = 0
};

struct sig_dsp_SignalListEvaluator* evaluator;
struct sig_host_GateIn* clockInput;
struct sig_dsp_ClockDetector* clockFreq;
struct sig_host_ClockedLFO* lfo1;
struct sig_host_ClockedLFO* lfo2;
struct sig_host_ClockedLFO* lfo3;
struct sig_host_ClockedLFO* lfo4;
struct sig_dsp_BinaryOp* lfo1PlusLFO3;
struct sig_dsp_BinaryOp* lfo2PlusLFO4;
struct sig_host_CVOut* lfo1And3Out;
struct sig_host_CVOut* lfo2And4Out;
struct sig_host_GateOut* gate1Out;

void InitCVInputs(struct sig_SignalContext* context,
    struct sig_Status* status) {
    clockInput = sig_host_GateIn_new(&allocator, context);
    clockInput->hardware = &host.device.hardware;
    clockInput->parameters.control = sig_host_GATE_IN_1;
    sig_List_append(&signals, clockInput, status);
}

void InitClock(struct sig_SignalContext* context, struct sig_Status* status) {
    clockFreq = sig_dsp_ClockDetector_new(&allocator, context);
    clockFreq->inputs.source = clockInput->outputs.main;
    sig_List_append(&signals, clockFreq, status);
}

void InitLFO(struct sig_SignalContext* context, struct sig_Status* status) {
    lfo1 = sig_host_ClockedLFO_new(&allocator, context);
    lfo1->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo1, status);
    lfo1->parameters.freqScaleCVInputControl = sig_host_CV_IN_1;
    lfo1->parameters.lfoGainCVInputControl = sig_host_CV_IN_2;
    lfo1->parameters.cvOutputControl = sig_host_CV_OUT_1;
    lfo1->inputs.clockFreq = clockFreq->outputs.main;

    lfo2 = sig_host_ClockedLFO_new(&allocator, context);
    lfo2->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo2, status);
    lfo2->parameters.freqScaleCVInputControl = sig_host_CV_IN_3;
    lfo2->parameters.lfoGainCVInputControl = sig_host_CV_IN_4;
    lfo2->parameters.cvOutputControl = sig_host_CV_OUT_2;
    lfo2->inputs.clockFreq = clockFreq->outputs.main;

    lfo3 = sig_host_ClockedLFO_new(&allocator, context);
    lfo3->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo3, status);
    lfo3->inputs.clockFreq = clockFreq->outputs.main;
    lfo3->parameters.freqScaleCVInputControl = sig_host_CV_IN_5;
    lfo3->parameters.lfoGainCVInputControl = sig_host_CV_IN_6;
    lfo3->parameters.cvOutputControl = sig_host_CV_OUT_3;

    lfo4 = sig_host_ClockedLFO_new(&allocator, context);
    lfo4->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo4, status);
    lfo4->inputs.clockFreq = clockFreq->outputs.main;
    lfo4->parameters.freqScaleCVInputControl = sig_host_CV_IN_7;
    lfo4->parameters.lfoGainCVInputControl = sig_host_CV_IN_8;
    lfo4->parameters.cvOutputControl = sig_host_CV_OUT_4;

    lfo1PlusLFO3 = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, lfo1PlusLFO3, status);
    lfo1PlusLFO3->inputs.left = lfo1->outputs.main;
    lfo1PlusLFO3->inputs.right = lfo3->outputs.main;

    lfo2PlusLFO4 = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, lfo2PlusLFO4, status);
    lfo2PlusLFO4->inputs.left = lfo2->outputs.main;
    lfo2PlusLFO4->inputs.right = lfo4->outputs.main;

    lfo1And3Out = sig_host_CVOut_new(&allocator, context);
    lfo1And3Out->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo1And3Out, status);
    lfo1And3Out->inputs.source = lfo1PlusLFO3->outputs.main;
    lfo1And3Out->parameters.control = sig_host_CV_OUT_5;
    lfo1And3Out->parameters.scale = 0.5f; // TODO: Replace with a wavefolder.

    lfo2And4Out = sig_host_CVOut_new(&allocator, context);
    lfo2And4Out->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo2And4Out, status);
    lfo2And4Out->inputs.source = lfo2PlusLFO4->outputs.main;
    lfo2And4Out->parameters.control = sig_host_CV_OUT_6;
    lfo2And4Out->parameters.scale = 0.5f;
}

void InitCVOutputs(struct sig_SignalContext* context,
    struct sig_Status* status) {
    gate1Out = sig_host_GateOut_new(&allocator, context);
    gate1Out->hardware = &host.device.hardware;
    gate1Out->parameters.control = sig_host_GATE_OUT_1;
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
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 48
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    struct sig_SignalContext* context = sig_SignalContext_new(
        &allocator, &audioSettings);
    buildSignalGraph(context, &status);
    host.Start();

    while (1) {}
}
