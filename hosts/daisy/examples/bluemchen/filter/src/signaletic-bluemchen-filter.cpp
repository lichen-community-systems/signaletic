#include <string>
#include <tlsf.h>
#include <libsignaletic.h>
#include "../../../../include/daisy-bluemchen-host.h"

using namespace kxmx;
using namespace daisy;

FixedCapStr<20> displayStr;

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

Bluemchen bluemchen;
struct sig_daisy_Host* host;

#define NUM_FILTER_MODES 6
#define NUM_FILTER_STAGES 5

float mixingCoefficients[NUM_FILTER_STAGES][NUM_FILTER_MODES] = {
    // 4 pole LP, 2 pole LP, 2 pole BP, 4 pole BP, 4 pole HP, 2 pole HP.
    {0, 0,  0,  0,  1,  1}, // Input gain (A)
    {0, 0,  2,  0, -4, -2}, // Stage 1 (B)
    {0, 1, -2,  4,  6,  1}, // Stage 2 (C)
    {0, 0,  0, -8, -4,  0}, // Stage 3 (D)
    {1, 0,  0,  4,  1,  0}  // Stage 4 (E)
};

const char filterModeStrings[NUM_FILTER_MODES][4] = {
    "4LP", "2LP", "2BP", "4BP", "4HP", "2HP"
};

struct sig_Buffer aCoefficientBuffer = {
    .length = NUM_FILTER_MODES,
    .samples = mixingCoefficients[0]
};

struct sig_Buffer bCoefficientBuffer = {
    .length = NUM_FILTER_MODES,
    .samples = mixingCoefficients[1]
};

struct sig_Buffer cCoefficientBuffer = {
    .length = NUM_FILTER_MODES,
    .samples = mixingCoefficients[2]
};

struct sig_Buffer dCoefficientBuffer = {
    .length = NUM_FILTER_MODES,
    .samples = mixingCoefficients[3]
};

struct sig_Buffer eCoefficientBuffer = {
    .length = NUM_FILTER_MODES,
    .samples = mixingCoefficients[4]
};

