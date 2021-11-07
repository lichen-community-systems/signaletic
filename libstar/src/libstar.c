// TODO: Macro for main sample processing loop
// TODO: Clarify memory allocation issues
//   - how are buffers allocated to the appropriate actual size?

#include "../include/libstar.h"

float star_midiToFreq(float midiNum) {
    return 440.0f * powf(2, (midiNum - 69.0f) * 1.0f / 12.0f);
}

void star_Buffer_fill(float value, float* buffer, int blockSize) {
    for (int i = 0; i < blockSize; i++) {
        buffer[i] = value;
    }
}

void star_Buffer_fillSilence(float* buffer, int blockSize) {
    star_Buffer_fill(0.0f, buffer, blockSize);
}

void star_sig_Value_init(void* self,
    struct star_AudioSettings *settings,
    float* output) {
    struct star_sig_Value* this = (struct star_sig_Value*) self;

    // TODO: memory allocation issues?
    struct star_sig_Value_Parameters params = {
        .value = 1.0
    };

    // TODO: memory allocation issues?
    struct star_sig_Signal signal = {
        .audioSettings = settings,
        .output = output,
        .generate = *star_sig_Value_generate
    };

    this->signal = signal;
    this->parameters = params;
}

void star_sig_Value_generate(void* self) {
    struct star_sig_Value* this = (struct star_sig_Value*) self;

    if (this->parameters.value == this->lastSample) {
        return;
    }

    for (int i = 0; i < this->signal.audioSettings->blockSize; i++) {
        this->signal.output[i] = this->parameters.value;
    }

    this->lastSample = this->parameters.value;
}

void star_sig_Sine_init(void* self,
    struct star_AudioSettings* settings,
    struct star_sig_Sine_Inputs* inputs,
    float* output) {
    struct star_sig_Sine* this = (struct star_sig_Sine*) self;

    // TODO: memory allocation issues?
    struct star_sig_Signal signal = {
        .audioSettings = settings,
        .output = output,
        .generate = *star_sig_Sine_generate
    };

    this->signal = signal;
    this->inputs = inputs;

    this->phaseAccumulator = 0.0f;
}

void star_sig_Sine_generate(void* self) {
    struct star_sig_Sine* this = (struct star_sig_Sine*) self;

    for (int i = 0; i < this->signal.audioSettings->blockSize; i++) {
        float modulatedPhase = fmod(this->phaseAccumulator +
            this->inputs->phaseOffset[i], star_TWOPI);

        this->signal.output[i] = sinf(modulatedPhase) *
            this->inputs->mul[i] + this->inputs->add[i];

        float phaseStep = this->inputs->freq[i] /
            this->signal.audioSettings->sampleRate * star_TWOPI;

        this->phaseAccumulator += phaseStep;
        if (this->phaseAccumulator > star_TWOPI) {
            this->phaseAccumulator -= star_TWOPI;
        }
    }
}

void star_sig_Gain_init(void* self,
    struct star_AudioSettings* settings, struct star_sig_Gain_Inputs* inputs, float* output) {
    struct star_sig_Gain* this = (struct star_sig_Gain*) self;

    // TODO: memory allocation issues?
    struct star_sig_Signal signal = {
        .audioSettings = settings,
        .output = output,
        .generate = *star_sig_Gain_generate
    };

    this->signal = signal;
    this->inputs = inputs;
};

void star_sig_Gain_generate(void* self) {
    struct star_sig_Gain* this = (struct star_sig_Gain*) self;

    for (int i = 0; i < this->signal.audioSettings->blockSize; i++) {
        this->signal.output[i] = this->inputs->source[i] *
            this->inputs->gain[i];
    }
}
