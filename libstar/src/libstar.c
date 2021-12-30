#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <tlsf.h>
#include <libstar.h>

// TODO: Unit tests
// TODO: Inline?
float star_clamp(float value, float min, float max) {
    const float minClamped = value < min ? min : value;
    return minClamped > max ? max : minClamped;
}

// TODO: Inline? http://www.greenend.org.uk/rjk/tech/inline.html
float star_midiToFreq(float midiNum) {
    return powf(2, (midiNum - 69.0f) / 12.0f) * 440.0f;
}

// TODO: Inline?
void star_fillWithValue(float value, float* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = value;
    }
}

// TODO: Inline?
void star_fillWithSilence(float* buffer, size_t size) {
    star_fillWithValue(0.0f, buffer, size);
}

// TODO: Unit tests.
// TODO: Inline?
float star_interpolate_linear(float idx, float* table,
    size_t length) {
    int32_t idxIntegral = (int32_t) idx;
    float idxFractional = idx - (float) idxIntegral;
    float a = table[idxIntegral];
    // TODO: Do we want to wrap around the end like this,
    // or should we expect users to provide us with idx within bounds?
    float b = table[(idxIntegral + 1) % length];

    return a + (b - a) * idxFractional;
}

// TODO: Unit tests.
// TODO: Inline?
float star_interpolate_cubic(float idx, float* table,
    size_t length) {
    size_t idxIntegral = (size_t) idx;
    float idxFractional = idx - (float) idxIntegral;

    // TODO: As above, are these modulo operations required,
    // or should we expect users to provide us in-bound values?
    const size_t i0 = idxIntegral % length;
    const float xm1 = table[i0 > 0 ? i0 - 1 : length - 1];
    const float x0 = table[i0];
    const float x1 = table[(i0 + 1) % length];
    const float x2 = table[(i0 + 2) % length];
    const float c = (x1 - xm1) * 0.5f;
    const float v = x0 - x1;
    const float w = c + v;
    const float a = w + v + (x2 - x0) * 0.5f;
    const float bNeg = w + a;

    return (((a * idxFractional) - bNeg) * idxFractional + c) *
        idxFractional + x0;
}

// TODO: Unit tests.
// TODO: Inline?
float star_filter_onepole(float current, float previous, float coeff) {
    return current + coeff * (previous - current);
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

float* star_samples_new(struct star_Allocator* allocator, size_t length) {
    return (float*) star_Allocator_malloc(allocator,
        sizeof(float) * length);
}

float* star_AudioBlock_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings) {
    return star_samples_new(allocator, settings->blockSize);
}

float* star_AudioBlock_newWithValue(float value,
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings) {
    float* block = star_AudioBlock_new(allocator, audioSettings);
    star_fillWithValue(value, block, audioSettings->blockSize);

    return block;
}

struct star_Buffer* star_Buffer_new(struct star_Allocator* allocator,
    size_t length) {
    struct star_Buffer* buffer = (struct star_Buffer*) star_Allocator_malloc(allocator, sizeof(struct star_Buffer));
    buffer->length = length;
    buffer->samples = star_samples_new(allocator, length);

    return buffer;
}

// TODO: Unit tests
void star_Buffer_fill(struct star_Buffer* self, float value) {
    star_fillWithValue(value, self->samples, self->length);
}

// TODO: Unit tests
void star_Buffer_fillWithSilence(struct star_Buffer* self) {
    star_fillWithSilence(self->samples, self->length);
}

void star_Buffer_destroy(struct star_Allocator* allocator, struct star_Buffer* buffer) {
    star_Allocator_free(allocator, buffer->samples);
    star_Allocator_free(allocator, buffer);
};

void star_sig_Signal_init(void* signal,
    struct star_AudioSettings* settings,
    float* output,
    star_sig_generateFn generate) {
    struct star_sig_Signal* self = (struct star_sig_Signal*) signal;

    self->audioSettings = settings;
    self->output = output;
    self->generate = generate;
};

/**
 * Generic generation function
 * that operates on any Signal and outputs only silence.
 */
void star_sig_Signal_generate(void* signal) {
    struct star_sig_Signal* self = (struct star_sig_Signal*) signal;
    star_fillWithSilence(self->output, self->audioSettings->blockSize);
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

    star_sig_Signal_init(self, settings, output,
        *star_sig_Value_generate);

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

    star_sig_Signal_init(self, settings, output,
        *star_sig_Sine_generate);

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

    star_sig_Signal_init(self, settings, output,
        *star_sig_Gain_generate);
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


void star_sig_OnePole_init(struct star_sig_OnePole* self,
    struct star_AudioSettings* settings,
    struct star_sig_OnePole_Inputs* inputs,
    float* output) {

    star_sig_Signal_init(self, settings, output,
        *star_sig_OnePole_generate);
    self->inputs = inputs;
    self->previousSample = 0.0f;
}

struct star_sig_OnePole* star_sig_OnePole_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_OnePole_Inputs* inputs) {

