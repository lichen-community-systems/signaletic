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
struct sig_dsp_LadderLPF* leftFilter;
struct sig_dsp_LadderLPF* rightFilter;
struct sig_daisy_AudioOut* leftOut;
struct sig_daisy_AudioOut* rightOut;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("  ~bob~  ");
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

    bluemchen.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    // Bluemchen AnalogControls are all unipolar,
    // so they need to be scaled to bipolar values.
    frequencyKnob = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, frequencyKnob, status);
    frequencyKnob->parameters.control = sig_daisy_Bluemchen_CV_IN_KNOB_1;
    frequencyKnob->parameters.scale = 9.5f; // 7.25f for Bob
    frequencyKnob->parameters.offset = -4.5f; // -4.0f for Bob

    vOctCVIn = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, vOctCVIn, status);
    vOctCVIn->parameters.control =  sig_daisy_Bluemchen_CV_IN_CV1;
    vOctCVIn->parameters.scale = 2.5f;
    vOctCVIn->parameters.offset = -1.25f;

    frequencyKnobPlusVOct = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, frequencyKnobPlusVOct, status);
    frequencyKnobPlusVOct->inputs.left = frequencyKnob->outputs.main;
    frequencyKnobPlusVOct->inputs.right = vOctCVIn->outputs.main;

    frequencySkewCV = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, frequencySkewCV, status);
    frequencySkewCV->parameters.control = sig_daisy_Bluemchen_CV_IN_CV2;
    frequencySkewCV->parameters.scale = 5.0f;
    frequencySkewCV->parameters.offset = -2.5f;

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
    resonanceKnob->parameters.scale = 1.7f; // 4.0f for Bob

    leftIn = sig_daisy_AudioIn_new(&allocator, context, host);
    sig_List_append(&signals, leftIn, status);
    leftIn->parameters.channel = 0;

    leftFilter = sig_dsp_LadderLPF_new(&allocator, context);
    sig_List_append(&signals, leftFilter, status);
    leftFilter->parameters.overdrive = 1.1f;
    leftFilter->parameters.passbandGain = 0.5f;
    leftFilter->inputs.source = leftIn->outputs.main;
    leftFilter->inputs.frequency = leftFrequency->outputs.main;
    leftFilter->inputs.resonance = resonanceKnob->outputs.main;

    leftOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = 0;
    leftOut->inputs.source = leftFilter->outputs.main;

    rightIn = sig_daisy_AudioIn_new(&allocator, context, host);
    sig_List_append(&signals, rightIn, status);
    rightIn->parameters.channel = 1;

    rightFilter = sig_dsp_LadderLPF_new(&allocator, context);
    sig_List_append(&signals, rightFilter, status);
    rightFilter->parameters.overdrive = 1.1f;
    leftFilter->parameters.passbandGain = 0.5f;
    rightFilter->inputs.source = rightIn->outputs.main;
    rightFilter->inputs.frequency = rightFrequency->outputs.main;
    rightFilter->inputs.resonance = resonanceKnob->outputs.main;

    rightOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = 1;
    rightOut->inputs.source = rightFilter->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 96000,
        .numChannels = 2,
        .blockSize = 8
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
