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

// TODO: Fix rampant cut and paste from libsignaletic.c's Ladder.
struct sig_dsp_BifocalsLadder_Inputs {
    float_array_ptr source;
    float_array_ptr frequency;
    float_array_ptr resonance;
    float_array_ptr inputGain;
    float_array_ptr pole1Gain;
    float_array_ptr pole2Gain;
    float_array_ptr pole3Gain;
    float_array_ptr pole4Gain;
    float_array_ptr wavefolderGain;
    float_array_ptr wavefolderFactor;
    float_array_ptr wavefolderPosition;
};

struct sig_dsp_BifocalsLadder {
    struct sig_dsp_Signal signal;
    struct sig_dsp_BifocalsLadder_Inputs inputs;
    struct sig_dsp_Ladder_Parameters parameters;
    struct sig_dsp_FourPoleFilter_Outputs outputs;

    uint8_t interpolation;
    float interpolationRecip;
    float alpha;
    float beta[4];
    float z0[4];
    float z1[4];
    float k;
    float fBase;
    float qAdjust;
    float prevFrequency;
    float prevInput;
};

inline void sig_dsp_BifocalsLadder_calcCoefficients(
    struct sig_dsp_BifocalsLadder* self, float freq) {
    float sampleRate = self->signal.audioSettings->sampleRate;
    freq = sig_clamp(freq, 5.0f, sampleRate * 0.425f);
    float wc = freq * (float) (sig_TWOPI /
        ((float)self->interpolation * sampleRate));
    float wc2 = wc * wc;
    self->alpha = 0.9892f * wc - 0.4324f *
        wc2 + 0.1381f * wc * wc2 - 0.0202f * wc2 * wc2;
    self->qAdjust = 1.006f + 0.0536f * wc - 0.095f * wc2 - 0.05f * wc2 * wc2;
}

inline float sig_dsp_BifocalsLadder_calcStage(
    struct sig_dsp_BifocalsLadder* self, float s, uint8_t i) {
    float ft = s * (1.0f/1.3f) + (0.3f/1.3f) * self->z0[i] - self->z1[i];
    ft = ft * self->alpha + self->z1[i];
    self->z1[i] = ft;
    self->z0[i] = s;
    return ft;
}

void sig_dsp_BifocalsLadder_generate(void* signal) {
    struct sig_dsp_BifocalsLadder* self =
        (struct sig_dsp_BifocalsLadder*) signal;
    float interpolationRecip = self->interpolationRecip;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float input = FLOAT_ARRAY(self->inputs.source)[i];
        float frequency = FLOAT_ARRAY(self->inputs.frequency)[i];
        float resonance = FLOAT_ARRAY(self->inputs.resonance)[i];
        float wavefolderGain = FLOAT_ARRAY(self->inputs.wavefolderGain)[i];
        float wavefolderFactor = FLOAT_ARRAY(self->inputs.wavefolderFactor)[i];
        float wavefolderPosition =
            FLOAT_ARRAY(self->inputs.wavefolderPosition)[i];
        float inputGain = FLOAT_ARRAY(self->inputs.inputGain)[i];
        float pole1Gain = FLOAT_ARRAY(self->inputs.pole1Gain)[i];
        float pole2Gain = FLOAT_ARRAY(self->inputs.pole2Gain)[i];
        float pole3Gain = FLOAT_ARRAY(self->inputs.pole3Gain)[i];
        float pole4Gain = FLOAT_ARRAY(self->inputs.pole4Gain)[i];
        float totals[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        float interp = 0.0f;

        // Recalculate coefficients if the frequency has changed.
        if (frequency != self->prevFrequency) {
            sig_dsp_BifocalsLadder_calcCoefficients(self, frequency);
            self->prevFrequency = frequency;
        }

        self->k = 4.0f * resonance;

        float main = 0.0f;
        for (size_t os = 0; os < self->interpolation; os++) {
            float inInterp = interp * self->prevInput + (1.0f - interp) * input;
            float u = inInterp - (self->z1[3] - self->parameters.passbandGain *
                inInterp) * self->k * self->qAdjust;

           if (wavefolderPosition <= 0.0f) {
                float prefolded = (u +
                    sinf(wavefolderFactor * u)) * wavefolderGain;
                u = prefolded;
            }
            u = sig_fastTanhf(u);

            totals[0] = u;
            float stage1 = sig_dsp_BifocalsLadder_calcStage(self, u, 0);
            totals[1] += stage1 * interpolationRecip;
            float stage2 = sig_dsp_BifocalsLadder_calcStage(self, stage1, 1);
            totals[2] += stage2 * interpolationRecip;
            float stage3 = sig_dsp_BifocalsLadder_calcStage(self, stage2, 2);
            totals[3] += stage3 * interpolationRecip;
            float stage4 = sig_dsp_BifocalsLadder_calcStage(self, stage3, 3);
            totals[4] += stage4 * interpolationRecip;

            float poleSum = (u * inputGain) +
                (stage1 * pole1Gain) +
                (stage2 * pole2Gain) +
                (stage3 * pole3Gain) +
                (stage4 * pole4Gain);

            if (wavefolderPosition > 0.0f) {
                poleSum = (poleSum +
                    sinf(wavefolderFactor * poleSum)) * wavefolderGain;

            }
            float postSaturated = tanhf(poleSum);
            main += postSaturated * interpolationRecip;
            interp += interpolationRecip;
        }
        self->prevInput = input;
        FLOAT_ARRAY(self->outputs.main)[i] = main;
        FLOAT_ARRAY(self->outputs.twoPole)[i] = totals[2];
        FLOAT_ARRAY(self->outputs.fourPole)[i] = totals[4];
    }
}

