#include <math.h>   // For powf, fmodf, sinf, roundf, fabsf, rand
#include <stdlib.h> // For RAND_MAX
#include <tlsf.h>   // Includes assert.h, limits.h, stddef.h
                    // stdio.h, stdlib.h, string.h (for errors etc.)
#include <libsignaletic.h>

float sig_fminf(float a, float b) {
    float r;
#ifdef __arm__
    asm("vminnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a < b) ? a : b;
#endif // __arm__
    return r;
}

float sig_fmaxf(float a, float b) {
    float r;
#ifdef __arm__
    asm("vmaxnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a > b) ? a : b;
#endif // __arm__
    return r;
}

// TODO: Unit tests
float sig_clamp(float value, float min, float max) {
    return sig_fminf(sig_fmaxf(value, min), max);
}

// TODO: Replace this with an object that implements
// the quick and dirty LCR method from Numerical Recipes:
//     unsigned long jran = seed,
//                   ia = 4096,
//                   ic = 150889,
//                   im = 714025;
//     jran=(jran*ia+ic) % im;
//     float ran=(float) jran / (float) im;
float sig_randf() {
    return (float) ((double) rand() / ((double) RAND_MAX + 1));
}

uint16_t sig_unipolarToUint12(float sample) {
    return (uint16_t) (sample * 4095.0f);
}

uint16_t sig_bipolarToUint12(float sample) {
    float normalized = sample * 0.5 + 0.5;
    return (uint16_t) (normalized * 4095.0f);
}

uint16_t sig_bipolarToInvUint12(float sample) {
    return sig_bipolarToUint12(-sample);
}

float sig_midiToFreq(float midiNum) {
    return powf(2, (midiNum - 69.0f) / 12.0f) * 440.0f;
}

float sig_randomFill(size_t i, float_array_ptr array) {
    return sig_randf();
}

void sig_fill(float_array_ptr array, size_t length,
    sig_array_filler filler) {
    for (size_t i = 0; i < length; i++) {
        FLOAT_ARRAY(array)[i] = filler(i, array);
    }
}

void sig_fillWithValue(float_array_ptr array, size_t size,
    float value) {
    for (size_t i = 0; i < size; i++) {
        FLOAT_ARRAY(array)[i] = value;
    }
}

void sig_fillWithSilence(float_array_ptr array, size_t size) {
    sig_fillWithValue(array, size, 0.0f);
}

// TODO: Unit tests.
float sig_interpolate_linear(float idx, float_array_ptr table,
    size_t length) {
    int32_t idxIntegral = (int32_t) idx;
    float idxFractional = idx - (float) idxIntegral;
    float a = FLOAT_ARRAY(table)[idxIntegral];
    // TODO: Do we want to wrap around the end like this,
    // or should we expect users to provide us with idx within bounds?
    float b = FLOAT_ARRAY(table)[(idxIntegral + 1) % length];

    return a + (b - a) * idxFractional;
}

// TODO: Unit tests.
float sig_interpolate_cubic(float idx, float_array_ptr table,
    size_t length) {
    size_t idxIntegral = (size_t) idx;
    float idxFractional = idx - (float) idxIntegral;

    // TODO: As above, are these modulo operations required,
    // or should we expect users to provide us in-bound values?
    const size_t i0 = idxIntegral % length;
    const float xm1 = FLOAT_ARRAY(table)[i0 > 0 ? i0 - 1 : length - 1];
    const float x0 = FLOAT_ARRAY(table)[i0];
    const float x1 = FLOAT_ARRAY(table)[(i0 + 1) % length];
    const float x2 = FLOAT_ARRAY(table)[(i0 + 2) % length];
    const float c = (x1 - xm1) * 0.5f;
    const float v = x0 - x1;
    const float w = c + v;
    const float a = w + v + (x2 - x0) * 0.5f;
    const float bNeg = w + a;

    return (((a * idxFractional) - bNeg) * idxFractional + c) *
        idxFractional + x0;
}

// TODO: Unit tests.
float sig_filter_onepole(float current, float previous, float coeff) {
    return current + coeff * (previous - current);
}

// TODO: Unit tests.
float sig_waveform_sine(float phase) {
    return sinf(phase);
}

// TODO: Unit tests.
float sig_waveform_square(float phase) {
    return phase <= sig_PI ? 1.0f : -1.0f;
}

// TODO: Unit tests.
float sig_waveform_saw(float phase) {
    return (2.0f * (phase * (1.0f / sig_TWOPI))) - 1.0f;
}

// TODO: Unit tests.
float sig_waveform_reverseSaw(float phase) {
    return 1.0f - 2.0f * (phase * (1.0f / sig_TWOPI));
}

// TODO: Unit tests.
float sig_waveform_triangle(float phase) {
    float val = sig_waveform_saw(phase);
    if (val < 0.0) {
        val = -val;
    }

    return 2.0f * (val - 0.5f);
}

// TODO: Implement enough test coverage for sig_Allocator
// to support a switch from TLSF to another memory allocator
// implementation sometime in the future (gh-26).
void sig_TLSFAllocator_init(struct sig_AllocatorHeap* heap) {
    tlsf_create_with_pool(heap->memory, heap->length);
}

void* sig_TLSFAllocator_malloc(struct sig_AllocatorHeap* heap, size_t size) {
    return tlsf_malloc(heap->memory, size);
}

void sig_TLSFAllocator_free(struct sig_AllocatorHeap* heap,
    void* obj) {
    tlsf_free(heap->memory, obj);
}


struct sig_AudioSettings* sig_AudioSettings_new(
    struct sig_Allocator* allocator) {

    struct sig_AudioSettings* settings =
        (struct sig_AudioSettings*)allocator->impl->malloc(
            allocator->heap, sizeof(struct sig_AudioSettings));

    settings->sampleRate = sig_DEFAULT_AUDIOSETTINGS.sampleRate;
    settings->numChannels = sig_DEFAULT_AUDIOSETTINGS.numChannels;
    settings->blockSize = sig_DEFAULT_AUDIOSETTINGS.blockSize;

    return settings;
}

void sig_AudioSettings_destroy(struct sig_Allocator* allocator,
    struct sig_AudioSettings* self) {
    allocator->impl->free(allocator->heap, self);
}

// TODO: Unit tests.
size_t sig_secondsToSamples(struct sig_AudioSettings* audioSettings,
    float duration) {
    float numSamplesF = audioSettings->sampleRate * duration;
    long rounded = lroundf(numSamplesF);
    return (size_t) labs(rounded);
}


float_array_ptr sig_samples_new(struct sig_Allocator* allocator,
    size_t length) {
    return (float_array_ptr) allocator->impl->malloc(allocator->heap,
        sizeof(float) * length);
}

// TODO: Does an AudioBlock type need to be introduced?
// TODO: Do we need a destroy function too?
float_array_ptr sig_AudioBlock_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings) {
    return sig_samples_new(allocator, audioSettings->blockSize);
}

float_array_ptr sig_AudioBlock_newWithValue(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    float value) {
    float_array_ptr block = sig_AudioBlock_new(allocator,
        audioSettings);
    sig_fillWithValue(block, audioSettings->blockSize, value);

    return block;
}

struct sig_Buffer* sig_Buffer_new(struct sig_Allocator* allocator,
    size_t length) {
    struct sig_Buffer* self = (struct sig_Buffer*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_Buffer));
    self->length = length;
    self->samples = sig_samples_new(allocator, length);

    return self;
}

