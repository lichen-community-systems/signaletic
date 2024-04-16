// Inspired by the pole-mixing multimode from the Oberheim XPander.
// https://electricdruid.net/multimode-filters-part-2-pole-mixing-filters/
// and Valimaki and Huovilainen, "Oscillator and Filter Algorithms for Virtual
// Analog Synthesis." Computer Music Journal, Vol. 30, No. 2 (Summer, 2006),
// pp. 19-31. https://www.jstor.org/stable/3682001

/*
Mixing coefficients for filter responses, as listed in Electric Druid (2020):
(in the form [Response name {Input, 1-pole, 2-pole, 3-pole, 4-pole}])
x = included in the Oberheim XPander
Low Pass:
  6dB lowpass {0, -1, 0, 0, 0} x
  12dB lowpass {0, 0, 1, 0, 0} x
  18dB lowpass {0, 0, 0, -1, 0} x
  24dB lowpass {0, 0, 0, 0, 1} x
High Pass:
  6dB highpass {1, -1, 0, 0, 0} x
  12dB highpass {1, -2, 1, 0, 0} x
  18dB highpass {1, -3, 3, -1, 0} x
  24dB highpass {1, -4, 6, -4, 1}
Band Pass:
  12dB bandpass  {0, -1, 1, 0, 0} x
  24dB bandpass {0, 0, 1, -2, 1} x
  6dB highpass + 12dB lowpass {0, 0, 1, -1, 0}
  6dB highpass + 18dB lowpass {0, 0, 0, -1, 1}
  12dB highpass + 6dB lowpass {0, -1, 2, -1, 0} x
  18dB highpass + 6dB lowpass {0, -1, 3, -3, 1} x
Notch:
  12dB notch {1, -2, 2, -0, 0} x
  18dB notch {1, -3, 3, -2, 0}
  12dB notch + 6db lowpass {0, -1, 2, -2, 0} x
All Pass:
  6dB all pass {1, -2, 0, 0, 0}
  12dB all pass {1, -4, 4, 0, 0}
  18dB all pass {1, -6, 12, -8, 0} x
  24dB all pass {1, -8, 24, -32, 16}
  18dB allpass + 6dB lowpass {0, -1, 3, -6, 4} x
All Pass Phaser:
  6dB phaser {1, -1, 0, 0, 0} (same as the 6dB high pass)
  12dB phaser {1, -2, 2, 0, 0}
  18dB phaser {1, -3, 6, -4, 0}
  24dB phaser {1, -4, 12, -16, 8}
*/

/*
Knob Scaling:
  Pole 1 (B): -4..4 (should be -8..0)
  Pole 2 (C): -6..6 (should be 0..24)
  Pole 3 (D): -8..0 (should be -32..0)
  Pole 4 (E): 0..4 (should be 0..16)
*/
#include <libsignaletic.h>
#include "../../../../include/lichen-bifocals-device.hpp"

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
sig::libdaisy::DaisyHost<lichen::bifocals::BifocalsDevice> host;

