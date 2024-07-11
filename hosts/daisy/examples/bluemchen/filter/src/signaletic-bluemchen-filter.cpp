#include <libsignaletic.h>
#include "../../../../include/kxmx-bluemchen-device.hpp"

FixedCapStr<20> displayStr;

#define SAMPLERATE 96000
#define HEAP_SIZE 1024 * 384 // 384 KB
#define MAX_NUM_SIGNALS 64

// The best, with band passes
#define NUM_FILTER_MODES 9
#define NUM_FILTER_STAGES 5
float mixingCoefficients[NUM_FILTER_STAGES][NUM_FILTER_MODES] = {
    // 4LP, 2LP, 2BP, 4-1LP, ??, 4APP, 2HP, 3HP, 4HP
    {  0,   0,   0,    0,    0,   1,   1,   1,   1  },
    {  0,   0,  -1,   -1,   -2,  -4,  -2,  -3,  -4  },
    {  0,   1,   1,    0,    2,  12,   1,   3,   6  },
    {  0,   0,   0,    0,   -4, -16,   0,  -1,  -4  },
    {  1,   0,   0,    1,    4,   8,   0,   0,   1  }
};

// TODO: Better names for these modes.
const char filterModeStrings[NUM_FILTER_MODES][5] = {
    "4LP", "2LP", "2BP", "41LP", "??", "4APP", "2HP", "3HP", "4HP"
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
sig::libdaisy::DaisyHost<kxmx::bluemchen::BluemchenDevice> host;

struct sig_host_EncoderIn* encoderIn;
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
struct sig_host_FilteredCVIn* frequencyKnob;
struct sig_host_FilteredCVIn* resonanceKnob;
struct sig_host_CVIn* vOctCVIn;
struct sig_dsp_Calibrator* vOctCalibrator;
struct sig_host_FilteredCVIn* skewCV;
struct sig_dsp_Abs* rectifiedSkew;
struct sig_dsp_BinaryOp* frequencyCVSkewAdder;
struct sig_dsp_BinaryOp* frequencyKnobPlusVOct;
struct sig_dsp_Branch* leftFrequencyCVSkewed;
struct sig_dsp_LinearToFreq* leftFrequency;
struct sig_dsp_Branch* rightFrequencyCVSkewed;
struct sig_dsp_LinearToFreq* rightFrequency;
struct sig_host_AudioIn* leftIn;
struct sig_host_AudioIn* rightIn;
struct sig_dsp_Ladder* leftFilter;
struct sig_dsp_Ladder* rightFilter;
struct sig_dsp_Tanh* leftSaturation;
struct sig_dsp_Tanh* rightSaturation;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void UpdateOled() {
    host.device.display.Fill(false);

    displayStr.Clear();
    displayStr.Append(" Bifocals");
    host.device.display.SetCursor(0, 0);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("F: ");
    displayStr.AppendFloat(leftFrequency->outputs.main[0], 0);
    host.device.display.SetCursor(0, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Hz");
    host.device.display.SetCursor(47, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("R: ");
    displayStr.AppendFloat(resonanceKnob->outputs.main[0], 2);
    host.device.display.SetCursor(0, 16);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append(filterModeStrings[(size_t) aList->outputs.index[0]]);
    host.device.display.SetCursor(0, 24);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    host.device.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    encoderIn = sig_host_EncoderIn_new(&allocator, context);
    encoderIn->hardware = &host.device.hardware;
    sig_List_append(&signals, encoderIn, status);
    encoderIn->parameters.turnControl = sig_host_ENCODER_1;
    encoderIn->parameters.buttonControl = sig_host_BUTTON_1;

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
    frequencyKnob = sig_host_FilteredCVIn_new(&allocator, context);
    frequencyKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, frequencyKnob, status);
    frequencyKnob->parameters.control = sig_host_KNOB_1;
    frequencyKnob->parameters.scale = 10.0f;
    frequencyKnob->parameters.offset = -5.0f;
    frequencyKnob->parameters.time = 0.1f;

    vOctCVIn = sig_host_CVIn_new(&allocator, context);
    vOctCVIn->hardware = &host.device.hardware;
    sig_List_append(&signals, vOctCVIn, status);
    vOctCVIn->parameters.control =  sig_host_CV_IN_1;
    vOctCVIn->parameters.scale = 5.0f;

    vOctCalibrator = sig_dsp_Calibrator_new(&allocator, context);
    sig_List_append(&signals, vOctCalibrator, status);
    vOctCalibrator->inputs.source = vOctCVIn->outputs.main;
    vOctCalibrator->inputs.gate = encoderIn->outputs.button;

    frequencyKnobPlusVOct = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, frequencyKnobPlusVOct, status);
    frequencyKnobPlusVOct->inputs.left = frequencyKnob->outputs.main;
    frequencyKnobPlusVOct->inputs.right = vOctCalibrator->outputs.main;

    skewCV = sig_host_FilteredCVIn_new(&allocator, context);
    skewCV->hardware = &host.device.hardware;
    sig_List_append(&signals, skewCV, status);
    skewCV->parameters.control = sig_host_CV_IN_2;
    skewCV->parameters.scale = 2.5f;
    skewCV->parameters.time = 0.1f;

    rectifiedSkew = sig_dsp_Abs_new(&allocator, context);
    sig_List_append(&signals, rectifiedSkew, status);
    rectifiedSkew->inputs.source = skewCV->outputs.main;

    frequencyCVSkewAdder = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, frequencyCVSkewAdder, status);
    frequencyCVSkewAdder->inputs.left = frequencyKnobPlusVOct->outputs.main;
    frequencyCVSkewAdder->inputs.right = rectifiedSkew->outputs.main;

    leftFrequencyCVSkewed = sig_dsp_Branch_new(&allocator, context);
    sig_List_append(&signals, leftFrequencyCVSkewed, status);
    leftFrequencyCVSkewed->inputs.condition = skewCV->outputs.main;
    leftFrequencyCVSkewed->inputs.on = frequencyKnobPlusVOct->outputs.main;
    leftFrequencyCVSkewed->inputs.off = frequencyCVSkewAdder->outputs.main;

    leftFrequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, leftFrequency, status);
    leftFrequency->inputs.source = leftFrequencyCVSkewed->outputs.main;

    rightFrequencyCVSkewed = sig_dsp_Branch_new(&allocator, context);
    sig_List_append(&signals, rightFrequencyCVSkewed, status);
    rightFrequencyCVSkewed->inputs.condition = skewCV->outputs.main;
    rightFrequencyCVSkewed->inputs.on = frequencyCVSkewAdder->outputs.main;
    rightFrequencyCVSkewed->inputs.off = frequencyKnobPlusVOct->outputs.main;

    rightFrequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, rightFrequency, status);
    rightFrequency->inputs.source = rightFrequencyCVSkewed->outputs.main;

    resonanceKnob = sig_host_FilteredCVIn_new(&allocator, context);
    resonanceKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, resonanceKnob, status);
    resonanceKnob->parameters.control = sig_host_KNOB_2;
    resonanceKnob->parameters.scale = 1.8f; // 4.0f for Bob

    leftIn = sig_host_AudioIn_new(&allocator, context);
    leftIn->hardware = &host.device.hardware;
    sig_List_append(&signals, leftIn, status);
    leftIn->parameters.channel = 0;

    leftFilter = sig_dsp_Ladder_new(&allocator, context);
    sig_List_append(&signals, leftFilter, status);
    leftFilter->parameters.passbandGain = 0.5f;
    leftFilter->inputs.source = leftIn->outputs.main;
    leftFilter->inputs.frequency = leftFrequency->outputs.main;
    leftFilter->inputs.resonance = resonanceKnob->outputs.main;
    leftFilter->inputs.inputGain = aSmooth->outputs.main;
    leftFilter->inputs.pole1Gain = bSmooth->outputs.main;
    leftFilter->inputs.pole2Gain = cSmooth->outputs.main;
    leftFilter->inputs.pole3Gain = dSmooth->outputs.main;
    leftFilter->inputs.pole4Gain = eSmooth->outputs.main;

    leftSaturation = sig_dsp_Tanh_new(&allocator, context);
    sig_List_append(&signals, leftSaturation, status);
    leftSaturation->inputs.source = leftFilter->outputs.main;

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = 0;
    leftOut->inputs.source = leftSaturation->outputs.main;

    rightIn = sig_host_AudioIn_new(&allocator, context);
    rightIn->hardware = &host.device.hardware;
    sig_List_append(&signals, rightIn, status);
    rightIn->parameters.channel = 1;

    rightFilter = sig_dsp_Ladder_new(&allocator, context);
    sig_List_append(&signals, rightFilter, status);
    leftFilter->parameters.passbandGain = 0.5f;
    rightFilter->inputs.source = rightIn->outputs.main;
    rightFilter->inputs.frequency = rightFrequency->outputs.main;
    rightFilter->inputs.resonance = resonanceKnob->outputs.main;
    rightFilter->inputs.inputGain = aSmooth->outputs.main;
    rightFilter->inputs.pole1Gain = bSmooth->outputs.main;
    rightFilter->inputs.pole2Gain = cSmooth->outputs.main;
    rightFilter->inputs.pole3Gain = dSmooth->outputs.main;
    rightFilter->inputs.pole4Gain = eSmooth->outputs.main;

    rightSaturation = sig_dsp_Tanh_new(&allocator, context);
    sig_List_append(&signals, rightSaturation, status);
    rightSaturation->inputs.source = rightFilter->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = 1;
    rightOut->inputs.source = rightSaturation->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 2
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

    while (1) {
        UpdateOled();
    }
}