void sig_Buffer_fill(struct sig_Buffer* self,
    sig_array_filler filler) {
    sig_fill(self->samples, self->length, filler);
}

void sig_Buffer_fillWithValue(struct sig_Buffer* self, float value) {
    sig_fillWithValue(self->samples, self->length, value);
}

void sig_Buffer_fillWithSilence(struct sig_Buffer* self) {
    sig_fillWithSilence(self->samples, self->length);
}

// TODO: Unit tests.
void sig_Buffer_fillWithWaveform(struct sig_Buffer* self,
    sig_waveform_generator generate, float sampleRate,
    float phase, float freq) {
    float phaseInc = freq * sig_TWOPI / sampleRate;
    for (size_t i = 0; i < self->length; i++) {
        FLOAT_ARRAY(self->samples)[i] = generate(phase);
        phase += phaseInc;
        if (phase >= sig_TWOPI) {
            phase -= sig_TWOPI;
        } else if (phase < 0.0) {
            phase += sig_TWOPI;
        }
    }
}

void sig_Buffer_destroy(struct sig_Allocator* allocator, struct sig_Buffer* self) {
    struct sig_AllocatorHeap* heap = allocator->heap;
    allocator->impl->free(heap, self->samples);
    allocator->impl->free(heap, self);
};


struct sig_Buffer* sig_BufferView_new(
    struct sig_Allocator* allocator,
    struct sig_Buffer* buffer, size_t startIdx, size_t length) {
    struct sig_Buffer* self = (struct sig_Buffer*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_Buffer));

    // TODO: Need to signal an error rather than
    // just returning a null pointer and a length of zero.
    if (startIdx < 0 || length > (buffer->length - startIdx)) {
        self->samples = NULL;
        self->length = 0;
    } else {
        self->samples = FLOAT_ARRAY(buffer->samples) + startIdx;
        self->length = length;
    }

    return self;
}

