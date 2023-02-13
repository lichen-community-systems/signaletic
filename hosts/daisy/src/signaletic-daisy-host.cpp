#include "../include/signaletic-daisy-host.h"

struct sig_daisy_Host* sig_daisy_Host_globalHost = NULL;

daisy::SaiHandle::Config::SampleRate sig_daisy_Host_convertSampleRate(
    float sampleRate) {
    // Copy-pasted from libdaisy
    // because the Seed has a completely different API
    // for setting sample rates than the PatchSM.
    // Srsly, Electrosmith?!
    // https://github.com/electro-smith/libDaisy/blob/v5.3.0/src/daisy_patch_sm.cpp#L383-L409
    daisy::SaiHandle::Config::SampleRate sai_sr;

    switch(int(sampleRate)) {
        case 8000:
            sai_sr = daisy::SaiHandle::Config::SampleRate::SAI_8KHZ;
            break;
        case 16000:
            sai_sr = daisy::SaiHandle::Config::SampleRate::SAI_16KHZ;
            break;
        case 32000:
            sai_sr = daisy::SaiHandle::Config::SampleRate::SAI_32KHZ;
            break;
        case 48000:
            sai_sr = daisy::SaiHandle::Config::SampleRate::SAI_48KHZ;
            break;
        case 96000:
            sai_sr = daisy::SaiHandle::Config::SampleRate::SAI_96KHZ;
            break;
        default:
            sai_sr = daisy::SaiHandle::Config::SampleRate::SAI_48KHZ;
            break;
    }

    return sai_sr;
}

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


float sig_daisy_HostImpl_noOpGetControl(struct sig_daisy_Host* host,
    int control) {
    return 0.0f;
}

void sig_daisy_HostImpl_noOpSetControl(struct sig_daisy_Host* host,
    int control, float value) {}

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

void sig_daisy_Host_writeValueToDACPolling(daisy::DacHandle* dac,
    int control, float value) {
    daisy::DacHandle::Channel channel = static_cast<daisy::DacHandle::Channel>(
        control);
    dac->WriteValue(channel, sig_bipolarToInvUint12(value));
}

float sig_daisy_HostImpl_getGateValue(struct sig_daisy_Host* host,
    int control) {
    if (control < 0 || control > host->board.config->numGateInputs) {
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
    if (control < 0 || control >= host->board.config->numGateOutputs) {
        return;
    }

    dsy_gpio* gate = host->board.gateOutputs[control];
    dsy_gpio_write(gate, value > 0.0f ? 0 : 1);
}

float sig_daisy_HostImpl_getSwitchValue(struct sig_daisy_Host* host,
    int control) {
    float sample = 0.0f;
    if (control > -1 && control < host->board.config->numSwitches) {
        daisy::Switch* sw = &(host->board.switches[control]);
        sw->Debounce();
        sample = (float) sw->Pressed();
    }

    return sample;
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
        // TODO: Should this only be called at the block rate?
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
        // TODO: Should this only be called at the block rate?
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

    float rawCV = host->impl->getControlValue(host, control);
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
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

struct sig_daisy_FilteredCVIn* sig_daisy_FilteredCVIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
    struct sig_daisy_Host* host) {
    struct sig_daisy_FilteredCVIn* self = sig_MALLOC(allocator,
        struct sig_daisy_FilteredCVIn);
    self->cvIn = sig_daisy_CVIn_new(allocator, context, host);
    self->filter = sig_dsp_Smooth_new(allocator, context);
    sig_daisy_FilteredCVIn_init(self, context, host);

    return self;
}

void sig_daisy_FilteredCVIn_init(struct sig_daisy_FilteredCVIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    sig_dsp_Signal_init(self, context, *sig_daisy_FilteredCVIn_generate);
    self->host = host;
    self->parameters = {
        .scale = 1.0f,
        .offset = 0.0f,
        .control = 0,
        .time = 0.01f
    };
    self->filter->inputs.source = self->cvIn->outputs.main;
    self->outputs = self->filter->outputs;
}

void sig_daisy_FilteredCVIn_generate(void* signal) {
    struct sig_daisy_FilteredCVIn* self =
        (struct sig_daisy_FilteredCVIn*) signal;
    // TODO: We have to update these parameters
    // at block rate because there's no way to know if there
    // was actually a parameter change made and parameters are
    // stored by value, not by pointer.
    self->filter->parameters.time = self->parameters.time;
    self->cvIn->parameters.control = self->parameters.control;
    self->cvIn->parameters.scale = self->parameters.scale;
    self->cvIn->parameters.offset = self->parameters.offset;

    self->cvIn->signal.generate(self->cvIn);
    self->filter->signal.generate(self->filter);
}

void sig_daisy_FilteredCVIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_FilteredCVIn* self) {
    sig_daisy_CVIn_destroy(allocator, self->cvIn);
    sig_dsp_Smooth_destroy(allocator, self->filter);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_daisy_VOctCVIn* sig_daisy_VOctCVIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
    struct sig_daisy_Host* host) {
    struct sig_daisy_VOctCVIn* self = sig_MALLOC(allocator,
        struct sig_daisy_VOctCVIn);
    self->cvIn = sig_daisy_CVIn_new(allocator, context, host);
    self->cvConverter = sig_dsp_LinearToFreq_new(allocator, context);
    sig_daisy_VOctCVIn_init(self, context, host);

    return self;
}

