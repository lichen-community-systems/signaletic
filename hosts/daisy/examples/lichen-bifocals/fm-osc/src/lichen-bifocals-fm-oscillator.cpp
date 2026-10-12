#include <libsignaletic.h>
#include "../../../../include/lichen-bifocals-device.hpp"
#include "../../../shared/include/summed-cv-in.h"
#include "../../../shared/include/bifocals-ladder.h"

using namespace lichen::bifocals;
using namespace sig::libdaisy;

#define SAMPLERATE 48000
#define BLOCK_SIZE 16
#define HEAP_SIZE 1024 * 256 // 256 KB
#define WAVETABLE_HEAP_SIZE 1024 * 1024 // 1 MB
#define MAX_NUM_SIGNALS 64
#define NUM_RATIOS 25

// The fundamental spans 1-200 Hz, centred on its geometric mean.
#define FUNDAMENTAL_CENTRE_FREQUENCY 14.142136f // sqrt(200)
#define FUNDAMENTAL_HALF_RANGE 3.821928f // log2(200) / 2 octaves

// In free mode, the modulator spans 12 octaves around the same centre.
#define FREE_MODULATOR_RANGE 12.0f

// Each wavetable level covers one octave from 1 Hz to 1024 Hz,
// which includes the highest modulator frequency (200 Hz * 4).
#define WAVETABLE_LENGTH 4096
#define NUM_WAVETABLE_LEVELS 10
#define LOWEST_WAVETABLE_FREQUENCY 1.0f
#define MAX_HARMONIC_FREQUENCY 1500.0f

#define LOW_PASS_FREQUENCY 300.0f
#define LOW_PASS_RESONANCE 0.3f
#define DC_BLOCK_FREQUENCY 0.15f

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

uint8_t DSY_SDRAM_BSS wavetableMemory[WAVETABLE_HEAP_SIZE];
struct sig_AllocatorHeap wavetableHeap = {
    .length = WAVETABLE_HEAP_SIZE,
    .memory = (void*) wavetableMemory
};

struct sig_Allocator wavetableAllocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &wavetableHeap
};

float ratios[NUM_RATIOS] = {
    // From Truax's normal form ratios ordered to the Farey Sequence,
    // limited to 4:1 to keep the modulator in low frequency territory.
    // https://www.sfu.ca/sonic-studio-webdav/handbook/fmtut.html
    1.0f/11.0f, 1.0f/10.0f, 1.0f/9.0f, 1.0f/8.0f,
    1.0f/7.0f, 1.0f/6.0f, 1.0f/5.0f,
    2.0f/9.0f, 1.0f/4.0f, 2.0f/7.0f,
    1.0f/3.0f, 3.0f/8.0f, 2.0f/5.0f,
    3.0f/7.0f, 4.0f/9.0f, 1.0f/2.0f,
    1.0f,
    2.0f/1.0f, 9.0f/4.0f, 7.0f/3.0f,
    5.0f/2.0f, 8.0f/3.0f, 3.0f/1.0f,
    7.0f/2.0f, 4.0f/1.0f
};

struct sig_Buffer ratioList = {
    .length = NUM_RATIOS,
    .samples = ratios
};

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;
DaisyHost<BifocalsDevice> host;

struct sig_Buffer* sineTable;
struct sig_AdditiveWavetableBank* wavetables;

