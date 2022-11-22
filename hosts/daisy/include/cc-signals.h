#include <libsignaletic.h>

struct cc_sig_DustGate_Inputs {
    float_array_ptr density;
    float_array_ptr durationPercentage;
};

struct cc_sig_DustGate_Parameters {
    float bipolar;
};

struct cc_sig_DustGate {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_dsp_Dust* dust;
    struct sig_dsp_BinaryOp* reciprocalDensity;
    struct sig_dsp_BinaryOp* densityDurationMultiplier;
    struct sig_dsp_TimedGate* gate;
};

void cc_sig_DustGate_generate(void* signal) {
    struct cc_sig_DustGate* self = (struct cc_sig_DustGate*) signal;

    self->reciprocalDensity->signal.generate(self->reciprocalDensity);
    self->densityDurationMultiplier->signal.generate(
        self->densityDurationMultiplier);
    self->dust->signal.generate(self->dust);
    self->gate->signal.generate(self->gate);
}

void cc_sig_DustGate_init(struct cc_sig_DustGate* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *cc_sig_DustGate_generate);
};

// TODO: There's an asymmetry here with the constructor signature,
// compared to other signals. The issue is that we need to be able to assign
// our top-level inputs as inputs to sub-signals. This will need to designed
// explicitly so that users can compose Signals together, including defining
// connections between them. It will likely require additional lifecycle hooks
// for a Signal.
struct cc_sig_DustGate* cc_sig_DustGate_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context, struct cc_sig_DustGate_Inputs inputs) {
    struct cc_sig_DustGate* self = sig_MALLOC(allocator,
        struct cc_sig_DustGate);

    self->dust = sig_dsp_Dust_new(allocator, context);
    self->dust->inputs.density = inputs.density;

    self->reciprocalDensity = sig_dsp_Div_new(allocator, context);
    self->reciprocalDensity->inputs.left =
        sig_AudioBlock_newWithValue(allocator, context->audioSettings, 1.0f);
    self->reciprocalDensity->inputs.right = inputs.density;

    self->densityDurationMultiplier = sig_dsp_Mul_new(allocator, context);
    self->densityDurationMultiplier->inputs.left =
        self->reciprocalDensity->outputs.main;
    self->densityDurationMultiplier->inputs.right =
        inputs.durationPercentage;

    self->gate = sig_dsp_TimedGate_new(allocator, context);
    self->gate->inputs.trigger = self->dust->outputs.main;
    self->gate->inputs.duration =
        self->densityDurationMultiplier->outputs.main;

    cc_sig_DustGate_init(self, context);
    self->outputs.main = self->gate->outputs.main;

    return self;
}

void cc_sig_DustGate_destroy(struct sig_Allocator* allocator,
    struct cc_sig_DustGate* self) {
    sig_dsp_TimedGate_destroy(allocator, self->gate);

    sig_dsp_Mul_destroy(allocator, self->densityDurationMultiplier);
    sig_AudioBlock_destroy(allocator, self->reciprocalDensity->inputs.left);
    sig_dsp_Div_destroy(allocator, self->reciprocalDensity);

    sig_dsp_Dust_destroy(allocator, self->dust);

    // We don't call sig_dsp_Signal_destroy
    // because our output is borrowed from self->gate,
    // and it was already freed in TimeGate's destructor.
    allocator->impl->free(allocator, self);
}
