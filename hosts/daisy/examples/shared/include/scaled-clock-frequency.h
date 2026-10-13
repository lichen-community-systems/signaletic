#ifndef SIGNALETIC_SCALED_CLOCK_FREQUENCY_H
#define SIGNALETIC_SCALED_CLOCK_FREQUENCY_H

#include <libsignaletic.h>

struct sig_dsp_ScaledClockFrequency_Inputs {
    float_array_ptr clock;
    float_array_ptr scale;
};

/**
 * @brief Tracks the frequency of a clock signal and applies a scaling
 * factor to it.
 *
 * Inputs:
 *  - clock: the clock pulses to measure
 *  - scale: the factor by which to multiply the clock's frequency
 *
 * Outputs:
 *  - main: the scaled frequency, in Hz
 */
struct sig_dsp_ScaledClockFrequency {
    struct sig_dsp_Signal signal;
    struct sig_dsp_ScaledClockFrequency_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_dsp_ClockDetector* clockFrequency;
    struct sig_dsp_BinaryOp* scaledFrequency;
};

void sig_dsp_ScaledClockFrequency_generate(void* signal) {
    struct sig_dsp_ScaledClockFrequency* self =
        (struct sig_dsp_ScaledClockFrequency*) signal;
    // FIXME: Need to either more formally separate construction from
    // initialization, or introduce an additional lifecycle hook for
    // handling this kind of initialization logic.
    self->clockFrequency->inputs.source = self->inputs.clock;
    self->scaledFrequency->inputs.right = self->inputs.scale;

    // Generate all child signal outputs.
    self->clockFrequency->signal.generate(self->clockFrequency);
    self->scaledFrequency->signal.generate(self->scaledFrequency);
}

void sig_dsp_ScaledClockFrequency_init(
    struct sig_dsp_ScaledClockFrequency* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_ScaledClockFrequency_generate);

    self->scaledFrequency->inputs.left = self->clockFrequency->outputs.main;
    self->outputs.main = self->scaledFrequency->outputs.main;

    sig_CONNECT_TO_SILENCE(self, clock, context);
    sig_CONNECT_TO_UNITY(self, scale, context);
}

struct sig_dsp_ScaledClockFrequency* sig_dsp_ScaledClockFrequency_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_ScaledClockFrequency* self = sig_MALLOC(allocator,
        struct sig_dsp_ScaledClockFrequency);

    self->clockFrequency = sig_dsp_ClockDetector_new(allocator, context);
    self->scaledFrequency = sig_dsp_Mul_new(allocator, context);

    sig_dsp_ScaledClockFrequency_init(self, context);

    return self;
}

void sig_dsp_ScaledClockFrequency_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ScaledClockFrequency* self) {
    sig_dsp_ClockDetector_destroy(allocator, self->clockFrequency);
    self->clockFrequency = NULL;
    sig_dsp_Mul_destroy(allocator, self->scaledFrequency);
    self->scaledFrequency = NULL;

    // We don't call sig_dsp_Signal_destroy
    // because our output is borrowed from scaledFrequency,
    // which was already freed above.
    allocator->impl->free(allocator, self);
}

#endif /* SIGNALETIC_SCALED_CLOCK_FREQUENCY_H */
