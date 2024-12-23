#include <libsignaletic.h>
#include "../../../../../include/lichen-four-device.hpp"
#include "dynamic-scale-offset.hpp"

struct ScaleOffsetScaleControl {
    struct sig_host_CVIn* in;
    struct sig_host_CVIn* scaleCV;
    struct sig_host_FilteredCVIn* offsetKnob;
    struct sig_dsp_DynamicScaleOffset* scaleOffset;
    struct sig_host_FilteredCVIn* postGainSlider;
    struct sig_dsp_BinaryOp* postGain;
    struct sig_dsp_Tanh* saturator;

    float_array_ptr output;
};

struct ScaleOffsetScaleControl_Config {
    int inputChannel;
    int scaleCVControl;
    int offsetKnobControl;
    int postGainSliderControl;
};

struct ScaleOffsetScaleControl* ScaleOffsetScaleControl_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct ScaleOffsetScaleControl* self = sig_MALLOC(allocator,
        struct ScaleOffsetScaleControl);

    self->in = sig_host_CVIn_new(allocator, context);
    self->scaleCV = sig_host_CVIn_new(allocator, context);
    self->offsetKnob = sig_host_FilteredCVIn_new(allocator, context);
    self->postGainSlider = sig_host_FilteredCVIn_new(allocator, context);
    self->scaleOffset = sig_dsp_DynamicScaleOffset_new(allocator, context);
    self->postGain = sig_dsp_Mul_new(allocator, context);
    self->saturator = sig_dsp_Tanh_new(allocator, context);

    return self;
}

void ScaleOffsetScaleControl_init(struct ScaleOffsetScaleControl* self,
    DaisyHost<lichen::four::FourDevice>* host,
    struct ScaleOffsetScaleControl_Config config,
    struct sig_List* signals, struct sig_Status* status) {
    self->in->hardware = &host->device.hardware;
    sig_List_append(signals, self->in, status);
    self->in->parameters.control = config.inputChannel;

    self->scaleCV->hardware = &host->device.hardware;
    sig_List_append(signals, self->scaleCV, status);
    self->scaleCV->parameters.control = config.scaleCVControl;
    self->scaleCV->parameters.scale = 0.5f;
    self->scaleCV->parameters.offset = 0.5f;

    self->offsetKnob->hardware = &host->device.hardware;
    sig_List_append(signals, self->offsetKnob, status);
    self->offsetKnob->parameters.control = config.offsetKnobControl;

    self->postGainSlider->hardware = &host->device.hardware;
    sig_List_append(signals, self->postGainSlider, status);
    self->postGainSlider->parameters.control = config.postGainSliderControl;

    sig_List_append(signals, self->scaleOffset, status);
    self->scaleOffset->inputs.source = self->in->outputs.main;
    self->scaleOffset->inputs.scale = self->scaleCV->outputs.main;
    self->scaleOffset->inputs.offset = self->offsetKnob->outputs.main;

    sig_List_append(signals, self->postGain, status);
    self->postGain->inputs.left = self->scaleOffset->outputs.main;
    self->postGain->inputs.right = self->postGainSlider->outputs.main;

    self->saturator->inputs.source = self->postGain->outputs.main;

    self->output = self->saturator->outputs.main;
}
