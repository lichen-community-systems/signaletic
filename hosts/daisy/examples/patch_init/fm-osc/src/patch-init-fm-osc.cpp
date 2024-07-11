#include <libsignaletic.h>
#include "../../../../include/electrosmith-patch-init-device.hpp"

#define SAMPLERATE 96000
#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 64
#define NUM_RATIOS 33

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

float ratios[NUM_RATIOS] = {
    // From Truax's normal form ratios
    // ordered to the Farey Sequence.
    // https://www.sfu.ca/sonic-studio-webdav/handbook/fmtut.html
    1.0f/11.0f, 1.0f/10.0f, 1.0f/9.0f, 1.0f/8.0f,
    1.0f/7.0f, 1.0f/6.0f, 1.0f/5.0f,
    2.0f/9.0f, 1.0f/4.0f, 2.0f/7.0f,
    1.0f/3.0f, 3.0f/8.0f, 2.0f/5.0f,
    3.0/7.0f, 4.0f/9.0f, 1.0f/2.0f,
    1.0f,
    2.0f/1.0f, 9.0f/4.0f, 7.0f/3.0f,
    5.0f/2.0f, 8.0f/3.0f, 3.0f/1.0f,
    7.0f/2.0f, 4.0f/1.0f, 9.0f/2.0f,
    5.0f/1.0f, 6.0f/1.0f, 7.0f/1.0f,
    8.0f/1.0f, 9.0f/1.0f, 10.0f/1.0f, 11.0f/1.0f
};

struct sig_Buffer ratioList = {
    .length = NUM_RATIOS,
    .samples = ratios
};

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;
sig::libdaisy::DaisyHost<electrosmith::patchinit::PatchInitDevice> host;

struct sig_host_FilteredCVIn* coarseFrequencyKnob;
struct sig_host_FilteredCVIn* fineFrequencyKnob;
struct sig_host_SwitchIn* button;
struct sig_host_CVIn* vOctCV;
struct sig_dsp_Calibrator* vOctCalibrator;
struct sig_dsp_BinaryOp* coarsePlusVOct;
struct sig_dsp_BinaryOp* coarseVOctPlusFine;
struct sig_dsp_LinearToFreq* fundamentalFrequency;
struct sig_host_SwitchIn* switchIn;
struct sig_dsp_Branch* ratioFreeBranch;
struct sig_host_FilteredCVIn* ratioKnob;
struct sig_dsp_List* ratioListSignal;
struct sig_host_FilteredCVIn* ratioCV;
struct sig_dsp_BinaryOp* combinedRatio;
struct sig_dsp_ConstantValue* ratioFreeScaleValue;
struct sig_dsp_BinaryOp* ratioFreeScale;
struct sig_dsp_ConstantValue* ratioFreeOffsetValue;
struct sig_dsp_BinaryOp* ratioFreeOffset;
struct sig_dsp_LinearToFreq* modulatorFreeFrequency;
struct sig_dsp_BinaryOp* ratioFree;
struct sig_host_FilteredCVIn* indexKnob;
struct sig_host_FilteredCVIn* indexCV;
struct sig_dsp_BinaryOp* combinedIndex;
struct sig_host_FilteredCVIn* indexSkewCV;
struct sig_dsp_Abs* rectifiedIndexSkew;
struct sig_dsp_BinaryOp* indexSkewAdder;
struct sig_dsp_Branch* leftIndex;
struct sig_dsp_TwoOpFM* leftOp;
struct sig_dsp_ConstantValue* rightOpPhaseOffset;
struct sig_dsp_Branch* rightIndex;
struct sig_dsp_TwoOpFM* rightOp;
struct sig_dsp_LinearToFreq* lfoFundamentalFrequency;
struct sig_dsp_TwoOpFM* lfo;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;
struct sig_host_CVOut* lfoCVOut;
struct sig_host_CVOut* lfoLEDOut;

void buildControlGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {
    coarseFrequencyKnob = sig_host_FilteredCVIn_new(allocator, context);
    coarseFrequencyKnob->hardware = &host.device.hardware;
    sig_List_append(signals, coarseFrequencyKnob, status);
    coarseFrequencyKnob->parameters.control = sig_host_KNOB_1;
    coarseFrequencyKnob->parameters.scale = 5.0f;
    coarseFrequencyKnob->parameters.offset = -2.5f;

    fineFrequencyKnob = sig_host_FilteredCVIn_new(allocator, context);
    fineFrequencyKnob->hardware = &host.device.hardware;
    sig_List_append(signals, fineFrequencyKnob, status);
    fineFrequencyKnob->parameters.control = sig_host_KNOB_3;
    fineFrequencyKnob->parameters.offset = -0.5f;

    switchIn = sig_host_SwitchIn_new(allocator, context);
    switchIn->hardware = &host.device.hardware;
    sig_List_append(signals, switchIn, status);
    switchIn->parameters.control = sig_host_TOGGLE_2;

    ratioKnob = sig_host_FilteredCVIn_new(allocator, context);
    ratioKnob->hardware = &host.device.hardware;
    sig_List_append(signals, ratioKnob, status);
    ratioKnob->parameters.control = sig_host_KNOB_2;
    ratioKnob->parameters.scale = 0.5f;

    indexKnob = sig_host_FilteredCVIn_new(allocator, context);
    indexKnob->hardware = &host.device.hardware;
    sig_List_append(signals, indexKnob, status);
    indexKnob->parameters.control = sig_host_KNOB_4;

    button = sig_host_SwitchIn_new(allocator, context);
    button->hardware = &host.device.hardware;
    sig_List_append(signals, button, status);
    button->parameters.control = sig_host_TOGGLE_1;
}

void buildCVInputGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {
    vOctCV = sig_host_CVIn_new(allocator, context);
    vOctCV->hardware = &host.device.hardware;
    sig_List_append(signals, vOctCV, status);
    vOctCV->parameters.control = sig_host_CV_IN_1;
    vOctCV->parameters.scale = 5.0f;

    vOctCalibrator = sig_dsp_Calibrator_new(allocator, context);
    sig_List_append(signals, vOctCalibrator, status);
    vOctCalibrator->inputs.source = vOctCV->outputs.main;
    vOctCalibrator->inputs.gate = button->outputs.main;

    ratioCV = sig_host_FilteredCVIn_new(allocator, context);
    ratioCV->hardware = &host.device.hardware;
    sig_List_append(signals, ratioCV, status);
    ratioCV->parameters.control = sig_host_CV_IN_2;
    ratioCV->parameters.scale = 0.25f;
    ratioCV->parameters.offset = 0.25f;

    combinedRatio = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, combinedRatio, status);
    combinedRatio->inputs.left = ratioKnob->outputs.main;
    combinedRatio->inputs.right = ratioCV->outputs.main;

    ratioListSignal = sig_dsp_List_new(allocator, context);
    sig_List_append(signals, ratioListSignal, status);
    ratioListSignal->list = &ratioList;
    ratioListSignal->inputs.index = combinedRatio->outputs.main;

    indexCV = sig_host_FilteredCVIn_new(allocator, context);
    indexCV->hardware = &host.device.hardware;
    sig_List_append(signals, indexCV, status);
    indexCV->parameters.control = sig_host_CV_IN_3;

    combinedIndex = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, combinedIndex, status);
    combinedIndex->inputs.left = indexKnob->outputs.main;
    combinedIndex->inputs.right = indexCV->outputs.main;

    indexSkewCV = sig_host_FilteredCVIn_new(allocator, context);
    indexSkewCV->hardware = &host.device.hardware;
    sig_List_append(signals, indexSkewCV, status);
    indexSkewCV->parameters.control = sig_host_CV_IN_4;
    indexSkewCV->parameters.scale = 1.125f; // Tuned by ear for subtlety.

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
    coarsePlusVOct->inputs.right = vOctCalibrator->outputs.main;

    coarseVOctPlusFine = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, coarseVOctPlusFine, status);
    coarseVOctPlusFine->inputs.left = coarsePlusVOct->outputs.main;
    coarseVOctPlusFine->inputs.right = fineFrequencyKnob->outputs.main;

    fundamentalFrequency = sig_dsp_LinearToFreq_new(allocator, context);
    sig_List_append(signals, fundamentalFrequency, status);
    fundamentalFrequency->inputs.source = coarseVOctPlusFine->outputs.main;

    ratioFreeScaleValue = sig_dsp_ConstantValue_new(allocator, context, 15.0f);
    ratioFreeScale = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, ratioFreeScale, status);
    ratioFreeScale->inputs.left = combinedRatio->outputs.main;
    ratioFreeScale->inputs.right = ratioFreeScaleValue->outputs.main;

    ratioFreeOffsetValue = sig_dsp_ConstantValue_new(allocator, context, -8.5f);
    ratioFreeOffset = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, ratioFreeOffset, status);
    ratioFreeOffset->inputs.left = ratioFreeScale->outputs.main;
    ratioFreeOffset->inputs.right = ratioFreeOffsetValue->outputs.main;

    modulatorFreeFrequency = sig_dsp_LinearToFreq_new(allocator, context);
    sig_List_append(signals, modulatorFreeFrequency, status);
    modulatorFreeFrequency->inputs.source = ratioFreeOffset->outputs.main;

    // TODO: Remove this extra division by adding a way to
    // directly address the modulator's frequency in free mode.
    ratioFree = sig_dsp_Div_new(allocator, context);
    sig_List_append(signals, ratioFree, status);
    ratioFree->inputs.left = modulatorFreeFrequency->outputs.main;
    ratioFree->inputs.right = fundamentalFrequency->outputs.main;

    ratioFreeBranch = sig_dsp_Branch_new(allocator, context);
    sig_List_append(signals, ratioFreeBranch, status);
    ratioFreeBranch->inputs.condition = switchIn->outputs.main;
    ratioFreeBranch->inputs.off = ratioFree->outputs.main;
    ratioFreeBranch->inputs.on = ratioListSignal->outputs.main;
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

    leftOut = sig_host_AudioOut_new(allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->parameters.scale = 0.9f; // PatchSM DAC is a just a touch hot.
    leftOut->inputs.source = leftOp->outputs.main;

    rightOut = sig_host_AudioOut_new(allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->parameters.scale = 0.9f;
    rightOut->inputs.source = rightOp->outputs.main;

    lfoCVOut = sig_host_CVOut_new(allocator, context);
    lfoCVOut->hardware = &host.device.hardware;
    sig_List_append(signals, lfoCVOut, status);
    lfoCVOut->parameters.control = sig_host_CV_OUT_1;
    lfoCVOut->inputs.source = lfo->outputs.main;

    lfoLEDOut = sig_host_CVOut_new(allocator, context);
    lfoLEDOut->hardware = &host.device.hardware;
    sig_List_append(signals, lfoLEDOut, status);
    lfoLEDOut->parameters.control = sig_host_CV_OUT_2;
    lfoLEDOut->inputs.source = lfo->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 128
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    buildSignalGraph(&allocator, &signals, context, &status);

    host.Start();

    while (1) {}
}