void sig_BufferView_destroy(struct sig_Allocator* allocator,
    struct sig_Buffer* self) {
    // Don't destroy the samples array;
    // it is shared with other Buffers.
    allocator->impl->free(allocator->heap, self);
}


void sig_dsp_Signal_init(void* signal,
    struct sig_AudioSettings* settings,
    float_array_ptr output,
    sig_dsp_generateFn generate) {
    struct sig_dsp_Signal* self = (struct sig_dsp_Signal*) signal;

    self->audioSettings = settings;
    self->output = output;
    self->generate = generate;
};

/**
 * Generic generation function
 * that operates on any Signal and outputs only silence.
 */
void sig_dsp_Signal_generate(void* signal) {
    struct sig_dsp_Signal* self = (struct sig_dsp_Signal*) signal;
    sig_fillWithSilence(self->output, self->audioSettings->blockSize);
}

void sig_dsp_Signal_destroy(struct sig_Allocator* allocator,
    void* self) {
    struct sig_AllocatorHeap* heap = allocator->heap;
    struct sig_dsp_Signal* signal = (struct sig_dsp_Signal*) self;
    allocator->impl->free(heap, signal->output);
    allocator->impl->free(heap, signal);
}

void sig_dsp_Value_init(struct sig_dsp_Value* self,
    struct sig_AudioSettings *settings,
    float_array_ptr output) {

    struct sig_dsp_Value_Parameters params = {
        .value = 1.0
    };

    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Value_generate);

    self->parameters = params;
}

struct sig_dsp_Value* sig_dsp_Value_new(struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_Value* self = (struct sig_dsp_Value*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_Value));
    sig_dsp_Value_init(self, settings, output);

    return self;
}

void sig_dsp_Value_destroy(struct sig_Allocator* allocator, struct sig_dsp_Value* self) {
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

void sig_dsp_Value_generate(void* signal) {
    struct sig_dsp_Value* self = (struct sig_dsp_Value*) signal;

    if (self->parameters.value == self->lastSample) {
        return;
    }

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->signal.output)[i] = self->parameters.value;
    }

    self->lastSample = self->parameters.value;
}


struct sig_dsp_BinaryOp* sig_dsp_Add_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_BinaryOp));
    sig_dsp_Add_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_Add_init(struct sig_dsp_BinaryOp* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Add_generate);
    self->inputs = inputs;
}

// TODO: Unit tests.
void sig_dsp_Add_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs->left)[i];
        float right = FLOAT_ARRAY(self->inputs->right)[i];
        float val = left + right;

        FLOAT_ARRAY(self->signal.output)[i] = val;
    }
}

void sig_dsp_Add_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_Mul_init(struct sig_dsp_BinaryOp* self,
    struct sig_AudioSettings* settings, struct sig_dsp_BinaryOp_Inputs* inputs, float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output, *sig_dsp_Mul_generate);
    self->inputs = inputs;
};

struct sig_dsp_BinaryOp* sig_dsp_Mul_new(struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_BinaryOp));
    sig_dsp_Mul_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_Mul_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_Signal_destroy(allocator, (void*) self);
};

void sig_dsp_Mul_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs->left)[i];
        float right = FLOAT_ARRAY(self->inputs->right)[i];
        float val = left * right;
        FLOAT_ARRAY(self->signal.output)[i] = val;
    }
}


struct sig_dsp_BinaryOp* sig_dsp_Div_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_BinaryOp));
    sig_dsp_Div_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_Div_init(struct sig_dsp_BinaryOp* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Div_generate);
    self->inputs = inputs;
}

void sig_dsp_Div_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs->left)[i];
        float right = FLOAT_ARRAY(self->inputs->right)[i];
        float val = left / right;
        FLOAT_ARRAY(self->signal.output)[i] = val;
    }
}

void sig_dsp_Div_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_Invert* sig_dsp_Invert_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Invert_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_Invert* self = (struct sig_dsp_Invert*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_Invert));
    sig_dsp_Invert_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_Invert_init(struct sig_dsp_Invert* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Invert_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Invert_generate);
    self->inputs = inputs;
}

