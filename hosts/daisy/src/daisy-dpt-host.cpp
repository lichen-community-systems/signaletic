#include "../include/daisy-dpt-host.h"

void sig_daisy_DPT_dacWriterCallback(void* dptHost) {
    struct sig_daisy_DPTHost* self = static_cast<struct sig_daisy_DPTHost*>(
        dptHost);
    daisy::Dac7554* expansionDAC = self->expansionDAC;
    expansionDAC->Write(self->expansionDACBuffer);
    expansionDAC->WriteDac7554();
}

struct sig_daisy_Host_BoardConfiguration sig_daisy_DPTConfig = {
    .numAudioInputChannels = 2,
    .numAudioOutputChannels = 2,
    .numAnalogInputs = sig_daisy_DPT_NUM_ANALOG_INPUTS,
    .numAnalogOutputs = sig_daisy_DPT_NUM_ANALOG_OUTPUTS,
    .numGateInputs = sig_daisy_DPT_NUM_GATE_INPUTS,
    .numGateOutputs = sig_daisy_DPT_NUM_GATE_OUTPUTS,
    .numSwitches = sig_daisy_DPT_NUM_SWITCHES,
    .numTriSwitches = sig_daisy_DPT_NUM_SWITCHES,
    .numEncoders = sig_daisy_DPT_NUM_ENCODERS
};

struct sig_daisy_Host_Impl sig_daisy_DPTHostImpl = {
    .getControlValue = sig_daisy_HostImpl_processControlValue,
    .setControlValue = sig_daisy_DPTHostImpl_setControlValue,
    .getGateValue = sig_daisy_HostImpl_getGateValue,
    .setGateValue = sig_daisy_HostImpl_setGateValue,
    .getSwitchValue = sig_daisy_HostImpl_noOpGetControl,
    .getTriSwitchValue = sig_daisy_HostImpl_noOpGetControl,
    .getEncoderIncrement = sig_daisy_HostImpl_noOpGetControl,
    .getEncoderButtonValue = sig_daisy_HostImpl_noOpGetControl,
    .start = sig_daisy_DPTHostImpl_start,
    .stop = sig_daisy_DPTHostImpl_stop,
};

void sig_daisy_DPTHostImpl_start(struct sig_daisy_Host* host) {
    daisy::dpt::DPT* dpt =
        static_cast<daisy::dpt::DPT*>(host->board.boardInstance);
    dpt->StartAdc();
    dpt->StartAudio(sig_daisy_Host_audioCallback);
}

void sig_daisy_DPTHostImpl_stop(struct sig_daisy_Host* host) {
    daisy::dpt::DPT* dpt =
        static_cast<daisy::dpt::DPT*>(host->board.boardInstance);
    dpt->StopAudio();
}

void sig_daisy_DPTHostImpl_setControlValue(struct sig_daisy_Host* host,
    int control, float value) {
    struct sig_daisy_DPTHost* self = (struct sig_daisy_DPTHost*) host;

    if (control < 0 || control >= sig_daisy_DPT_NUM_ANALOG_OUTPUTS) {
        return;
    }

    uint16_t convertedValue = sig_bipolarToInvUint12(value);

    // Controls are indexed 0-5,
    // but 2-5 need to be placed into a buffer that will be
    // written in the DAC timer interrupt.
    // And libdaisy indexes the two CV outs on the Seed/Patch SM
    // as 1 and 2, because 0 is reserved for writing to both outputs.
    if (control > 1) {
        self->expansionDACBuffer[control - 2] = convertedValue;
    } else {
        daisy::dpt::DPT* dpt = static_cast<daisy::dpt::DPT*>(
            self->host.board.boardInstance);
        dpt->WriteCvOut(control + 1, convertedValue, true);
    }
}

struct sig_daisy_Host* sig_daisy_DPTHost_new(struct sig_Allocator* allocator,
    sig_AudioSettings* audioSettings,
    daisy::dpt::DPT* dpt,
    struct sig_dsp_SignalEvaluator* evaluator) {
    struct sig_daisy_DPTHost* self = sig_MALLOC(allocator,
        struct sig_daisy_DPTHost);
    sig_daisy_DPTHost_init(self, audioSettings, dpt, evaluator);

    return &self->host;
}

void sig_daisy_DPTHost_Board_init(struct sig_daisy_Host_Board* self,
    daisy::dpt::DPT* dpt) {
    self->boardInstance = (void*) dpt;
    self->config = &sig_daisy_DPTConfig;
    self->analogControls = &(dpt->controls[0]);
    self->gateInputs[0] = &dpt->gate_in_1;
    self->gateInputs[1] = &dpt->gate_in_2;
    self->gateOutputs[0] = &dpt->gate_out_1;
    self->gateOutputs[1] = &dpt->gate_out_2;
    self->dac = &dpt->dac;
}

void sig_daisy_DPTHost_init(struct sig_daisy_DPTHost* self,
    sig_AudioSettings* audioSettings,
    daisy::dpt::DPT* dpt,
    struct sig_dsp_SignalEvaluator* evaluator) {
    self->host.impl = &sig_daisy_DPTHostImpl;
    self->host.audioSettings = audioSettings;
    self->host.evaluator = evaluator;
    self->expansionDAC = &dpt->dac_exp;
    self->expansionDACBuffer[0] = 4095;
    self->expansionDACBuffer[1] = 4095;
    self->expansionDACBuffer[2] = 4095;
    self->expansionDACBuffer[3] = 4095;

    dpt->Init();
    dpt->SetAudioBlockSize(audioSettings->blockSize);
    dpt->SetAudioSampleRate(audioSettings->sampleRate);
    sig_daisy_DPTHost_Board_init(&self->host.board, dpt);
    sig_daisy_Host_init(&self->host);
    dpt->InitTimer(&sig_daisy_DPT_dacWriterCallback, self);
}

void sig_daisy_DPTHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_DPTHost* self) {
    allocator->impl->free(allocator, self);
}
