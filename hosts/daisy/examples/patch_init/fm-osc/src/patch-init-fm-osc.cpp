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
struct sig_dsp_ConstantValue* smoothCoefficient;
struct sig_daisy_CVIn* coarseFrequencyKnob;
struct sig_dsp_OnePole* coarseFrequencyLPF;
struct sig_daisy_CVIn* fineFrequencyKnob;
struct sig_dsp_OnePole* fineFrequencyLPF;
struct sig_daisy_CVIn* vOctCV;
struct sig_dsp_BinaryOp* coarsePlusVOct;
struct sig_dsp_BinaryOp* coarseVOctPlusFine;
struct sig_dsp_LinearToFreq* fundamentalFrequency;
struct sig_daisy_CVIn* ratioKnob;
struct sig_dsp_BinaryOp* modulatorFrequency;
struct sig_daisy_CVIn* indexKnob;
struct sig_dsp_BinaryOp* deviation;
struct sig_dsp_Oscillator* modulator;
struct sig_dsp_BinaryOp* carrierFrequency;
struct sig_dsp_Oscillator* carrier;
struct sig_dsp_ConstantValue* lfoCarrierDivision;
struct sig_dsp_ConstantValue* lfoModulatorDivision;
struct sig_dsp_BinaryOp* lfoFundamentalFrequency;
struct sig_dsp_BinaryOp* lfoModulatorFrequency;
struct sig_dsp_BinaryOp* lfoDeviation;
struct sig_dsp_Oscillator* lfoModulator;
struct sig_dsp_BinaryOp* lfoCarrierFrequency;
struct sig_dsp_Oscillator* lfoCarrier;
// TODO: Add a nice LPF after the FM.
struct sig_dsp_Tanh* distortion;
// TODO: Add skew controls for index/ratio for stereo separation.
struct sig_daisy_AudioOut* leftOut;
struct sig_daisy_AudioOut* rightOut;
struct sig_daisy_CVOut* eocCVOut;
struct sig_daisy_CVOut* eocLED;