// TODO: Unit tests.
void sig_dsp_Invert_generate(void* signal) {
    struct sig_dsp_Invert* self = (struct sig_dsp_Invert*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float inSamp = FLOAT_ARRAY(self->inputs->source)[i];
        FLOAT_ARRAY(self->signal.output)[i] = -inSamp;
    }
}

void sig_dsp_Invert_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Invert* self) {
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_Accumulate* sig_dsp_Accumulate_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Accumulate_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_Accumulate* self = (struct sig_dsp_Accumulate*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_Accumulate));
    sig_dsp_Accumulate_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_Accumulate_init(
    struct sig_dsp_Accumulate* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Accumulate_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Accumulate_generate);

    struct sig_dsp_Accumulate_Parameters parameters = {
        .accumulatorStart = 1.0
    };

    self->inputs = inputs;
    self->parameters = parameters;
    self->accumulator = parameters.accumulatorStart;
    self->previousReset = 0.0f;
}

// TODO: Implement an audio rate version of this signal.
// TODO: Unit tests
void sig_dsp_Accumulate_generate(void* signal) {
    struct sig_dsp_Accumulate* self =
        (struct sig_dsp_Accumulate*) signal;

    float reset = FLOAT_ARRAY(self->inputs->reset)[0];
    if (reset > 0.0f && self->previousReset <= 0.0f) {
        // Reset the accumulator if we received a trigger.
        self->accumulator = self->parameters.accumulatorStart;
    }

    self->accumulator += FLOAT_ARRAY(self->inputs->source)[0];

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->signal.output)[i] = self->accumulator;
    }

    self->previousReset = reset;
}

void sig_dsp_Accumulate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Accumulate* self) {
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_dsp_GatedTimer* sig_dsp_GatedTimer_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_GatedTimer_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_GatedTimer* self = (struct sig_dsp_GatedTimer*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_GatedTimer));
    sig_dsp_GatedTimer_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_GatedTimer_init(struct sig_dsp_GatedTimer* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_GatedTimer_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_GatedTimer_generate);
    self->inputs = inputs;
    self->timer = 0;
    self->hasFired = false;
    self->prevGate = 0.0f;
}

// TODO: Unit tests
void sig_dsp_GatedTimer_generate(void* signal) {
    struct sig_dsp_GatedTimer* self =
        (struct sig_dsp_GatedTimer*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        // TODO: MSVC compiler warning loss of precision.
        unsigned long durationSamps = (unsigned long)
            FLOAT_ARRAY(self->inputs->duration)[i] *
            self->signal.audioSettings->sampleRate;
        float gate = FLOAT_ARRAY(self->inputs->gate)[i];

        if (gate > 0.0f) {
            // Gate is open.
            if (!self->hasFired ||
                FLOAT_ARRAY(self->inputs->loop)[i] > 0.0f) {
                self->timer++;
            }

            if (self->timer >= durationSamps) {
                // We reached the duration time.
                FLOAT_ARRAY(self->signal.output)[i] = 1.0f;

                // Reset the timer counter and note
                // that we've already fired while
                // this gate was open.
                self->timer = 0;
                self->hasFired = true;

                continue;
            }
        } else if (gate <= 0.0f && self->prevGate > 0.0f) {
            // Gate just closed. Reset all timer state.
            self->timer = 0;
            self->hasFired = false;
        }

        FLOAT_ARRAY(self->signal.output)[i] = 0.0f;
        self->prevGate = gate;
    }
}

void sig_dsp_GatedTimer_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_GatedTimer* self) {
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

struct sig_dsp_TimedTriggerCounter* sig_dsp_TimedTriggerCounter_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_TimedTriggerCounter_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_TimedTriggerCounter* self =
        (struct sig_dsp_TimedTriggerCounter*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_TimedTriggerCounter));
    sig_dsp_TimedTriggerCounter_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_TimedTriggerCounter_init(
    struct sig_dsp_TimedTriggerCounter* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_TimedTriggerCounter_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_TimedTriggerCounter_generate);

    self->inputs = inputs;
    self->numTriggers = 0;
    self->timer = 0;
    self->isTimerActive = false;
    self->previousSource = 0.0f;
}

