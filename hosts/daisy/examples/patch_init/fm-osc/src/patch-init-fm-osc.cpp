#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../include/daisy-patch-sm-host.h"

#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 64
#define NUM_RATIOS 9

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

// TODO: Tune these better.
float ratios[NUM_RATIOS] =
    {
        1.0f/5.0f, 1.0f/4.0f, 1.0f/3.0f, 1.0f/2.0f,
        1.0f, // Centre point of the knob.
        2.0f, 3.0f, 4.0f, 5.0f
    };

struct sig_Buffer ratioList = {
    .length = NUM_RATIOS,
    .samples = ratios
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
struct sig_daisy_SwitchIn* switchIn;
struct sig_dsp_Branch* ratioFreeBranch;
struct sig_daisy_FilteredCVIn* ratioKnob;
struct sig_dsp_List* ratioListSignal;
struct sig_daisy_FilteredCVIn* ratioCV;
struct sig_dsp_BinaryOp* combinedRatio;
struct sig_dsp_ConstantValue* ratioFreeScale;
struct sig_dsp_BinaryOp* ratioFree;
struct sig_daisy_FilteredCVIn* indexKnob;
struct sig_daisy_FilteredCVIn* indexCV;
struct sig_dsp_BinaryOp* combinedIndex;
struct sig_daisy_FilteredCVIn* indexSkewCV;
struct sig_dsp_Abs* rectifiedIndexSkew;
struct sig_dsp_BinaryOp* indexSkewAdder;
struct sig_dsp_Branch* leftIndex;
struct sig_dsp_TwoOpFM* leftOp;
struct sig_dsp_ConstantValue* rightOpPhaseOffset;
struct sig_dsp_Branch* rightIndex;
struct sig_dsp_TwoOpFM* rightOp;
struct sig_dsp_LinearToFreq* lfoFundamentalFrequency;
struct sig_dsp_TwoOpFM* lfo;
struct sig_daisy_AudioOut* leftOut;
struct sig_daisy_AudioOut* rightOut;
struct sig_daisy_CVOut* eocCVOut;
struct sig_daisy_CVOut* eocLED;

void buildControlGraph(struct sig_Allocator* allocator,
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

    switchIn = sig_daisy_SwitchIn_new(allocator, context, host);
    sig_List_append(signals, switchIn, status);
    switchIn->parameters.control = sig_daisy_PatchInit_TOGGLE;

    ratioKnob = sig_daisy_FilteredCVIn_new(allocator, context, host);
    sig_List_append(signals, ratioKnob, status);
    ratioKnob->parameters.control = sig_daisy_PatchInit_KNOB_2;

    indexKnob = sig_daisy_FilteredCVIn_new(allocator, context, host);
    sig_List_append(signals, indexKnob, status);
    indexKnob->parameters.control = sig_daisy_PatchInit_KNOB_4;
    indexKnob->parameters.scale = 5.0f;
    indexKnob->parameters.time = 0.01f;
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

    combinedRatio = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, combinedRatio, status);
    combinedRatio->inputs.left = ratioKnob->outputs.main;
    combinedRatio->inputs.right = ratioCV->outputs.main;

    ratioListSignal = sig_dsp_List_new(allocator, context);
    sig_List_append(signals, ratioListSignal, status);
    ratioListSignal->list = &ratioList;
    ratioListSignal->inputs.index = combinedRatio->outputs.main;

    // In free mode we scale the ratio by 2.1f (tuned by ear)
    // for more impact.
    ratioFreeScale = sig_dsp_ConstantValue_new(allocator, context, 2.1f);

    ratioFree = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, ratioFree, status);
    ratioFree->inputs.left = combinedRatio->outputs.main;
    ratioFree->inputs.right = ratioFreeScale->outputs.main;

    ratioFreeBranch = sig_dsp_Branch_new(allocator, context);
    sig_List_append(signals, ratioFreeBranch, status);
    ratioFreeBranch->inputs.condition = switchIn->outputs.main;
    ratioFreeBranch->inputs.off = ratioFree->outputs.main;
    ratioFreeBranch->inputs.on = ratioListSignal->outputs.main;

    indexCV = sig_daisy_FilteredCVIn_new(allocator, context, host);
    sig_List_append(signals, indexCV, status);
    indexCV->parameters.control = sig_daisy_PatchInit_CV_IN_3;
    indexCV->parameters.scale = 5.0f;

    combinedIndex = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, combinedIndex, status);
    combinedIndex->inputs.left = indexKnob->outputs.main;
    combinedIndex->inputs.right = indexCV->outputs.main;

    indexSkewCV = sig_daisy_FilteredCVIn_new(allocator, context, host);
    sig_List_append(signals, indexSkewCV, status);
    indexSkewCV->parameters.control = sig_daisy_PatchInit_CV_IN_4;
    indexSkewCV->parameters.scale = 1.5f;

    rectifiedIndexSkew = sig_dsp_Abs_new(allocator, context);
    sig_List_append(signals, rectifiedIndexSkew, status);
    rectifiedIndexSkew->inputs.source = indexSkewCV->outputs.main;

    indexSkewAdder = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, indexSkewAdder, status);
    indexSkewAdder->inputs.left = combinedIndex->outputs.main;
    indexSkewAdder->inputs.right = rectifiedIndexSkew->outputs.main;
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
}

void buildSignalGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {

    buildControlGraph(allocator, signals, context, status);
    buildCVInputGraph(allocator, signals, context, status);
    buildFrequencyGraph(allocator, signals, context, status);

    leftIndex = sig_dsp_Branch_new(allocator, context);
    sig_List_append(signals, leftIndex, status);
    leftIndex->inputs.condition = indexSkewCV->outputs.main;
    leftIndex->inputs.on = combinedIndex->outputs.main;
    leftIndex->inputs.off = indexSkewAdder->outputs.main;

    leftOp = sig_dsp_TwoOpFM_new(allocator, context);
    sig_List_append(signals, leftOp, status);
    leftOp->inputs.frequency = fundamentalFrequency->outputs.main;
    leftOp->inputs.ratio = ratioFreeBranch->outputs.main;
    leftOp->inputs.index = leftIndex->outputs.main;

    rightOpPhaseOffset = sig_dsp_ConstantValue_new(allocator, context, 0.25f);

    rightIndex = sig_dsp_Branch_new(allocator, context);
    sig_List_append(signals, rightIndex, status);
    rightIndex->inputs.condition = indexSkewCV->outputs.main;
    rightIndex->inputs.on = indexSkewAdder->outputs.main;
    rightIndex->inputs.off = combinedIndex->outputs.main;

    rightOp = sig_dsp_TwoOpFM_new(allocator, context);
    sig_List_append(signals, rightOp, status);
    rightOp->inputs.frequency = fundamentalFrequency->outputs.main;
    rightOp->inputs.ratio = ratioFreeBranch->outputs.main;
    rightOp->inputs.index = rightIndex->outputs.main;
    rightOp->inputs.phaseOffset = rightOpPhaseOffset->outputs.main;

    lfoFundamentalFrequency = sig_dsp_LinearToFreq_new(allocator, context);
    sig_List_append(signals, lfoFundamentalFrequency, status);
    lfoFundamentalFrequency->inputs.source = coarseVOctPlusFine->outputs.main;
    lfoFundamentalFrequency->parameters.middleFreq = sig_FREQ_C4 /
        powf(2, 12); // LFO is centred twelve octaves below
                     // the audio carrier frequency.

    lfo = sig_dsp_TwoOpFM_new(allocator, context);
    sig_List_append(signals, lfo, status);
    lfo->inputs.frequency = lfoFundamentalFrequency->outputs.main;
    lfo->inputs.ratio = ratioFreeBranch->outputs.main;
    lfo->inputs.index = combinedIndex->outputs.main;

    leftOut = sig_daisy_AudioOut_new(allocator, context, host);
    sig_List_append(signals, leftOut, status);
    leftOut->parameters.channel = sig_daisy_AUDIO_OUT_1;
    leftOut->inputs.source = leftOp->outputs.main;

    rightOut = sig_daisy_AudioOut_new(allocator, context, host);
    sig_List_append(signals, rightOut, status);
    rightOut->parameters.channel = sig_daisy_AUDIO_OUT_2;
    rightOut->inputs.source = rightOp->outputs.main;

    eocCVOut = sig_daisy_CVOut_new(allocator, context, host);
    sig_List_append(signals, eocCVOut, status);
    eocLED->parameters.control = sig_daisy_PatchInit_CV_OUT;
    eocCVOut->inputs.source = lfo->outputs.main;

    eocLED = sig_daisy_CVOut_new(allocator, context, host);
    sig_List_append(signals, eocLED, status);
    eocLED->parameters.control = sig_daisy_PatchInit_LED;
    eocLED->inputs.source = lfo->outputs.main;
}

int main(void) {
    struct sig_AudioSettings audioSettings = {
        .sampleRate = 96000,
        .numChannels = 2,
        .blockSize = 64
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
