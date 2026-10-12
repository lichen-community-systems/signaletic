#include <libsignaletic.h>
#include "../../../../include/lichen-four-device.hpp"
#include "../../../shared/include/summed-cv-in.h"
#include "../../../shared/include/scaled-clock-frequency.h"

using namespace lichen::four;
using namespace sig::libdaisy;

#define SAMPLERATE 48000
#define WAVETABLE_SIZE 2048
#define HEAP_SIZE 1024 * 384 // 384 KB
#define MAX_NUM_SIGNALS 64
#define NUM_CLOCK_STEPS 37

// The index of the 1:1 ratio in each clock list. The slider covers this
// many ratios above 1:1, and CV can push it this many ratios in either
// direction.
// Each list needs this many ratios below 1:1 and twice as many above it.
#define UNITY_CLOCK_STEP 12

#define WAVETABLE_HEAP_SIZE 1024 * 1024 // 1 MB

// LFOs 1 and 4 go up to audio rate, so their wavetables are bandlimited.
// Below about 10 Hz, the table length limits the number of harmonics that
// can be represented, so the lowest level starts at 8 Hz.
// The highest level covers 8-16 kHz, and harmonics are always below 20 kHz.
#define NUM_BANDLIMITED_LEVELS 11
#define LOWEST_BANDLIMITED_FREQUENCY 8.0f
#define MAX_HARMONIC_FREQUENCY 20000.0f

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

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;
DaisyHost<FourDevice> host;

// Below 1:1, the slow LFOs speed up through more closely spaced ratios,
// stopping at 5:1 so that their CV outputs stay well below audio rate.
float clockDivisions[NUM_CLOCK_STEPS] = {
    5.0f,
    9.0f/2.0f,
    4.0f,
    7.0f/2.0f,
    3.0f,
    5.0f/2.0f,
    2.0f,
    5.0f/3.0f,
    3.0f/2.0f,
    4.0f/3.0f,
    5.0f/4.0f,
    6.0f/5.0f,
    1.0f,
    1.0f/2.0f,
    1.0f/3.0f,
    1.0f/4.0f,
    1.0f/5.0f,
    1.0f/6.0f,
    1.0f/7.0f,
    1.0f/8.0f,
    1.0f/9.0f,
    1.0f/10.0f,
    1.0f/11.0f,
    1.0f/13.0f,
    1.0f/17.0f,
    1.0f/19.0f,
    1.0f/20.0f,
    1.0f/23.0f,
    1.0f/29.0f,
    1.0f/31.0f,
    1.0f/67.0f,
    1.0f/100.0f,
    1.0f/131.0f,
    1.0f/200.0f,
    1.0f/263.0f,
    1.0f/300.0f,
    1.0f/523.0f
};

struct sig_Buffer clockDivisionsBuffer = {
    .length = NUM_CLOCK_STEPS,
    .samples = clockDivisions
};

// Below 1:1, the fast LFOs slow down, mirroring the ratios above it.
float clockMultiplications[NUM_CLOCK_STEPS] = {
    1.0f/17.0f,
    1.0f/13.0f,
    1.0f/11.0f,
    1.0f/10.0f,
    1.0f/9.0f,
    1.0f/8.0f,
    1.0f/7.0f,
    1.0f/6.0f,
    1.0f/5.0f,
    1.0f/4.0f,
    1.0f/3.0f,
    1.0f/2.0f,
    1.0f,
    2.0f,
    3.0f,
    4.0f,
    5.0f,
    6.0f,
    7.0f,
    8.0f,
    9.0f,
    10.0f,
    11.0f,
    13.0f,
    17.0f,
    19.0f,
    20.0f,
    23.0f,
    29.0f,
    31.0f,
    67.0f,
    100.0f,
    131.0f,
    200.0f,
    263.0f,
    300.0f,
    523.0f
};

struct sig_Buffer clockMultiplicationsBuffer = {
    .length = NUM_CLOCK_STEPS,
    .samples = clockMultiplications
};

struct sig_WavetableBank* lfoWavetables;
struct sig_AdditiveWavetableBank* bandlimitedWavetables;
struct sig_host_GateIn* clockIn;
struct sig_host_SwitchIn* tapTempoButton;
struct sig_dsp_ClockSource* clock;
struct sig_dsp_AdditiveWavetableBankOscillator* lfo1;
struct sig_dsp_WavetableBankOscillator* lfo2;
struct sig_dsp_WavetableBankOscillator* lfo3;
struct sig_dsp_AdditiveWavetableBankOscillator* lfo4;
struct sig_host_AudioOut* out1;
struct sig_host_CVOut* out2;
struct sig_host_CVOut* out3;
struct sig_host_AudioOut* out4;
struct sig_host_GateOut* clockOut;
struct sig_host_GateOut* led;