void sig_dsp_TimedTriggerCounter_generate(void* signal) {
    struct sig_dsp_TimedTriggerCounter* self =
        (struct sig_dsp_TimedTriggerCounter*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs->source)[i];
        float outputSample = 0.0f;

        if (source > 0.0f && self->previousSource == 0.0f) {
            // Received the rising edge of a trigger.
            if (!self->isTimerActive) {
                // It's the first trigger,
                // so start the timer.
                self->isTimerActive = true;
            }
        }

        if (self->isTimerActive) {
            // The timer is running.
            if (source <= 0.0f && self->previousSource > 0.0f) {
                // Received the falling edge of a trigger,
                // so count it.
                self->numTriggers++;
            }

            self->timer++;

            // Truncate the duration to the nearest sample.
            long durSamps = (long) (FLOAT_ARRAY(
                self->inputs->duration)[i] *
                self->signal.audioSettings->sampleRate);

            if (self->timer >= durSamps) {
                // Time's up.
                // Fire a trigger if we've the right number of
                // incoming triggers, otherwise just reset.
                if (self->numTriggers ==
                    (int) FLOAT_ARRAY(self->inputs->count)[i]) {
                    outputSample = 1.0f;
                }

                self->isTimerActive = false;
                self->numTriggers = 0;
                self->timer = 0;
            }
        }

        self->previousSource = source;
        FLOAT_ARRAY(self->signal.output)[i] = outputSample;
    }
}

void sig_dsp_TimedTriggerCounter_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_TimedTriggerCounter* self) {
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_dsp_ToggleGate* sig_dsp_ToggleGate_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_ToggleGate_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_ToggleGate* self = (struct sig_dsp_ToggleGate*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_ToggleGate));
    sig_dsp_ToggleGate_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_ToggleGate_init(
    struct sig_dsp_ToggleGate* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_ToggleGate_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_ToggleGate_generate);
    self->inputs = inputs;
    self->isGateOpen = false;
    self->prevTrig = 0.0f;
}

// TODO: Unit tests
void sig_dsp_ToggleGate_generate(void* signal) {
    struct sig_dsp_ToggleGate* self =
        (struct sig_dsp_ToggleGate*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float trigger = FLOAT_ARRAY(self->inputs->trigger)[i];
        if (trigger > 0.0f && self->prevTrig <= 0.0f) {
            // Received a trigger, toggle the gate.
            self->isGateOpen = !self->isGateOpen;
        }

        FLOAT_ARRAY(self->signal.output)[i] = (float) self->isGateOpen;

        self->prevTrig = trigger;
    }
}

void sig_dsp_ToggleGate_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_ToggleGate* self) {
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


void sig_dsp_Sine_init(struct sig_dsp_Sine* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Sine_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Sine_generate);

    self->inputs = inputs;
    self->phaseAccumulator = 0.0f;
}

struct sig_dsp_Sine* sig_dsp_Sine_new(struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Sine_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_Sine* self = (struct sig_dsp_Sine*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_Sine));
    sig_dsp_Sine_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_Sine_destroy(struct sig_Allocator* allocator, struct sig_dsp_Sine* self) {
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

void sig_dsp_Sine_generate(void* signal) {
    struct sig_dsp_Sine* self = (struct sig_dsp_Sine*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float modulatedPhase = fmodf(self->phaseAccumulator +
            FLOAT_ARRAY(self->inputs->phaseOffset)[i], sig_TWOPI);

        FLOAT_ARRAY(self->signal.output)[i] = sinf(modulatedPhase) *
            FLOAT_ARRAY(self->inputs->mul)[i] +
            FLOAT_ARRAY(self->inputs->add)[i];

        float phaseStep = FLOAT_ARRAY(self->inputs->freq)[i] /
            self->signal.audioSettings->sampleRate * sig_TWOPI;

        self->phaseAccumulator += phaseStep;
        if (self->phaseAccumulator > sig_TWOPI) {
            self->phaseAccumulator -= sig_TWOPI;
        }
    }
}


void sig_dsp_OnePole_init(struct sig_dsp_OnePole* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_OnePole_Inputs* inputs,
    float_array_ptr output) {

    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_OnePole_generate);
    self->inputs = inputs;
    self->previousSample = 0.0f;
}

struct sig_dsp_OnePole* sig_dsp_OnePole_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_OnePole_Inputs* inputs) {

    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_OnePole* self = (struct sig_dsp_OnePole*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_OnePole));
    sig_dsp_OnePole_init(self, settings, inputs, output);

    return self;
}

// TODO: Unit tests
void sig_dsp_OnePole_generate(void* signal) {
    struct sig_dsp_OnePole* self = (struct sig_dsp_OnePole*) signal;

    float previousSample = self->previousSample;
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->signal.output)[i] = previousSample =
            sig_filter_onepole(
                FLOAT_ARRAY(self->inputs->source)[i], previousSample,
                FLOAT_ARRAY(self->inputs->coefficient)[i]);
    }
    self->previousSample = previousSample;
}

