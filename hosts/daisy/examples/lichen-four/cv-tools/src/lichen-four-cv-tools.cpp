#include <libsignaletic.h>
#include "../../../../include/lichen-four-device.hpp"
#include "../include/scale-offset-scale-control.hpp"

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

struct sig_host_SwitchIn* mixModeButton;
struct ScaleOffsetScaleControl* control1;
struct sig_host_AudioOut* out1;
struct ScaleOffsetScaleControl* control2;
struct sig_host_CVOut* out2;
struct ScaleOffsetScaleControl* control3;
struct sig_host_CVOut* out3;
struct ScaleOffsetScaleControl* control4;
struct sig_host_AudioOut* out4;

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    // TODO:
    struct ScaleOffsetScaleControl_Config control1Config = {
        .inputChannel = sig_host_CV_IN_1,
        .scaleCVControl = sig_host_CV_IN_8,
        .offsetKnobControl = sig_host_KNOB_5,
        .postGainSliderControl = sig_host_KNOB_1
    };

    control1 = ScaleOffsetScaleControl_new(&allocator, context);
    ScaleOffsetScaleControl_init(control1, &host, control1Config, &signals,
        status);

    out1 = sig_host_AudioOut_new(&allocator, context);
    out1->hardware = &host.device.hardware;
    sig_List_append(&signals, out1, status);
    out1->parameters.channel = sig_host_AUDIO_OUT_1;
    out1->inputs.source = control1->output;

    struct ScaleOffsetScaleControl_Config control2Config = {
        .inputChannel = sig_host_CV_IN_2,
        .scaleCVControl = sig_host_CV_IN_7,
        .offsetKnobControl = sig_host_KNOB_6,
        .postGainSliderControl = sig_host_KNOB_2
    };

    control2 = ScaleOffsetScaleControl_new(&allocator, context);
    ScaleOffsetScaleControl_init(control2, &host, control2Config, &signals,
        status);

    out2 = sig_host_CVOut_new(&allocator, context);
    out2->hardware = &host.device.hardware;
    sig_List_append(&signals, out2, status);
    out2->parameters.control = sig_host_CV_OUT_1;
    out2->inputs.source = control2->output;

    struct ScaleOffsetScaleControl_Config control3Config = {
        .inputChannel = sig_host_CV_IN_3,
        .scaleCVControl = sig_host_CV_IN_5,
        .offsetKnobControl = sig_host_KNOB_7,
        .postGainSliderControl = sig_host_KNOB_3
    };

    control3 = ScaleOffsetScaleControl_new(&allocator, context);
    ScaleOffsetScaleControl_init(control3, &host, control3Config, &signals,
        status);

    out3 = sig_host_CVOut_new(&allocator, context);
    out3->hardware = &host.device.hardware;
    sig_List_append(&signals, out3, status);
    out3->parameters.control = sig_host_CV_OUT_2;
    out3->inputs.source = control3->output;

    struct ScaleOffsetScaleControl_Config control4Config = {
        .inputChannel = sig_host_CV_IN_4,
        .scaleCVControl = sig_host_CV_IN_6,
        .offsetKnobControl = sig_host_KNOB_8,
        .postGainSliderControl = sig_host_KNOB_4
    };

    control4 = ScaleOffsetScaleControl_new(&allocator, context);
    ScaleOffsetScaleControl_init(control4, &host, control4Config, &signals,
        status);

    out4 = sig_host_AudioOut_new(&allocator, context);
    out4->hardware = &host.device.hardware;
    sig_List_append(&signals, out4, status);
    out4->parameters.channel = sig_host_AUDIO_OUT_2;
    out4->inputs.source = control4->output;
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