struct sig_daisy_EncoderIn* encoderIn;
struct sig_dsp_List* aList;
struct sig_dsp_Smooth* aSmooth;
struct sig_dsp_List* bList;
struct sig_dsp_Smooth* bSmooth;
struct sig_dsp_List* cList;
struct sig_dsp_Smooth* cSmooth;
struct sig_dsp_List* dList;
struct sig_dsp_Smooth* dSmooth;
struct sig_dsp_List* eList;
struct sig_dsp_Smooth* eSmooth;
struct sig_daisy_FilteredCVIn* frequencyKnob;
struct sig_daisy_FilteredCVIn* resonanceKnob;
struct sig_daisy_FilteredCVIn* vOctCVIn;
struct sig_daisy_FilteredCVIn* frequencySkewCV;
struct sig_dsp_Abs* rectifiedSkew;
struct sig_dsp_BinaryOp* frequencyCVSkewAdder;
struct sig_dsp_BinaryOp* frequencyKnobPlusVOct;
struct sig_dsp_Branch* leftFrequencyCVSkewed;
struct sig_dsp_LinearToFreq* leftFrequency;
struct sig_dsp_Branch* rightFrequencyCVSkewed;
struct sig_dsp_LinearToFreq* rightFrequency;
struct sig_daisy_AudioIn* leftIn;
struct sig_daisy_AudioIn* rightIn;
struct sig_dsp_Ladder* leftFilter;
struct sig_dsp_Ladder* rightFilter;
struct sig_dsp_Tanh* leftSaturation;
struct sig_dsp_Tanh* rightSaturation;
struct sig_daisy_AudioOut* leftOut;
struct sig_daisy_AudioOut* rightOut;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append(" Bifocals");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("F: ");
    displayStr.AppendFloat(leftFrequency->outputs.main[0], 0);
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Hz");
    bluemchen.display.SetCursor(47, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("R: ");
    displayStr.AppendFloat(resonanceKnob->outputs.main[0], 2);
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append(filterModeStrings[(size_t) aList->outputs.index[0]]);
    bluemchen.display.SetCursor(0, 24);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    bluemchen.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    encoderIn = sig_daisy_EncoderIn_new(&allocator, context, host);
    sig_List_append(&signals, encoderIn, status);
    encoderIn->parameters.control = 0;

    aList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, aList, status);
    aList->list = &aCoefficientBuffer;
    aList->parameters.wrap = 1.0f;
    aList->parameters.normalizeIndex = 0.0f;
    aList->inputs.index = encoderIn->outputs.main;

    aSmooth = sig_dsp_Smooth_new(&allocator, context);
    sig_List_append(&signals, aSmooth, status);
    aSmooth->inputs.source = aList->outputs.main;
    aSmooth->parameters.time = 0.1f;

    bList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, bList, status);
    bList->list = &bCoefficientBuffer;
    bList->parameters.wrap = 1.0f;
    bList->parameters.normalizeIndex = 0.0f;
    bList->inputs.index = encoderIn->outputs.main;

    bSmooth = sig_dsp_Smooth_new(&allocator, context);
    sig_List_append(&signals, bSmooth, status);
    bSmooth->inputs.source = bList->outputs.main;
    bSmooth->parameters.time = 0.1f;

    cList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, cList, status);
    cList->list = &cCoefficientBuffer;
    cList->parameters.wrap = 1.0f;
    cList->parameters.normalizeIndex = 0.0f;
    cList->inputs.index = encoderIn->outputs.main;

    cSmooth = sig_dsp_Smooth_new(&allocator, context);
    sig_List_append(&signals, cSmooth, status);
    cSmooth->inputs.source = cList->outputs.main;
    cSmooth->parameters.time = 0.1f;

    dList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, dList, status);
    dList->list = &dCoefficientBuffer;
    dList->parameters.wrap = 1.0f;
    dList->parameters.normalizeIndex = 0.0f;
    dList->inputs.index = encoderIn->outputs.main;

    dSmooth = sig_dsp_Smooth_new(&allocator, context);
    sig_List_append(&signals, dSmooth, status);
    dSmooth->inputs.source = dList->outputs.main;
    dSmooth->parameters.time = 0.1f;

    eList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, eList, status);
    eList->list = &eCoefficientBuffer;
    eList->parameters.wrap = 1.0f;
    eList->parameters.normalizeIndex = 0.0f;
    eList->inputs.index = encoderIn->outputs.main;

    eSmooth = sig_dsp_Smooth_new(&allocator, context);
    sig_List_append(&signals, eSmooth, status);
    eSmooth->inputs.source = eList->outputs.main;
    eSmooth->parameters.time = 0.1f;

    // Bluemchen AnalogControls are all unipolar,
    // so they need to be scaled to bipolar values.
    frequencyKnob = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, frequencyKnob, status);
    frequencyKnob->parameters.control = sig_daisy_Bluemchen_CV_IN_KNOB_1;
    frequencyKnob->parameters.scale = 10.0f;
    frequencyKnob->parameters.offset = -5.0f;
    frequencyKnob->parameters.time = 0.1f;

    vOctCVIn = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, vOctCVIn, status);
    vOctCVIn->parameters.control =  sig_daisy_Bluemchen_CV_IN_CV1;
    vOctCVIn->parameters.scale = 4.0f;
    vOctCVIn->parameters.offset = -2.0f;
    vOctCVIn->parameters.time = 0.1f;

    frequencyKnobPlusVOct = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, frequencyKnobPlusVOct, status);
    frequencyKnobPlusVOct->inputs.left = frequencyKnob->outputs.main;
    frequencyKnobPlusVOct->inputs.right = vOctCVIn->outputs.main;

    frequencySkewCV = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, frequencySkewCV, status);
    frequencySkewCV->parameters.control = sig_daisy_Bluemchen_CV_IN_CV2;
    frequencySkewCV->parameters.scale = 5.0f;
    frequencySkewCV->parameters.offset = -2.5f;
    frequencySkewCV->parameters.time = 0.1f;

    rectifiedSkew = sig_dsp_Abs_new(&allocator, context);
    sig_List_append(&signals, rectifiedSkew, status);
    rectifiedSkew->inputs.source = frequencySkewCV->outputs.main;

    frequencyCVSkewAdder = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, frequencyCVSkewAdder, status);
    frequencyCVSkewAdder->inputs.left = frequencyKnobPlusVOct->outputs.main;
    frequencyCVSkewAdder->inputs.right = rectifiedSkew->outputs.main;

    leftFrequencyCVSkewed = sig_dsp_Branch_new(&allocator, context);
    sig_List_append(&signals, leftFrequencyCVSkewed, status);
    leftFrequencyCVSkewed->inputs.condition = frequencySkewCV->outputs.main;
    leftFrequencyCVSkewed->inputs.on = frequencyKnobPlusVOct->outputs.main;
    leftFrequencyCVSkewed->inputs.off = frequencyCVSkewAdder->outputs.main;

    leftFrequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, leftFrequency, status);
    leftFrequency->inputs.source = leftFrequencyCVSkewed->outputs.main;

    rightFrequencyCVSkewed = sig_dsp_Branch_new(&allocator, context);
    sig_List_append(&signals, rightFrequencyCVSkewed, status);
    rightFrequencyCVSkewed->inputs.condition = frequencySkewCV->outputs.main;
    rightFrequencyCVSkewed->inputs.on = frequencyCVSkewAdder->outputs.main;
    rightFrequencyCVSkewed->inputs.off = frequencyKnobPlusVOct->outputs.main;

    rightFrequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, rightFrequency, status);
    rightFrequency->inputs.source = rightFrequencyCVSkewed->outputs.main;

    resonanceKnob = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, resonanceKnob, status);
    resonanceKnob->parameters.control = sig_daisy_Bluemchen_CV_IN_KNOB_2;
    resonanceKnob->parameters.scale = 1.8f; // 4.0f for Bob

    leftIn = sig_daisy_AudioIn_new(&allocator, context, host);
    sig_List_append(&signals, leftIn, status);
    leftIn->parameters.channel = 0;

    leftFilter = sig_dsp_Ladder_new(&allocator, context);
    sig_List_append(&signals, leftFilter, status);
    leftFilter->parameters.overdrive = 1.1f;
    leftFilter->parameters.passbandGain = 0.5f;
    leftFilter->inputs.source = leftIn->outputs.main;
    leftFilter->inputs.frequency = leftFrequency->outputs.main;
    leftFilter->inputs.resonance = resonanceKnob->outputs.main;
    leftFilter->inputs.inputMix = aSmooth->outputs.main;
    leftFilter->inputs.stage1Mix = bSmooth->outputs.main;
    leftFilter->inputs.stage2Mix = cSmooth->outputs.main;
    leftFilter->inputs.stage3Mix = dSmooth->outputs.main;
    leftFilter->inputs.stage4Mix = eSmooth->outputs.main;

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
    rightFilter->inputs.resonance = resonanceKnob->outputs.main;
    rightFilter->inputs.inputMix = aSmooth->outputs.main;
    rightFilter->inputs.stage1Mix = bSmooth->outputs.main;
    rightFilter->inputs.stage2Mix = cSmooth->outputs.main;
    rightFilter->inputs.stage3Mix = dSmooth->outputs.main;
    rightFilter->inputs.stage4Mix = eSmooth->outputs.main;

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
    host = sig_daisy_BluemchenHost_new(&allocator,
        &audioSettings,
        &bluemchen,
        (struct sig_dsp_SignalEvaluator*) evaluator);
    sig_daisy_Host_registerGlobalHost(host);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(context, &status);
    host->impl->start(host);

    while (1) {
        UpdateOled();
    }
}
