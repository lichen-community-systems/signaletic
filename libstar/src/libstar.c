#include <math.h>
#include <stddef.h>
#include <tlsf.h>
#include <libstar.h>

float star_midiToFreq(float midiNum) {
    return powf(2, (midiNum - 69.0f) / 12.0f) * 440.0f;
}

void star_Allocator_init(struct star_Allocator* self) {
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

void star_Buffer_fill(float value, float* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = value;
    }
}

void star_Buffer_fillSilence(float* buffer, size_t size) {
    star_Buffer_fill(0.0f, buffer, size);
}

float* star_Buffer_new(struct star_Allocator* allocator,
    size_t numSamples) {
    return (float*) star_Allocator_malloc(allocator,
        sizeof(float) * numSamples);
}

float* star_AudioBlock_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings) {
    return star_Buffer_new(allocator, settings->blockSize);
}

float* star_AudioBlock_newWithValue(float value,
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings) {
    float* block = star_AudioBlock_new(allocator, audioSettings);
    star_Buffer_fill(value, block, audioSettings->blockSize);

    return block;
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

void star_sig_Signal_destroy(struct star_Allocator* allocator, void* signal) {
    star_Allocator_free(allocator,
        ((struct star_sig_Signal*) signal)->output);
    star_Allocator_free(allocator, signal);
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

struct star_sig_Value* star_sig_Value_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings) {
    float* output = star_AudioBlock_new(allocator, settings);
    struct star_sig_Value* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_Value));
    star_sig_Value_init(self, settings, output);

    return self;
}

void star_sig_Value_destroy(struct star_Allocator* allocator, struct star_sig_Value* self) {
    star_sig_Signal_destroy(allocator, (void*) self);
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

struct star_sig_Sine* star_sig_Sine_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Sine_Inputs* inputs) {
    float* output = star_AudioBlock_new(allocator, settings);
    struct star_sig_Sine* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_Sine));
    star_sig_Sine_init(self, settings, inputs, output);

    return self;
}

void star_sig_Sine_destroy(struct star_Allocator* allocator, struct star_sig_Sine* self) {
    star_sig_Signal_destroy(allocator, (void*) self);
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

struct star_sig_Gain* star_sig_Gain_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Gain_Inputs* inputs) {
    float* output = star_AudioBlock_new(allocator, settings);
    struct star_sig_Gain* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_Gain));
    star_sig_Gain_init(self, settings, inputs, output);

    return self;
}

void star_sig_Gain_destroy(struct star_Allocator* allocator,
    struct star_sig_Gain* self) {
    star_sig_Signal_destroy(allocator, (void*) self);
};

void star_sig_Gain_generate(void* signal) {
    struct star_sig_Gain* self = (struct star_sig_Gain*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        self->signal.output[i] = self->inputs->source[i] *
            self->inputs->gain[i];
    }
}
