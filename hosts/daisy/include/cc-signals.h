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
    struct cc_sig_DustGate_Inputs* inputs;
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
    struct sig_AudioSettings* audioSettings, float_array_ptr output) {
    sig_dsp_Signal_init(self, audioSettings, output,
        *cc_sig_DustGate_generate);
};

struct cc_sig_DustGate* cc_sig_DustGate_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    cc_sig_DustGate_Inputs* inputs) {

    struct cc_sig_DustGate* self =
        (cc_sig_DustGate*) allocator->impl->malloc(allocator->heap,
        sizeof(struct cc_sig_DustGate));

    struct sig_dsp_Dust_Inputs* dustInputs = (struct sig_dsp_Dust_Inputs*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_Dust_Inputs));
    dustInputs->density = inputs->density;

    self->dust = sig_dsp_Dust_new(allocator,
        audioSettings, dustInputs);

    struct sig_dsp_BinaryOp_Inputs* reciprocalDensityInputs =
        (struct sig_dsp_BinaryOp_Inputs*) allocator->impl->malloc(
            allocator->heap, sizeof(struct sig_dsp_BinaryOp_Inputs));
    reciprocalDensityInputs->left =
        sig_AudioBlock_newWithValue(allocator, audioSettings, 1.0f);
    reciprocalDensityInputs->right = inputs->density;

    self->reciprocalDensity = sig_dsp_Div_new(allocator,
        audioSettings, reciprocalDensityInputs);

    struct sig_dsp_BinaryOp_Inputs* densityDurationMuliplierInputs =
        (struct sig_dsp_BinaryOp_Inputs*)
            allocator->impl->malloc(allocator->heap,
                sizeof(struct sig_dsp_BinaryOp_Inputs));
    densityDurationMuliplierInputs->left =
        self->reciprocalDensity->signal.output;
    densityDurationMuliplierInputs->right = inputs->durationPercentage;

    self->densityDurationMultiplier = sig_dsp_Mul_new(allocator,
        audioSettings, densityDurationMuliplierInputs);

    struct sig_dsp_TimedGate_Inputs* gateInputs =
        (struct sig_dsp_TimedGate_Inputs*) allocator->impl->malloc(
            allocator->heap,
            sizeof(struct sig_dsp_TimedGate_Inputs));
    gateInputs->trigger =self->dust->signal.output;
    gateInputs->duration =
        self->densityDurationMultiplier->signal.output;

    self->gate = sig_dsp_TimedGate_new(allocator,
        audioSettings, gateInputs);

    cc_sig_DustGate_init(self, audioSettings, self->gate->signal.output);

    return self;
}

void cc_sig_DustGate_destroy(struct sig_Allocator* allocator,
    struct cc_sig_DustGate* self) {
    allocator->impl->free(allocator->heap, self->gate->inputs);
    sig_dsp_TimedGate_destroy(allocator, self->gate);

    allocator->impl->free(allocator->heap,
        self->densityDurationMultiplier->inputs);
    sig_dsp_Mul_destroy(allocator,
        self->densityDurationMultiplier);

    allocator->impl->free(allocator->heap,
        self->reciprocalDensity->inputs);
    sig_dsp_Div_destroy(allocator, self->reciprocalDensity);

    allocator->impl->free(allocator->heap, self->dust->inputs);
    sig_dsp_Dust_destroy(allocator, self->dust);

    // We don't call sig_dsp_Signal_destroy
    // because our output is borrowed from self->gate,
    // and it was already freed in TimeGate's destructor.
    allocator->impl->free(allocator->heap, self);
}
