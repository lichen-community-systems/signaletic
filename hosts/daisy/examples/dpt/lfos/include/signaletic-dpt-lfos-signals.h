#include <libsignaletic.h>
#include "../../../../include/signaletic-daisy-host.h"

struct sig_daisy_ClockedLFO_Inputs {
    float_array_ptr clockFreq;
};

struct sig_daisy_ClockedLFO_Parameters {
    int freqScaleCVInputControl;
    int lfoGainCVInputControl;
    int cvOutputControl;
};

// TODO: This isn't quite the correct name.
struct sig_daisy_ClockedLFO {
    struct sig_dsp_Signal signal;
    struct sig_daisy_ClockedLFO_Inputs inputs;
    struct sig_daisy_ClockedLFO_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_daisy_FilteredCVIn* freqScaleIn;
    struct sig_dsp_BinaryOp* clockFreqMultiplier;
    struct sig_daisy_FilteredCVIn* lfoGainIn;
    struct sig_dsp_Oscillator* lfo;
    struct sig_daisy_CVOut* cvOut;
};


struct sig_daisy_ClockedLFO* sig_daisy_ClockedLFO_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
    struct sig_daisy_Host* host);
void sig_daisy_ClockedLFO_init(struct sig_daisy_ClockedLFO* self,
    struct sig_SignalContext* context);
void sig_daisy_ClockedLFO_generate(void* signal);
void sig_daisy_ClockedLFO_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_ClockedLFO* self);


struct sig_daisy_ClockedLFO* sig_daisy_ClockedLFO_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
    struct sig_daisy_Host* host) {
    struct sig_daisy_ClockedLFO* self = sig_MALLOC(allocator,
        struct sig_daisy_ClockedLFO);

    self->freqScaleIn = sig_daisy_FilteredCVIn_new(allocator, context, host);
    self->clockFreqMultiplier = sig_dsp_Mul_new(allocator, context);
    self->lfoGainIn = sig_daisy_FilteredCVIn_new(allocator, context, host);
    self->lfo = sig_dsp_LFTriangle_new(allocator, context);
    self->cvOut = sig_daisy_CVOut_new(allocator, context, host);

    sig_daisy_ClockedLFO_init(self, context);

    return self;
}

void sig_daisy_ClockedLFO_init(struct sig_daisy_ClockedLFO* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_daisy_ClockedLFO_generate);
    self->parameters.freqScaleCVInputControl = 1;
    self->parameters.lfoGainCVInputControl = 2;
    self->parameters.cvOutputControl = 1;

    self->freqScaleIn->parameters.scale = 9.99f;
    self->freqScaleIn->parameters.offset = 0.01f;

    // TODO: Implement proper calibration for CV output
    // My DPT seems to output -4.67V to 7.96V,
    // this was tuned by hand with the (uncalibrated) VCV Rack oscilloscope.
    // cvOut->parameters.scale = 0.68;
    // cvOut->parameters.offset = -0.32;

    self->clockFreqMultiplier->inputs.right = self->freqScaleIn->outputs.main;
    self->lfo->inputs.freq = self->clockFreqMultiplier->outputs.main;
    self->lfo->inputs.mul = self->lfoGainIn->outputs.main;
    self->outputs.main = self->lfo->outputs.main;
    self->cvOut->inputs.source = self->lfo->outputs.main;
    sig_CONNECT_TO_SILENCE(self, clockFreq, context);
}

void sig_daisy_ClockedLFO_generate(void* signal) {
    struct sig_daisy_ClockedLFO* self =
        (struct sig_daisy_ClockedLFO*) signal;
    self->freqScaleIn->parameters.control =
        self->parameters.freqScaleCVInputControl;
    self->lfoGainIn->parameters.control =
        self->parameters.lfoGainCVInputControl;
    self->cvOut->parameters.control = self->parameters.cvOutputControl;

    self->clockFreqMultiplier->inputs.left = self->inputs.clockFreq;

    self->freqScaleIn->signal.generate(self->freqScaleIn);
    self->clockFreqMultiplier->signal.generate(self->clockFreqMultiplier);
    self->lfoGainIn->signal.generate(self->lfoGainIn);
    self->lfo->signal.generate(self->lfo);
    self->cvOut->signal.generate(self->cvOut);
}

void sig_daisy_ClockedLFO_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_ClockedLFO* self) {
    sig_daisy_FilteredCVIn_destroy(allocator, self->freqScaleIn);
    sig_dsp_Mul_destroy(allocator, self->clockFreqMultiplier);
    sig_daisy_FilteredCVIn_destroy(allocator, self->lfoGainIn);
    sig_dsp_LFTriangle_destroy(allocator, self->lfo);
    sig_daisy_CVOut_destroy(allocator, self->cvOut);

    // We don't call sig_dsp_Signal_destroy
    // because our output is borrowed from self->lfo,
    // which was already freed in LFTriangle's destructor.
    allocator->impl->free(allocator, self);
}