void sig_dsp_OnePole_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_OnePole* self) {
        sig_dsp_Signal_destroy(allocator, (void*) self);
}


void sig_dsp_Tanh_init(struct sig_dsp_Tanh* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Tanh_Inputs* inputs,
    float_array_ptr output) {

    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Tanh_generate);
    self->inputs = inputs;
}

struct sig_dsp_Tanh* sig_dsp_Tanh_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Tanh_Inputs* inputs) {

    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_Tanh* self = (struct sig_dsp_Tanh*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_Tanh));
    sig_dsp_Tanh_init(self, settings, inputs, output);

    return self;
}

// TODO: Unit tests.
void sig_dsp_Tanh_generate(void* signal) {
    struct sig_dsp_Tanh* self = (struct sig_dsp_Tanh*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float inSamp = FLOAT_ARRAY(self->inputs->source)[i];
        float outSamp = tanhf(inSamp);
        FLOAT_ARRAY(self->signal.output)[i] = outSamp;
    }
}

void sig_dsp_Tanh_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Tanh* self) {
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_Looper_init(struct sig_dsp_Looper* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Looper_Inputs* inputs,
    float_array_ptr output) {

    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Looper_generate);

    self->inputs = inputs;
    self->isBufferEmpty = true;
    self->previousRecord = 0.0f;
    self->playbackPos = 0.0f;

    // TODO: Deal with how buffers get here.
    self->buffer = NULL;
}

struct sig_dsp_Looper* sig_dsp_Looper_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Looper_Inputs* inputs) {

    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_Looper* self = (struct sig_dsp_Looper*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_Looper));
    sig_dsp_Looper_init(self, settings, inputs, output);

    return self;
}

// TODO:
// * Reduce clicks by crossfading the end and start of the window.
//      - should it be a true cross fade, requiring a few samples
//        on each end of the clip, or a very quick fade in/out
//        (e.g. 1-10ms/48-480 samples)?
// * Fade out before clearing. A whole loop's duration, or shorter?
// * Address glitches when the length is very short
// * Should we check if the buffer is null and output silence,
//   or should this be considered a user error?
//   (Or should we introduce some kind of validation function for signals?)
// * Unit tests
void sig_dsp_Looper_generate(void* signal) {
    struct sig_dsp_Looper* self = (struct sig_dsp_Looper*) signal;
    float* samples = FLOAT_ARRAY(self->buffer->samples);
    float playbackPos = self->playbackPos;
    float bufferLastIdx = (float)(self->buffer->length - 1);

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float speed = FLOAT_ARRAY(self->inputs->speed)[i];
        float start = sig_clamp(FLOAT_ARRAY(self->inputs->start)[i],
            0.0, 1.0);
        float end = sig_clamp(FLOAT_ARRAY(self->inputs->end)[i],
            0.0, 1.0);

        // Flip the start and end points if they're reversed.
        if (start > end) {
            float temp = start;
            start = end;
            end = temp;
        }

        float startPos = roundf(bufferLastIdx * start);
        float endPos = roundf(bufferLastIdx * end);

        // If the loop size is smaller than the speed
        // we're playing back at, just output silence.
        if ((endPos - startPos) <= fabsf(speed)) {
            FLOAT_ARRAY(self->signal.output)[i] = 0.0f;
            continue;
        }

        if (FLOAT_ARRAY(self->inputs->record)[i] > 0.0f) {
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
            float sample = samples[playbackIdx] +
                FLOAT_ARRAY(self->inputs->source)[i];

            // Add a little distortion/limiting.
            sample = tanhf(sample);
            samples[playbackIdx] = sample;

            // No interpolation is needed because we're
            // playing/recording at regular speed.
            FLOAT_ARRAY(self->signal.output)[i] = sample;
        } else {
            // We're playing back.
            if (self->previousRecord > 0.0f) {
                // We just finished recording.
                self->isBufferEmpty = false;
            }

            if (FLOAT_ARRAY(self->inputs->clear)[i] > 0.0f &&
                self->previousClear == 0.0f) {
                // TODO: Fade out before clearing the buffer
                // (gh-28)
                sig_Buffer_fillWithSilence(self->buffer);
                self->isBufferEmpty = true;
            }

            // TODO: The sig_interpolate_linear implementation
            // may wrap around inappropriately to the beginning of
            // the buffer (not to the startPos) if we're right at
            // the end of the buffer.
            FLOAT_ARRAY(self->signal.output)[i] = sig_interpolate_linear(
                playbackPos, samples, self->buffer->length) +
                FLOAT_ARRAY(self->inputs->source)[i];
        }

        playbackPos += speed;
        if (playbackPos > endPos) {
            playbackPos = startPos + (playbackPos - endPos);
        } else if (playbackPos < startPos) {
            playbackPos = endPos - (startPos - playbackPos);
        }

        self->previousRecord = FLOAT_ARRAY(self->inputs->record)[i];
        self->previousClear = FLOAT_ARRAY(self->inputs->clear)[i];
    }

    self->playbackPos = playbackPos;
}

