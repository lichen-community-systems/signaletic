#include <libsignaletic.h>
#include "../../../../include/ne-versio-device.hpp"

using namespace sig::libdaisy;

#define SAMPLERATE 96000
#define HEAP_SIZE 1024 * 384 // 384 KB
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

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;
DaisyHost<ne::versio::VersioDevice> host;

struct sig_host_CVIn* freqCV;
struct sig_dsp_Oscillator* osc;
struct sig_host_CVIn* gain;
struct sig_dsp_BinaryOp* vca;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    freqCV = sig_host_CVIn_new(&allocator, context);
    freqCV->hardware = &host.device.hardware;
    sig_List_append(&signals, freqCV, status);
    freqCV->parameters.control = sig_host_CV_IN_1;
    freqCV->parameters.scale = 1760.0f;

    osc = sig_dsp_SineOscillator_new(&allocator, context);
    sig_List_append(&signals, osc, status);
    osc->inputs.freq = freqCV->outputs.main;

    gain = sig_host_CVIn_new(&allocator, context);
    gain->hardware = &host.device.hardware;
    sig_List_append(&signals, gain, status);
    gain->parameters.control = sig_host_CV_IN_2;
    gain->parameters.scale = 0.5f;
    gain->parameters.offset = 0.5f;

    vca = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, vca, status);
    vca->inputs.left = osc->outputs.main;
    vca->inputs.right = gain->outputs.main;

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = vca->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->inputs.source = vca->outputs.main;
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

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    buildSignalGraph(context, &status);

    host.Start();

    while (1) {}
}
