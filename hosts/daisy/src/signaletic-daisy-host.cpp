#include "../include/signaletic-daisy-host.h"

struct sig_daisy_Host* sig_daisy_Host_globalHost = NULL;

// TODO: userData is shared, but the API makes it seem as if
// different pointers could be provided for each of these callbacks.
// We either need to support two pointers to user data, or provide
// a different API for registering a single shared userData
// pointer for all callbacks.
void sig_daisy_Host_addOnEvaluateSignalsListener(
    struct sig_daisy_Host* self,
    sig_daisy_Host_onEvaluateSignals listener,
    void* userData) {
    self->onEvaluateSignals = listener;
    self->userData = userData;
}

void sig_daisy_Host_addAfterEvaluateSignalsListener(
    struct sig_daisy_Host* self,
    sig_daisy_Host_onEvaluateSignals listener,
    void* userData) {
    self->afterEvaluateSignals = listener;
    self->userData = userData;
}

void sig_daisy_Host_registerGlobalHost(struct sig_daisy_Host* host) {
    if (sig_daisy_Host_globalHost != NULL &&
        sig_daisy_Host_globalHost != host) {
        // TODO: Clean up memory leak here,
        // or throw an error and do nothing if this is called more than once.
    }

    sig_daisy_Host_globalHost = host;
}

void sig_daisy_Host_noOpAudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size,
    struct sig_daisy_Host* host, void* userData) {}

void sig_daisy_Host_init(struct sig_daisy_Host* self) {
    self->onEvaluateSignals = sig_daisy_Host_noOpAudioCallback;
    self->afterEvaluateSignals = sig_daisy_Host_noOpAudioCallback;
};

void sig_daisy_Host_audioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    struct sig_daisy_Host* self = sig_daisy_Host_globalHost;
    self->audioBlocks.inputs = in;
    self->audioBlocks.outputs = out;

    // Invoke callback before evaluating the signal graph.
    sig_daisy_Host_globalHost->onEvaluateSignals(in, out, size,
        self, self->userData);

    // Evaluate the signal graph.
    struct sig_dsp_SignalEvaluator* evaluator =
        sig_daisy_Host_globalHost->evaluator;
    evaluator->evaluate(evaluator);

    // Invoke the after callback.
    sig_daisy_Host_globalHost->afterEvaluateSignals(in, out, size, self,
        self->userData);
}

float sig_daisy_processControlValue(struct sig_daisy_Host* host,
    int control) {
    return control > -1 && control < host->impl->numAnalogControls ?
        host->analogControls[control].Process() : 0.0f;
}

struct sig_daisy_GateIn* sig_daisy_GateIn_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_daisy_Host* host) {
    struct sig_daisy_GateIn* self = sig_MALLOC(allocator,
        struct sig_daisy_GateIn);
    sig_daisy_GateIn_init(self, context, host);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_GateIn_init(struct sig_daisy_GateIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    sig_dsp_Signal_init(self, context, *sig_daisy_GateIn_generate);
    self->host = host;
    self->parameters = {
        .scale = 1.0f,
        .offset = 0.0f,
        .control = 0
    };
}

void sig_daisy_GateIn_generate(void* signal) {
    struct sig_daisy_GateIn* self = (struct sig_daisy_GateIn*) signal;
    struct sig_daisy_Host* host = self->host;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float sample = host->impl->getGateValue(host, control);
        FLOAT_ARRAY(self->outputs.main)[i] = sample * scale + offset;
    }
}

void sig_daisy_GateIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_GateIn* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_daisy_CVIn* sig_daisy_CVIn_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    struct sig_daisy_CVIn* self = sig_MALLOC(allocator,
        struct sig_daisy_CVIn);
    sig_daisy_CVIn_init(self, context, host);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_CVIn_init(struct sig_daisy_CVIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    sig_dsp_Signal_init(self, context, *sig_daisy_CVIn_generate);
    self->host = host;
    self->parameters = {
        .scale = 1.0f,
        .offset = 0.0f,
        .control = 0
    };
}

void sig_daisy_CVIn_generate(void* signal) {
    struct sig_daisy_CVIn* self = (struct sig_daisy_CVIn*) signal;
    struct sig_daisy_Host* host = self->host;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float rawCV = host->impl->getControlValue(host, control);
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
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    struct sig_daisy_CVOut* self = sig_MALLOC(allocator,
        struct sig_daisy_CVOut);
    sig_daisy_CVOut_init(self, context, host);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_CVOut_init(struct sig_daisy_CVOut* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    sig_dsp_Signal_init(self, context, *sig_daisy_CVOut_generate);
    self->host = host;
    self->parameters = {
        .scale = 1.0f,
        .offset = 0.0f,
        .control = 0
    };

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_daisy_CVOut_generate(void* signal) {
    struct sig_daisy_CVOut* self = (struct sig_daisy_CVOut*) signal;
    struct sig_daisy_Host* host = self->host;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    // Pass through the value to the output buffer,
    // so even sink signals can be chained.
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float sample = source * scale + offset;
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
        host->impl->setControlValue(host, control, sample);
    }
}

void sig_daisy_CVIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_CVOut* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

struct sig_daisy_AudioOut* sig_daisy_AudioOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_daisy_Host* host) {
    struct sig_daisy_AudioOut* self = sig_MALLOC(allocator,
        struct sig_daisy_AudioOut);
    sig_daisy_AudioOut_init(self, context, host);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_AudioOut_init(struct sig_daisy_AudioOut* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    sig_dsp_Signal_init(self, context, *sig_daisy_AudioOut_generate);
    self->host = host;
    self->parameters = {
        .channel = 0
    };

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_daisy_AudioOut_generate(void* signal) {
    struct sig_daisy_AudioOut* self = (struct sig_daisy_AudioOut*) signal;
    struct sig_daisy_Host* host = self->host;
    daisy::AudioHandle::OutputBuffer out = host->audioBlocks.outputs;
    float_array_ptr source = self->inputs.source;
    size_t channel = self->parameters.channel;

    // TODO: We need a validation stage for Signals so that we can
    // avoid these conditionals happening at block rate.
    if (channel < 0 ||
        channel >= host->audioBlocks.numOutputChannels) {
        // There's a channel mismatch, just do nothing.
        return;
    }

    // TODO: Is it safe to assume here that the Signal's block size is
    // the same as the Host's?
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        out[channel][i] = FLOAT_ARRAY(source)[i];
    }
}

void sig_daisy_AudioOut_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_AudioOut* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}
