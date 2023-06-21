#include "../include/daisy-versio-host.h"

struct sig_daisy_Host_BoardConfiguration sig_daisy_VersioConfig = {
    .numAudioInputChannels = 2,
    .numAudioOutputChannels = 2,
    .numAnalogInputs = sig_daisy_Versio_NUM_ANALOG_INPUTS,
    .numAnalogOutputs = sig_daisy_Versio_NUM_ANALOG_OUTPUTS,
    .numGateInputs = sig_daisy_Versio_NUM_GATE_INPUTS,
    .numGateOutputs = sig_daisy_Versio_NUM_GATE_OUTPUTS,
    .numSwitches = sig_daisy_Versio_NUM_SWITCHES,
    .numTriSwitches = sig_daisy_Versio_NUM_TRI_SWITCHES,
    .numEncoders = sig_daisy_Versio_NUM_ENCODERS
};

struct sig_daisy_Host_Impl sig_daisy_VersioHostImpl = {
    .getControlValue = sig_daisy_HostImpl_processControlValue,
    .setControlValue = sig_daisy_HostImpl_noOpSetControl,
    .getGateValue = sig_daisy_HostImpl_noOpGetControl,
    .setGateValue = sig_daisy_HostImpl_noOpSetControl,
    .getSwitchValue = sig_daisy_HostImpl_getSwitchValue,
    .getTriSwitchValue = sig_daisy_HostImpl_getTriSwitchValue,
    .getEncoderIncrement = sig_daisy_HostImpl_noOpGetControl,
    .getEncoderButtonValue = sig_daisy_HostImpl_noOpGetControl,
    .start = sig_daisy_VersioHostImpl_start,
    .stop = sig_daisy_VersioHostImpl_stop
};

void sig_daisy_VersioHostImpl_start(struct sig_daisy_Host* host) {
    daisy::DaisyVersio* versio = static_cast<daisy::DaisyVersio*>(
        host->board.boardInstance);
    versio->StartAdc();
    versio->StartAudio(sig_daisy_Host_audioCallback);
}

void sig_daisy_VersioHostImpl_stop(struct sig_daisy_Host* host) {
    daisy::DaisyVersio* versio = static_cast<daisy::DaisyVersio*>(
        host->board.boardInstance);
     versio->StopAudio();
}

struct sig_daisy_Host* sig_daisy_VersioHost_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    daisy::DaisyVersio* versio,
    struct sig_dsp_SignalEvaluator* evaluator) {
    struct sig_daisy_Host* self = sig_MALLOC(allocator,
        struct sig_daisy_Host);
    sig_daisy_VersioHost_init(self, audioSettings, versio, evaluator);

    return self;
}

void sig_daisy_VersioHost_Board_init(struct sig_daisy_Host_Board* self,
    daisy::DaisyVersio* versio) {
    self->config = &sig_daisy_VersioConfig;
    self->analogControls = &versio->knobs[0];
    self->dac = &versio->seed.dac;
    self->gateInputs[0] = &versio->gate;
    self->gateInputs[1] = NULL;
    self->gateOutputs[0] = NULL;
    self->gateOutputs[1] = NULL;
    self->switches[0] = versio->tap;
    self->triSwitches[0] = versio->sw[0];
    self->boardInstance = (void*) versio;
}

void sig_daisy_VersioHost_init(struct sig_daisy_Host* self,
    struct sig_AudioSettings* audioSettings,
    daisy::DaisyVersio* versio,
    struct sig_dsp_SignalEvaluator* evaluator) {
    self->impl = &sig_daisy_VersioHostImpl;
    self->audioSettings = audioSettings;
    self->evaluator = evaluator;

    versio->Init();
    versio->SetAudioBlockSize(audioSettings->blockSize);
    daisy::SaiHandle::Config::SampleRate daisySR =
        sig_daisy_Host_convertSampleRate(audioSettings->sampleRate);
    versio->SetAudioSampleRate(daisySR);
    sig_daisy_VersioHost_Board_init(&self->board, versio);
    sig_daisy_Host_init(self);
}

void sig_daisy_VersioHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self) {
    allocator->impl->free(allocator, self);
}
