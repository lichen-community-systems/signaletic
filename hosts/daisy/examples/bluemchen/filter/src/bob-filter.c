#include "../include/bob-filter.h"

struct sig_dsp_Bob* sig_dsp_Bob_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Bob* self = sig_MALLOC(allocator,
        struct sig_dsp_Bob);
    sig_dsp_Bob_init(self, context);
    sig_dsp_FourPoleFilter_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Bob_init(struct sig_dsp_Bob* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Bob_generate);

    self->state[0] = self->state[1] = self->state[2] = self->state[3] = 0.0f;
    self->saturation = 3.0f;
    self->saturationInv = 1.0f / self->saturation;
    self->oversample = 2;
	self->stepSize = 1.0f /
        ((float) self->oversample * self->signal.audioSettings->sampleRate);

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, frequency, context);
    sig_CONNECT_TO_SILENCE(self, resonance, context);
    sig_CONNECT_TO_SILENCE(self, inputGain, context);
    sig_CONNECT_TO_SILENCE(self, pole1Gain, context);
    sig_CONNECT_TO_SILENCE(self, pole2Gain, context);
    sig_CONNECT_TO_SILENCE(self, pole3Gain, context);
    sig_CONNECT_TO_UNITY(self, pole4Gain, context);
}

inline float sig_dsp_Bob_clip(float value, float saturation,
    float saturationInv) {
    // From original comments in PD:
    // A lower cost tanh approximation used to simulate
    // the clipping function of a transistor pair.
    // To 4th order, tanh is x - x*x*x/3.
    // It clamps values to to +/- 1.0 and evaluates the cubic.
    // This is pretty coarse;
    // for instance if you clip a sinusoid this way you
    // can sometimes hear the discontinuity in 4th derivative
    // at the clip point.
    float v2 = (value * saturationInv > 1.0f ? 1.0f :
        (value * saturationInv < -1.0f ? -1.0f : value * saturationInv));
    return (saturation * (v2 - (1.0f / 3.0f) * v2 * v2 * v2));
}

static inline void sig_dsp_Bob_calculateDerivatives(float input,
    float saturation, float saturationInv, float cutoff, float resonance,
    float* deriv, float* state) {
    float satState0 = sig_dsp_Bob_clip(state[0], saturation, saturationInv);
    float satState1 = sig_dsp_Bob_clip(state[1], saturation, saturationInv);
    float satState2 = sig_dsp_Bob_clip(state[2], saturation, saturationInv);
    float satState3 = sig_dsp_Bob_clip(state[3], saturation, saturationInv);
    float inputSat = sig_dsp_Bob_clip(input - resonance * state[3],
        saturation, saturationInv);
    deriv[0] = cutoff * (inputSat - satState0);
    deriv[1] = cutoff * (satState0 - satState1);
    deriv[2] = cutoff * (satState1 - satState2);
    deriv[3] = cutoff * (satState2 - satState3);
}

static inline void sig_dsp_Bob_updateTempState(float* tempState,
    float* state, float stepSize, float* deriv) {
    tempState[0] = state[0] + 0.5 * stepSize * deriv[0];
    tempState[1] = state[1] + 0.5 * stepSize * deriv[1];
    tempState[2] = state[2] + 0.5 * stepSize * deriv[2];
    tempState[3] = state[3] + 0.5 * stepSize * deriv[3];
}

static inline void sig_dsp_Bob_updateState(float* state, float stepSize,
    float* deriv1, float* deriv2, float* deriv3, float* deriv4) {
    state[0] += (1.0 / 6.0) * stepSize *
        (deriv1[0] + 2.0 * deriv2[0] + 2.0 * deriv3[0] + deriv4[0]);
    state[1] += (1.0 / 6.0) * stepSize *
        (deriv1[1] + 2.0 * deriv2[1] + 2.0 * deriv3[1] + deriv4[1]);
    state[2] += (1.0 / 6.0) * stepSize *
        (deriv1[2] + 2.0 * deriv2[2] + 2.0 * deriv3[2] + deriv4[2]);
    state[3] += (1.0 / 6.0) * stepSize *
        (deriv1[3] + 2.0 * deriv2[3] + 2.0 * deriv3[3] + deriv4[3]);

}

static inline void sig_dsp_Bob_solve(struct sig_dsp_Bob* self,
    float input, float cutoff, float resonance) {
    float stepSize = self->stepSize;
    float saturation = self->saturation;
    float saturationInv = self->saturationInv;
    float* state = self->state;
    float* tempState = self->tempState;
    float* deriv1 = self->deriv1;
    float* deriv2 = self->deriv2;
    float* deriv3 = self->deriv3;
    float* deriv4 = self->deriv4;

    sig_dsp_Bob_calculateDerivatives(input, saturation, saturationInv,
        cutoff, resonance, deriv1, state);
    sig_dsp_Bob_updateTempState(tempState, state, stepSize, deriv1);

    sig_dsp_Bob_calculateDerivatives(input, saturation, saturationInv,
        cutoff, resonance, deriv2, tempState);
    sig_dsp_Bob_updateTempState(tempState, state, stepSize, deriv2);

    sig_dsp_Bob_calculateDerivatives(input, saturation, saturationInv,
        cutoff, resonance, deriv3, tempState);
    sig_dsp_Bob_updateTempState(tempState, state, stepSize, deriv3);

    sig_dsp_Bob_calculateDerivatives(input, saturation, saturationInv,
        cutoff, resonance, deriv4, tempState);

    sig_dsp_Bob_updateState(state, stepSize, deriv1, deriv2, deriv3, deriv4);
}

void sig_dsp_Bob_generate(void* signal) {
    struct sig_dsp_Bob* self = (struct sig_dsp_Bob*) signal;
    uint8_t oversample = self->oversample;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float input = FLOAT_ARRAY(self->inputs.source)[i];
        float cutoff = sig_TWOPI * FLOAT_ARRAY(self->inputs.frequency)[i];
        float resonance = FLOAT_ARRAY(self->inputs.resonance)[i];
        resonance = resonance < 0.0f ? 0.0f : resonance;

        for (uint8_t j = 0; j < oversample; j++) {
            sig_dsp_Bob_solve(self, input, cutoff, resonance);
        }

        FLOAT_ARRAY(self->outputs.main)[i] =
            (input * FLOAT_ARRAY(self->inputs.inputGain)[i]) +
            (self->state[0] * FLOAT_ARRAY(self->inputs.pole1Gain)[i]) +
            (self->state[1] * FLOAT_ARRAY(self->inputs.pole2Gain)[i]) +
            (self->state[2] * FLOAT_ARRAY(self->inputs.pole3Gain)[i]) +
            (self->state[3] * FLOAT_ARRAY(self->inputs.pole4Gain)[i]);
        FLOAT_ARRAY(self->outputs.fourPole)[i] = self->state[3];
        FLOAT_ARRAY(self->outputs.twoPole)[i] = self->state[1];
    }
}

void sig_dsp_Bob_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Bob* self) {
    sig_dsp_FourPoleFilter_Outputs_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}