// TODO: Refactor this into a library function.
void buildWavetables() {
    lfoWavetables = sig_WavetableBank_new(&allocator,
        sig_StandardWaveform_COUNT, WAVETABLE_SIZE);

    for (size_t i = 0; i < lfoWavetables->length; i++) {
        struct sig_Buffer* wavetable = lfoWavetables->waves[i];
        sig_Buffer_fillWithWaveform(wavetable,
            sig_STANDARD_WAVEFORMS[i].generate, WAVETABLE_SIZE, 0.0f, 1.0f);
    }

    // The SDRAM is only available after the board has been initialized.
    wavetableAllocator.impl->init(&wavetableAllocator);
    bandlimitedWavetables = sig_AdditiveWavetableBank_new(
        &wavetableAllocator, sig_STANDARD_WAVEFORMS,
        sig_StandardWaveform_COUNT, NUM_BANDLIMITED_LEVELS, WAVETABLE_SIZE,
        LOWEST_BANDLIMITED_FREQUENCY, MAX_HARMONIC_FREQUENCY,
        sig_HarmonicTaper_LANCZOS);
}

struct sig_host_SummedCVIn* buildSummedCVIn(
    struct sig_SignalContext* context, struct sig_Status* status,
    int knob, int cv, float scale) {
    struct sig_host_SummedCVIn* summedCVIn = sig_host_SummedCVIn_new(
        &allocator, context);
    summedCVIn->hardware = &host.device.hardware;
    sig_List_append(&signals, summedCVIn, status);
    summedCVIn->leftCVIn->parameters.control = knob;
    summedCVIn->rightCVIn->parameters.control = cv;
    summedCVIn->leftCVIn->parameters.scale = scale;
    summedCVIn->rightCVIn->parameters.scale = scale;

    return summedCVIn;
}

// The wavetable index is cyclic, so the waveform knob and CV span
// up to the last table rather than crossfading back to the first.
struct sig_host_SummedCVIn* buildWaveformCV(
    struct sig_SignalContext* context, struct sig_Status* status,
    int knob, int cv, size_t numTables) {
    float lastTable = sig_osc_WavetableBank_tableIndex(numTables - 1,
        numTables);

    return buildSummedCVIn(context, status, knob, cv, lastTable);
}

// An LFO's frequency is the clock's frequency scaled by a ratio
// selected by the knob and CV from clockScales.
struct sig_dsp_ScaledClockFrequency* buildLFOFrequency(
    struct sig_SignalContext* context, struct sig_Status* status,
    struct sig_Buffer* clockScales, int knob, int cv) {
    // TODO: Add greater audio rate range for LFO1 and LFO4,
    // which are connected to DC-coupled audio outputs.
    float unityIndex = (float) UNITY_CLOCK_STEP /
        (float) (NUM_CLOCK_STEPS - 1);
    struct sig_host_SummedCVIn* frequencyCV = buildSummedCVIn(context,
        status, knob, cv, unityIndex);
    frequencyCV->rightCVIn->parameters.offset = unityIndex;

    struct sig_dsp_List* clockScale = sig_dsp_List_new(&allocator, context);
    sig_List_append(&signals, clockScale, status);
    clockScale->list = clockScales;
    clockScale->parameters.wrap = 0.0f;
    clockScale->parameters.interpolate = 1.0f;
    clockScale->inputs.index = frequencyCV->outputs.main;

    struct sig_dsp_ScaledClockFrequency* frequency =
        sig_dsp_ScaledClockFrequency_new(&allocator, context);
    sig_List_append(&signals, frequency, status);
    frequency->inputs.clock = clock->outputs.main;
    frequency->inputs.scale = clockScale->outputs.main;

    return frequency;
}

struct sig_dsp_WavetableBankOscillator* buildLFO(
    struct sig_SignalContext* context, struct sig_Status* status,
    struct sig_Buffer* clockScales, int frequencyKnob, int frequencyCV,
    int waveformKnob, int waveformCV) {
    struct sig_dsp_ScaledClockFrequency* frequency = buildLFOFrequency(
        context, status, clockScales, frequencyKnob, frequencyCV);
    struct sig_host_SummedCVIn* waveform = buildWaveformCV(context, status,
        waveformKnob, waveformCV, sig_StandardWaveform_COUNT);

    struct sig_dsp_WavetableBankOscillator* lfo =
        sig_dsp_WavetableBankOscillator_new(&allocator, context);
    sig_List_append(&signals, lfo, status);
    lfo->wavetables = lfoWavetables;
    lfo->inputs.freq = frequency->outputs.main;
    lfo->inputs.tableIndex = waveform->outputs.main;

    return lfo;
}

