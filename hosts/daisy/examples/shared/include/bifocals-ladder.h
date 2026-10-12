#ifndef SIGNALETIC_BIFOCALS_LADDER_H
#define SIGNALETIC_BIFOCALS_LADDER_H

#include <libsignaletic.h>

// TODO: Fix rampant cut and paste from libsignaletic.c's Ladder.
struct sig_dsp_BifocalsLadder_Inputs {
    float_array_ptr source;
    float_array_ptr frequency;
    float_array_ptr resonance;
    float_array_ptr inputGain;
    float_array_ptr pole1Gain;
    float_array_ptr pole2Gain;
    float_array_ptr pole3Gain;
    float_array_ptr pole4Gain;
    float_array_ptr wavefolderGain;
    float_array_ptr wavefolderFactor;
    float_array_ptr wavefolderPosition;
};

struct sig_dsp_BifocalsLadder {
    struct sig_dsp_Signal signal;
    struct sig_dsp_BifocalsLadder_Inputs inputs;
    struct sig_dsp_Ladder_Parameters parameters;
    struct sig_dsp_FourPoleFilter_Outputs outputs;

    uint8_t interpolation;
    float interpolationRecip;
    float alpha;
    float beta[4];
    float z0[4];
    float z1[4];
    float k;
    float fBase;
    float qAdjust;
    float prevFrequency;
    float prevInput;
};

inline void sig_dsp_BifocalsLadder_calcCoefficients(
    struct sig_dsp_BifocalsLadder* self, float freq) {
    float sampleRate = self->signal.audioSettings->sampleRate;
    freq = sig_clamp(freq, 5.0f, sampleRate * 0.425f);
    float wc = freq * (float) (sig_TWOPI /
        ((float)self->interpolation * sampleRate));
    float wc2 = wc * wc;
    self->alpha = 0.9892f * wc - 0.4324f *
        wc2 + 0.1381f * wc * wc2 - 0.0202f * wc2 * wc2;
    self->qAdjust = 1.006f + 0.0536f * wc - 0.095f * wc2 - 0.05f * wc2 * wc2;
}

inline float sig_dsp_BifocalsLadder_calcStage(
    struct sig_dsp_BifocalsLadder* self, float s, uint8_t i) {
    float ft = s * (1.0f/1.3f) + (0.3f/1.3f) * self->z0[i] - self->z1[i];
    ft = ft * self->alpha + self->z1[i];
    self->z1[i] = ft;
    self->z0[i] = s;
    return ft;
}

