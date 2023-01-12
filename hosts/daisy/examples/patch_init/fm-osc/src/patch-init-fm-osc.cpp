#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../include/daisy-patch-sm-host.h"

#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 64

struct sig_Status status;

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

daisy::patch_sm::DaisyPatchSM patchInit;
struct sig_daisy_Host* host;

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List* signals;

struct sig_dsp_SignalListEvaluator* evaluator;
struct sig_daisy_FilteredCVIn* coarseFrequencyKnob;
struct sig_daisy_FilteredCVIn* fineFrequencyKnob;
struct sig_daisy_CVIn* vOctCV;
struct sig_dsp_BinaryOp* coarsePlusVOct;
struct sig_dsp_BinaryOp* coarseVOctPlusFine;
struct sig_dsp_LinearToFreq* fundamentalFrequency;
struct sig_daisy_FilteredCVIn* ratioKnob;
struct sig_daisy_FilteredCVIn* ratioCV;
struct sig_dsp_BinaryOp* combinedRatio;
struct sig_dsp_BinaryOp* modulatorFrequency;
struct sig_daisy_FilteredCVIn* indexKnob;
struct sig_daisy_FilteredCVIn* indexCV;
struct sig_dsp_BinaryOp* combinedIndex;
struct sig_dsp_Oscillator* modulator;
struct sig_dsp_Oscillator* carrier;
struct sig_dsp_LinearToFreq* lfoFundamentalFrequency;
struct sig_dsp_BinaryOp* lfoModulatorFrequency;
struct sig_dsp_Oscillator* lfoModulator;
struct sig_dsp_Oscillator* lfoCarrier;
// TODO: Add a nice LPF.
struct sig_dsp_Tanh* distortion;
// TODO: Add skew controls for index/ratio for stereo separation.
struct sig_daisy_AudioOut* leftOut;
struct sig_daisy_AudioOut* rightOut;
struct sig_daisy_CVOut* eocCVOut;
struct sig_daisy_CVOut* eocLED;

void buildKnobGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {

    coarseFrequencyKnob = sig_daisy_FilteredCVIn_new(allocator,
        context, host);
    sig_List_append(signals, coarseFrequencyKnob, status);
    coarseFrequencyKnob->parameters.control = sig_daisy_PatchInit_KNOB_1;
    coarseFrequencyKnob->parameters.scale = 7.0f;
    coarseFrequencyKnob->parameters.offset = -3.5f;

    fineFrequencyKnob = sig_daisy_FilteredCVIn_new(allocator, context, host);
    sig_List_append(signals, fineFrequencyKnob, status);
    fineFrequencyKnob->parameters.control = sig_daisy_PatchInit_KNOB_3;
    fineFrequencyKnob->parameters.offset = -0.5f;

    ratioKnob = sig_daisy_FilteredCVIn_new(allocator, context, host);
    sig_List_append(signals, ratioKnob, status);
    ratioKnob->parameters.control = sig_daisy_PatchInit_KNOB_2;
    ratioKnob->parameters.scale = 2.1f;

    indexKnob = sig_daisy_FilteredCVIn_new(allocator, context, host);
    sig_List_append(signals, indexKnob, status);
    indexKnob->parameters.control = sig_daisy_PatchInit_KNOB_4;
    indexKnob->parameters.scale = 5.0f;
}

void buildCVInputGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {
    vOctCV = sig_daisy_CVIn_new(allocator, context, host);
    sig_List_append(signals, vOctCV, status);
    vOctCV->parameters.control = sig_daisy_PatchInit_CV_IN_1;
    vOctCV->parameters.scale = 2.5f;

    ratioCV = sig_daisy_FilteredCVIn_new(allocator, context, host);
    sig_List_append(signals, ratioCV, status);
    ratioCV->parameters.control = sig_daisy_PatchInit_CV_IN_2;
    ratioCV->parameters.scale = 2.1f;

    combinedRatio = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, combinedRatio, status);
    combinedRatio->inputs.left = ratioKnob->outputs.main;
    combinedRatio->inputs.right = ratioCV->outputs.main;

    indexCV = sig_daisy_FilteredCVIn_new(allocator, context, host);
    sig_List_append(signals, indexCV, status);
    indexCV->parameters.control = sig_daisy_PatchInit_CV_IN_3;
    indexCV->parameters.scale = 5.0f;

    combinedIndex = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, combinedIndex, status);
    combinedIndex->inputs.left = indexKnob->outputs.main;
    combinedIndex->inputs.right = indexCV->outputs.main;
}

void buildFrequencyGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {
    coarsePlusVOct = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, coarsePlusVOct, status);
    coarsePlusVOct->inputs.left = coarseFrequencyKnob->outputs.main;
    coarsePlusVOct->inputs.right = vOctCV->outputs.main;

    coarseVOctPlusFine = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, coarseVOctPlusFine, status);
    coarseVOctPlusFine->inputs.left = coarsePlusVOct->outputs.main;
    coarseVOctPlusFine->inputs.right = fineFrequencyKnob->outputs.main;

    fundamentalFrequency = sig_dsp_LinearToFreq_new(allocator, context);
    sig_List_append(signals, fundamentalFrequency, status);
    fundamentalFrequency->inputs.source = coarseVOctPlusFine->outputs.main;

    modulatorFrequency = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, modulatorFrequency, status);
    modulatorFrequency->inputs.left = fundamentalFrequency->outputs.main;
    modulatorFrequency->inputs.right = combinedRatio->outputs.main;
}

void buildLFOGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {

    lfoFundamentalFrequency = sig_dsp_LinearToFreq_new(allocator, context);
    sig_List_append(signals, lfoFundamentalFrequency, status);
    lfoFundamentalFrequency->inputs.source = coarseVOctPlusFine->outputs.main;
    lfoFundamentalFrequency->parameters.middleFreq = sig_FREQ_C4 / powf(2, 12); // LFO is centred twelve octaves below the audio carrier frequency.

    lfoModulatorFrequency = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, lfoModulatorFrequency, status);
    lfoModulatorFrequency->inputs.left = lfoFundamentalFrequency->outputs.main;
    lfoModulatorFrequency->inputs.right = combinedRatio->outputs.main;

    lfoModulator = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, lfoModulator, status);
    lfoModulator->inputs.freq = lfoModulatorFrequency->outputs.main;
    lfoModulator->inputs.mul = combinedIndex->outputs.main;

    lfoCarrier = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, lfoCarrier, status);
    lfoCarrier->inputs.freq = lfoFundamentalFrequency->outputs.main;
    lfoCarrier->inputs.phaseOffset = lfoModulator->outputs.main;
    lfoCarrier->inputs.mul = context->unity->outputs.main;
}

void buildOscillatorGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {
    modulator = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, modulator, status);
    modulator->inputs.freq = modulatorFrequency->outputs.main;
    modulator->inputs.mul = combinedIndex->outputs.main;

    carrier = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, carrier, status);
    carrier->inputs.freq = fundamentalFrequency->outputs.main;
    carrier->inputs.phaseOffset = modulator->outputs.main;
    carrier->inputs.mul = context->unity->outputs.main;
}

// TODO: Factor out a 2-op FM signal so that there is less
// code duplication between the audio oscillator and the LFO.
void buildSignalGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {

    buildKnobGraph(allocator, signals, context, status);
    buildCVInputGraph(allocator, signals, context, status);
    buildFrequencyGraph(allocator, signals, context, status);
    buildOscillatorGraph(allocator, signals, context, status);
    buildLFOGraph(allocator, signals, context, status);

    distortion = sig_dsp_Tanh_new(allocator, context);
    sig_List_append(signals, distortion, status);
    distortion->inputs.source = modulator->outputs.main;

    leftOut = sig_daisy_AudioOut_new(allocator, context, host);
    sig_List_append(signals, leftOut, status);
    leftOut->parameters.channel = sig_daisy_AUDIO_OUT_1;
    leftOut->inputs.source = carrier->outputs.main;

    rightOut = sig_daisy_AudioOut_new(allocator, context, host);
    sig_List_append(signals, rightOut, status);
    rightOut->parameters.channel = sig_daisy_AUDIO_OUT_2;
    rightOut->inputs.source = distortion->outputs.main;

    eocCVOut = sig_daisy_CVOut_new(allocator, context, host);
    sig_List_append(signals, eocCVOut, status);
    eocLED->parameters.control = sig_daisy_PatchInit_CV_OUT;
    eocCVOut->inputs.source = lfoCarrier->outputs.main;

    eocLED = sig_daisy_CVOut_new(allocator, context, host);
    sig_List_append(signals, eocLED, status);
    eocLED->parameters.control = sig_daisy_PatchInit_LED;
    eocLED->inputs.source = lfoCarrier->outputs.main;
}

int main(void) {
    struct sig_AudioSettings audioSettings = {
        .sampleRate = 96000,
        .numChannels = 2,
        .blockSize = 8
    };

    sig_Status_init(&status);
    allocator.impl->init(&allocator);
    signals = sig_List_new(&allocator, MAX_NUM_SIGNALS);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, signals);

    host = sig_daisy_PatchSMHost_new(&allocator,
        &audioSettings, &patchInit,
        (struct sig_dsp_SignalEvaluator*) evaluator);
    sig_daisy_Host_registerGlobalHost(host);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(&allocator, signals, context, &status);

    host->impl->start(host);

    while (1) {

    }
}