struct sig_dsp_AdditiveWavetableBankOscillator* buildBandlimitedLFO(
    struct sig_SignalContext* context, struct sig_Status* status,
    struct sig_Buffer* clockScales, int frequencyKnob, int frequencyCV,
    int waveformKnob, int waveformCV) {
    struct sig_dsp_ScaledClockFrequency* frequency = buildLFOFrequency(
        context, status, clockScales, frequencyKnob, frequencyCV);
    struct sig_host_SummedCVIn* waveform = buildWaveformCV(context, status,
        waveformKnob, waveformCV, sig_StandardWaveform_COUNT);

    struct sig_dsp_AdditiveWavetableBankOscillator* lfo =
        sig_dsp_AdditiveWavetableBankOscillator_new(&allocator, context);
    sig_List_append(&signals, lfo, status);
    lfo->wavetables = bandlimitedWavetables;
    lfo->inputs.freq = frequency->outputs.main;
    lfo->inputs.tableIndex = waveform->outputs.main;

    return lfo;
}

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    clockIn = sig_host_GateIn_new(&allocator, context);
    clockIn->hardware = &host.device.hardware;
    sig_List_append(&signals, clockIn, status);
    clockIn->parameters.control = sig_host_GATE_IN_1;

    tapTempoButton = sig_host_SwitchIn_new(&allocator, context);
    tapTempoButton->hardware = &host.device.hardware;
    sig_List_append(&signals, tapTempoButton, status);
    tapTempoButton->parameters.control = sig_host_TOGGLE_1;

    clock = sig_dsp_ClockSource_new(&allocator, context);
    sig_List_append(&signals, clock, status);
    clock->inputs.pulse = clockIn->outputs.main;
    clock->inputs.tap = tapTempoButton->outputs.main;

    buildWavetables();

    lfo1 = buildBandlimitedLFO(context, status, &clockMultiplicationsBuffer,
        sig_host_KNOB_1, sig_host_CV_IN_8, sig_host_KNOB_5, sig_host_CV_IN_1);

    out1 = sig_host_AudioOut_new(&allocator, context);
    out1->hardware = &host.device.hardware;
    sig_List_append(&signals, out1, status);
    out1->parameters.channel = sig_host_AUDIO_OUT_1;
    out1->inputs.source = lfo1->outputs.main;

    lfo2 = buildLFO(context, status, &clockDivisionsBuffer,
        sig_host_KNOB_2, sig_host_CV_IN_7, sig_host_KNOB_6, sig_host_CV_IN_2);

    out2 = sig_host_CVOut_new(&allocator, context);
    out2->hardware = &host.device.hardware;
    sig_List_append(&signals, out2, status);
    out2->parameters.control = sig_host_CV_OUT_1;
    out2->inputs.source = lfo2->outputs.main;

    lfo3 = buildLFO(context, status, &clockDivisionsBuffer,
        sig_host_KNOB_3, sig_host_CV_IN_5, sig_host_KNOB_7, sig_host_CV_IN_3);

    out3 = sig_host_CVOut_new(&allocator, context);
    out3->hardware = &host.device.hardware;
    sig_List_append(&signals, out3, status);
    out3->parameters.control = sig_host_CV_OUT_2;
    out3->inputs.source = lfo3->outputs.main;

    lfo4 = buildBandlimitedLFO(context, status, &clockMultiplicationsBuffer,
        sig_host_KNOB_4, sig_host_CV_IN_6, sig_host_KNOB_8, sig_host_CV_IN_4);

    out4 = sig_host_AudioOut_new(&allocator, context);
    out4->hardware = &host.device.hardware;
    sig_List_append(&signals, out4, status);
    out4->parameters.channel = sig_host_AUDIO_OUT_2;
    out4->inputs.source = lfo4->outputs.main;

    clockOut = sig_host_GateOut_new(&allocator, context);
    clockOut->hardware = &host.device.hardware;
    sig_List_append(&signals, clockOut, status);
    clockOut->parameters.control = sig_host_GATE_OUT_1;
    clockOut->inputs.source = clock->outputs.main;

    led = sig_host_GateOut_new(&allocator, context);
    led->hardware = &host.device.hardware;
    sig_List_append(&signals, led, status);
    led->parameters.control = sig_host_GATE_OUT_2;
    led->inputs.source = clock->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 48
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
