#include <libstar.h>

struct cc_sig_DustingGate_Inputs {
    float_array_ptr density;
    float_array_ptr durationPercentage;
};

struct cc_sig_DustingGate_Parameters {
    float bipolar;
};

struct cc_sig_DustingGate {
    struct star_sig_Signal signal;
    struct cc_sig_DustingGate_Inputs* inputs;
    struct star_sig_Dust* dust;
    struct star_sig_BinaryOp* reciprocalDensity;
    struct star_sig_BinaryOp* densityDurationMultiplier;
    struct star_sig_TimedGate* gate;
};

void cc_sig_DustingGate_generate(void* signal) {
    struct cc_sig_DustingGate* self = (struct cc_sig_DustingGate*) signal;

    self->reciprocalDensity->signal.generate(self->reciprocalDensity);
    self->densityDurationMultiplier->signal.generate(
        self->densityDurationMultiplier);
    self->dust->signal.generate(self->dust);
    self->gate->signal.generate(self->gate);
}

void cc_sig_DustingGate_init(struct cc_sig_DustingGate* self,
    struct star_AudioSettings* audioSettings, float_array_ptr output) {
    star_sig_Signal_init(self, audioSettings, output,
        *cc_sig_DustingGate_generate);
};

struct cc_sig_DustingGate* cc_sig_DustingGate_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings,
    cc_sig_DustingGate_Inputs* inputs) {

    struct cc_sig_DustingGate* self =
        (cc_sig_DustingGate*) star_Allocator_malloc(allocator,
        sizeof(struct cc_sig_DustingGate));

    struct star_sig_Dust_Inputs* dustInputs = (struct star_sig_Dust_Inputs*)
        star_Allocator_malloc(allocator,
            sizeof(struct star_sig_Dust_Inputs));
    dustInputs->density = inputs->density;

    self->dust = star_sig_Dust_new(allocator,
        audioSettings, dustInputs);

    struct star_sig_BinaryOp_Inputs* reciprocalDensityInputs =
        (struct star_sig_BinaryOp_Inputs*) star_Allocator_malloc(allocator,
            sizeof(struct star_sig_BinaryOp_Inputs));
    reciprocalDensityInputs->left =
        star_AudioBlock_newWithValue(allocator, audioSettings, 1.0f);
    reciprocalDensityInputs->right = inputs->density;

    self->reciprocalDensity = star_sig_Div_new(allocator,
        audioSettings, reciprocalDensityInputs);

    struct star_sig_BinaryOp_Inputs* densityDurationMuliplierInputs =
        (struct star_sig_BinaryOp_Inputs*)
            star_Allocator_malloc(allocator,
                sizeof(struct star_sig_BinaryOp_Inputs));
    densityDurationMuliplierInputs->left =
        self->reciprocalDensity->signal.output;
    densityDurationMuliplierInputs->right = inputs->durationPercentage;

    self->densityDurationMultiplier = star_sig_Mul_new(allocator,
        audioSettings, densityDurationMuliplierInputs);

    struct star_sig_TimedGate_Inputs* gateInputs =
        (struct star_sig_TimedGate_Inputs*) star_Allocator_malloc(allocator,
            sizeof(struct star_sig_TimedGate_Inputs));
    gateInputs->trigger =self->dust->signal.output;
    gateInputs->duration =
        self->densityDurationMultiplier->signal.output;

    self->gate = star_sig_TimedGate_new(allocator,
        audioSettings, gateInputs);

    cc_sig_DustingGate_init(self, audioSettings, self->gate->signal.output);

    return self;
}

void cc_sig_DustingGate_destroy(struct star_Allocator* allocator,
    struct cc_sig_DustingGate* self) {
    star_Allocator_free(allocator, self->gate->inputs);
    star_sig_TimedGate_destroy(allocator, self->gate);

    star_Allocator_free(allocator,
        self->densityDurationMultiplier->inputs);
    star_sig_Mul_destroy(allocator,
        self->densityDurationMultiplier);

    star_Allocator_free(allocator, self->reciprocalDensity->inputs);
    star_sig_Div_destroy(allocator, self->reciprocalDensity);

    star_Allocator_free(allocator, self->dust->inputs);
    star_sig_Dust_destroy(allocator, self->dust);

    // We don't call star_sig_Signal_destroy
    // because our output is borrowed from self->gate,
    // and it was already freed in TimeGate's destructor.
    star_Allocator_free(allocator, self);
}
