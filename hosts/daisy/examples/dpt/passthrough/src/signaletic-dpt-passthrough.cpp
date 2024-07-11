#include "../../../../include/dspcoffee-dpt-device.hpp"

#define SAMPLERATE 96000
#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 128

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

DaisyHost<dspcoffee::dpt::DPTDevice> host;

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals = {
    .items = (void**) &listStorage,
    .capacity = MAX_NUM_SIGNALS,
    .length = 0
};

struct sig_dsp_SignalListEvaluator* evaluator;
struct sig_host_CVIn* cv1In;
struct sig_host_CVIn* cv2In;
struct sig_host_CVIn* cv3In;
struct sig_host_CVIn* cv4In;
struct sig_host_CVIn* cv5In;
struct sig_host_CVIn* cv6In;
struct sig_host_CVOut* cv1Out;
struct sig_host_CVOut* cv2Out;
struct sig_host_CVOut* cv3Out;
struct sig_host_CVOut* cv4Out;
struct sig_host_CVOut* cv5Out;
struct sig_host_CVOut* cv6Out;
struct sig_host_GateIn* gate1In;
struct sig_host_GateIn* gate2In;
struct sig_host_GateOut* gate1Out;
struct sig_host_GateOut* gate2Out;
struct sig_host_AudioIn* leftAudioIn;
struct sig_host_AudioIn* rightAudioIn;
struct sig_host_AudioOut* leftAudioOut;
struct sig_host_AudioOut* rightAudioOut;

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    cv1In = sig_host_CVIn_new(&allocator, context);
    cv1In->hardware = &host.device.hardware;
    sig_List_append(&signals, cv1In, status);
    cv1In->parameters.control = sig_host_CV_IN_1;

    cv2In = sig_host_CVIn_new(&allocator, context);
    cv2In->hardware = &host.device.hardware;
    sig_List_append(&signals, cv2In, status);
    cv2In->parameters.control = sig_host_CV_IN_2;

    cv3In = sig_host_CVIn_new(&allocator, context);
    cv3In->hardware = &host.device.hardware;
    sig_List_append(&signals, cv3In, status);
    cv3In->parameters.control = sig_host_CV_IN_3;

    cv4In = sig_host_CVIn_new(&allocator, context);
    cv4In->hardware = &host.device.hardware;
    sig_List_append(&signals, cv4In, status);
    cv4In->parameters.control = sig_host_CV_IN_4;

    cv5In = sig_host_CVIn_new(&allocator, context);
    cv5In->hardware = &host.device.hardware;
    sig_List_append(&signals, cv5In, status);
    cv5In->parameters.control = sig_host_CV_IN_5;

    cv6In = sig_host_CVIn_new(&allocator, context);
    cv6In->hardware = &host.device.hardware;
    sig_List_append(&signals, cv6In, status);
    cv6In->parameters.control = sig_host_CV_IN_6;

    cv1Out = sig_host_CVOut_new(&allocator, context);
    cv1Out->hardware = &host.device.hardware;
    sig_List_append(&signals, cv1Out, status);
    cv1Out->inputs.source = cv1In->outputs.main;
    cv1Out->parameters.control = sig_host_CV_OUT_1;

    cv2Out = sig_host_CVOut_new(&allocator, context);
    cv2Out->hardware = &host.device.hardware;
    sig_List_append(&signals, cv2Out, status);
    cv2Out->inputs.source = cv2In->outputs.main;
    cv2Out->parameters.control = sig_host_CV_OUT_2;

    cv3Out = sig_host_CVOut_new(&allocator, context);
    cv3Out->hardware = &host.device.hardware;
    sig_List_append(&signals, cv3Out, status);
    cv3Out->inputs.source = cv3In->outputs.main;
    cv3Out->parameters.control = sig_host_CV_OUT_3;

    cv4Out = sig_host_CVOut_new(&allocator, context);
    cv4Out->hardware = &host.device.hardware;
    sig_List_append(&signals, cv4Out, status);
    cv4Out->inputs.source = cv4In->outputs.main;
    cv4Out->parameters.control = sig_host_CV_OUT_4;

    cv5Out = sig_host_CVOut_new(&allocator, context);
    cv5Out->hardware = &host.device.hardware;
    sig_List_append(&signals, cv5Out, status);
    cv5Out->inputs.source = cv5In->outputs.main;
    cv5Out->parameters.control = sig_host_CV_OUT_5;

    cv6Out = sig_host_CVOut_new(&allocator, context);
    cv6Out->hardware = &host.device.hardware;
    sig_List_append(&signals, cv6Out, status);
    cv6Out->inputs.source = cv6In->outputs.main;
    cv6Out->parameters.control = sig_host_CV_OUT_6;

    gate1In = sig_host_GateIn_new(&allocator, context);
    gate1In->hardware = &host.device.hardware;
    sig_List_append(&signals, gate1In, status);
    gate1In->parameters.control = sig_host_GATE_IN_1;

    gate2In = sig_host_GateIn_new(&allocator, context);
    gate2In->hardware = &host.device.hardware;
    sig_List_append(&signals, gate2In, status);
    gate2In->parameters.control = sig_host_GATE_IN_2;

    gate1Out = sig_host_GateOut_new(&allocator, context);
    gate1Out->hardware = &host.device.hardware;
    sig_List_append(&signals, gate1Out, status);
    gate1Out->parameters.control = sig_host_GATE_OUT_1;
    gate1Out->inputs.source = gate1In->outputs.main;

    gate2Out = sig_host_GateOut_new(&allocator, context);
    gate2Out->hardware = &host.device.hardware;
    sig_List_append(&signals, gate2Out, status);
    gate2Out->parameters.control = sig_host_GATE_OUT_2;
    gate2Out->inputs.source = gate2In->outputs.main;

    leftAudioIn = sig_host_AudioIn_new(&allocator, context);
    leftAudioIn->hardware = &host.device.hardware;
    sig_List_append(&signals, leftAudioIn, status);
    leftAudioIn->parameters.channel = sig_host_AUDIO_IN_1;

    rightAudioIn = sig_host_AudioIn_new(&allocator, context);
    rightAudioIn->hardware = &host.device.hardware;
    sig_List_append(&signals, rightAudioIn, status);
    rightAudioIn->parameters.channel = sig_host_AUDIO_IN_2;

    leftAudioOut = sig_host_AudioOut_new(&allocator, context);
    leftAudioOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftAudioOut, status);
    leftAudioOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftAudioOut->inputs.source = leftAudioIn->outputs.main;


    rightAudioOut = sig_host_AudioOut_new(&allocator, context);
    rightAudioOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightAudioOut, status);
    rightAudioOut->parameters.channel = sig_host_AUDIO_IN_2;
    rightAudioOut->inputs.source = rightAudioIn->outputs.main;
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

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    struct sig_SignalContext* context = sig_SignalContext_new(
        &allocator, &audioSettings);
    buildSignalGraph(context, &status);
    host.Start();

    while (1) {}
}
