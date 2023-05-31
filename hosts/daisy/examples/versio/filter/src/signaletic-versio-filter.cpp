#include <string>
#include <tlsf.h>
#include <libsignaletic.h>
#include "../../../../include/daisy-versio-host.h"

using namespace daisy;

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

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;

DaisyVersio versio;
struct sig_daisy_Host* host;

struct sig_daisy_FilteredCVIn* frequencyCV;
struct sig_daisy_FilteredCVIn* resonance;
// struct sig_daisy_FilteredCVIn* frequencySkewCV;
struct sig_daisy_FilteredCVIn* a;
struct sig_daisy_FilteredCVIn* b;
struct sig_daisy_FilteredCVIn* c;
struct sig_daisy_FilteredCVIn* d;
struct sig_daisy_FilteredCVIn* e;

// struct sig_dsp_Abs* rectifiedSkew;
// struct sig_dsp_BinaryOp* frequencyCVSkewAdder;
// struct sig_dsp_Branch* leftFrequencyCVSkewed;
struct sig_dsp_LinearToFreq* leftFrequency;
// struct sig_dsp_Branch* rightFrequencyCVSkewed;
struct sig_dsp_LinearToFreq* rightFrequency;
struct sig_daisy_AudioIn* leftIn;
struct sig_daisy_AudioIn* rightIn;
struct sig_dsp_Ladder* leftFilter;
struct sig_dsp_Ladder* rightFilter;
struct sig_dsp_Tanh* leftSaturation;
struct sig_dsp_Tanh* rightSaturation;
struct sig_daisy_AudioOut* leftOut;
struct sig_daisy_AudioOut* rightOut;


