#include "../include/daisy-bluemchen-host.h"

struct sig_daisy_Host_BoardConfiguration sig_daisy_BluemchenConfig = {
    .numAudioInputChannels = 2,
    .numAudioOutputChannels = 2,
    .numAnalogInputs = sig_daisy_Bluemchen_NUM_ANALOG_INPUTS,
    .numAnalogOutputs = sig_daisy_Bluemchen_NUM_ANALOG_OUTPUTS,
    .numGateInputs = sig_daisy_Bluemchen_NUM_GATE_INPUTS,
    .numGateOutputs = sig_daisy_Bluemchen_NUM_GATE_OUTPUTS,
    .numSwitches = sig_daisy_Bluemchen_NUM_SWITCHES
};

struct sig_daisy_Host_BoardConfiguration sig_daisy_NehcmeulbConfig = {
    .numAudioInputChannels = 2,
    .numAudioOutputChannels = 2,
    .numAnalogInputs = sig_daisy_Nehcmeulb_NUM_ANALOG_INPUTS,
    .numAnalogOutputs = sig_daisy_Nehcmeulb_NUM_ANALOG_OUTPUTS,
    .numGateInputs = sig_daisy_Nehcmeulb_NUM_GATE_INPUTS,
    .numGateOutputs = sig_daisy_Nehcmeulb_NUM_GATE_OUTPUTS,
    .numSwitches = sig_daisy_Nehcmeulb_NUM_SWITCHES
};

struct sig_daisy_Host_Impl sig_daisy_BluemchenHostImpl = {
    .getControlValue = sig_daisy_HostImpl_processControlValue,
    .setControlValue = sig_daisy_HostImpl_setControlValue,
    .getGateValue = sig_daisy_HostImpl_noOpGetControl,
    .setGateValue = sig_daisy_HostImpl_noOpSetControl,
    .getSwitchValue = sig_daisy_HostImpl_noOpGetControl,
    .start = sig_daisy_BluemchenHostImpl_start,
    .stop = sig_daisy_BluemchenHostImpl_stop
};

void sig_daisy_BluemchenHostImpl_start(struct sig_daisy_Host* host) {
    kxmx::Bluemchen* bluemchen = static_cast<kxmx::Bluemchen*>(
        host->board.boardInstance);
    bluemchen->StartAdc();
    bluemchen->StartAudio(sig_daisy_Host_audioCallback);
}

void sig_daisy_BluemchenHostImpl_stop(struct sig_daisy_Host* host) {
    kxmx::Bluemchen* bluemchen = static_cast<kxmx::Bluemchen*>(
        host->board.boardInstance);
    bluemchen->StopAudio();
}

struct sig_daisy_Host* sig_daisy_BluemchenHost_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    kxmx::Bluemchen* bluemchen,
    struct sig_dsp_SignalEvaluator* evaluator) {
    struct sig_daisy_Host* self = sig_MALLOC(allocator, struct sig_daisy_Host);
    sig_daisy_BluemchenHost_init(self,
        audioSettings,
        &sig_daisy_BluemchenConfig,
        bluemchen,
        evaluator);

    return self;
}

struct sig_daisy_Host* sig_daisy_Nehcmeulb_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    kxmx::Bluemchen* bluemchen,
    struct sig_dsp_SignalEvaluator* evaluator) {
    struct sig_daisy_Host* self = sig_MALLOC(allocator, struct sig_daisy_Host);
    sig_daisy_BluemchenHost_init(self,
        audioSettings,
        &sig_daisy_NehcmeulbConfig,
        bluemchen,
        evaluator);

    return self;
}

void sig_daisy_BluemchenHost_Board_init(
    struct sig_daisy_Host_Board* self,
    kxmx::Bluemchen* bluemchen,
    struct sig_daisy_Host_BoardConfiguration* boardConfig) {
    self->boardInstance = (void*) bluemchen;
    self->config = boardConfig;
    self->analogControls = &bluemchen->controls[0];
    self->dac = &bluemchen->seed.dac;
    // Bluemchen has no gates.
    self->gateInputs[0] = NULL;
    self->gateInputs[1] = NULL;
    self->gateOutputs[0] = NULL;
    self->gateOutputs[1] = NULL;
}


void sig_daisy_BluemchenHost_init(
    struct sig_daisy_Host* self,
    struct sig_AudioSettings* audioSettings,
    struct sig_daisy_Host_BoardConfiguration* boardConfig,
    kxmx::Bluemchen* bluemchen,
    struct sig_dsp_SignalEvaluator* evaluator) {
    self->impl = &sig_daisy_BluemchenHostImpl;
    self->audioSettings = audioSettings;
    self->evaluator = evaluator;

    bluemchen->Init();
    daisy::SaiHandle::Config::SampleRate sampleRate =
        sig_daisy_Host_convertSampleRate(audioSettings->sampleRate);
    bluemchen->SetAudioSampleRate(sampleRate);
    bluemchen->SetAudioBlockSize(audioSettings->blockSize);
    sig_daisy_BluemchenHost_Board_init(&self->board, bluemchen, boardConfig);
    sig_daisy_Host_init(self);
}

void sig_daisy_BluemchenHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self) {
    allocator->impl->free(allocator, self);
}

void sig_daisy_NehcmeulbHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self) {
    sig_daisy_BluemchenHost_destroy(allocator, self);
}
