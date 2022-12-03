#include "../include/signaletic-daisy-host.h"

// TODO: Move vendored DPT (and Blumchen) out from examples
// into the daisy directory.
// TODO: Refactor this file into the generic Daisy-related parts
// with separate host files for Bluemchen and DPT.
#include "../vendor/dpt/lib/daisy_dpt.h"

void sig_daisy_DPT_dacWriterCallback(void* hostState) {
    struct sig_daisy_DPTState* dptState =
        static_cast<struct sig_daisy_DPTState*>(hostState);
    daisy::Dac7554* dac = &dptState->dpt->dac_exp;

    dac->Write(dptState->dacCVOuts);
    dac->WriteDac7554();
}

struct sig_daisy_Host_Impl sig_daisy_DPTHostImpl = {
    .getControlValue = sig_daisy_DPTHostImpl_getControlValue,
    .setControlValue = sig_daisy_DPTHostImpl_setControlValue,
    .getGateValue = sig_daisy_DPTHostImpl_getGateValue
};

float sig_daisy_DPTHostImpl_getControlValue(struct sig_daisy_Host* host,
    int control) {
    struct sig_daisy_DPTState* dptState =
        static_cast<struct sig_daisy_DPTState*>(host->state);

    return dptState->dpt->controls[control].Process();
}

void sig_daisy_DPTHostImpl_setControlValue(struct sig_daisy_Host* host,
    int control, float value) {
    struct sig_daisy_DPTState* dptState =
        static_cast<struct sig_daisy_DPTState*>(host->state);

    if (control > 5 || control < 0) {
        return;
    }

    uint16_t convertedValue = sig_bipolarToInvUint12(value);

    // Controls are indexed 0-5,
    // but 2-5 need to be placed into a cache that will be
    // written in the DAC timer interrupt.
    // And libdaisy indexes the two CV outs on the Seed/Patch SM
    // as 1 and 2, because 0 is reserved for writing to both outputs.
    if (control > 1) {
        dptState->dacCVOuts[control - 2] = convertedValue;
    } else {
        dptState->dpt->WriteCvOut(control + 1, convertedValue, true);
    }
}

float sig_daisy_DPTHostImpl_getGateValue(struct sig_daisy_Host* host,
    int control) {
    struct sig_daisy_DPTState* dptState =
        static_cast<struct sig_daisy_DPTState*>(host->state);

    daisy::GateIn* gate = control == sig_daisy_GATEIN_2 ?
        &dptState->dpt->gate_in_2 : &dptState->dpt->gate_in_1;

    // The gate is inverted (i.e. true when voltage is 0V).
    // See https://electro-smith.github.io/libDaisy/classdaisy_1_1_gate_in.html#a08f75c6621307249de3107df96cfab2d
    float sample = gate->State() ? 0.0f : 1.0f;

    return sample;
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