void sig_daisy_VOctCVIn_init(struct sig_daisy_VOctCVIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    sig_dsp_Signal_init(self, context, *sig_daisy_VOctCVIn_generate);
    self->host = host;
    self->parameters = {
        .scale = 1.0f,
        .offset = 0.0f,
        .control = 0,
        .middleFreq = self->cvConverter->parameters.middleFreq
    };
    self->outputs = self->cvConverter->outputs;
    self->cvConverter->inputs.source = self->cvIn->outputs.main;
}

void sig_daisy_VOctCVIn_generate(void* signal) {
    struct sig_daisy_VOctCVIn* self = (struct sig_daisy_VOctCVIn*) signal;
    // TODO: We always have to update these parameters
    // at block rate because there's no way to know if there
    // was actually a parameter change made and parameters are not pointers.
    self->cvConverter->parameters.middleFreq = self->parameters.middleFreq;
    self->cvIn->parameters.control = self->parameters.control;
    self->cvIn->parameters.scale = self->parameters.scale;
    self->cvIn->parameters.offset = self->parameters.offset;

    self->cvIn->signal.generate(self->cvIn);
    self->cvConverter->signal.generate(self->cvConverter);
}

void sig_daisy_VOctCVIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_VOctCVIn* self) {
    sig_daisy_CVIn_destroy(allocator, self->cvIn);
    sig_dsp_LinearToFreq_destroy(allocator, self->cvConverter);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_daisy_SwitchIn* sig_daisy_SwitchIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
    struct sig_daisy_Host* host) {
    struct sig_daisy_SwitchIn* self = sig_MALLOC(allocator,
        struct sig_daisy_SwitchIn);
    sig_daisy_SwitchIn_init(self, context, host);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_SwitchIn_init(struct sig_daisy_SwitchIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    sig_dsp_Signal_init(self, context, *sig_daisy_SwitchIn_generate);
    self->host = host;
    self->parameters = {
        .scale = 1.0f,
        .offset = 0.0f,
        .control = 0
    };
}

void sig_daisy_SwitchIn_generate(void* signal) {
    struct sig_daisy_SwitchIn* self = (struct sig_daisy_SwitchIn*) signal;
    struct sig_daisy_Host* host = self->host;
    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    int control = self->parameters.control;

    float sample = host->impl->getSwitchValue(host, control);
    float scaledSample = sample * scale + offset;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = scaledSample;
    }
}

void sig_daisy_SwitchIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_SwitchIn* self) {
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
        // TODO: Should this only be called at the block rate?
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


struct sig_daisy_AudioIn* sig_daisy_AudioIn_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_daisy_Host* host) {
    struct sig_daisy_AudioIn* self = sig_MALLOC(allocator,
        struct sig_daisy_AudioIn);
    sig_daisy_AudioIn_init(self, context, host);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_daisy_AudioIn_init(struct sig_daisy_AudioIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host) {
    sig_dsp_Signal_init(self, context, *sig_daisy_AudioIn_generate);
    self->host = host;
    self->parameters = {
        .channel = 0
    };
}

void sig_daisy_AudioIn_generate(void* signal) {
    struct sig_daisy_AudioIn* self = (struct sig_daisy_AudioIn*) signal;
    struct sig_daisy_Host* host = self->host;
    daisy::AudioHandle::InputBuffer in = host->board.audioInputs;
    int channel = self->parameters.channel;

    // TODO: We need a validation stage for Signals so that we can
    // avoid these conditionals happening at block rate.
    if (channel < 0 ||
        channel >= host->board.config->numAudioInputChannels) {
        // There's a channel mismatch, just do nothing.
        return;
    }

    // TODO: Is it safe to assume here that the Signal's block size is
    // the same as the Host's?
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = in[channel][i];
    }

}

void sig_daisy_AudioIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_AudioIn* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}