void sig_dsp_Looper_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Looper* self) {
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_Dust_init(struct sig_dsp_Dust* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Dust_Inputs* inputs,
    float_array_ptr output) {

    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Dust_generate);

    struct sig_dsp_Dust_Parameters parameters = {
        .bipolar = 0.0
    };

    self->inputs = inputs;
    self->parameters = parameters;
    self->sampleDuration = 1.0 / settings->sampleRate;
    self->previousDensity = 0.0;
    self->threshold = 0.0;
}

struct sig_dsp_Dust* sig_dsp_Dust_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Dust_Inputs* inputs) {

    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_Dust* self = (struct sig_dsp_Dust*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_Dust));
    sig_dsp_Dust_init(self, settings, inputs, output);

    return self;
}

void sig_dsp_Dust_generate(void* signal) {
    struct sig_dsp_Dust* self = (struct sig_dsp_Dust*) signal;

    float scaleDiv = self->parameters.bipolar > 0.0f ? 2.0f : 1.0f;
    float scaleSub = self->parameters.bipolar > 0.0f ? 1.0f : 0.0f;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float density = FLOAT_ARRAY(self->inputs->density)[i];

        if (density != self->previousDensity) {
            self->previousDensity = density;
            self->threshold = density * self->sampleDuration;
            self->scale = self->threshold > 0.0f ?
                scaleDiv / self->threshold : 0.0f;
        }

        float rand = sig_randf();
        float val = rand < self->threshold ?
            rand * self->scale - scaleSub : 0.0f;
        FLOAT_ARRAY(self->signal.output)[i] = val;
    }
}

void sig_dsp_Dust_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Dust* self) {
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_TimedGate_init(struct sig_dsp_TimedGate* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_TimedGate_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_TimedGate_generate);

    struct sig_dsp_TimedGate_Parameters parameters = {
        .resetOnTrigger = 0.0,
        .bipolar = 0.0
    };

    self->inputs = inputs;
    self->parameters = parameters;
    self->previousTrigger = 0.0f;
    self->previousDuration = 0.0f;
    self->gateValue = 0.0f;
    self->durationSamps = 0;
    self->samplesRemaining = 0;
}

struct sig_dsp_TimedGate* sig_dsp_TimedGate_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_TimedGate_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_TimedGate* self = (struct sig_dsp_TimedGate*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_TimedGate));
    sig_dsp_TimedGate_init(self, settings, inputs, output);

    return self;
}

static inline void sig_dsp_TimedGate_outputHigh(struct sig_dsp_TimedGate* self,
    size_t index) {
    FLOAT_ARRAY(self->signal.output)[index] = self->gateValue;
    self->samplesRemaining--;
}

static inline void sig_dsp_TimedGate_outputLow(struct sig_dsp_TimedGate* self,
    size_t index) {
    FLOAT_ARRAY(self->signal.output)[index] = 0.0f;
}

void sig_dsp_TimedGate_generate(void* signal) {
    struct sig_dsp_TimedGate* self = (struct sig_dsp_TimedGate*)
        signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float currentTrigger = FLOAT_ARRAY(self->inputs->trigger)[i];
        float duration = FLOAT_ARRAY(self->inputs->duration)[i];

        if ((currentTrigger > 0.0f && self->previousTrigger <= 0.0f) ||
            (self->parameters.bipolar > 0.0 && currentTrigger < 0.0f && self->previousTrigger >= 0.0f)) {
            // A new trigger was received.
            self->gateValue = currentTrigger;

            if (duration != self->previousDuration) {
                // The duration input has changed.
                self->durationSamps = lroundf(duration *
                    self->signal.audioSettings->sampleRate);
                self->previousDuration = duration;
            }

            if (self->parameters.resetOnTrigger > 0.0f &&
                self->samplesRemaining > 0) {
                // Gate is open and needs to be reset.
                // Close the gate for one sample,
                // and don't count down the duration
                // until next time.
                sig_dsp_TimedGate_outputLow(self, i);
                self->samplesRemaining = self->durationSamps;
            } else {
                self->samplesRemaining = self->durationSamps;
                sig_dsp_TimedGate_outputHigh(self, i);
            }
        } else if (self->samplesRemaining > 0) {
            sig_dsp_TimedGate_outputHigh(self, i);
        } else {
            sig_dsp_TimedGate_outputLow(self, i);
        }

        self->previousTrigger = currentTrigger;
    }
}