struct sig_host_FilteredCVIn* frequencyKnob;
struct sig_host_CVIn* vOctCV;
struct sig_dsp_BinaryOp* pitch;
struct sig_dsp_Clamp* clampedPitch;
struct sig_dsp_LinearToFreq* fundamental;
struct sig_host_SummedCVIn* indexIn;
struct sig_dsp_Smooth* modulationIndex;
struct sig_host_FilteredCVIn* ratioKnob;
struct sig_host_FilteredCVIn* ratioCV;
struct sig_dsp_BinaryOp* combinedRatio;
struct sig_dsp_List* ratioListSignal;
struct sig_dsp_BinaryOp* ratioModulatorFrequency;
struct sig_dsp_ScaleOffset* freeModulatorPitch;
struct sig_dsp_LinearToFreq* freeModulatorFrequency;
struct sig_host_SwitchIn* modeButton;
struct sig_dsp_ToggleGate* freeMode;
struct sig_host_CVOut* led;
struct sig_dsp_Branch* modulatorFrequency;
struct sig_host_AudioIn* audioIn;
struct sig_host_SummedCVIn* phaseModulationAmountIn;
struct sig_dsp_Smooth* phaseModulationAmount;
struct sig_dsp_BinaryOp* externalPhaseModulation;
struct sig_host_SummedCVIn* waveform;
struct sig_dsp_WavetableOscillator* sineModulator;
struct sig_dsp_AdditiveWavetableBankOscillator* waveformCarrier;
struct sig_dsp_AdditiveWavetableBankOscillator* waveformModulator;
struct sig_dsp_WavetableOscillator* sineCarrier;
struct sig_dsp_LinearMap* wavefolderFactor;
struct sig_dsp_LinearMap* wavefolderGain;
struct sig_dsp_ConstantValue* lowPassFrequency;
struct sig_dsp_ConstantValue* lowPassResonance;
struct sig_dsp_BifocalsLadder* leftFilter;
struct sig_dsp_BifocalsLadder* rightFilter;
struct sig_dsp_DCBlock* leftDCBlocker;
struct sig_dsp_DCBlock* rightDCBlocker;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void buildWavetables() {
    sineTable = sig_Buffer_new(&allocator, WAVETABLE_LENGTH);
    sig_Buffer_fillWithWaveform(sineTable, sig_waveform_sine,
        WAVETABLE_LENGTH, 0.0f, 1.0f);

    wavetables = sig_AdditiveWavetableBank_new(&wavetableAllocator,
        sig_STANDARD_WAVEFORMS, sig_StandardWaveform_COUNT,
        NUM_WAVETABLE_LEVELS, WAVETABLE_LENGTH, LOWEST_WAVETABLE_FREQUENCY,
        MAX_HARMONIC_FREQUENCY, sig_HarmonicTaper_LANCZOS);
}

void buildFrequencyGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    frequencyKnob = sig_host_FilteredCVIn_new(&allocator, context);
    frequencyKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, frequencyKnob, status);
    frequencyKnob->parameters.control = sig_host_KNOB_1;
    frequencyKnob->parameters.scale = FUNDAMENTAL_HALF_RANGE * 2.0f;
    frequencyKnob->parameters.offset = -FUNDAMENTAL_HALF_RANGE;

    vOctCV = sig_host_CVIn_new(&allocator, context);
    vOctCV->hardware = &host.device.hardware;
    sig_List_append(&signals, vOctCV, status);
    vOctCV->parameters.control = sig_host_CV_IN_2;
    vOctCV->parameters.scale = 5.0f;

    pitch = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, pitch, status);
    pitch->inputs.left = frequencyKnob->outputs.main;
    pitch->inputs.right = vOctCV->outputs.main;

    clampedPitch = sig_dsp_Clamp_new(&allocator, context);
    sig_List_append(&signals, clampedPitch, status);
    clampedPitch->inputs.source = pitch->outputs.main;
    clampedPitch->parameters.min = -FUNDAMENTAL_HALF_RANGE;
    clampedPitch->parameters.max = FUNDAMENTAL_HALF_RANGE;

    fundamental = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, fundamental, status);
    fundamental->inputs.source = clampedPitch->outputs.main;
    fundamental->parameters.middleFreq = FUNDAMENTAL_CENTRE_FREQUENCY;
}

void buildModulatorFrequencyGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    ratioKnob = sig_host_FilteredCVIn_new(&allocator, context);
    ratioKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, ratioKnob, status);
    ratioKnob->parameters.control = sig_host_KNOB_4;
    ratioKnob->parameters.scale = 0.5f;

    ratioCV = sig_host_FilteredCVIn_new(&allocator, context);
    ratioCV->hardware = &host.device.hardware;
    sig_List_append(&signals, ratioCV, status);
    ratioCV->parameters.control = sig_host_CV_IN_1;
    ratioCV->parameters.scale = 0.25f;
    ratioCV->parameters.offset = 0.25f;

    combinedRatio = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, combinedRatio, status);
    combinedRatio->inputs.left = ratioKnob->outputs.main;
    combinedRatio->inputs.right = ratioCV->outputs.main;

    ratioListSignal = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, ratioListSignal, status);
    ratioListSignal->list = &ratioList;
    ratioListSignal->inputs.index = combinedRatio->outputs.main;

    ratioModulatorFrequency = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, ratioModulatorFrequency, status);
    ratioModulatorFrequency->inputs.left = fundamental->outputs.main;
    ratioModulatorFrequency->inputs.right = ratioListSignal->outputs.main;

    freeModulatorPitch = sig_dsp_ScaleOffset_new(&allocator, context);
    sig_List_append(&signals, freeModulatorPitch, status);
    freeModulatorPitch->inputs.source = combinedRatio->outputs.main;
    freeModulatorPitch->parameters.scale = FREE_MODULATOR_RANGE;
    freeModulatorPitch->parameters.offset = -FREE_MODULATOR_RANGE / 2.0f;

    freeModulatorFrequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, freeModulatorFrequency, status);
    freeModulatorFrequency->inputs.source = freeModulatorPitch->outputs.main;
    freeModulatorFrequency->parameters.middleFreq = FUNDAMENTAL_CENTRE_FREQUENCY;

    modeButton = sig_host_SwitchIn_new(&allocator, context);
    modeButton->hardware = &host.device.hardware;
    sig_List_append(&signals, modeButton, status);
    modeButton->parameters.control = sig_host_TOGGLE_1;

    freeMode = sig_dsp_ToggleGate_new(&allocator, context);
    sig_List_append(&signals, freeMode, status);
    freeMode->inputs.trigger = modeButton->outputs.main;

    led = sig_host_CVOut_new(&allocator, context);
    led->hardware = &host.device.hardware;
    sig_List_append(&signals, led, status);
    led->parameters.control = sig_host_CV_OUT_1;
    led->parameters.scale = 0.51f;
    led->inputs.source = freeMode->outputs.main;

    modulatorFrequency = sig_dsp_Branch_new(&allocator, context);
    sig_List_append(&signals, modulatorFrequency, status);
    modulatorFrequency->inputs.condition = freeMode->outputs.main;
    modulatorFrequency->inputs.on = freeModulatorFrequency->outputs.main;
    modulatorFrequency->inputs.off = ratioModulatorFrequency->outputs.main;
}

void buildModulationGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    indexIn = sig_host_SummedCVIn_new(&allocator, context);
    indexIn->hardware = &host.device.hardware;
    sig_List_append(&signals, indexIn, status);
    indexIn->leftCVIn->parameters.control = sig_host_KNOB_2;
    indexIn->rightCVIn->parameters.control = sig_host_CV_IN_4;

    // Discontinuities in the amplitude of a phase modulator
    // are really noticeable, so lots of smoothing is important.
    modulationIndex = sig_dsp_Smooth_new(&allocator, context);
    sig_List_append(&signals, modulationIndex, status);
    modulationIndex->inputs.source = indexIn->outputs.main;
    modulationIndex->parameters.time = 0.02f;

    audioIn = sig_host_AudioIn_new(&allocator, context);
    audioIn->hardware = &host.device.hardware;
    sig_List_append(&signals, audioIn, status);
    audioIn->parameters.channel = sig_host_AUDIO_IN_1;

    phaseModulationAmountIn = sig_host_SummedCVIn_new(&allocator, context);
    phaseModulationAmountIn->hardware = &host.device.hardware;
    sig_List_append(&signals, phaseModulationAmountIn, status);
    phaseModulationAmountIn->leftCVIn->parameters.control = sig_host_KNOB_5;
    phaseModulationAmountIn->rightCVIn->parameters.control = sig_host_CV_IN_3;

    phaseModulationAmount = sig_dsp_Smooth_new(&allocator, context);
    sig_List_append(&signals, phaseModulationAmount, status);
    phaseModulationAmount->inputs.source =
        phaseModulationAmountIn->outputs.main;
    phaseModulationAmount->parameters.time = 0.02f;

    externalPhaseModulation = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, externalPhaseModulation, status);
    externalPhaseModulation->inputs.left = audioIn->outputs.main;
    externalPhaseModulation->inputs.right = phaseModulationAmount->outputs.main;

    // The wavetable index can wrap around.
    // The knob spans the full bank, whle CV can push it past either end,
    // crossfading between the last waveform and the first.
    float lastWaveform = sig_osc_WavetableBank_tableIndex(
        sig_StandardWaveform_REVERSE_SAW, sig_StandardWaveform_COUNT);

    waveform = sig_host_SummedCVIn_new(&allocator, context);
    waveform->hardware = &host.device.hardware;
    sig_List_append(&signals, waveform, status);
    waveform->leftCVIn->parameters.control = sig_host_KNOB_3;
    waveform->rightCVIn->parameters.control = sig_host_CV_IN_5;
    waveform->leftCVIn->parameters.scale = lastWaveform;
    waveform->rightCVIn->parameters.scale = lastWaveform;
}

// First output: carrier is the selected waveform, modulator is a sine.
void buildWaveformCarrierVoice(struct sig_SignalContext* context,
    struct sig_Status* status) {
    sineModulator = sig_dsp_WavetableOscillator_new(&allocator, context);
    sig_List_append(&signals, sineModulator, status);
    sineModulator->wavetable = sineTable;
    sineModulator->inputs.freq = modulatorFrequency->outputs.main;
    sineModulator->inputs.phaseOffset = externalPhaseModulation->outputs.main;
    sineModulator->inputs.mul = modulationIndex->outputs.main;

    waveformCarrier = sig_dsp_AdditiveWavetableBankOscillator_new(
        &allocator, context);
    sig_List_append(&signals, waveformCarrier, status);
    waveformCarrier->wavetables = wavetables;
    waveformCarrier->inputs.freq = fundamental->outputs.main;
    waveformCarrier->inputs.phaseOffset = sineModulator->outputs.main;
    waveformCarrier->inputs.tableIndex = waveform->outputs.main;
}

// Second output is the opposite: carrier is a sine, modulator is the selected waveform.
void buildWaveformModulatorVoice(struct sig_SignalContext* context,
    struct sig_Status* status) {
    waveformModulator = sig_dsp_AdditiveWavetableBankOscillator_new(
        &allocator, context);
    sig_List_append(&signals, waveformModulator, status);
    waveformModulator->wavetables = wavetables;
    waveformModulator->inputs.freq = modulatorFrequency->outputs.main;
    waveformModulator->inputs.phaseOffset =
        externalPhaseModulation->outputs.main;
    waveformModulator->inputs.tableIndex = waveform->outputs.main;
    waveformModulator->inputs.mul = modulationIndex->outputs.main;

    sineCarrier = sig_dsp_WavetableOscillator_new(&allocator, context);
    sig_List_append(&signals, sineCarrier, status);
    sineCarrier->wavetable = sineTable;
    sineCarrier->inputs.freq = fundamental->outputs.main;
    sineCarrier->inputs.phaseOffset = waveformModulator->outputs.main;
}

