#include "../include/signaletic-daisy-host.h"

float sig_daisy_processControlValue(struct sig_daisy_Host* host,
    int control) {
    return control > -1 && control < host->impl->numAnalogControls ?
        host->analogControls[control].Process() : 0.0f;
}

struct sig_daisy_GateIn* sig_daisy_GateIn_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context, struct sig_daisy_Host* host,
    int control) {
    struct sig_daisy_GateIn* self = sig_MALLOC(allocator,
        struct sig_daisy_GateIn);
    sig_daisy_GateIn_init(self, context, host, control);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_GateIn_init(struct sig_daisy_GateIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host,
    int control) {
    sig_dsp_Signal_init(self, context, *sig_daisy_GateIn_generate);
    self->host = host;
    self->control = control;
}

void sig_daisy_GateIn_generate(void* signal) {
    struct sig_daisy_GateIn* self = (struct sig_daisy_GateIn*) signal;
    struct sig_daisy_Host* host = self->host;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float sample = host->impl->getGateValue(self->host, self->control);
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
    }
}

void sig_daisy_GateIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_GateIn* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_daisy_CVIn* sig_daisy_CVIn_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context, struct sig_daisy_Host* host,
    int control) {
    struct sig_daisy_CVIn* self = sig_MALLOC(allocator,
        struct sig_daisy_CVIn);
    sig_daisy_CVIn_init(self, context, host, control);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_CVIn_init(struct sig_daisy_CVIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host,
    int control) {
    struct sig_daisy_CV_Parameters params = {
        .scale = 1.0f,
        .offset = 0.0f
    };

    sig_dsp_Signal_init(self, context, *sig_daisy_CVIn_generate);

    self->parameters = params;
    self->host = host;
    self->control = control;
}

void sig_daisy_CVIn_generate(void* signal) {
    struct sig_daisy_CVIn* self = (struct sig_daisy_CVIn*) signal;
    struct sig_daisy_Host* host = self->host;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float rawCV = host->impl->getControlValue(self->host, self->control);
        float sample = rawCV * scale + offset;
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
    }
}

void sig_daisy_CVIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_CVIn* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_daisy_CVOut* sig_daisy_CVOut_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context, struct sig_daisy_Host* host,
    int control) {
    struct sig_daisy_CVOut* self = sig_MALLOC(allocator,
        struct sig_daisy_CVOut);
    sig_daisy_CVOut_init(self, context, host, control);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_CVOut_init(struct sig_daisy_CVOut* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host,
    int control) {
    sig_dsp_Signal_init(self, context, *sig_daisy_CVOut_generate);

    self->parameters = {
        .scale = 1.0f,
        .offset = 0.0f
    };
    self->host = host;
    self->control = control;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_daisy_CVOut_generate(void* signal) {
    struct sig_daisy_CVOut* self = (struct sig_daisy_CVOut*) signal;
    struct sig_daisy_Host* host = self->host;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;

    // Pass through the value to the output buffer,
    // so even sink signals can be chained.
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float sample = source * scale + offset;
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
        host->impl->setControlValue(self->host, self->control, sample);
    }

}

void sig_daisy_CVIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_CVOut* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}
