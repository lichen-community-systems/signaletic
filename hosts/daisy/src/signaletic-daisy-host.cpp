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
    self->board.audioInputs = in;
    self->board.audioOutputs = out;

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

float sig_daisy_HostImpl_processControlValue(struct sig_daisy_Host* host,
    int control) {
    return control > -1 && control < host->board.config->numAnalogInputs ?
        host->board.analogControls[control].Process() : 0.0f;
}

void sig_daisy_HostImpl_setControlValue(struct sig_daisy_Host* host,
    int control, float value) {
    if (control > -1 && control < host->board.config->numAnalogOutputs) {
        sig_daisy_Host_writeValueToDACPolling(host->board.dac, control, value);
    }
}

void sig_daisy_Host_writeValueToDACPolling(daisy::DacHandle* dac, int control, float value) {
    daisy::DacHandle::Channel channel = static_cast<daisy::DacHandle::Channel>(
        control);
    dac->WriteValue(channel, sig_bipolarToInvUint12(value));
}

float sig_daisy_HostImpl_getGateValue(struct sig_daisy_Host* host,
    int control) {
    if (control < 0 || control > host->board.config->numGates) {
        return 0.0f;
    }

    daisy::GateIn* gate = host->board.gateInputs[control];
    // The gate is inverted (i.e. true when voltage is 0V).
    // See https://electro-smith.github.io/libDaisy/classdaisy_1_1_gate_in.html#a08f75c6621307249de3107df96cfab2d
    float sample = gate->State() ? 0.0f : 1.0f;

    return sample;
}

void sig_daisy_HostImpl_setGateValue(struct sig_daisy_Host* host,
    int control, float value) {
    if (control < 0 || control >= host->board.config->numGates) {
        return;
    }

    dsy_gpio* gate = host->board.gateOutputs[control];
    dsy_gpio_write(gate, value > 0.0f ? 0 : 1);
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


struct sig_daisy_GateOut* sig_daisy_GateOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_daisy_Host* host) {
    struct sig_daisy_GateOut* self = sig_MALLOC(allocator,
        struct sig_daisy_GateOut);
    sig_daisy_GateOut_init(self, context, host);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_GateOut_init(struct sig_daisy_GateOut* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    sig_dsp_Signal_init(self, context, *sig_daisy_GateOut_generate);
    self->host = host;
    self->parameters = {
        .scale = 1.0f,
        .offset = 0.0f,
        .control = 0
    };

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_daisy_GateOut_generate(void* signal) {
    struct sig_daisy_GateOut* self = (struct sig_daisy_GateOut*) signal;
    struct sig_daisy_Host* host = self->host;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float value = FLOAT_ARRAY(self->inputs.source)[i] * scale + offset;
        FLOAT_ARRAY(self->outputs.main)[i] = value;
        host->impl->setGateValue(host, control, value);
    }
}

void sig_daisy_GateOut_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_GateOut* self) {
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
    daisy::AudioHandle::OutputBuffer out = host->board.audioOutputs;
    float_array_ptr source = self->inputs.source;
    int channel = self->parameters.channel;

    // TODO: We need a validation stage for Signals so that we can
    // avoid these conditionals happening at block rate.
    if (channel < 0 ||
        channel >= host->board.config->numAudioOutputChannels) {
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
