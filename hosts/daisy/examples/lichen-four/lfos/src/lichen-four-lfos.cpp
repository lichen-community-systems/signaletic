#include <libsignaletic.h>
#include "../../../../include/lichen-four-device.hpp"
#include "../include/clock-dividing-lfo.h"

using namespace lichen::four;
using namespace sig::libdaisy;

#define SAMPLERATE 96000
#define HEAP_SIZE 1024 * 384 // 384 KB
#define MAX_NUM_SIGNALS 32
#define NUM_CLOCK_DIVISIONS 37

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
DaisyHost<FourDevice> host;

float clockDivisions[NUM_CLOCK_DIVISIONS] = {
    // Frequency multiplications and divisions,
    // which are mapped to the lfo frequency slider control.
    1.0f/100.0f,
    1.0f/30.0f,
    1.0f/29.0f,
    1.0f/23.0f,
    1.0f/20.0f,
    1.0f/19.0f,
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
    30.0f,
    100.0f
};

struct sig_Buffer clockDivisionsBuffer = {
    .length = NUM_CLOCK_DIVISIONS,
    .samples = clockDivisions
};

struct sig_host_GateIn* clockIn;
struct sig_host_SwitchIn* tapTempoButton;
struct sig_dsp_ClockSource* clock;
struct sig_host_ClockDividingLFO* lfo1;
struct sig_host_ClockDividingLFO* lfo2;
struct sig_host_ClockDividingLFO* lfo3;
struct sig_host_ClockDividingLFO* lfo4;
struct sig_host_AudioOut* out1;
struct sig_host_CVOut* out2;
struct sig_host_CVOut* out3;
struct sig_host_AudioOut* out4;
struct sig_host_GateOut* clockOut;
struct sig_host_GateOut* led;

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

    lfo1 = sig_host_ClockDividingLFO_new(&allocator, context);
    lfo1->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo1, status);
    lfo1->clockDivisionsBuffer = &clockDivisionsBuffer;
    lfo1->inputs.clock = clock->outputs.main;
    lfo1->frequencyCV->leftCVIn->parameters.control = sig_host_KNOB_1;
    lfo1->frequencyCV->rightCVIn->parameters.control = sig_host_CV_IN_8;
    // TODO: These scalings result a deadzone at the bottom of the
    // range when the bipolar CV input is in the negative and the
    // slider is at the bottom (0.0f).
    // TODO: Add greater audio rate range for LFO1 and LFO4,
    // which are connected to DC-coupled audio outputs.
    lfo1->frequencyCV->leftCVIn->parameters.scale = 0.75f;
    lfo1->frequencyCV->rightCVIn->parameters.scale = 0.25f;
    lfo1->scale->leftCVIn->parameters.control = sig_host_KNOB_5;
    lfo1->scale->rightCVIn->parameters.control = sig_host_CV_IN_1;
    lfo1->scale->leftCVIn->parameters.scale = 0.5525f;
    lfo1->scale->rightCVIn->parameters.scale = 0.4f;

    out1 = sig_host_AudioOut_new(&allocator, context);
    out1->hardware = &host.device.hardware;
    sig_List_append(&signals, out1, status);
    out1->parameters.channel = sig_host_AUDIO_OUT_1;
    out1->inputs.source = lfo1->outputs.main;

    lfo2 = sig_host_ClockDividingLFO_new(&allocator, context);
    lfo2->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo2, status);
    lfo2->clockDivisionsBuffer = &clockDivisionsBuffer;
    lfo2->inputs.clock = clock->outputs.main;
    lfo2->frequencyCV->leftCVIn->parameters.control = sig_host_KNOB_2;
    lfo2->frequencyCV->rightCVIn->parameters.control = sig_host_CV_IN_7;
    lfo2->frequencyCV->leftCVIn->parameters.scale = 0.75f;
    lfo2->frequencyCV->rightCVIn->parameters.scale = 0.25f;
    lfo2->scale->leftCVIn->parameters.control = sig_host_KNOB_6;
    lfo2->scale->rightCVIn->parameters.control = sig_host_CV_IN_2;
    // TODO: Clipping!
    lfo2->scale->leftCVIn->parameters.scale = 0.95f;
    lfo2->scale->rightCVIn->parameters.scale = 0.4f;

    out2 = sig_host_CVOut_new(&allocator, context);
    out2->hardware = &host.device.hardware;
    sig_List_append(&signals, out2, status);
    out2->parameters.control = sig_host_CV_OUT_1;
    out2->inputs.source = lfo2->outputs.main;

    lfo3 = sig_host_ClockDividingLFO_new(&allocator, context);
    lfo3->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo3, status);
    lfo3->clockDivisionsBuffer = &clockDivisionsBuffer;
    lfo3->inputs.clock = clock->outputs.main;
    lfo3->frequencyCV->leftCVIn->parameters.control = sig_host_KNOB_3;
    lfo3->frequencyCV->rightCVIn->parameters.control = sig_host_CV_IN_5;
    lfo3->frequencyCV->leftCVIn->parameters.scale = 0.75f;
    lfo3->frequencyCV->rightCVIn->parameters.scale = 0.25f;
    lfo3->scale->leftCVIn->parameters.control = sig_host_KNOB_7;
    lfo3->scale->rightCVIn->parameters.control = sig_host_CV_IN_3;
    lfo3->scale->leftCVIn->parameters.scale = 0.95f;
    lfo3->scale->rightCVIn->parameters.scale = 0.4f;

    out3 = sig_host_CVOut_new(&allocator, context);
    out3->hardware = &host.device.hardware;
    sig_List_append(&signals, out3, status);
    out3->parameters.control = sig_host_CV_OUT_2;
    out3->inputs.source = lfo3->outputs.main;

    lfo4 = sig_host_ClockDividingLFO_new(&allocator, context);
    lfo4->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo4, status);
    lfo4->clockDivisionsBuffer = &clockDivisionsBuffer;
    lfo4->inputs.clock = clock->outputs.main;
    lfo4->frequencyCV->leftCVIn->parameters.control = sig_host_KNOB_4;
    lfo4->frequencyCV->rightCVIn->parameters.control = sig_host_CV_IN_6;
    lfo4->frequencyCV->leftCVIn->parameters.scale = 0.75f;
    lfo4->frequencyCV->rightCVIn->parameters.scale = 0.25f;
    lfo4->scale->leftCVIn->parameters.control = sig_host_KNOB_8;
    lfo4->scale->rightCVIn->parameters.control = sig_host_CV_IN_4;
    lfo4->scale->leftCVIn->parameters.scale = 0.5525f;
    lfo4->scale->rightCVIn->parameters.scale = 0.4f;

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
        .blockSize = 1
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
