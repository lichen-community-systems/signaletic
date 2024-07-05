#include "../../../../include/dspcoffee-dpt-device.hpp"

#define SAMPLERATE 96000
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
struct sig_host_CVIn* frequency;
struct sig_host_CVIn* triangleGain;
struct sig_dsp_Oscillator* triangle;
struct sig_dsp_BinaryOp* scaledTriangle;
struct sig_host_CVOut* cv1Out;
struct sig_host_CVOut* cv3Out;
float cv1Value;
float cv2Value;

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    frequency = sig_host_CVIn_new(&allocator, context);
    frequency->hardware = &host.device.hardware;
    sig_List_append(&signals, frequency, status);
    frequency->parameters.control = sig_host_CV_IN_1;
    frequency->parameters.scale = 110.0f;

    triangle = sig_dsp_LFTriangle_new(&allocator, context);
    sig_List_append(&signals, triangle, status);
    triangle->inputs.freq = frequency->outputs.main;

    triangleGain =sig_host_CVIn_new(&allocator, context);        triangleGain->hardware = &host.device.hardware;
    sig_List_append(&signals, triangleGain, status);
    triangleGain->parameters.control = sig_host_CV_IN_2;

    scaledTriangle= sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, scaledTriangle, status);
    scaledTriangle->inputs.left = triangle->outputs.main;
    scaledTriangle->inputs.right = triangleGain->outputs.main;

    cv1Out = sig_host_CVOut_new(&allocator, context);
    cv1Out->hardware = &host.device.hardware;
    sig_List_append(&signals, cv1Out, status);
    cv1Out->inputs.source = scaledTriangle->outputs.main;
    cv1Out->parameters.control = sig_host_CV_OUT_1;

    cv3Out = sig_host_CVOut_new(&allocator, context);
    cv3Out->hardware = &host.device.hardware;
    sig_List_append(&signals, cv3Out, status);
    cv3Out->inputs.source = scaledTriangle->outputs.main;
    cv3Out->parameters.control = sig_host_CV_OUT_3;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 1
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
