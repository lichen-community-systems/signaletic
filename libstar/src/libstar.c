#include <math.h>
#include <stddef.h>
#include <tlsf.h>
#include <libstar.h>

float star_midiToFreq(float midiNum) {
    return powf(2, (midiNum - 69.0f) / 12.0f) * 440.0f;
}

void star_Buffer_fill(float value, float* buffer, size_t blockSize) {
    for (size_t i = 0; i < blockSize; i++) {
        buffer[i] = value;
    }
}

void star_Buffer_fillSilence(float* buffer, size_t blockSize) {
    star_Buffer_fill(0.0f, buffer, blockSize);
}

void star_Allocator_create(struct star_Allocator* self) {
    tlsf_create_with_pool(self->heap, self->heapSize);
}

void star_Allocator_destroy(struct star_Allocator* self) {
    tlsf_destroy(self->heap);
}

void* star_Allocator_malloc(struct star_Allocator* self, size_t size) {
    return tlsf_malloc(self->heap, size);
}

void star_Allocator_free(struct star_Allocator* self, void* obj) {
    tlsf_free(self->heap, obj);
}

/**
 * Generic generation function
 * that operates on any Signal and outputs only silence.
 */
void star_sig_Signal_generate(void* signal) {
    struct star_sig_Signal* self = (struct star_sig_Signal*) signal;

    for (size_t i = 0; i < self->audioSettings->blockSize; i++) {
        self->output[i] = 0.0f;
    }
}

void star_sig_Value_init(struct star_sig_Value* self,
    struct star_AudioSettings *settings,
    float* output) {

    struct star_sig_Value_Parameters params = {
        .value = 1.0
    };

    struct star_sig_Signal signal = {
        .audioSettings = settings,
        .output = output,
        .generate = *star_sig_Value_generate
    };

    self->signal = signal;
    self->parameters = params;
}

void star_sig_Value_generate(void* signal) {
    struct star_sig_Value* self = (struct star_sig_Value*) signal;

    if (self->parameters.value == self->lastSample) {
        return;
    }

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        self->signal.output[i] = self->parameters.value;
    }

    self->lastSample = self->parameters.value;
}

void star_sig_Sine_init(struct star_sig_Sine* self,
    struct star_AudioSettings* settings,
    struct star_sig_Sine_Inputs* inputs,
    float* output) {

    struct star_sig_Signal signal = {
        .audioSettings = settings,
        .output = output,
        .generate = *star_sig_Sine_generate
    };

    self->signal = signal;
    self->inputs = inputs;

    self->phaseAccumulator = 0.0f;
}

void star_sig_Sine_generate(void* signal) {
    struct star_sig_Sine* self = (struct star_sig_Sine*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float modulatedPhase = fmodf(self->phaseAccumulator +
            self->inputs->phaseOffset[i], star_TWOPI);

        self->signal.output[i] = sinf(modulatedPhase) *
            self->inputs->mul[i] + self->inputs->add[i];

        float phaseStep = self->inputs->freq[i] /
            self->signal.audioSettings->sampleRate * star_TWOPI;

        self->phaseAccumulator += phaseStep;
        if (self->phaseAccumulator > star_TWOPI) {
            self->phaseAccumulator -= star_TWOPI;
        }
    }
}

void star_sig_Gain_init(struct star_sig_Gain* self,
    struct star_AudioSettings* settings, struct star_sig_Gain_Inputs* inputs, float* output) {

    struct star_sig_Signal signal = {
        .audioSettings = settings,
        .output = output,
        .generate = *star_sig_Gain_generate
    };

    self->signal = signal;
    self->inputs = inputs;
};

void star_sig_Gain_generate(void* signal) {
    struct star_sig_Gain* self = (struct star_sig_Gain*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        self->signal.output[i] = self->inputs->source[i] *
            self->inputs->gain[i];
    }
}
