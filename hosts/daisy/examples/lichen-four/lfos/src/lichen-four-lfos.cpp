#include <libsignaletic.h>
#include "../../../shared/include/clocked-lfo.h"
#include "../../../shared/include/summed-cv-in.h"
#include "../../../../include/lichen-four-device.hpp"

using namespace lichen::four;
using namespace sig::libdaisy;

#define SAMPLERATE 96000
#define HEAP_SIZE 1024 * 384 // 384 KB
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
DaisyHost<FourDevice> host;

struct sig_host_GateIn* clockIn;
struct sig_host_SwitchIn* tapTempoButton;
struct sig_dsp_ClockSource* clock;
struct sig_host_SummedCVIn* lfo1Frequency;
struct sig_host_SummedCVIn* lfo2Frequency;
struct sig_host_SummedCVIn* lfo3Frequency;
struct sig_host_SummedCVIn* lfo4Frequency;
struct sig_host_SummedCVIn* lfo1Scale;
struct sig_host_SummedCVIn* lfo2Scale;
struct sig_host_SummedCVIn* lfo3Scale;
struct sig_host_SummedCVIn* lfo4Scale;
struct sig_dsp_ClockedLFO* lfo1;
struct sig_dsp_ClockedLFO* lfo2;
struct sig_dsp_ClockedLFO* lfo3;
struct sig_dsp_ClockedLFO* lfo4;
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

    lfo1Frequency = sig_host_SummedCVIn_new(&allocator, context);
    lfo1Frequency->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo1Frequency, status);
    lfo1Frequency->leftCVIn->parameters.control = sig_host_KNOB_1;
    lfo1Frequency->rightCVIn->parameters.control = sig_host_CV_IN_8;
    lfo1Frequency->leftCVIn->parameters.scale = 10.0f;
    lfo1Frequency->rightCVIn->parameters.scale = 10.0f;

    lfo1Scale = sig_host_SummedCVIn_new(&allocator, context);
    lfo1Scale->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo1Scale, status);
    lfo1Scale->leftCVIn->parameters.control = sig_host_KNOB_5;
    lfo1Scale->rightCVIn->parameters.control = sig_host_CV_IN_1;
    lfo1Scale->leftCVIn->parameters.scale = 0.5525f;
    lfo1Scale->rightCVIn->parameters.scale = 0.4f;

    lfo1 = sig_dsp_ClockedLFO_new(&allocator, context);
    sig_List_append(&signals, lfo1, status);
    lfo1->inputs.clock = clock->outputs.main;
    lfo1->inputs.frequencyScale = lfo1Frequency->outputs.main;
    lfo1->inputs.scale = lfo1Scale->outputs.main;

    out1 = sig_host_AudioOut_new(&allocator, context);
    out1->hardware = &host.device.hardware;
    sig_List_append(&signals, out1, status);
    out1->parameters.channel = sig_host_AUDIO_OUT_1;
    out1->inputs.source = lfo1->outputs.main;

    lfo2Frequency = sig_host_SummedCVIn_new(&allocator, context);
    lfo2Frequency->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo2Frequency, status);    lfo2Frequency->leftCVIn->parameters.control = sig_host_KNOB_2;
    lfo2Frequency->rightCVIn->parameters.control = sig_host_CV_IN_7;
    lfo2Frequency->leftCVIn->parameters.scale = 10.0f;
    lfo2Frequency->rightCVIn->parameters.scale = 10.0f;

    lfo2Scale = sig_host_SummedCVIn_new(&allocator, context);
    lfo2Scale->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo2Scale, status);
    lfo2Scale->leftCVIn->parameters.control = sig_host_KNOB_6;
    lfo2Scale->rightCVIn->parameters.control = sig_host_CV_IN_2;
    // TODO: Clipping!
    lfo2Scale->leftCVIn->parameters.scale = 0.95f;
    lfo2Scale->rightCVIn->parameters.scale = 0.4f;

    lfo2 = sig_dsp_ClockedLFO_new(&allocator, context);
    sig_List_append(&signals, lfo2, status);
    lfo2->inputs.clock = clock->outputs.main;
    lfo2->inputs.frequencyScale = lfo2Frequency->outputs.main;
    lfo2->inputs.scale = lfo2Scale->outputs.main;

    out2 = sig_host_CVOut_new(&allocator, context);
    out2->hardware = &host.device.hardware;
    sig_List_append(&signals, out2, status);
    out2->parameters.control = sig_host_CV_OUT_1;
    out2->inputs.source = lfo2->outputs.main;

    lfo3Frequency = sig_host_SummedCVIn_new(&allocator, context);
    lfo3Frequency->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo3Frequency, status);    lfo3Frequency->leftCVIn->parameters.control = sig_host_KNOB_3;
    lfo3Frequency->rightCVIn->parameters.control = sig_host_CV_IN_5;
    lfo3Frequency->leftCVIn->parameters.scale = 10.0f;
    lfo3Frequency->rightCVIn->parameters.scale = 10.0f;

    lfo3Scale = sig_host_SummedCVIn_new(&allocator, context);
    lfo3Scale->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo3Scale, status);
    lfo3Scale->leftCVIn->parameters.control = sig_host_KNOB_7;
    lfo3Scale->rightCVIn->parameters.control = sig_host_CV_IN_3;
    // TODO: Clipping!
    lfo3Scale->leftCVIn->parameters.scale = 0.95f;
    lfo3Scale->rightCVIn->parameters.scale = 0.4f;

    lfo3 = sig_dsp_ClockedLFO_new(&allocator, context);
    sig_List_append(&signals, lfo3, status);
    lfo3->inputs.clock = clock->outputs.main;
    lfo3->inputs.frequencyScale = lfo3Frequency->outputs.main;
    lfo3->inputs.scale = lfo3Scale->outputs.main;

    out3 = sig_host_CVOut_new(&allocator, context);
    out3->hardware = &host.device.hardware;
    sig_List_append(&signals, out3, status);
    out3->parameters.control = sig_host_CV_OUT_2;
    out3->inputs.source = lfo3->outputs.main;

    lfo4Frequency = sig_host_SummedCVIn_new(&allocator, context);
    lfo4Frequency->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo4Frequency, status);    lfo4Frequency->leftCVIn->parameters.control = sig_host_KNOB_4;
    lfo4Frequency->rightCVIn->parameters.control = sig_host_CV_IN_6;
    lfo4Frequency->leftCVIn->parameters.scale = 10.0f;
    lfo4Frequency->rightCVIn->parameters.scale = 10.0f;

    lfo4Scale = sig_host_SummedCVIn_new(&allocator, context);
    lfo4Scale->hardware = &host.device.hardware;
    sig_List_append(&signals, lfo4Scale, status);
    lfo4Scale->leftCVIn->parameters.control = sig_host_KNOB_8;
    lfo4Scale->rightCVIn->parameters.control = sig_host_CV_IN_4;
    lfo4Scale->leftCVIn->parameters.scale = 0.5525f;
    lfo4Scale->rightCVIn->parameters.scale = 0.4f;

    lfo4 = sig_dsp_ClockedLFO_new(&allocator, context);
    sig_List_append(&signals, lfo4, status);
    lfo4->inputs.clock = clock->outputs.main;
    lfo4->inputs.frequencyScale = lfo4Frequency->outputs.main;
    lfo4->inputs.scale = lfo4Scale->outputs.main;

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
