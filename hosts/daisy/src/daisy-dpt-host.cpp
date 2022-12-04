#include "../include/daisy-dpt-host.h"

void sig_daisy_DPT_dacWriterCallback(void* hostState) {
    struct sig_daisy_DPTState* dptState =
        static_cast<struct sig_daisy_DPTState*>(hostState);
    daisy::Dac7554* dac = &dptState->dpt->dac_exp;

    dac->Write(dptState->dacCVOuts);
    dac->WriteDac7554();
}

void sig_daisy_DPTHostImpl_setControlValue(struct sig_daisy_Host* host,
    int control, float value) {
    struct sig_daisy_DPTState* dptState =
        static_cast<struct sig_daisy_DPTState*>(host->boardState);

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
        static_cast<struct sig_daisy_DPTState*>(host->boardState);

    if (control < 0 || control > 1) {
        return 0.0f;
    }

    daisy::GateIn* gate = control == sig_daisy_GATEIN_2 ?
        &dptState->dpt->gate_in_2 : &dptState->dpt->gate_in_1;

    // The gate is inverted (i.e. true when voltage is 0V).
    // See https://electro-smith.github.io/libDaisy/classdaisy_1_1_gate_in.html#a08f75c6621307249de3107df96cfab2d
    float sample = gate->State() ? 0.0f : 1.0f;

    return sample;
}


struct sig_daisy_Host_Impl sig_daisy_DPTHostImpl = {
    .numAnalogControls = sig_daisy_DPT_NUM_ANALOG_CONTROLS,
    .getControlValue = sig_daisy_processControlValue,
    .setControlValue = sig_daisy_DPTHostImpl_setControlValue,
    .getGateValue = sig_daisy_DPTHostImpl_getGateValue
};

struct sig_daisy_Host* sig_daisy_DPTHost_new(struct sig_Allocator* allocator,
    daisy::dpt::DPT* board) {
    struct sig_daisy_Host* self = sig_MALLOC(allocator, struct sig_daisy_Host);
    struct sig_daisy_DPTState* boardState = sig_MALLOC(allocator,
        struct sig_daisy_DPTState);
    sig_daisy_DPTState_init(boardState, board);
    sig_daisy_DPTHost_init(self, boardState);

    return self;
}

void sig_daisy_DPTState_init(struct sig_daisy_DPTState* self,
    daisy::dpt::DPT* board) {
    self->dpt = board;
    self->dacCVOuts[0] = 4095;
    self->dacCVOuts[1] = 4095;
    self->dacCVOuts[2] = 4095;
    self->dacCVOuts[3] = 4095;
}

void sig_daisy_DPTHost_init(struct sig_daisy_Host* self,
    struct sig_daisy_DPTState* boardState) {
    self->impl = &sig_daisy_DPTHostImpl;
    self->analogControls = &(boardState->dpt->controls[0]);
    self->boardState = (void*) boardState;
}

void sig_daisy_DPTHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self) {
    allocator->impl->free(allocator, self->boardState);
    allocator->impl->free(allocator, self);
}
