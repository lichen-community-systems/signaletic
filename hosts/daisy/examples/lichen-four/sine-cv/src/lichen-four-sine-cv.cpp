#include <libsignaletic.h>
#include "../../../../include/lichen-four-device.hpp"

using namespace lichen::four;
using namespace sig::libdaisy;

#define SAMPLERATE 48000
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
DaisyHost<FourDevice> host;

struct sig_host_FilteredCVIn* lfo2Frequency;
struct sig_dsp_ConstantValue* lfo3Frequency;
struct sig_dsp_FastLFSineOscillator* lfo2;
struct sig_dsp_FastLFSineOscillator* lfo3;
struct sig_host_CVOut* out2;
struct sig_host_CVOut* out3;

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    lfo2Frequency = sig_host_FilteredCVIn_new(&allocator, context);
    lfo2Frequency->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo2Frequency, status);
    lfo2Frequency->parameters.control = sig_host_KNOB_2;
    lfo2Frequency->parameters.scale = 10.0f;

    lfo3Frequency = sig_dsp_ConstantValue_new(&allocator, context, 2.0f);

    lfo2 = sig_dsp_FastLFSineOscillator_new(&allocator, context);
    sig_List_append(&signals, lfo2, status);
    lfo2->inputs.frequency = lfo2Frequency->outputs.main;

    out2 = sig_host_CVOut_new(&allocator, context);
    out2->hardware = &host.device.hardware;
    sig_List_append(&signals, out2, status);
    out2->parameters.control = sig_host_CV_OUT_1;
    out2->inputs.source = lfo2->outputs.main;

    lfo3 = sig_dsp_FastLFSineOscillator_new(&allocator, context);
    sig_List_append(&signals, lfo3, status);
    lfo3->inputs.frequency = lfo3Frequency->outputs.main;

    out3 = sig_host_CVOut_new(&allocator, context);
    out3->hardware = &host.device.hardware;
    sig_List_append(&signals, out3, status);
    out3->parameters.control = sig_host_CV_OUT_2;
    out3->inputs.source = lfo3->outputs.main;
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

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    buildSignalGraph(context, &status);

    host.Start();

    while (1) {}
}
