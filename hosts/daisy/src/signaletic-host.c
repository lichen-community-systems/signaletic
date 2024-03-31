#include "../include/signaletic-host.h"

static struct sig_host_HardwareInterface* sig_host_globalHardwareInterface;

void sig_host_registerGlobalHardwareInterface(
    struct sig_host_HardwareInterface* hardware) {
    sig_host_globalHardwareInterface = hardware;
}

struct sig_host_HardwareInterface* sig_host_getGlobalHardwareInterface() {
    return sig_host_globalHardwareInterface;
}

void sig_host_noOpAudioEventCallback(size_t size,
    struct sig_host_HardwareInterface* hardware) {};

struct sig_host_GateIn* sig_host_GateIn_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_host_GateIn* self = sig_MALLOC(allocator,
        struct sig_host_GateIn);
    sig_host_GateIn_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}


void sig_host_GateIn_init(struct sig_host_GateIn* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_GateIn_generate);
    self->parameters.scale = 1.0f;
    self->parameters.offset = 0.0f;
    self->parameters.control = 0;
}

void sig_host_GateIn_generate(void* signal) {
    struct sig_host_GateIn* self = (struct sig_host_GateIn*) signal;
    struct sig_host_HardwareInterface* hardware = self->hardware;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float sample = hardware->gateInputs[control];
        FLOAT_ARRAY(self->outputs.main)[i] = sample * scale + offset;
    }
}

void sig_host_GateIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_GateIn* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

struct sig_host_GateOut* sig_host_GateOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_host_GateOut* self = sig_MALLOC(allocator,
        struct sig_host_GateOut);
    sig_host_GateOut_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_host_GateOut_init(struct sig_host_GateOut* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_GateOut_generate);
    self->parameters.scale = 1.0f;
    self->parameters.offset = 0.0f;
    self->parameters.control = 0;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_host_GateOut_generate(void* signal) {
    struct sig_host_GateOut* self = (struct sig_host_GateOut*) signal;
    struct sig_host_HardwareInterface* hardware = self->hardware;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float value = FLOAT_ARRAY(self->inputs.source)[i] * scale + offset;
        FLOAT_ARRAY(self->outputs.main)[i] = value;
        // TODO: Should this only be called at the block rate?
        hardware->gpioOutputs[control] = value;
    }
}

void sig_host_GateOut_destroy(struct sig_Allocator* allocator,
    struct sig_host_GateOut* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_host_CVIn* sig_host_CVIn_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_host_CVIn* self = sig_MALLOC(allocator,
        struct sig_host_CVIn);
    sig_host_CVIn_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_host_CVIn_init(struct sig_host_CVIn* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_CVIn_generate);
    self->parameters.scale = 1.0f;
    self->parameters.offset = 0.0f;
    self->parameters.control = 0;
}

void sig_host_CVIn_generate(void* signal) {
    struct sig_host_CVIn* self = (struct sig_host_CVIn*) signal;
    struct sig_host_HardwareInterface* hardware = self->hardware;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    float rawCV = hardware->adcChannels[control];
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float sample = rawCV * scale + offset;
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
    }
}

void sig_host_CVIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_CVIn* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_host_FilteredCVIn* sig_host_FilteredCVIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_host_FilteredCVIn* self = sig_MALLOC(allocator,
        struct sig_host_FilteredCVIn);
    self->cvIn = sig_host_CVIn_new(allocator, context);
    self->filter = sig_dsp_Smooth_new(allocator, context);
    sig_host_FilteredCVIn_init(self, context);

    return self;
}

void sig_host_FilteredCVIn_init(struct sig_host_FilteredCVIn* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_FilteredCVIn_generate);
    self->parameters.scale = 1.0f;
    self->parameters.offset = 0.0f;
    self->parameters.control = 0;
    self->parameters.time = 0.01f;
    self->filter->inputs.source = self->cvIn->outputs.main;
    self->outputs = self->filter->outputs;
}

void sig_host_FilteredCVIn_generate(void* signal) {
    struct sig_host_FilteredCVIn* self =
        (struct sig_host_FilteredCVIn*) signal;
    // TODO: We have to update these parameters
    // at block rate because there's no way to know if there
    // was actually a parameter change made and parameters are
    // stored by value, not by pointer.
    self->cvIn->hardware = self->hardware;
    self->filter->parameters.time = self->parameters.time;
    self->cvIn->parameters.control = self->parameters.control;
    self->cvIn->parameters.scale = self->parameters.scale;
    self->cvIn->parameters.offset = self->parameters.offset;

    self->cvIn->signal.generate(self->cvIn);
    self->filter->signal.generate(self->filter);
}

void sig_host_FilteredCVIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_FilteredCVIn* self) {
    sig_host_CVIn_destroy(allocator, self->cvIn);
    sig_dsp_Smooth_destroy(allocator, self->filter);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_host_VOctCVIn* sig_host_VOctCVIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_host_VOctCVIn* self = sig_MALLOC(allocator,
        struct sig_host_VOctCVIn);
    self->cvIn = sig_host_CVIn_new(allocator, context);
    self->cvConverter = sig_dsp_LinearToFreq_new(allocator, context);
    sig_host_VOctCVIn_init(self, context);

    return self;
}

void sig_host_VOctCVIn_init(struct sig_host_VOctCVIn* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_VOctCVIn_generate);
    self->parameters.scale = 1.0f;
    self->parameters.offset = 0.0f;
    self->parameters.control = 0;
    self->parameters.middleFreq = self->cvConverter->parameters.middleFreq;

    self->outputs = self->cvConverter->outputs;
    self->cvConverter->inputs.source = self->cvIn->outputs.main;
}