void sig_dsp_TimedGate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_TimedGate* self) {
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_ClockFreqDetector_init(
    struct sig_dsp_ClockFreqDetector* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_ClockFreqDetector_Inputs* inputs,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_ClockFreqDetector_generate);

    struct sig_dsp_ClockFreqDetector_Parameters params = {
        .threshold = 0.1f,
        .timeoutDuration = 120.0f
    };

    self->inputs = inputs;
    self->parameters = params;
    self->previousTrigger = 0.0f;
    self->samplesSinceLastPulse = 0;
    self->clockFreq = 0.0f;
    self->pulseDurSamples = 0;
}

struct sig_dsp_ClockFreqDetector* sig_dsp_ClockFreqDetector_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_ClockFreqDetector_Inputs* inputs) {

    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_ClockFreqDetector* self =
        (struct sig_dsp_ClockFreqDetector*)
        allocator->impl->malloc(allocator->heap,
            sizeof(struct sig_dsp_ClockFreqDetector));
    sig_dsp_ClockFreqDetector_init(self, settings, inputs, output);

    return self;
}

static inline float sig_dsp_ClockFreqDetector_calcClockFreq(
    float sampleRate, uint32_t samplesSinceLastPulse,
    float prevFreq) {
    float freq = sampleRate / (float) samplesSinceLastPulse;
    // TODO: Is an LPF good, or is a moving average better?
    return sig_filter_onepole(freq, prevFreq, 0.01f);
}

void sig_dsp_ClockFreqDetector_generate(void* signal) {
    struct sig_dsp_ClockFreqDetector* self =
        (struct sig_dsp_ClockFreqDetector*) signal;
    float_array_ptr source = self->inputs->source;
    float_array_ptr output = self->signal.output;

    float previousTrigger = self->previousTrigger;
    float clockFreq = self->clockFreq;
    bool isRisingEdge = self->isRisingEdge;
    uint32_t samplesSinceLastPulse = self->samplesSinceLastPulse;
    float sampleRate = self->signal.audioSettings->sampleRate;
    float threshold = self->parameters.threshold;
    float timeoutDuration = self->parameters.timeoutDuration;
    uint32_t pulseDurSamples = self->pulseDurSamples;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        samplesSinceLastPulse++;

        float sourceSamp = FLOAT_ARRAY(source)[i];
        if (sourceSamp > 0.0f && previousTrigger <= 0.0f) {
            // Start of rising edge.
            isRisingEdge = true;
        } else if (sourceSamp < previousTrigger) {
            // Failed to reach the threshold before
            // the signal fell again.
            isRisingEdge = false;
        }

        if (isRisingEdge && sourceSamp >= threshold) {
            // Signal is rising and threshold has been reached,
            // so this is a pulse.
            clockFreq = sig_dsp_ClockFreqDetector_calcClockFreq(
                sampleRate, samplesSinceLastPulse, clockFreq);
            pulseDurSamples = samplesSinceLastPulse;
            samplesSinceLastPulse = 0;
            isRisingEdge = false;
        } else if (samplesSinceLastPulse > sampleRate * timeoutDuration) {
            // It's been too long since we've received a pulse.
            clockFreq = 0.0f;
        } else if (samplesSinceLastPulse > pulseDurSamples) {
            // Tempo is slowing down; recalculate it.
            clockFreq = sig_dsp_ClockFreqDetector_calcClockFreq(
                sampleRate, samplesSinceLastPulse, clockFreq);
        }

        FLOAT_ARRAY(output)[i] = clockFreq;
        previousTrigger = sourceSamp;
    }

    self->previousTrigger = previousTrigger;
    self->clockFreq = clockFreq;
    self->isRisingEdge = isRisingEdge;
    self->samplesSinceLastPulse = samplesSinceLastPulse;
    self->pulseDurSamples = pulseDurSamples;
}

void sig_dsp_ClockFreqDetector_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ClockFreqDetector* self) {
    sig_dsp_Signal_destroy(allocator, self);
}