void sig_dsp_BifocalsLadder_init(
    struct sig_dsp_BifocalsLadder* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_BifocalsLadder_generate);

    struct sig_dsp_Ladder_Parameters parameters = {
        .passbandGain = 0.5f
    };
    self->parameters = parameters;

    self->interpolation = 8;
    self->interpolationRecip = 1.0f / self->interpolation;
    self->alpha = 1.0f;
    self->beta[0] = self->beta[1] = self->beta[2] = self->beta[3] = 0.0f;
    self->z0[0] = self->z0[1] = self->z0[2] = self->z0[3] = 0.0f;
    self->z1[0] = self->z1[1] = self->z1[2] = self->z1[3] = 0.0f;
    self->k = 1.0f;
    self->fBase = 1000.0f;
    self->qAdjust = 1.0f;
    self->prevFrequency = -1.0f;
    self->prevInput = 0.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, frequency, context);
    sig_CONNECT_TO_SILENCE(self, resonance, context);
    sig_CONNECT_TO_SILENCE(self, inputGain, context);
    sig_CONNECT_TO_SILENCE(self, pole1Gain, context);
    sig_CONNECT_TO_SILENCE(self, pole2Gain, context);
    sig_CONNECT_TO_SILENCE(self, pole3Gain, context);
    sig_CONNECT_TO_UNITY(self, pole4Gain, context);
    sig_CONNECT_TO_SILENCE(self, wavefolderGain, context);
    sig_CONNECT_TO_SILENCE(self, wavefolderFactor, context);
    sig_CONNECT_TO_SILENCE(self, wavefolderPosition, context);
}

