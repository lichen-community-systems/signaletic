#include <libsignaletic.h>
#include "../../../../include/kxmx-nehcmeulb-device.hpp"

#define SAMPLERATE 48000
#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 32

FixedCapStr<20> displayStr;

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
sig::libdaisy::DaisyHost<kxmx::bluemchen::NehcmeulbDevice> host;

struct sig_host_EncoderIn* tapTempo;
struct sig_host_FilteredCVIn* densityKnob;
struct sig_host_FilteredCVIn* durationKnob;
struct sig_host_CVIn* clockIn;
struct sig_dsp_BinaryOp* clockPulseSum;
struct sig_dsp_ClockDetector* clock;
struct sig_dsp_BinaryOp* densityClockSum;
struct sig_dsp_DustGate* cvDustGate;
struct sig_dsp_BinaryOp* audioDensity;
struct sig_dsp_ConstantValue* audioDensityScale;
struct sig_dsp_DustGate* audioDustGate;
struct sig_host_CVOut* dustCVOut;
struct sig_host_CVOut* clockCVOut;
struct sig_host_AudioOut* leftAudioOut;
struct sig_host_AudioOut* rightAudioOut;

void UpdateOled() {
    host.device.display.Fill(false);

    displayStr.Clear();
    displayStr.Append(" Dust Dust");
    host.device.display.SetCursor(0, 0);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    float density = densityClockSum->outputs.main[0];
    float formatted = 0.0f;
    if (density < 6.0f) {
        formatted = density * 60.0f;
        displayStr.Append("bpm");
    } else {
        formatted = density;
        displayStr.Append(" Hz");
    }

    host.device.display.SetCursor(44, 16);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.AppendFloat(formatted, 0);
    host.device.display.SetCursor(0, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_11x18, true);

    float gateState = cvDustGate->outputs.main[0];
    if (gateState > 0.0f) {
        displayStr.Clear();
        displayStr.Append("o");
        host.device.display.SetCursor(28, 24);
        host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);
    }

    host.device.display.Update();
}

void buildGraph(struct sig_SignalContext* context, struct sig_Status* status) {
    tapTempo = sig_host_EncoderIn_new(&allocator, context);
    tapTempo->hardware = &host.device.hardware;
    sig_List_append(&signals, tapTempo, status);

    densityKnob = sig_host_FilteredCVIn_new(&allocator, context);
    densityKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, densityKnob, status);
    densityKnob->parameters.control = sig_host_KNOB_1;
    densityKnob->parameters.scale = 10.0f;

    durationKnob = sig_host_FilteredCVIn_new(&allocator, context);
    durationKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, durationKnob, status);
    durationKnob->parameters.control = sig_host_KNOB_2;

    // Nehcmeulb splits the input coming from the audio in jacks
    // to both the ADC and the audio codec inputs.
    // Here, we want to read DC pulses so the CV input is the right place.
    clockIn = sig_host_CVIn_new(&allocator, context);
    clockIn->hardware = &host.device.hardware;
    sig_List_append(&signals, clockIn, status);
    clockIn->parameters.control = sig_host_CV_IN_1;

    clockPulseSum = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, clockPulseSum, status);
    clockPulseSum->inputs.left = tapTempo->outputs.button;
    clockPulseSum->inputs.right = clockIn->outputs.main;

    clock = sig_dsp_ClockDetector_new(&allocator, context);
    sig_List_append(&signals, clock, status);
    clock->inputs.source = clockPulseSum->outputs.main;

    // TODO: Add a CV input for controlling density,
    // which is centred on 0.0 and allows for negative values
    // so that an incoming clock can be slowed down.

    densityClockSum = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, densityClockSum, status);
    densityClockSum->inputs.left = clock->outputs.main;
    densityClockSum->inputs.right = densityKnob->outputs.main;

    cvDustGate = sig_dsp_DustGate_new(&allocator, context);
    sig_List_append(&signals, cvDustGate, status);
    cvDustGate->inputs.density = densityClockSum->outputs.main;
    cvDustGate->inputs.durationPercentage = durationKnob->outputs.main;

    dustCVOut = sig_host_CVOut_new(&allocator, context);
    dustCVOut->hardware = &host.device.hardware;
    sig_List_append(&signals, dustCVOut, status);
    dustCVOut->parameters.control = sig_host_CV_OUT_1;
    dustCVOut->inputs.source = cvDustGate->outputs.main;

    // TODO: Output an envelope on each trigger on CV_OUT_2

    clockCVOut = sig_host_CVOut_new(&allocator, context);
    clockCVOut->hardware = &host.device.hardware;
    sig_List_append(&signals, clockCVOut, status);
    clockCVOut->parameters.control = sig_host_CV_OUT_2;
    clockCVOut->inputs.source = cvDustGate->outputs.main;

    audioDensityScale = sig_dsp_ConstantValue_new(&allocator, context, 200.0f);

    audioDensity = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, audioDensity, status);
    audioDensity->inputs.left = densityClockSum->outputs.main;
    audioDensity->inputs.right = audioDensityScale->outputs.main;

    audioDustGate = sig_dsp_DustGate_new(&allocator, context);
    sig_List_append(&signals, audioDustGate, status);
    audioDustGate->parameters.bipolar = 1.0f;
    audioDustGate->inputs.density = audioDensity->outputs.main;
    audioDustGate->inputs.durationPercentage = durationKnob->outputs.main;

    leftAudioOut = sig_host_AudioOut_new(&allocator, context);
    leftAudioOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftAudioOut, status);
    leftAudioOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftAudioOut->inputs.source = audioDustGate->outputs.main;

    rightAudioOut = sig_host_AudioOut_new(&allocator, context);
    rightAudioOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightAudioOut, status);
    rightAudioOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightAudioOut->inputs.source = audioDustGate->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 48
    };

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    buildGraph(context, &status);

    host.Start();

    while (1) {
        UpdateOled();
    }
}