struct sig_dsp_BifocalsLadder* buildFilter(struct sig_SignalContext* context,
    struct sig_Status* status, float_array_ptr source) {
    struct sig_dsp_BifocalsLadder* filter = sig_dsp_BifocalsLadder_new(
        &allocator, context);
    sig_List_append(&signals, filter, status);
    filter->parameters.passbandGain = 0.5f;
    filter->inputs.source = source;
    filter->inputs.frequency = lowPassFrequency->outputs.main;
    filter->inputs.resonance = lowPassResonance->outputs.main;
    filter->inputs.wavefolderGain = wavefolderGain->outputs.main;
    filter->inputs.wavefolderFactor = wavefolderFactor->outputs.main;

    return filter;
}

// The ladder filter's default pole mix is a four pole low pass,
// and its wavefolder defaults to folding before the filter.
void buildFilterGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    // The wavefolder fades in at the top of the index range,
    // using the same mapping as the Bifocals filter's gain knob.
    wavefolderFactor = sig_dsp_LinearMap_new(&allocator, context);
    sig_List_append(&signals, wavefolderFactor, status);
    wavefolderFactor->inputs.source = modulationIndex->outputs.main;
    wavefolderFactor->parameters.fromMin = 1.0f/3.0f;
    wavefolderFactor->parameters.fromMax = 1.0f;
    wavefolderFactor->parameters.toMin = 0.0f;
    wavefolderFactor->parameters.toMax = 6.0f;

    wavefolderGain = sig_dsp_LinearMap_new(&allocator, context);
    sig_List_append(&signals, wavefolderGain, status);
    wavefolderGain->inputs.source = modulationIndex->outputs.main;
    wavefolderGain->parameters.fromMin = 0.75f;
    wavefolderGain->parameters.fromMax = 1.0f;
    wavefolderGain->parameters.toMin = 1.0f;
    wavefolderGain->parameters.toMax = 2.0f;

    lowPassFrequency = sig_dsp_ConstantValue_new(&allocator, context,
        LOW_PASS_FREQUENCY);
    lowPassResonance = sig_dsp_ConstantValue_new(&allocator, context,
        LOW_PASS_RESONANCE);

    leftFilter = buildFilter(context, status, waveformCarrier->outputs.main);
    rightFilter = buildFilter(context, status, sineCarrier->outputs.main);
}

struct sig_host_AudioOut* buildOutput(struct sig_SignalContext* context,
    struct sig_Status* status, float_array_ptr source,
    struct sig_dsp_DCBlock** dcBlocker, int channel) {
    *dcBlocker = sig_dsp_DCBlock_new(&allocator, context);
    sig_List_append(&signals, *dcBlocker, status);
    (*dcBlocker)->inputs.source = source;
    (*dcBlocker)->parameters.frequency = DC_BLOCK_FREQUENCY;

    struct sig_host_AudioOut* out = sig_host_AudioOut_new(&allocator,
        context);
    out->hardware = &host.device.hardware;
    sig_List_append(&signals, out, status);
    out->parameters.channel = channel;
    out->inputs.source = (*dcBlocker)->outputs.main;

    return out;
}

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    buildFrequencyGraph(context, status);
    buildModulatorFrequencyGraph(context, status);
    buildModulationGraph(context, status);
    buildWaveformCarrierVoice(context, status);
    buildWaveformModulatorVoice(context, status);
    buildFilterGraph(context, status);

    leftOut = buildOutput(context, status, leftFilter->outputs.main,
        &leftDCBlocker, sig_host_AUDIO_OUT_1);
    rightOut = buildOutput(context, status, rightFilter->outputs.main,
        &rightDCBlocker, sig_host_AUDIO_OUT_2);
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = BLOCK_SIZE
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    // The SDRAM is only available after the board has been initialized.
    wavetableAllocator.impl->init(&wavetableAllocator);
    buildWavetables();
    buildSignalGraph(context, &status);

    host.Start();

    while (1) {}
}
