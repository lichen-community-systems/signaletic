#include <libsignaletic.h>
#include "../../../shared/include/clocked-lfo.h"
#include "../../../../include/dspcoffee-dpt-device.hpp"

#define SAMPLERATE 48000
#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 128
#define NUM_LFOS 4
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
struct sig_host_GateIn* clockIn;
struct sig_host_CVIn* lfoFrequency[NUM_LFOS];
struct sig_host_CVIn* lfoScale[NUM_LFOS];
struct sig_dsp_ClockedLFO* lfo[NUM_LFOS];
struct sig_host_CVOut* out[NUM_LFOS];
struct sig_dsp_BinaryOp* lfo1PlusLFO3;
struct sig_dsp_BinaryOp* lfo2PlusLFO4;
struct sig_host_CVOut* lfo1And3Out;
struct sig_host_CVOut* lfo2And4Out;
struct sig_host_GateOut* gate1Out;

struct sig_host_CVIn* makeCVInput(struct sig_SignalContext* context,
    struct sig_Status* status, uint8_t control, float scale, float offset) {
    struct sig_host_CVIn* cvInput = sig_host_CVIn_new(&allocator, context);
    cvInput->hardware = &host.device.hardware;
    sig_List_append(&signals, cvInput, status);
    cvInput->parameters.control = control;
    cvInput->parameters.scale = scale;
    cvInput->parameters.offset = offset;

    return cvInput;
}

void makeCVInputs(struct sig_SignalContext* context,
    struct sig_Status* status, struct sig_host_CVIn** cvInputs, uint8_t* controls, size_t numCVInputs, float scale, float offset) {
    for (size_t i = 0; i < numCVInputs; i++) {
        cvInputs[i] = makeCVInput(context, status, controls[i], scale, offset);
    }
}

struct sig_dsp_ClockedLFO* makeLFO(struct sig_SignalContext* context,
    struct sig_Status* status,
    struct sig_host_CVIn* frequencyInput,
    struct sig_host_CVIn* scaleInput) {
    struct sig_dsp_ClockedLFO* lfo = sig_dsp_ClockedLFO_new(&allocator,
        context);
    sig_List_append(&signals, lfo, status);
    lfo->inputs.clock = clockIn->outputs.main;
    lfo->inputs.frequencyScale = frequencyInput->outputs.main;
    lfo->inputs.scale = scaleInput->outputs.main;

    return lfo;
}

void makeLFOs(struct sig_SignalContext* context,
    struct sig_Status* status, struct sig_dsp_ClockedLFO** lfos,
    struct sig_host_CVIn** frequencyInputs,
    struct sig_host_CVIn** scaleInputs, size_t numLFOs) {
    for (size_t i = 0; i < numLFOs; i++) {
        lfos[i] = makeLFO(context, status, frequencyInputs[i], scaleInputs[i]);
    }
}

struct sig_host_CVOut* makeCVOutput(struct sig_SignalContext* context,
    struct sig_Status* status, uint8_t control,
    struct sig_dsp_ClockedLFO* lfo) {
    struct sig_host_CVOut* output = sig_host_CVOut_new(&allocator, context);
    output->hardware = &host.device.hardware;
    sig_List_append(&signals, output, status);
    output->parameters.control = control;
    output->inputs.source = lfo->outputs.main;

    return output;
}

void makeCVOutputs(struct sig_SignalContext* context,
    struct sig_Status* status, struct sig_host_CVOut** outputs,
    struct sig_dsp_ClockedLFO** lfos,
    uint8_t* controls, size_t numOutputs) {
    for (size_t i = 0; i < numOutputs; i++) {
        outputs[i] = makeCVOutput(context, status, controls[i], lfos[i]);
    }
}

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    clockIn = sig_host_GateIn_new(&allocator, context);
    sig_List_append(&signals, clockIn, status);
    clockIn->hardware = &host.device.hardware;
    clockIn->parameters.control = sig_host_GATE_IN_1;

    uint8_t frequencyControls[NUM_LFOS] = {
        sig_host_CV_IN_1, sig_host_CV_IN_3,
        sig_host_CV_IN_5, sig_host_CV_IN_7
    };
    makeCVInputs(context, status, lfoFrequency, frequencyControls,
        NUM_LFOS, 10.0f, 0.0f);


    uint8_t scaleControls[NUM_LFOS] = {
        sig_host_CV_IN_2, sig_host_CV_IN_4,
        sig_host_CV_IN_6, sig_host_CV_IN_8
    };
    makeCVInputs(context, status, lfoScale, scaleControls,
        NUM_LFOS, 1.0f, 0.0f);
    makeLFOs(context, status, lfo, lfoFrequency, lfoScale, NUM_LFOS);

    lfo1PlusLFO3 = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, lfo1PlusLFO3, status);
    lfo1PlusLFO3->inputs.left = lfo[0]->outputs.main;
    lfo1PlusLFO3->inputs.right = lfo[2]->outputs.main;

    lfo2PlusLFO4 = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, lfo2PlusLFO4, status);
    lfo2PlusLFO4->inputs.left = lfo[1]->outputs.main;
    lfo2PlusLFO4->inputs.right = lfo[3]->outputs.main;

    uint8_t outputControls[NUM_LFOS] = {
        sig_host_CV_OUT_1, sig_host_CV_OUT_2,
        sig_host_CV_OUT_3, sig_host_CV_OUT_4
    };
    makeCVOutputs(context, status, out, lfo, outputControls, NUM_LFOS);

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

    gate1Out = sig_host_GateOut_new(&allocator, context);
    gate1Out->hardware = &host.device.hardware;
    gate1Out->parameters.control = sig_host_GATE_OUT_1;
    gate1Out->inputs.source = clockIn->outputs.main;
    sig_List_append(&signals, gate1Out, status);
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