void sig_host_VOctCVIn_generate(void* signal) {
    struct sig_host_VOctCVIn* self = (struct sig_host_VOctCVIn*) signal;
    // TODO: We always have to update these parameters
    // at block rate because there's no way to know if there
    // was actually a parameter change made and parameters are not pointers.
    self->cvIn->hardware = self->hardware;
    self->cvConverter->parameters.middleFreq = self->parameters.middleFreq;
    self->cvIn->parameters.control = self->parameters.control;
    self->cvIn->parameters.scale = self->parameters.scale;
    self->cvIn->parameters.offset = self->parameters.offset;

    self->cvIn->signal.generate(self->cvIn);
    self->cvConverter->signal.generate(self->cvConverter);
}

void sig_host_VOctCVIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_VOctCVIn* self) {
    sig_host_CVIn_destroy(allocator, self->cvIn);
    sig_dsp_LinearToFreq_destroy(allocator, self->cvConverter);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_host_CVOut* sig_host_CVOut_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_host_CVOut* self = sig_MALLOC(allocator,
        struct sig_host_CVOut);
    sig_host_CVOut_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_host_CVOut_init(struct sig_host_CVOut* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_CVOut_generate);
    self->parameters.scale = 1.0f;
    self->parameters.offset = 0.0f;
    self->parameters.control = 0;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_host_CVOut_generate(void* signal) {
    struct sig_host_CVOut* self = (struct sig_host_CVOut*) signal;
    struct sig_host_HardwareInterface* hardware = self->hardware;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    // Pass through the value to the output buffer,
    // so even sink signals can be chained.
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float sample = source * scale + offset;
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
        // TODO: Should this only be called at the block rate?
        hardware->dacChannels[control] = sample;
    }
}


struct sig_host_AudioOut* sig_host_AudioOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_host_AudioOut* self = sig_MALLOC(allocator,
        struct sig_host_AudioOut);
    sig_host_AudioOut_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_host_AudioOut_init(struct sig_host_AudioOut* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_AudioOut_generate);
    self->parameters.channel = 0;
    self->parameters.scale = 1.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_host_AudioOut_generate(void* signal) {
    struct sig_host_AudioOut* self = (struct sig_host_AudioOut*) signal;
    struct sig_host_HardwareInterface* hardware = self->hardware;
    float** out = hardware->audioOutputChannels;
    float_array_ptr source = self->inputs.source;
    int channel = self->parameters.channel;
    float scale = self->parameters.scale;

    // TODO: We need a validation stage for Signals so that we can
    // avoid these conditionals happening at block rate.
    if (channel < 0 ||
        channel >= hardware->numAudioOutputChannels) {
        // There's a channel mismatch, just do nothing.
        return;
    }

    // TODO: Is it safe to assume here that the Signal's block size is
    // the same as the Host's?
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        out[channel][i] = FLOAT_ARRAY(source)[i] * scale;
    }
}

void sig_host_AudioOut_destroy(struct sig_Allocator* allocator,
    struct sig_host_AudioOut* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_host_AudioIn* sig_host_AudioIn_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_host_AudioIn* self = sig_MALLOC(allocator,
        struct sig_host_AudioIn);
    sig_host_AudioIn_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_host_AudioIn_init(struct sig_host_AudioIn* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_AudioIn_generate);
    self->parameters.channel = 0;
    self->parameters.scale = 1.0f;
}

void sig_host_AudioIn_generate(void* signal) {
    struct sig_host_AudioIn* self = (struct sig_host_AudioIn*) signal;
    struct sig_host_HardwareInterface* hardware = self->hardware;
    float** in = hardware->audioInputChannels;
    int channel = self->parameters.channel;
    float scale = self->parameters.scale;

    // TODO: We need a validation stage for Signals so that we can
    // avoid these conditionals happening at block rate.
    if (channel < 0 ||
        channel >= hardware->numAudioInputChannels) {
        // There's a channel mismatch, just do nothing.
        return;
    }

    // TODO: Is it safe to assume here that the Signal's block size is
    // the same as the Host's?
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = in[channel][i] * scale;
    }
}

void sig_host_AudioIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_AudioIn* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_host_SwitchIn* sig_host_SwitchIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_host_SwitchIn* self = sig_MALLOC(allocator,
        struct sig_host_SwitchIn);
    sig_host_SwitchIn_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_host_SwitchIn_init(struct sig_host_SwitchIn* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_SwitchIn_generate);
    self->parameters.scale = 1.0f;
    self->parameters.offset = 0.0f;
    self->parameters.control = 0;
}

void sig_host_SwitchIn_generate(void* signal) {
    struct sig_host_SwitchIn* self = (struct sig_host_SwitchIn*) signal;
    struct sig_host_HardwareInterface* hardware = self->hardware;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    float sample = hardware->toggles[control];
    float scaledSample = sample * scale + offset;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = scaledSample;
    }
}

void sig_host_SwitchIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_SwitchIn* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_host_TriSwitchIn* sig_host_TriSwitchIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_host_TriSwitchIn* self = sig_MALLOC(allocator,
        struct sig_host_TriSwitchIn);
    sig_host_TriSwitchIn_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_host_TriSwitchIn_init(struct sig_host_TriSwitchIn* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_TriSwitchIn_generate);
    self->parameters.scale = 1.0f;
    self->parameters.offset = 0.0f;
    self->parameters.control = 0;
}

void sig_host_TriSwitchIn_generate(void* signal) {
    struct sig_host_TriSwitchIn* self = (struct sig_host_TriSwitchIn*) signal;
    struct sig_host_HardwareInterface* hardware = self->hardware;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    float sample = hardware->triSwitches[control];
    float scaledSample = sample * scale + offset;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = scaledSample;
    }
}

void sig_host_TriSwitchIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_TriSwitchIn* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}