void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    // Versio controls are all unipolar,
    // so they need to be scaled to bipolar values.
    frequencyCV = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, frequencyCV, status);
    frequencyCV->parameters.control = sig_daisy_Versio_CV_IN_1;
    frequencyCV->parameters.scale = 10.0f;
    frequencyCV->parameters.offset = -5.0f;
    frequencyCV->parameters.time = 0.1f;

    // frequencySkewCV = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    // sig_List_append(&signals, frequencySkewCV, status);
    // frequencySkewCV->parameters.control = sig_daisy_Versio_CV_IN_3;
    // frequencySkewCV->parameters.scale = 5.0f;
    // frequencySkewCV->parameters.offset = -2.5f;
    // frequencySkewCV->parameters.time = 0.1f;

    // rectifiedSkew = sig_dsp_Abs_new(&allocator, context);
    // sig_List_append(&signals, rectifiedSkew, status);
    // rectifiedSkew->inputs.source = frequencySkewCV->outputs.main;

    // frequencyCVSkewAdder = sig_dsp_Add_new(&allocator, context);
    // sig_List_append(&signals, frequencyCVSkewAdder, status);
    // frequencyCVSkewAdder->inputs.left = frequencyCV->outputs.main;
    // frequencyCVSkewAdder->inputs.right = rectifiedSkew->outputs.main;

    // leftFrequencyCVSkewed = sig_dsp_Branch_new(&allocator, context);
    // sig_List_append(&signals, leftFrequencyCVSkewed, status);
    // leftFrequencyCVSkewed->inputs.condition = frequencySkewCV->outputs.main;
    // leftFrequencyCVSkewed->inputs.on = frequencyCV->outputs.main;
    // leftFrequencyCVSkewed->inputs.off = frequencyCVSkewAdder->outputs.main;

    leftFrequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, leftFrequency, status);
    leftFrequency->inputs.source = frequencyCV->outputs.main;

    // rightFrequencyCVSkewed = sig_dsp_Branch_new(&allocator, context);
    // sig_List_append(&signals, rightFrequencyCVSkewed, status);
    // rightFrequencyCVSkewed->inputs.condition = frequencySkewCV->outputs.main;
    // rightFrequencyCVSkewed->inputs.on = frequencyCVSkewAdder->outputs.main;
    // rightFrequencyCVSkewed->inputs.off = frequencyCV->outputs.main;

    rightFrequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, rightFrequency, status);
    rightFrequency->inputs.source = frequencyCV->outputs.main;

    resonance = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, resonance, status);
    resonance->parameters.control = sig_daisy_Versio_CV_IN_2;
    resonance->parameters.scale = 1.8f;

    a = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, a, status);
    a->parameters.control = sig_daisy_Versio_CV_IN_3;
    a->parameters.time = 0.1f;

    b = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, b, status);
    b->parameters.control = sig_daisy_Versio_CV_IN_4;
    b->parameters.scale = 6.0f;
    b->parameters.offset = -4.0f;
    b->parameters.time = 0.1f;

    c = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, c, status);
    c->parameters.control = sig_daisy_Versio_CV_IN_5;
    c->parameters.scale = 8.0f;
    c->parameters.offset = -2.0f;
    c->parameters.time = 0.1f;

    d = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, d, status);
    d->parameters.control = sig_daisy_Versio_CV_IN_6;
    d->parameters.scale = 8.0f;
    d->parameters.offset = -8.0f;
    d->parameters.time = 0.1f;

    e = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, e, status);
    e->parameters.control = sig_daisy_Versio_CV_IN_7;
    e->parameters.scale = 4.0f;
    e->parameters.time = 0.1f;

    leftIn = sig_daisy_AudioIn_new(&allocator, context, host);
    sig_List_append(&signals, leftIn, status);
    leftIn->parameters.channel = 0;

    leftFilter = sig_dsp_Ladder_new(&allocator, context);
    sig_List_append(&signals, leftFilter, status);
    leftFilter->parameters.overdrive = 1.1f;
    leftFilter->parameters.passbandGain = 0.5f;
    leftFilter->inputs.source = leftIn->outputs.main;
    leftFilter->inputs.frequency = leftFrequency->outputs.main;
    leftFilter->inputs.resonance = resonance->outputs.main;
    leftFilter->inputs.inputMix = a->outputs.main;
    leftFilter->inputs.stage1Mix = b->outputs.main;
    leftFilter->inputs.stage2Mix = c->outputs.main;
    leftFilter->inputs.stage3Mix = d->outputs.main;
    leftFilter->inputs.stage4Mix = e->outputs.main;

    leftSaturation = sig_dsp_Tanh_new(&allocator, context);
    sig_List_append(&signals, leftSaturation, status);
    leftSaturation->inputs.source = leftFilter->outputs.main;

    leftOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = 0;
    leftOut->inputs.source = leftSaturation->outputs.main;

    rightIn = sig_daisy_AudioIn_new(&allocator, context, host);
    sig_List_append(&signals, rightIn, status);
    rightIn->parameters.channel = 1;

    rightFilter = sig_dsp_Ladder_new(&allocator, context);
    sig_List_append(&signals, rightFilter, status);
    rightFilter->parameters.overdrive = 1.1f;
    leftFilter->parameters.passbandGain = 0.5f;
    rightFilter->inputs.source = rightIn->outputs.main;
    rightFilter->inputs.frequency = rightFrequency->outputs.main;
    rightFilter->inputs.resonance = resonance->outputs.main;
    rightFilter->inputs.inputMix = a->outputs.main;
    rightFilter->inputs.stage1Mix = b->outputs.main;
    rightFilter->inputs.stage2Mix = c->outputs.main;
    rightFilter->inputs.stage3Mix = d->outputs.main;
    rightFilter->inputs.stage4Mix = e->outputs.main;

    rightSaturation = sig_dsp_Tanh_new(&allocator, context);
    sig_List_append(&signals, rightSaturation, status);
    rightSaturation->inputs.source = rightFilter->outputs.main;

    rightOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = 1;
    rightOut->inputs.source = rightSaturation->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 96000,
        .numChannels = 2,
        .blockSize = 16
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host = sig_daisy_VersioHost_new(&allocator, &audioSettings, &versio,
        (struct sig_dsp_SignalEvaluator*) evaluator);
    sig_daisy_Host_registerGlobalHost(host);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(context, &status);
    host->impl->start(host);

    while (1) {
    }
}
