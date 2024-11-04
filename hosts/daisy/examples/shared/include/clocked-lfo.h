#ifndef SIGNALETIC_CLOCKED_LFO_H
#define SIGNALETIC_CLOCKED_LFO_H

#include <libsignaletic.h>
#include "../../../../include/signaletic-host.h"

struct sig_dsp_ClockedLFO_Inputs {
    float_array_ptr clock;
    float_array_ptr frequencyScale;
    float_array_ptr scale;
    float_array_ptr offset;
};

struct sig_dsp_ClockedLFO {
    struct sig_dsp_Signal signal;
    struct sig_dsp_ClockedLFO_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_dsp_ClockDetector* clockFrequency;
    struct sig_dsp_BinaryOp* clockFrequencyMultiplier;
    struct sig_dsp_Oscillator* lfo;
};

void sig_dsp_ClockedLFO_generate(void* signal) {
    struct sig_dsp_ClockedLFO* self = (struct sig_dsp_ClockedLFO*) signal;
    // FIXME: Need to either more formally separate construction from
    // initialization, or introduce an additional lifecycle hook for
    // handling this kind of initialization logic.
    self->clockFrequency->inputs.source = self->inputs.clock;
    self->clockFrequencyMultiplier->inputs.right = self->inputs.frequencyScale;
    self->lfo->inputs.mul = self->inputs.scale;
    self->lfo->inputs.add = self->inputs.offset;

    // Generate all child signal outputs.
    self->clockFrequency->signal.generate(self->clockFrequency);
    self->clockFrequencyMultiplier->signal.generate(
        self->clockFrequencyMultiplier);
    self->lfo->signal.generate(self->lfo);
}

void sig_dsp_ClockedLFO_init(struct sig_dsp_ClockedLFO* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_ClockedLFO_generate);

    self->clockFrequencyMultiplier->inputs.left =
        self->clockFrequency->outputs.main;
    self->lfo->inputs.freq = self->clockFrequencyMultiplier->outputs.main;
    self->outputs.main = self->lfo->outputs.main;

    sig_CONNECT_TO_SILENCE(self, clock, context);
    sig_CONNECT_TO_UNITY(self, frequencyScale, context);
    sig_CONNECT_TO_UNITY(self, scale, context);
    sig_CONNECT_TO_SILENCE(self, offset, context);
}

struct sig_dsp_ClockedLFO* sig_dsp_ClockedLFO_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_ClockedLFO* self = sig_MALLOC(allocator,
        struct sig_dsp_ClockedLFO);

    self->clockFrequency = sig_dsp_ClockDetector_new(allocator, context);
    self->clockFrequencyMultiplier = sig_dsp_Mul_new(allocator, context);
    self->lfo = sig_dsp_LFTriangle_new(allocator, context);

    sig_dsp_ClockedLFO_init(self, context);

    return self;
}

void sig_dsp_ClockedLFO_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ClockedLFO* self) {
    sig_dsp_ClockDetector_destroy(allocator, self->clockFrequency);
    self->clockFrequency = NULL;
    sig_dsp_Mul_destroy(allocator, self->clockFrequencyMultiplier);
    self->clockFrequencyMultiplier = NULL;
    sig_dsp_LFTriangle_destroy(allocator, self->lfo);
    self->lfo = NULL;

    // We don't call sig_dsp_Signal_destroy
    // because our output is borrowed from self->lfo,
    // which was already freed in LFTriangle's destructor.
    allocator->impl->free(allocator, self);
    self = NULL;
}

#endif /* SIGNALETIC_CLOCKED_LFO_H */