struct sig_dsp_BifocalsLadder* sig_dsp_BifocalsLadder_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_BifocalsLadder* self = sig_MALLOC(allocator,
        struct sig_dsp_BifocalsLadder);
    sig_dsp_BifocalsLadder_init(self, context);
    sig_dsp_FourPoleFilter_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_BifocalsLadder_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BifocalsLadder* self) {
    sig_dsp_FourPoleFilter_Outputs_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}

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
struct sig_host_FilteredCVIn* shapeCVIn;
struct sig_dsp_BinaryOp* shape;
struct sig_host_FilteredCVIn* frequencyKnob;
struct sig_host_FilteredCVIn* resonanceKnob;
struct sig_host_FilteredCVIn* resonanceCV;
struct sig_dsp_BinaryOp* resonance;
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
struct sig_dsp_ScaleOffset* scaledGainKnob;
struct sig_host_FilteredCVIn* gainCV;
struct sig_dsp_BinaryOp* gain;
struct sig_host_AudioIn* rightIn;
struct sig_dsp_BinaryOp* leftVCA;
struct sig_dsp_BinaryOp* rightVCA;
struct sig_dsp_LinearMap* wavefolderGain;
struct sig_dsp_LinearMap* wavefolderFactor;
struct sig_host_SwitchIn* wavefolderPositionButton;
struct sig_dsp_ToggleGate* wavefolderPosition;
struct sig_host_CVOut* led;
struct sig_dsp_BifocalsLadder* leftFilter;
struct sig_dsp_BifocalsLadder* rightFilter;
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
    skewKnob->parameters.control = sig_host_KNOB_3;
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
    resonanceKnob->parameters.control = sig_host_KNOB_5;
    resonanceKnob->parameters.scale = 1.6f; // 4.0f for Bob

    resonanceCV = sig_host_FilteredCVIn_new(&allocator, context);
    resonanceCV->hardware = &host.device.hardware;
    sig_List_append(&signals, resonanceCV, status);
    resonanceCV->parameters.control = sig_host_CV_IN_3;
    resonanceCV->parameters.scale = 0.8f; // 4.0f for Bob

    resonance = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, resonance, status);
    resonance->inputs.left = resonanceKnob->outputs.main;
    resonance->inputs.right = resonanceCV->outputs.main;

    gainKnob = sig_host_FilteredCVIn_new(&allocator, context);
    gainKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, gainKnob, status);
    gainKnob->parameters.control = sig_host_KNOB_2;

    scaledGainKnob = sig_dsp_ScaleOffset_new(&allocator, context);
    sig_List_append(&signals, scaledGainKnob, status);
    scaledGainKnob->inputs.source = gainKnob->outputs.main;
    scaledGainKnob->parameters.scale = 2.0f;
    scaledGainKnob->parameters.offset = 0.0f;

    gainCV = sig_host_FilteredCVIn_new(&allocator, context);
    gainCV->hardware = &host.device.hardware;
    sig_List_append(&signals, gainCV, status);
    gainCV->parameters.control = sig_host_CV_IN_4;
    gainCV->parameters.scale = 2.0f;

    gain = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, gain, status);
    gain->inputs.left = scaledGainKnob->outputs.main;
    gain->inputs.right = gainCV->outputs.main;

    leftIn = sig_host_AudioIn_new(&allocator, context);
    leftIn->hardware = &host.device.hardware;
    sig_List_append(&signals, leftIn, status);
    leftIn->parameters.channel = sig_host_AUDIO_IN_1;

    leftVCA = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, leftVCA, status);
    leftVCA->inputs.left = leftIn->outputs.main;
    leftVCA->inputs.right = gain->outputs.main;

    wavefolderFactor = sig_dsp_LinearMap_new(&allocator, context);
    sig_List_append(&signals, wavefolderFactor, status);
    wavefolderFactor->inputs.source = gainKnob->outputs.main;
    wavefolderFactor->parameters.fromMin = 1.0/3.0f;
    wavefolderFactor->parameters.fromMax = 1.0f;
    wavefolderFactor->parameters.toMin = 0.0f;
    wavefolderFactor->parameters.toMax = 6.0f;

    wavefolderGain = sig_dsp_LinearMap_new(&allocator, context);
    sig_List_append(&signals, wavefolderGain, status);
    wavefolderGain->inputs.source = gainKnob->outputs.main;
    wavefolderGain->parameters.fromMin = 0.75f;
    wavefolderGain->parameters.fromMax = 1.0f;
    wavefolderGain->parameters.toMin = 1.0f;
    wavefolderGain->parameters.toMax = 2.0f;

    wavefolderPositionButton = sig_host_SwitchIn_new(&allocator, context);
    wavefolderPositionButton->hardware = &host.device.hardware;
    sig_List_append(&signals, wavefolderPositionButton, status);
    wavefolderPositionButton->parameters.control = sig_host_TOGGLE_1;

    wavefolderPosition = sig_dsp_ToggleGate_new(&allocator, context);
    sig_List_append(&signals, wavefolderPosition, status);
    wavefolderPosition->inputs.trigger = wavefolderPositionButton->outputs.main;

    led = sig_host_CVOut_new(&allocator, context);
    led->hardware = &host.device.hardware;
    sig_List_append(&signals, led, status);
    led->parameters.control = sig_host_CV_OUT_1;
    led->inputs.source = wavefolderPosition->outputs.main;
    led->parameters.scale = 0.51f;

    leftFilter = sig_dsp_BifocalsLadder_new(&allocator, context);
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
    leftFilter->inputs.wavefolderGain = wavefolderGain->outputs.main;
    leftFilter->inputs.wavefolderFactor = wavefolderFactor->outputs.main;
    leftFilter->inputs.wavefolderPosition = wavefolderPosition->outputs.main;

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = leftFilter->outputs.main;

    rightIn = sig_host_AudioIn_new(&allocator, context);
    rightIn->hardware = &host.device.hardware;
    sig_List_append(&signals, rightIn, status);
    rightIn->parameters.channel = sig_host_AUDIO_IN_2;

    rightVCA = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, rightVCA, status);
    rightVCA->inputs.left = rightIn->outputs.main;
    rightVCA->inputs.right = gain->outputs.main;

    rightFilter = sig_dsp_BifocalsLadder_new(&allocator, context);
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
    rightFilter->inputs.wavefolderGain = wavefolderGain->outputs.main;
    rightFilter->inputs.wavefolderFactor = wavefolderFactor->outputs.main;
    rightFilter->inputs.wavefolderPosition = wavefolderPosition->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->inputs.source = rightFilter->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 16
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
