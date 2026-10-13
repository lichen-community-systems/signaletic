#ifndef SIGNALETIC_BIFOCALS_LADDER_H
#define SIGNALETIC_BIFOCALS_LADDER_H

#include <libsignaletic.h>

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
    /**
     * @brief The signal's base state.
     *
     */
    struct sig_dsp_Signal signal;

    /**
     * @brief The filter's inputs.
     *
     */
    struct sig_dsp_BifocalsLadder_Inputs inputs;

    /**
     * @brief The filter's parameters.
     *
     */
    struct sig_dsp_Ladder_Parameters parameters;

    /**
     * @brief The filter's outputs.
     *
     */
    struct sig_dsp_FourPoleFilter_Outputs outputs;

    /**
     * @brief The filter's state.
     *
     */
    struct sig_filter_Ladder state;

    /**
     * @brief The frequency from the previous sample, in Hz.
     *
     */
    float previousFrequency;
};

void sig_dsp_BifocalsLadder_generate(void* signal) {
    struct sig_dsp_BifocalsLadder* self =
        (struct sig_dsp_BifocalsLadder*) signal;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    const float passbandGain = self->parameters.passbandGain;
    const uint8_t interpolation = self->state.interpolation;
    const float interpolationRecip = self->state.interpolationRecip;
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT freqIn = FLOAT_ARRAY(self->inputs.frequency);
    const float* SIG_RESTRICT resIn = FLOAT_ARRAY(self->inputs.resonance);
    const float* SIG_RESTRICT inputGainIn = FLOAT_ARRAY(self->inputs.inputGain);
    const float* SIG_RESTRICT pole1GainIn = FLOAT_ARRAY(self->inputs.pole1Gain);
    const float* SIG_RESTRICT pole2GainIn = FLOAT_ARRAY(self->inputs.pole2Gain);
    const float* SIG_RESTRICT pole3GainIn = FLOAT_ARRAY(self->inputs.pole3Gain);
    const float* SIG_RESTRICT pole4GainIn = FLOAT_ARRAY(self->inputs.pole4Gain);
    const float* SIG_RESTRICT wavefolderGainIn = FLOAT_ARRAY(
        self->inputs.wavefolderGain);
    const float* SIG_RESTRICT wavefolderFactorIn = FLOAT_ARRAY(
        self->inputs.wavefolderFactor);
    const float* SIG_RESTRICT wavefolderPositionIn = FLOAT_ARRAY(
        self->inputs.wavefolderPosition);
    float* SIG_RESTRICT mainOut = FLOAT_ARRAY(self->outputs.main);
    float* SIG_RESTRICT twoPoleOut = FLOAT_ARRAY(self->outputs.twoPole);
    float* SIG_RESTRICT fourPoleOut = FLOAT_ARRAY(self->outputs.fourPole);

    for (size_t i = 0; i < blockSize; i++) {
        float input = in[i];
        float frequency = freqIn[i];
        float resonance = resIn[i];

        // Recalculate coefficients if the frequency has changed.
        if (frequency != self->previousFrequency) {
            sig_filter_Ladder_calculateCoefficients(&self->state, frequency,
                sampleRate);
            self->previousFrequency = frequency;
        }

        float wavefolderGain = wavefolderGainIn[i];
        float wavefolderFactor = wavefolderFactorIn[i];
        float wavefolderPosition = wavefolderPositionIn[i];
        float inputGain = inputGainIn[i];
        float pole1Gain = pole1GainIn[i];
        float pole2Gain = pole2GainIn[i];
        float pole3Gain = pole3GainIn[i];
        float pole4Gain = pole4GainIn[i];
        float twoPole = 0.0f;
        float fourPole = 0.0f;
        float interp = 0.0f;
        float main = 0.0f;

        for (size_t os = 0; os < interpolation; os++) {
            float u = sig_filter_Ladder_calcStageInput(&self->state, input,
                interp, resonance, passbandGain);

            if (wavefolderPosition <= 0.0f) {
                u = (u + sinf(wavefolderFactor * u)) * wavefolderGain;
            }
            u = sig_fastTanhf(u);

            struct sig_filter_Ladder_Taps step;
            sig_filter_Ladder_calcStages(&self->state, u, &step);

            float poleSum = (step.input * inputGain) +
                (step.pole1 * pole1Gain) +
                (step.pole2 * pole2Gain) +
                (step.pole3 * pole3Gain) +
                (step.pole4 * pole4Gain);

            if (wavefolderPosition > 0.0f) {
                poleSum = (poleSum +
                    sinf(wavefolderFactor * poleSum)) * wavefolderGain;
            }
            main += tanhf(poleSum) * interpolationRecip;
            twoPole += step.pole2 * interpolationRecip;
            fourPole += step.pole4 * interpolationRecip;
            interp += interpolationRecip;
        }
        self->state.previousInput = input;
        mainOut[i] = main;
        twoPoleOut[i] = twoPole;
        fourPoleOut[i] = fourPole;
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

    sig_filter_Ladder_init(&self->state, 8);
    self->previousFrequency = -1.0f;

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
