#ifndef SIGNALETIC_CLOCK_DIVIDING_LFO_H
#define SIGNALETIC_CLOCK_DIVIDING_LFO_H

#include <libsignaletic.h>
#include "../../../shared/include/clocked-lfo.h"
#include "../../../shared/include/summed-cv-in.h"

struct sig_host_ClockDividingLFO_Inputs {
    float_array_ptr clock;
};

struct sig_host_ClockDividingLFO {
    struct sig_dsp_Signal signal;
    struct sig_host_ClockDividingLFO_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;

    struct sig_Buffer* clockDivisionsBuffer;

    struct sig_host_SummedCVIn* frequencyCV;
    struct sig_dsp_List* clockDivisions;
    struct sig_host_SummedCVIn* waveform;
    struct sig_dsp_ClockedLFO* lfo;
};

void sig_host_ClockDividingLFO_generate(void* signal) {
    struct sig_host_ClockDividingLFO* self =
        (struct sig_host_ClockDividingLFO*) signal;
    self->frequencyCV->hardware = self->hardware;
    self->waveform->hardware = self->hardware;
    self->lfo->inputs.clock = self->inputs.clock;
    self->clockDivisions->list = self->clockDivisionsBuffer;

    // Generate child signal outputs.
    self->frequencyCV->signal.generate(self->frequencyCV);
    self->clockDivisions->signal.generate(self->clockDivisions);
    self->waveform->signal.generate(self->waveform);
    self->lfo->signal.generate(self->lfo);
}

void sig_host_ClockDividingLFO_init(struct sig_host_ClockDividingLFO* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_ClockDividingLFO_generate);

    self->clockDivisions->parameters.wrap = 0.0f;
    self->clockDivisions->parameters.interpolate = 1.0f;
    self->clockDivisions->inputs.index = self->frequencyCV->outputs.main;

    self->lfo->inputs.frequencyScale = self->clockDivisions->outputs.main;
    self->lfo->lfo->inputs.tableIndex = self->waveform->outputs.main;
    self->outputs.main = self->lfo->outputs.main;

    sig_CONNECT_TO_SILENCE(self, clock, context);
}

struct sig_host_ClockDividingLFO* sig_host_ClockDividingLFO_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_host_ClockDividingLFO* self = sig_MALLOC(allocator,
        struct sig_host_ClockDividingLFO);

    self->frequencyCV = sig_host_SummedCVIn_new(allocator, context);
    self->clockDivisions = sig_dsp_List_new(allocator, context);
    self->waveform = sig_host_SummedCVIn_new(allocator, context);
    self->lfo = sig_dsp_ClockedLFO_new(allocator, context);

    sig_host_ClockDividingLFO_init(self, context);

    return self;
}

void sig_host_ClockDividingLFO_destroy(struct sig_Allocator* allocator,
    struct sig_host_ClockDividingLFO* self) {
    sig_dsp_ClockedLFO_destroy(allocator, self->lfo);
    self->lfo = NULL;
    sig_host_SummedCVIn_destroy(allocator, self->waveform);
    self->waveform = NULL;
    sig_dsp_List_destroy(allocator, self->clockDivisions);
    self->clockDivisions = NULL;
    sig_host_SummedCVIn_destroy(allocator, self->frequencyCV);
    self->frequencyCV = NULL;

    allocator->impl->free(allocator, self);
    self = NULL;
}

#endif /* SIGNALETIC_CLOCK_DIVIDING_LFO_H */