// TODO: Factor out a 2-op FM signal so that there is less
// code duplication between the audio oscillator and the LFO.
// TODO: Factor out the v/oct tracking code into a custom signal.
void buildSignalGraph(struct sig_Allocator* allocator,
    struct sig_List* signals, struct sig_SignalContext* context,
    struct sig_Status* status) {
    smoothCoefficient = sig_dsp_ConstantValue_new(allocator, context, 0.01);

    coarseFrequencyKnob = sig_daisy_CVIn_new(allocator, context, host);
    sig_List_append(signals, coarseFrequencyKnob, status);
    coarseFrequencyKnob->parameters.control = sig_daisy_PatchInit_KNOB_1;
    coarseFrequencyKnob->parameters.scale = 7.0f;
    coarseFrequencyKnob->parameters.offset = -3.5f;

    coarseFrequencyLPF = sig_dsp_OnePole_new(allocator, context);
    sig_List_append(signals, coarseFrequencyLPF, status);
    coarseFrequencyLPF->inputs.coefficient = smoothCoefficient->outputs.main;
    coarseFrequencyLPF->inputs.source = coarseFrequencyKnob->outputs.main;

    fineFrequencyKnob = sig_daisy_CVIn_new(allocator, context, host);
    sig_List_append(signals, fineFrequencyKnob, status);
    fineFrequencyKnob->parameters.control = sig_daisy_PatchInit_KNOB_3;
    fineFrequencyKnob->parameters.offset = -0.5f;

    fineFrequencyLPF = sig_dsp_OnePole_new(allocator, context);
    sig_List_append(signals, fineFrequencyLPF, status);
    fineFrequencyLPF->inputs.coefficient = smoothCoefficient->outputs.main;
    fineFrequencyLPF->inputs.source = fineFrequencyKnob->outputs.main;

    vOctCV = sig_daisy_CVIn_new(allocator, context, host);
    sig_List_append(signals, vOctCV, status);
    vOctCV->parameters.control = sig_daisy_PatchInit_CV_IN_1;
    vOctCV->parameters.scale = 2.5f;

    coarsePlusVOct = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, coarsePlusVOct, status);
    coarsePlusVOct->inputs.left = coarseFrequencyLPF->outputs.main;
    coarsePlusVOct->inputs.right = vOctCV->outputs.main;

    coarseVOctPlusFine = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, coarseVOctPlusFine, status);
    coarseVOctPlusFine->inputs.left = coarsePlusVOct->outputs.main;
    coarseVOctPlusFine->inputs.right = fineFrequencyLPF->outputs.main;

    fundamentalFrequency = sig_dsp_LinearToFreq_new(allocator, context);
    sig_List_append(signals, fundamentalFrequency, status);
    fundamentalFrequency->inputs.source = coarseVOctPlusFine->outputs.main;

    ratioKnob = sig_daisy_CVIn_new(allocator, context, host);
    sig_List_append(signals, ratioKnob, status);
    ratioKnob->parameters.control = sig_daisy_PatchInit_KNOB_2;

    modulatorFrequency = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, modulatorFrequency, status);
    modulatorFrequency->inputs.left = fundamentalFrequency->outputs.main;
    modulatorFrequency->inputs.right = ratioKnob->outputs.main;

    indexKnob = sig_daisy_CVIn_new(allocator, context, host);
    sig_List_append(signals, indexKnob, status);
    indexKnob->parameters.control = sig_daisy_PatchInit_KNOB_4;
    indexKnob->parameters.scale = 25.0f;

    deviation = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, deviation, status);
    deviation->inputs.left = indexKnob->outputs.main;
    deviation->inputs.right = modulatorFrequency->outputs.main;

    modulator = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, modulator, status);
    modulator->inputs.freq = modulatorFrequency->outputs.main;
    modulator->inputs.mul = deviation->outputs.main;

    carrierFrequency = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, carrierFrequency, status);
    carrierFrequency->inputs.left = fundamentalFrequency->outputs.main;
    carrierFrequency->inputs.right = modulator->outputs.main;

    carrier = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, carrier, status);
    carrier->inputs.freq = carrierFrequency->outputs.main;
    carrier->inputs.mul = context->unity->outputs.main;

    lfoCarrierDivision = sig_dsp_ConstantValue_new(allocator, context,
        10000.0f);
    lfoModulatorDivision = sig_dsp_ConstantValue_new(allocator, context,
        1000.0f);

    lfoFundamentalFrequency = sig_dsp_Div_new(allocator, context);
    sig_List_append(signals, lfoFundamentalFrequency, status);
    lfoFundamentalFrequency->inputs.left = fundamentalFrequency->outputs.main;
    lfoFundamentalFrequency->inputs.right = lfoCarrierDivision->outputs.main;

    lfoModulatorFrequency = sig_dsp_Div_new(allocator, context);
    sig_List_append(signals, lfoModulatorFrequency, status);
    lfoModulatorFrequency->inputs.left = modulatorFrequency->outputs.main;
    lfoModulatorFrequency->inputs.right = lfoModulatorDivision->outputs.main;

    lfoDeviation = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, lfoDeviation, status);
    lfoDeviation->inputs.left = indexKnob->outputs.main;
    lfoDeviation->inputs.right = lfoModulatorFrequency->outputs.main;

    lfoModulator = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, lfoModulator, status);
    lfoModulator->inputs.freq = lfoModulatorFrequency->outputs.main;
    lfoModulator->inputs.mul = lfoDeviation->outputs.main;

    lfoCarrierFrequency = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, lfoCarrierFrequency, status);
    lfoCarrierFrequency->inputs.left = lfoFundamentalFrequency->outputs.main;
    lfoCarrierFrequency->inputs.right = lfoModulator->outputs.main;

    lfoCarrier = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, lfoCarrier, status);
    lfoCarrier->inputs.freq = lfoCarrierFrequency->outputs.main;
    lfoCarrier->inputs.mul = context->unity->outputs.main;

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
        .blockSize = 96
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
