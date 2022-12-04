#include "../include/daisy-bluemchen-host.h"

struct sig_daisy_Host_Impl sig_daisy_BluemchenHostImpl = {
    .numAnalogControls = sig_daisy_Bluemchen_NUM_ANALOG_CONTROLS,
    .getControlValue = sig_daisy_processControlValue,
    .setControlValue = sig_daisy_BluemchenHostImpl_setControlValue,
    .getGateValue = sig_daisy_BluemchenHostImpl_getGateValue
};

struct sig_daisy_Host* sig_daisy_BluemchenHost_new(
    struct sig_Allocator* allocator, kxmx::Bluemchen* board) {
    struct sig_daisy_Host* self = sig_MALLOC(allocator, struct sig_daisy_Host);
    struct sig_daisy_BluemchenState* boardState = sig_MALLOC(allocator,
        struct sig_daisy_BluemchenState);
    sig_daisy_BluemchenState_init(boardState, board);
    sig_daisy_BluemchenHost_init(self, boardState);

    return self;
}

void sig_daisy_BluemchenState_init(struct sig_daisy_BluemchenState* self,
    kxmx::Bluemchen* board) {
    self->bluemchen = board;
}

void sig_daisy_BluemchenHost_init(struct sig_daisy_Host* self,
    struct sig_daisy_BluemchenState* boardState) {
    self->impl = &sig_daisy_BluemchenHostImpl;
    self->analogControls = &(boardState->bluemchen->controls[0]);
    self->boardState = (void*) boardState;
}

void sig_daisy_BluemchenHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self) {
    allocator->impl->free(allocator, self->boardState);
    allocator->impl->free(allocator, self);
}

void sig_daisy_BluemchenHostImpl_setControlValue(
    struct sig_daisy_Host* host, int control, float value) {
    struct sig_daisy_BluemchenState* state =
        static_cast<struct sig_daisy_BluemchenState*>(host->boardState);

    // CV output is only implemented on Nechmeulb,
    // but this should be a no-op on Bluemchen.
    if (control > -1 && control < sig_daisy_Nechmeulb_CVOUT_LAST) {
        state->bluemchen->seed.dac.WriteValue(
            static_cast<daisy::DacHandle::Channel>(control),
            sig_bipolarToInvUint12(value));
    }
}

float sig_daisy_BluemchenHostImpl_getGateValue(
    struct sig_daisy_Host* host, int control) {
    // Bluemchen does not have gate inputs.
    return 0.0f;
}