void sig_dsp_BifocalsLadder_generate(void* signal) {
    struct sig_dsp_BifocalsLadder* self =
        (struct sig_dsp_BifocalsLadder*) signal;
    float interpolationRecip = self->interpolationRecip;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float input = FLOAT_ARRAY(self->inputs.source)[i];
        float frequency = FLOAT_ARRAY(self->inputs.frequency)[i];
        float resonance = FLOAT_ARRAY(self->inputs.resonance)[i];
        float wavefolderGain = FLOAT_ARRAY(self->inputs.wavefolderGain)[i];
        float wavefolderFactor = FLOAT_ARRAY(self->inputs.wavefolderFactor)[i];
        float wavefolderPosition =
            FLOAT_ARRAY(self->inputs.wavefolderPosition)[i];
        float inputGain = FLOAT_ARRAY(self->inputs.inputGain)[i];
        float pole1Gain = FLOAT_ARRAY(self->inputs.pole1Gain)[i];
        float pole2Gain = FLOAT_ARRAY(self->inputs.pole2Gain)[i];
        float pole3Gain = FLOAT_ARRAY(self->inputs.pole3Gain)[i];
        float pole4Gain = FLOAT_ARRAY(self->inputs.pole4Gain)[i];
        float totals[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        float interp = 0.0f;

        // Recalculate coefficients if the frequency has changed.
        if (frequency != self->prevFrequency) {
            sig_dsp_BifocalsLadder_calcCoefficients(self, frequency);
            self->prevFrequency = frequency;
        }

        self->k = 4.0f * resonance;

        float main = 0.0f;
        for (size_t os = 0; os < self->interpolation; os++) {
            float inInterp = interp * self->prevInput + (1.0f - interp) * input;
            float u = inInterp - (self->z1[3] - self->parameters.passbandGain *
                inInterp) * self->k * self->qAdjust;

           if (wavefolderPosition <= 0.0f) {
                float prefolded = (u +
                    sinf(wavefolderFactor * u)) * wavefolderGain;
                u = prefolded;
            }
            u = sig_fastTanhf(u);

            totals[0] = u;
            float stage1 = sig_dsp_BifocalsLadder_calcStage(self, u, 0);
            totals[1] += stage1 * interpolationRecip;
            float stage2 = sig_dsp_BifocalsLadder_calcStage(self, stage1, 1);
            totals[2] += stage2 * interpolationRecip;
            float stage3 = sig_dsp_BifocalsLadder_calcStage(self, stage2, 2);
            totals[3] += stage3 * interpolationRecip;
            float stage4 = sig_dsp_BifocalsLadder_calcStage(self, stage3, 3);
            totals[4] += stage4 * interpolationRecip;

            float poleSum = (u * inputGain) +
                (stage1 * pole1Gain) +
                (stage2 * pole2Gain) +
                (stage3 * pole3Gain) +
                (stage4 * pole4Gain);

            if (wavefolderPosition > 0.0f) {
                poleSum = (poleSum +
                    sinf(wavefolderFactor * poleSum)) * wavefolderGain;

            }
            float postSaturated = tanhf(poleSum);
            main += postSaturated * interpolationRecip;
            interp += interpolationRecip;
        }
        self->prevInput = input;
        FLOAT_ARRAY(self->outputs.main)[i] = main;
        FLOAT_ARRAY(self->outputs.twoPole)[i] = totals[2];
        FLOAT_ARRAY(self->outputs.fourPole)[i] = totals[4];
    }
}

void sig_dsp_BifocalsLadder_init(
    struct sig_dsp_BifocalsLadder* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_BifocalsLadder_generate);

    struct sig_dsp_Ladder_Parameters parameters = {
        .passbandGain = 0.5f
    };
    self->parameters = parameters;

    self->interpolation = 8;
    self->interpolationRecip = 1.0f / self->interpolation;
    self->alpha = 1.0f;
    self->beta[0] = self->beta[1] = self->beta[2] = self->beta[3] = 0.0f;
    self->z0[0] = self->z0[1] = self->z0[2] = self->z0[3] = 0.0f;
    self->z1[0] = self->z1[1] = self->z1[2] = self->z1[3] = 0.0f;
    self->k = 1.0f;
    self->fBase = 1000.0f;
    self->qAdjust = 1.0f;
    self->prevFrequency = -1.0f;
    self->prevInput = 0.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, frequency, context);
    sig_CONNECT_TO_SILENCE(self, resonance, context);
    sig_CONNECT_TO_SILENCE(self, inputGain, context);
    sig_CONNECT_TO_SILENCE(self, pole1Gain, context);
    sig_CONNECT_TO_SILENCE(self, pole2Gain, context);
    sig_CONNECT_TO_SILENCE(self, pole3Gain, context);
    sig_CONNECT_TO_UNITY(self, pole4Gain, context);
    sig_CONNECT_TO_SILENCE(self, wavefolderGain, context);
    sig_CONNECT_TO_SILENCE(self, wavefolderFactor, context);
    sig_CONNECT_TO_SILENCE(self, wavefolderPosition, context);
}

struct sig_dsp_BifocalsLadder* sig_dsp_BifocalsLadder_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_BifocalsLadder* self = sig_MALLOC(allocator,
        struct sig_dsp_BifocalsLadder);
    sig_dsp_BifocalsLadder_init(self, context);
    sig_dsp_FourPoleFilter_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_BifocalsLadder_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BifocalsLadder* self) {
    sig_dsp_FourPoleFilter_Outputs_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}

#endif /* SIGNALETIC_BIFOCALS_LADDER_H */