    float* output = star_AudioBlock_new(allocator, settings);
    struct star_sig_OnePole* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_OnePole));
    star_sig_OnePole_init(self, settings, inputs, output);

    return self;
}

void star_sig_OnePole_generate(void* signal) {
    struct star_sig_OnePole* self = (struct star_sig_OnePole*) signal;

    float previousSample = self->previousSample;
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        self->signal.output[i] = previousSample = star_filter_onepole(
            self->inputs->source[i], previousSample,
            self->inputs->coefficient[i]);
    }
    self->previousSample = previousSample;
}

void star_sig_OnePole_destroy(struct star_Allocator* allocator,
    struct star_sig_OnePole* self) {
        star_sig_Signal_destroy(allocator, (void*) self);
}


void star_sig_Looper_init(struct star_sig_Looper* self,
    struct star_AudioSettings* settings,
    struct star_sig_Looper_Inputs* inputs,
    float* output) {

    star_sig_Signal_init(self, settings, output,
        *star_sig_Looper_generate);

    self->inputs = inputs;
    self->isBufferEmpty = true;
    self->previousRecord = 0.0f;
    self->playbackPos = 0.0f;

    // TODO: Deal with how buffers get here.
    self->buffer = NULL;
}

struct star_sig_Looper* star_sig_Looper_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Looper_Inputs* inputs) {

    float* output = star_AudioBlock_new(allocator, settings);
    struct star_sig_Looper* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_Looper));
    star_sig_Looper_init(self, settings, inputs, output);

    return self;
}

// TODO:
// * Set the maximum loop end point automatically after the first overdub
// * Reduce clicks by crossfading the end and start of the window.
// * Address glitches when the length is very short
// * Ignore loop duration while recording the first overdub?
// * Adjust speed scaling so regular playback speed is at 0.0
// * Display a visual rendering of the loop points on screen
// * Should we check if the buffer is null and output silence,
//   or should this be considered a user error?
//   (Or should we introduce some kind of validation function for signals?)
void star_sig_Looper_generate(void* signal) {
    struct star_sig_Looper* self = (struct star_sig_Looper*) signal;
    float* samples = self->buffer->samples;
    float playbackPos = self->playbackPos;
    float bufferLastIdx = (float)(self->buffer->length - 1);

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float speed = self->inputs->speed[i];
        float start = star_clamp(self->inputs->start[i], 0.0, 1.0);
        float length = star_clamp(self->inputs->length[i], 0.0, 1.0);
        float startPos = roundf(bufferLastIdx * start);
        float endPos = roundf(startPos + ((bufferLastIdx - startPos) *
            length));

        // If the loop size is smaller than the speed
        // we're playing back at, just output silence.
        if ((endPos - startPos) <= fabsf(speed)) {
            self->signal.output[i] = 0.0f;
            continue;
        }

        if (self->inputs->record[i] > 0.0f) {
            // We're recording.
            if (self->previousRecord <= 0.0f) {
                // We've just started recording.
                if (self->isBufferEmpty) {
                    // This is the first overdub.
                    playbackPos = startPos;
                }
            }

            // Playback has to be at regular speed
            // while recording, so ignore any modulation
            // and only use its direction.
            // TODO: Add support for cool tape-style effects
            // when overdubbing at different speeds.
            // Note: Naively omitting this will work,
            // but introduces lots pitched resampling artifacts.
            speed = speed > 0.0f ? 1.0f : -1.0f;

            // Overdub the current audio input into the loop buffer.
            size_t playbackIdx = (size_t) playbackPos;
            float sample = samples[playbackIdx] + self->inputs->source[i];
            samples[playbackIdx] = sample;

            // No interpolation is needed because we're
            // playing/recording at regular speed.
            self->signal.output[i] = sample;
        } else {
            // We're playing back.
            if (self->previousRecord > 0.0f) {
                self->isBufferEmpty = false;
            }

            if (self->inputs->clear[i] > 0.0f &&
                self->previousClear == 0.0f) {
                // TODO: The cost of erasing the entire buffer
                // while in the midst of generating one sample
                // seems very high and may limit buffer sizes.
                // But an alternative implementation needs to take
                // into account the samples outside the current
                // window.
                star_Buffer_fillWithSilence(self->buffer);
                self->isBufferEmpty = true;
            }

            // TODO: The star_interpolate_linear implementation
            // may wrap around inappropriately to the beginning of
            // the buffer (not to the startPos) if we're right at
            // the end of the buffer.
            self->signal.output[i] = star_interpolate_linear(
                playbackPos, samples, self->buffer->length) +
                self->inputs->source[i];
        }

        playbackPos += speed;
        if (playbackPos > endPos) {
            playbackPos = startPos + (playbackPos - endPos);
        } else if (playbackPos < startPos) {
            playbackPos = endPos - (startPos - playbackPos);
        }

        self->previousRecord = self->inputs->record[i];
        self->previousClear = self->inputs->clear[i];
    }

    self->playbackPos = playbackPos;
}

void star_sig_Looper_destroy(struct star_Allocator* allocator,
    struct star_sig_Looper* self) {
    star_sig_Signal_destroy(allocator, self);
}