struct sig_dsp_List* aList;
struct sig_dsp_List* bList;
struct sig_dsp_List* cList;
struct sig_dsp_List* dList;
struct sig_dsp_List* eList;
struct sig_host_FilteredCVIn* shapeKnob;
struct sig_host_FilteredCVIn* frequencyKnob;
struct sig_host_FilteredCVIn* resonanceKnob;
struct sig_host_FilteredCVIn* resonanceCV;
struct sig_dsp_BinaryOp* resonance;
struct sig_host_FilteredCVIn* shapeCVIn;
struct sig_dsp_BinaryOp* shape;
struct sig_host_CVIn* vOctCVIn;
struct sig_host_FilteredCVIn* skewKnob;
struct sig_host_FilteredCVIn* skewCV;
struct sig_dsp_BinaryOp* skew;
struct sig_dsp_Abs* rectifiedSkew;
struct sig_dsp_BinaryOp* skewedFrequency;
struct sig_dsp_BinaryOp* frequency;
struct sig_dsp_Branch* leftSkewedFrequency;
struct sig_dsp_LinearToFreq* leftFrequency;
struct sig_dsp_Branch* rightSkewedFrequency;
struct sig_dsp_LinearToFreq* rightFrequency;
struct sig_host_AudioIn* leftIn;
struct sig_host_FilteredCVIn* gainKnob;
struct sig_host_FilteredCVIn* gainCV;
struct sig_dsp_BinaryOp* gain;
struct sig_host_AudioIn* rightIn;
struct sig_dsp_BinaryOp* leftVCA;
struct sig_dsp_BinaryOp* rightVCA;
struct sig_dsp_Ladder* leftFilter;
struct sig_dsp_Ladder* rightFilter;
struct sig_dsp_Tanh* leftSaturation;
struct sig_dsp_Tanh* rightSaturation;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    shapeKnob = sig_host_FilteredCVIn_new(&allocator, context);
    shapeKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, shapeKnob, status);
    shapeKnob->parameters.control = sig_host_KNOB_4;

    shapeCVIn = sig_host_FilteredCVIn_new(&allocator, context);
    shapeCVIn->hardware = &host.device.hardware;
    sig_List_append(&signals, shapeCVIn, status);
    shapeCVIn->parameters.control = sig_host_CV_IN_1;
    shapeCVIn->parameters.scale = 2.0f;

    shape = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, shape, status);
    shape->inputs.left = shapeKnob->outputs.main;
    shape->inputs.right = shapeCVIn->outputs.main;

    aList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, aList, status);
    aList->list = &aCoefficientBuffer;
    aList->parameters.wrap = 0.0f;
    aList->parameters.interpolate = 1.0f;
    aList->inputs.index = shape->outputs.main;

    bList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, bList, status);
    bList->list = &bCoefficientBuffer;
    bList->parameters.wrap = 0.0f;
    bList->parameters.interpolate = 1.0f;
    bList->inputs.index = shape->outputs.main;

    cList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, cList, status);
    cList->list = &cCoefficientBuffer;
    cList->parameters.wrap = 0.0f;
    cList->parameters.interpolate = 1.0f;
    cList->inputs.index = shape->outputs.main;

    dList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, dList, status);
    dList->list = &dCoefficientBuffer;
    dList->parameters.wrap = 0.0f;
    dList->parameters.interpolate = 1.0f;
    dList->inputs.index = shape->outputs.main;

    eList = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, eList, status);
    eList->list = &eCoefficientBuffer;
    eList->parameters.wrap = 0.0f;
    eList->parameters.interpolate = 1.0f;
    eList->inputs.index = shape->outputs.main;

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
    vOctCVIn->parameters.control = sig_host_CV_IN_2;
    vOctCVIn->parameters.scale = 5.0f;

    frequency = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, frequency, status);
    frequency->inputs.left = frequencyKnob->outputs.main;
    frequency->inputs.right = vOctCVIn->outputs.main;

    skewKnob = sig_host_FilteredCVIn_new(&allocator, context);
    skewKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, skewKnob, status);
    skewKnob->parameters.control = sig_host_KNOB_5;
    skewKnob->parameters.scale = 5.0f;
    skewKnob->parameters.offset = -2.5f;
    skewKnob->parameters.time = 0.1f;

    skewCV = sig_host_FilteredCVIn_new(&allocator, context);
    skewCV->hardware = &host.device.hardware;
    sig_List_append(&signals, skewCV, status);
    skewCV->parameters.control = sig_host_CV_IN_5;
    skewCV->parameters.scale = 2.5f;
    skewCV->parameters.time = 0.1f;

    skew = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, skew, status);
    skew->inputs.left = skewKnob->outputs.main;
    skew->inputs.right = skewCV->outputs.main;

    rectifiedSkew = sig_dsp_Abs_new(&allocator, context);
    sig_List_append(&signals, rectifiedSkew, status);
    rectifiedSkew->inputs.source = skew->outputs.main;

    skewedFrequency = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, skewedFrequency, status);
    skewedFrequency->inputs.left = frequency->outputs.main;
    skewedFrequency->inputs.right = rectifiedSkew->outputs.main;

    leftSkewedFrequency = sig_dsp_Branch_new(&allocator, context);
    sig_List_append(&signals, leftSkewedFrequency, status);
    leftSkewedFrequency->inputs.condition = skew->outputs.main;
    leftSkewedFrequency->inputs.on = frequency->outputs.main;
    leftSkewedFrequency->inputs.off = skewedFrequency->outputs.main;

    leftFrequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, leftFrequency, status);
    leftFrequency->inputs.source = leftSkewedFrequency->outputs.main;

    rightSkewedFrequency = sig_dsp_Branch_new(&allocator, context);
    sig_List_append(&signals, rightSkewedFrequency, status);
    rightSkewedFrequency->inputs.condition = skew->outputs.main;
    rightSkewedFrequency->inputs.on = skewedFrequency->outputs.main;
    rightSkewedFrequency->inputs.off = frequency->outputs.main;

    rightFrequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, rightFrequency, status);
    rightFrequency->inputs.source = rightSkewedFrequency->outputs.main;

    resonanceKnob = sig_host_FilteredCVIn_new(&allocator, context);
    resonanceKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, resonanceKnob, status);
    resonanceKnob->parameters.control = sig_host_KNOB_2;
    resonanceKnob->parameters.scale = 1.8f; // 4.0f for Bob

    resonanceCV = sig_host_FilteredCVIn_new(&allocator, context);
    resonanceCV->hardware = &host.device.hardware;
    sig_List_append(&signals, resonanceCV, status);
    resonanceCV->parameters.control = sig_host_CV_IN_3;
    resonanceCV->parameters.scale = 1.8f; // 4.0f for Bob

    resonance = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, resonance, status);
    resonance->inputs.left = resonanceKnob->outputs.main;
    resonance->inputs.right = resonanceCV->outputs.main;

    // TODO: Clamp or calibrate to between 0 and 4
    gainKnob = sig_host_FilteredCVIn_new(&allocator, context);
    gainKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, gainKnob, status);
    gainKnob->parameters.control = sig_host_KNOB_3;
    gainKnob->parameters.scale = 3.0f;

    gainCV = sig_host_FilteredCVIn_new(&allocator, context);
    gainCV->hardware = &host.device.hardware;
    sig_List_append(&signals, gainCV, status);
    gainCV->parameters.control = sig_host_CV_IN_4;
    gainCV->parameters.scale = 2.0f;

    gain = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, gain, status);
    gain->inputs.left = gainKnob->outputs.main;
    gain->inputs.right = gainCV->outputs.main;

    leftIn = sig_host_AudioIn_new(&allocator, context);
    leftIn->hardware = &host.device.hardware;
    sig_List_append(&signals, leftIn, status);
    leftIn->parameters.channel = sig_host_AUDIO_IN_1;

    leftVCA = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, leftVCA, status);
    leftVCA->inputs.left = leftIn->outputs.main;
    leftVCA->inputs.right = gain->outputs.main;

    leftFilter = sig_dsp_Ladder_new(&allocator, context);
    sig_List_append(&signals, leftFilter, status);
    leftFilter->parameters.passbandGain = 0.5f;
    leftFilter->inputs.source = leftVCA->outputs.main;
    leftFilter->inputs.frequency = leftFrequency->outputs.main;
    leftFilter->inputs.resonance = resonance->outputs.main;
    leftFilter->inputs.inputGain = aList->outputs.main;
    leftFilter->inputs.pole1Gain = bList->outputs.main;
    leftFilter->inputs.pole2Gain = cList->outputs.main;
    leftFilter->inputs.pole3Gain = dList->outputs.main;
    leftFilter->inputs.pole4Gain = eList->outputs.main;

    leftSaturation = sig_dsp_Tanh_new(&allocator, context);
    sig_List_append(&signals, leftSaturation, status);
    leftSaturation->inputs.source = leftFilter->outputs.main;

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = leftSaturation->outputs.main;

    rightIn = sig_host_AudioIn_new(&allocator, context);
    rightIn->hardware = &host.device.hardware;
    sig_List_append(&signals, rightIn, status);
    rightIn->parameters.channel = sig_host_AUDIO_IN_2;

    rightVCA = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, rightVCA, status);
    rightVCA->inputs.left = rightIn->outputs.main;
    rightVCA->inputs.right = gain->outputs.main;

    rightFilter = sig_dsp_Ladder_new(&allocator, context);
    sig_List_append(&signals, rightFilter, status);
    rightFilter->parameters.passbandGain = 0.5f;
    rightFilter->inputs.source = rightVCA->outputs.main;
    rightFilter->inputs.frequency = rightFrequency->outputs.main;
    rightFilter->inputs.resonance = resonance->outputs.main;
    rightFilter->inputs.inputGain = aList->outputs.main;
    rightFilter->inputs.pole1Gain = bList->outputs.main;
    rightFilter->inputs.pole2Gain = cList->outputs.main;
    rightFilter->inputs.pole3Gain = dList->outputs.main;
    rightFilter->inputs.pole4Gain = eList->outputs.main;

    rightSaturation = sig_dsp_Tanh_new(&allocator, context);
    sig_List_append(&signals, rightSaturation, status);
    rightSaturation->inputs.source = rightFilter->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
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

    while (1) {}
}
