#include "../include/daisy-patch-sm-host.h"

struct sig_daisy_Host_BoardConfiguration sig_daisy_PatchSMConfig = {
    .numAudioInputChannels = 2,
    .numAudioOutputChannels = 2,
    .numAnalogInputs = sig_daisy_PatchSM_NUM_ANALOG_INPUTS,
    .numAnalogOutputs = sig_daisy_PatchSM_NUM_ANALOG_OUTPUTS,
    .numGateInputs = sig_daisy_PatchSM_NUM_GATE_INPUTS,
    .numGateOutputs = sig_daisy_PatchSM_NUM_GATE_OUTPUTS,
    .numSwitches = sig_daisy_PatchSM_NUM_SWITCHES
};

struct sig_daisy_Host_BoardConfiguration sig_daisy_PatchInitConfig = {
    .numAudioInputChannels = 2,
    .numAudioOutputChannels = 2,
    .numAnalogInputs = sig_daisy_PatchInit_NUM_ANALOG_INPUTS,
    .numAnalogOutputs = sig_daisy_PatchInit_NUM_ANALOG_OUTPUTS,
    .numGateInputs = sig_daisy_PatchInit_NUM_GATE_INPUTS,
    .numGateOutputs = sig_daisy_PatchInit_NUM_GATE_OUTPUTS,
    .numSwitches = sig_daisy_PatchInit_NUM_SWITCHES
};

struct sig_daisy_Host_Impl sig_daisy_PatchSMHostImpl = {
    .getControlValue = sig_daisy_HostImpl_processControlValue,
    .setControlValue = sig_daisy_PatchSMHostImpl_setControlValue,
    .getGateValue = sig_daisy_HostImpl_getGateValue,
    .setGateValue = sig_daisy_HostImpl_setGateValue,
    .getSwitchValue = sig_daisy_HostImpl_getSwitchValue,
    .start = sig_daisy_PatchSMHostImpl_start,
    .stop = sig_daisy_PatchSMHostImpl_stop
};

void sig_daisy_PatchSMHostImpl_start(struct sig_daisy_Host* host) {
    daisy::patch_sm::DaisyPatchSM* patchSM =
        static_cast<daisy::patch_sm::DaisyPatchSM*>(host->board.boardInstance);
    patchSM->StartAdc();
    patchSM->StartAudio(sig_daisy_Host_audioCallback);
}

void sig_daisy_PatchSMHostImpl_stop(struct sig_daisy_Host* host) {
    daisy::patch_sm::DaisyPatchSM* patchSM =
        static_cast<daisy::patch_sm::DaisyPatchSM*>(host->board.boardInstance);
    patchSM->StopAudio();
}

// TODO: Implement a generic version of this that can work
// with any Daisy board that has DMA enabled.
void sig_daisy_PatchSMHostImpl_setControlValue(struct sig_daisy_Host* host,
    int control, float value) {
    daisy::patch_sm::DaisyPatchSM* patchSM =
        static_cast<daisy::patch_sm::DaisyPatchSM*>(host->board.boardInstance);

    float rectified = fabsf(value);
    float scaled = rectified * 5.0f;

    // Controls are off by one because 0 signifies both in the Daisy API.
    // We don't expose this to Signaletic users, because it is awkward.
    patchSM->WriteCvOut(control, scaled);
}

struct sig_daisy_Host* sig_daisy_PatchSMHost_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    daisy::patch_sm::DaisyPatchSM* patchSM,
    struct sig_dsp_SignalEvaluator* evaluator) {
    struct sig_daisy_Host* self = sig_MALLOC(allocator,
        struct sig_daisy_Host);
    sig_daisy_PatchSMHost_init(self, audioSettings, patchSM, evaluator);

    return self;
}

void sig_daisy_PatchSMHost_Board_init(struct sig_daisy_Host_Board* self,
    daisy::patch_sm::DaisyPatchSM* patchSM) {
    self->config = &sig_daisy_PatchSMConfig;
    self->analogControls = &patchSM->controls[0];
    self->dac = &patchSM->dac;
    self->gateInputs[0] = &patchSM->gate_in_1;
    self->gateInputs[1] = &patchSM->gate_in_2;
    self->gateOutputs[0] = &patchSM->gate_out_1;
    self->gateOutputs[1] = &patchSM->gate_out_2;
    self->switches[0].Init(patchSM->B7, patchSM->AudioCallbackRate());
    self->switches[1].Init(patchSM->B8, patchSM->AudioCallbackRate());
    self->boardInstance = (void*) patchSM;
}

void sig_daisy_PatchSMHost_init(struct sig_daisy_Host* self,
    struct sig_AudioSettings* audioSettings,
    daisy::patch_sm::DaisyPatchSM* patchSM,
    struct sig_dsp_SignalEvaluator* evaluator) {
    self->impl = &sig_daisy_PatchSMHostImpl;
    self->audioSettings = audioSettings;
    self->evaluator = evaluator;

    patchSM->Init();
    patchSM->SetAudioBlockSize(audioSettings->blockSize);
    patchSM->SetAudioSampleRate(audioSettings->sampleRate);
    sig_daisy_PatchSMHost_Board_init(&self->board, patchSM);
    sig_daisy_Host_init(self);
}

void sig_daisy_PatchSMHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self) {
    allocator->impl->free(allocator, self);
}
