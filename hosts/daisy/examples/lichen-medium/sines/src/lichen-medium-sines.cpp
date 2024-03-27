#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../include/lichen-medium-module.h"

#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 32

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

DaisyHost<lichen::medium::MediumDevice> host;

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;

struct sig_dsp_ConstantValue* ampScale;
struct sig_host_CVIn* knobIn;
struct sig_host_CVIn* cvIn;
struct sig_dsp_LinearToFreq* vOct;
struct sig_dsp_Oscillator* sine;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void buildGraph(struct sig_SignalContext* context, struct sig_Status* status) {
    ampScale = sig_dsp_ConstantValue_new(&allocator, context, 0.75f);

    knobIn = sig_host_CVIn_new(&allocator, context);
    knobIn->hardware = &host.device.hardware;
    sig_List_append(&signals, knobIn, status);
    knobIn->parameters.control = sig_host_KNOB_6;
    knobIn->parameters.scale = 5.0f;

    vOct = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, vOct, status);
    vOct->inputs.source = knobIn->outputs.main;

    sine = sig_dsp_Sine_new(&allocator, context);
    sig_List_append(&signals, sine, status);
    sine->inputs.freq = vOct->outputs.main;
    sine->inputs.mul = ampScale->outputs.main;

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = sine->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->inputs.source = sine->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 48000,
        .numChannels = 2,
        .blockSize = 1
    };

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);
    buildGraph(context, &status);
    host.Start();

    while (1) {}
}
