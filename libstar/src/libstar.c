#include <math.h>   // For powf, fmodf, sinf, roundf, fabsf, rand
#include <stdlib.h> // For RAND_MAX
#include <tlsf.h>   // Includes assert.h, limits.h, stddef.h
                    // stdio.h, stdlib.h, string.h (for errors etc.)
#include <libstar.h>

float star_fminf(float a, float b) {
    float r;
#ifdef __arm__
    asm("vminnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a < b) ? a : b;
#endif // __arm__
    return r;
}

float star_fmaxf(float a, float b) {
    float r;
#ifdef __arm__
    asm("vmaxnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a > b) ? a : b;
#endif // __arm__
    return r;
}

// TODO: Unit tests
float star_clamp(float value, float min, float max) {
    return star_fminf(star_fmaxf(value, min), max);
}

// TODO: Replace this with an object that implements
// the quick and dirty LCR method from Numerical Recipes:
//     unsigned long jran = seed,
//                   ia = 4096,
//                   ic = 150889,
//                   im = 714025;
//     jran=(jran*ia+ic) % im;
//     float ran=(float) jran / (float) im;
float star_randf() {
    return (float) ((double) rand() / ((double) RAND_MAX + 1));
}

uint16_t star_unipolarToUint12(float sample) {
    return (uint16_t) (sample * 4095.0f);
}

uint16_t star_bipolarToUint12(float sample) {
    float normalized = sample * 0.5 + 0.5;
    return (uint16_t) (normalized * 4095.0f);
}

uint16_t star_bipolarToInvUint12(float sample) {
    return star_bipolarToUint12(-sample);
}

float star_midiToFreq(float midiNum) {
    return powf(2, (midiNum - 69.0f) / 12.0f) * 440.0f;
}

void star_fillWithValue(float_array_ptr buffer, size_t size,
    float value) {
    for (size_t i = 0; i < size; i++) {
        FLOAT_ARRAY(buffer)[i] = value;
    }
}

void star_fillWithSilence(float_array_ptr buffer, size_t size) {
    star_fillWithValue(buffer, size, 0.0f);
}

// TODO: Unit tests.
float star_interpolate_linear(float idx, float_array_ptr table,
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
float star_interpolate_cubic(float idx, float_array_ptr table,
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
float star_filter_onepole(float current, float previous, float coeff) {
    return current + coeff * (previous - current);
}

// TODO: Unit tests.
float star_waveform_sine(float phase) {
    return sinf(phase);
}

// TODO: Unit tests.
float star_waveform_square(float phase) {
    return phase <= star_PI ? 1.0f : -1.0f;
}

// TODO: Unit tests.
float star_waveform_saw(float phase) {
    return (2.0f * (phase * (1.0f / star_TWOPI))) - 1.0f;
}

// TODO: Unit tests.
float star_waveform_reverseSaw(float phase) {
    return 1.0f - 2.0f * (phase * (1.0f / star_TWOPI));
}

// TODO: Unit tests.
float star_waveform_triangle(float phase) {
    float val = star_waveform_saw(phase);
    if (val < 0.0) {
        val = -val;
    }

    return 2.0f * (val - 0.5f);
}

// TODO: Implement enough test coverage for star_Allocator
// to support a switch from TLSF to another memory allocator
// implementation sometime in the future (gh-26).
void star_Allocator_init(struct star_Allocator* self) {
    tlsf_create_with_pool(self->heap, self->heapSize);
}

void* star_Allocator_malloc(struct star_Allocator* self, size_t size) {
    return tlsf_malloc(self->heap, size);
}

void star_Allocator_free(struct star_Allocator* self, void* obj) {
    tlsf_free(self->heap, obj);
}

struct star_AudioSettings* star_AudioSettings_new(
    struct star_Allocator* allocator) {
        struct star_AudioSettings* settings =
            (struct star_AudioSettings*) star_Allocator_malloc(
                allocator, sizeof(struct star_AudioSettings));

    settings->sampleRate = star_DEFAULT_AUDIOSETTINGS.sampleRate;
    settings->numChannels = star_DEFAULT_AUDIOSETTINGS.numChannels;
    settings->blockSize = star_DEFAULT_AUDIOSETTINGS.blockSize;

    return settings;
}

void star_AudioSettings_destroy(struct star_Allocator* allocator,
    struct star_AudioSettings* self) {
    star_Allocator_free(allocator, self);
}


float_array_ptr star_samples_new(struct star_Allocator* allocator,
    size_t length) {
    return (float_array_ptr) star_Allocator_malloc(allocator,
        sizeof(float) * length);
}

// TODO: Does an AudioBlock type need to be introduced?
// TODO: Do we need a destroy function too?
float_array_ptr star_AudioBlock_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings) {
    return star_samples_new(allocator, audioSettings->blockSize);
}

float_array_ptr star_AudioBlock_newWithValue(
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings,
    float value) {
    float_array_ptr block = star_AudioBlock_new(allocator,
        audioSettings);
    star_fillWithValue(block, audioSettings->blockSize, value);

    return block;
}

struct star_Buffer* star_Buffer_new(struct star_Allocator* allocator,
    size_t length) {
    struct star_Buffer* buffer = (struct star_Buffer*) star_Allocator_malloc(allocator, sizeof(struct star_Buffer));
    buffer->length = length;
    buffer->samples = star_samples_new(allocator, length);

    return buffer;
}

void star_Buffer_fill(struct star_Buffer* self, float value) {
    star_fillWithValue(self->samples, self->length, value);
}

void star_Buffer_fillWithSilence(struct star_Buffer* self) {
    star_fillWithSilence(self->samples, self->length);
}

// TODO: Unit tests.
void star_Buffer_fillWithWaveform(struct star_Buffer* self,
    star_waveform_generator generate, float sampleRate,
    float phase, float freq) {
    float phaseInc = freq * star_TWOPI / sampleRate;
    for (size_t i = 0; i < self->length; i++) {
        FLOAT_ARRAY(self->samples)[i] = generate(phase);
        phase += phaseInc;
        if (phase >= star_TWOPI) {
            phase -= star_TWOPI;
        } else if (phase < 0.0) {
            phase += star_TWOPI;
        }
    }
}

void star_Buffer_destroy(struct star_Allocator* allocator, struct star_Buffer* self) {
    star_Allocator_free(allocator, self->samples);
    star_Allocator_free(allocator, self);
};



void star_sig_Signal_init(void* signal,
    struct star_AudioSettings* settings,
    float_array_ptr output,
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
    float_array_ptr output) {

    struct star_sig_Value_Parameters params = {
        .value = 1.0
    };

    star_sig_Signal_init(self, settings, output,
        *star_sig_Value_generate);

    self->parameters = params;
}

struct star_sig_Value* star_sig_Value_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
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
        FLOAT_ARRAY(self->signal.output)[i] = self->parameters.value;
    }

    self->lastSample = self->parameters.value;
}


struct star_sig_BinaryOp* star_sig_Add_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_BinaryOp* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_BinaryOp));
    star_sig_Add_init(self, settings, inputs, output);

    return self;
}

void star_sig_Add_init(struct star_sig_BinaryOp* self,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_Add_generate);
    self->inputs = inputs;
}

// TODO: Unit tests.
void star_sig_Add_generate(void* signal) {
    struct star_sig_BinaryOp* self = (struct star_sig_BinaryOp*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs->left)[i];
        float right = FLOAT_ARRAY(self->inputs->right)[i];
        float val = left + right;

        FLOAT_ARRAY(self->signal.output)[i] = val;
    }
}

void star_sig_Add_destroy(struct star_Allocator* allocator,
    struct star_sig_BinaryOp* self) {
    star_sig_Signal_destroy(allocator, self);
}


void star_sig_Mul_init(struct star_sig_BinaryOp* self,
    struct star_AudioSettings* settings, struct star_sig_BinaryOp_Inputs* inputs, float_array_ptr output) {
    star_sig_Signal_init(self, settings, output, *star_sig_Mul_generate);
    self->inputs = inputs;
};

struct star_sig_BinaryOp* star_sig_Mul_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_BinaryOp* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_BinaryOp));
    star_sig_Mul_init(self, settings, inputs, output);

    return self;
}

void star_sig_Mul_destroy(struct star_Allocator* allocator,
    struct star_sig_BinaryOp* self) {
    star_sig_Signal_destroy(allocator, (void*) self);
};

void star_sig_Mul_generate(void* signal) {
    struct star_sig_BinaryOp* self = (struct star_sig_BinaryOp*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs->left)[i];
        float right = FLOAT_ARRAY(self->inputs->right)[i];
        float val = left * right;
        FLOAT_ARRAY(self->signal.output)[i] = val;
    }
}


struct star_sig_BinaryOp* star_sig_Div_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_BinaryOp* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_BinaryOp));
    star_sig_Div_init(self, settings, inputs, output);

    return self;
}

void star_sig_Div_init(struct star_sig_BinaryOp* self,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_Div_generate);
    self->inputs = inputs;
}

void star_sig_Div_generate(void* signal) {
    struct star_sig_BinaryOp* self = (struct star_sig_BinaryOp*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs->left)[i];
        float right = FLOAT_ARRAY(self->inputs->right)[i];
        float val = left / right;
        FLOAT_ARRAY(self->signal.output)[i] = val;
    }
}

void star_sig_Div_destroy(struct star_Allocator* allocator,
    struct star_sig_BinaryOp* self) {
    star_sig_Signal_destroy(allocator, self);
}


struct star_sig_Invert* star_sig_Invert_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Invert_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_Invert* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_Invert));
    star_sig_Invert_init(self, settings, inputs, output);

    return self;
}

void star_sig_Invert_init(struct star_sig_Invert* self,
    struct star_AudioSettings* settings,
    struct star_sig_Invert_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_Invert_generate);
    self->inputs = inputs;
}

// TODO: Unit tests.
void star_sig_Invert_generate(void* signal) {
    struct star_sig_Invert* self = (struct star_sig_Invert*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float inSamp = FLOAT_ARRAY(self->inputs->source)[i];
        FLOAT_ARRAY(self->signal.output)[i] = -inSamp;
    }
}

void star_sig_Invert_destroy(struct star_Allocator* allocator,
    struct star_sig_Invert* self) {
    star_sig_Signal_destroy(allocator, self);
}


struct star_sig_Accumulate* star_sig_Accumulate_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Accumulate_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_Accumulate* self = star_Allocator_malloc(
        allocator,
        sizeof(struct star_sig_Accumulate));
    star_sig_Accumulate_init(self, settings, inputs, output);

    return self;
}

void star_sig_Accumulate_init(
    struct star_sig_Accumulate* self,
    struct star_AudioSettings* settings,
    struct star_sig_Accumulate_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_Accumulate_generate);

    struct star_sig_Accumulate_Parameters parameters = {
        .accumulatorStart = 1.0
    };

    self->inputs = inputs;
    self->parameters = parameters;
    self->accumulator = parameters.accumulatorStart;
    self->previousReset = 0.0f;
}

// TODO: Implement an audio rate version of this signal.
// TODO: Unit tests
void star_sig_Accumulate_generate(void* signal) {
    struct star_sig_Accumulate* self =
        (struct star_sig_Accumulate*) signal;

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

void star_sig_Accumulate_destroy(struct star_Allocator* allocator,
    struct star_sig_Accumulate* self) {
    star_sig_Signal_destroy(allocator, (void*) self);
}


struct star_sig_GatedTimer* star_sig_GatedTimer_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_GatedTimer_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_GatedTimer* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_GatedTimer));
    star_sig_GatedTimer_init(self, settings, inputs, output);

    return self;
}

void star_sig_GatedTimer_init(struct star_sig_GatedTimer* self,
    struct star_AudioSettings* settings,
    struct star_sig_GatedTimer_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_GatedTimer_generate);
    self->inputs = inputs;
    self->timer = 0;
    self->hasFired = false;
    self->prevGate = 0.0f;
}

// TODO: Unit tests
void star_sig_GatedTimer_generate(void* signal) {
    struct star_sig_GatedTimer* self =
        (struct star_sig_GatedTimer*) signal;

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

void star_sig_GatedTimer_destroy(struct star_Allocator* allocator,
    struct star_sig_GatedTimer* self) {
    star_sig_Signal_destroy(allocator, (void*) self);
}

struct star_sig_TimedTriggerCounter* star_sig_TimedTriggerCounter_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_TimedTriggerCounter_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_TimedTriggerCounter* self = star_Allocator_malloc(
        allocator,
        sizeof(struct star_sig_TimedTriggerCounter));
    star_sig_TimedTriggerCounter_init(self, settings, inputs, output);

    return self;
}

void star_sig_TimedTriggerCounter_init(
    struct star_sig_TimedTriggerCounter* self,
    struct star_AudioSettings* settings,
    struct star_sig_TimedTriggerCounter_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_TimedTriggerCounter_generate);

    self->inputs = inputs;
    self->numTriggers = 0;
    self->timer = 0;
    self->isTimerActive = false;
    self->previousSource = 0.0f;
}

void star_sig_TimedTriggerCounter_generate(void* signal) {
    struct star_sig_TimedTriggerCounter* self =
        (struct star_sig_TimedTriggerCounter*) signal;

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

void star_sig_TimedTriggerCounter_destroy(
    struct star_Allocator* allocator,
    struct star_sig_TimedTriggerCounter* self) {
    star_sig_Signal_destroy(allocator, (void*) self);
}


struct star_sig_ToggleGate* star_sig_ToggleGate_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_ToggleGate_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_ToggleGate* self = star_Allocator_malloc(
        allocator, sizeof(struct star_sig_ToggleGate));
    star_sig_ToggleGate_init(self, settings, inputs, output);

    return self;

}

void star_sig_ToggleGate_init(
    struct star_sig_ToggleGate* self,
    struct star_AudioSettings* settings,
    struct star_sig_ToggleGate_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_ToggleGate_generate);
    self->inputs = inputs;
    self->isGateOpen = false;
    self->prevTrig = 0.0f;
}

// TODO: Unit tests
void star_sig_ToggleGate_generate(void* signal) {
    struct star_sig_ToggleGate* self =
        (struct star_sig_ToggleGate*) signal;

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

void star_sig_ToggleGate_destroy(
    struct star_Allocator* allocator,
    struct star_sig_ToggleGate* self) {
    star_sig_Signal_destroy(allocator, (void*) self);
}


void star_sig_Sine_init(struct star_sig_Sine* self,
    struct star_AudioSettings* settings,
    struct star_sig_Sine_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_Sine_generate);

    self->inputs = inputs;
    self->phaseAccumulator = 0.0f;
}

struct star_sig_Sine* star_sig_Sine_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Sine_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
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
            FLOAT_ARRAY(self->inputs->phaseOffset)[i], star_TWOPI);

        FLOAT_ARRAY(self->signal.output)[i] = sinf(modulatedPhase) *
            FLOAT_ARRAY(self->inputs->mul)[i] +
            FLOAT_ARRAY(self->inputs->add)[i];

        float phaseStep = FLOAT_ARRAY(self->inputs->freq)[i] /
            self->signal.audioSettings->sampleRate * star_TWOPI;

        self->phaseAccumulator += phaseStep;
        if (self->phaseAccumulator > star_TWOPI) {
            self->phaseAccumulator -= star_TWOPI;
        }
    }
}


void star_sig_OnePole_init(struct star_sig_OnePole* self,
    struct star_AudioSettings* settings,
    struct star_sig_OnePole_Inputs* inputs,
    float_array_ptr output) {

    star_sig_Signal_init(self, settings, output,
        *star_sig_OnePole_generate);
    self->inputs = inputs;
    self->previousSample = 0.0f;
}

struct star_sig_OnePole* star_sig_OnePole_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_OnePole_Inputs* inputs) {

    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_OnePole* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_OnePole));
    star_sig_OnePole_init(self, settings, inputs, output);

    return self;
}

// TODO: Unit tests
void star_sig_OnePole_generate(void* signal) {
    struct star_sig_OnePole* self = (struct star_sig_OnePole*) signal;

    float previousSample = self->previousSample;
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->signal.output)[i] = previousSample =
            star_filter_onepole(
                FLOAT_ARRAY(self->inputs->source)[i], previousSample,
                FLOAT_ARRAY(self->inputs->coefficient)[i]);
    }
    self->previousSample = previousSample;
}

void star_sig_OnePole_destroy(struct star_Allocator* allocator,
    struct star_sig_OnePole* self) {
        star_sig_Signal_destroy(allocator, (void*) self);
}


void star_sig_Tanh_init(struct star_sig_Tanh* self,
    struct star_AudioSettings* settings,
    struct star_sig_Tanh_Inputs* inputs,
    float_array_ptr output) {

    star_sig_Signal_init(self, settings, output,
        *star_sig_Tanh_generate);
    self->inputs = inputs;
}

struct star_sig_Tanh* star_sig_Tanh_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Tanh_Inputs* inputs) {

    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_Tanh* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_Tanh));
    star_sig_Tanh_init(self, settings, inputs, output);

    return self;
}

// TODO: Unit tests.
void star_sig_Tanh_generate(void* signal) {
    struct star_sig_Tanh* self = (struct star_sig_Tanh*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float inSamp = FLOAT_ARRAY(self->inputs->source)[i];
        float outSamp = tanhf(inSamp);
        FLOAT_ARRAY(self->signal.output)[i] = outSamp;
    }
}

void star_sig_Tanh_destroy(struct star_Allocator* allocator,
    struct star_sig_Tanh* self) {
    star_sig_Signal_destroy(allocator, self);
}


void star_sig_Looper_init(struct star_sig_Looper* self,
    struct star_AudioSettings* settings,
    struct star_sig_Looper_Inputs* inputs,
    float_array_ptr output) {

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

    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_Looper* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_Looper));
    star_sig_Looper_init(self, settings, inputs, output);

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
void star_sig_Looper_generate(void* signal) {
    struct star_sig_Looper* self = (struct star_sig_Looper*) signal;
    float* samples = FLOAT_ARRAY(self->buffer->samples);
    float playbackPos = self->playbackPos;
    float bufferLastIdx = (float)(self->buffer->length - 1);

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float speed = FLOAT_ARRAY(self->inputs->speed)[i];
        float start = star_clamp(FLOAT_ARRAY(self->inputs->start)[i],
            0.0, 1.0);
        float end = star_clamp(FLOAT_ARRAY(self->inputs->end)[i],
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
                star_Buffer_fillWithSilence(self->buffer);
                self->isBufferEmpty = true;
            }

            // TODO: The star_interpolate_linear implementation
            // may wrap around inappropriately to the beginning of
            // the buffer (not to the startPos) if we're right at
            // the end of the buffer.
            FLOAT_ARRAY(self->signal.output)[i] = star_interpolate_linear(
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

void star_sig_Looper_destroy(struct star_Allocator* allocator,
    struct star_sig_Looper* self) {
    star_sig_Signal_destroy(allocator, self);
}


void star_sig_Dust_init(struct star_sig_Dust* self,
    struct star_AudioSettings* settings,
    struct star_sig_Dust_Inputs* inputs,
    float_array_ptr output) {

    star_sig_Signal_init(self, settings, output,
        *star_sig_Dust_generate);

    struct star_sig_Dust_Parameters parameters = {
        .bipolar = 0.0
    };

    self->inputs = inputs;
    self->parameters = parameters;
    self->sampleDuration = 1.0 / settings->sampleRate;
    self->previousDensity = 0.0;
    self->threshold = 0.0;
}

struct star_sig_Dust* star_sig_Dust_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Dust_Inputs* inputs) {

    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_Dust* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_Dust));
    star_sig_Dust_init(self, settings, inputs, output);

    return self;
}

void star_sig_Dust_generate(void* signal) {
    struct star_sig_Dust* self = (struct star_sig_Dust*) signal;

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

        float rand = star_randf();
        float val = rand < self->threshold ?
            rand * self->scale - scaleSub : 0.0f;
        FLOAT_ARRAY(self->signal.output)[i] = val;
    }
}

void star_sig_Dust_destroy(struct star_Allocator* allocator,
    struct star_sig_Dust* self) {
    star_sig_Signal_destroy(allocator, self);
}


void star_sig_TimedGate_init(struct star_sig_TimedGate* self,
    struct star_AudioSettings* settings,
    struct star_sig_TimedGate_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_TimedGate_generate);

    struct star_sig_TimedGate_Parameters parameters = {
        .resetOnTrigger = 0.0,
        .bipolar = 0.0
    };

    self->inputs = inputs;
    self->parameters = parameters;
    self->previousTrigger = 0.0f;
    self->gateValue = 0.0f;
    self->durationSamps = 0;
    self->samplesRemaining = 0;
}

struct star_sig_TimedGate* star_sig_TimedGate_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_TimedGate_Inputs* inputs) {
    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_TimedGate* self = star_Allocator_malloc(allocator,
        sizeof(struct star_sig_TimedGate));
    star_sig_TimedGate_init(self, settings, inputs, output);

    return self;
}

static inline void star_sig_TimedGate_outputHigh(struct star_sig_TimedGate* self,
    size_t index) {
    FLOAT_ARRAY(self->signal.output)[index] = self->gateValue;
    self->samplesRemaining--;
}

static inline void star_sig_TimedGate_outputLow(struct star_sig_TimedGate* self,
    size_t index) {
    FLOAT_ARRAY(self->signal.output)[index] = 0.0f;
}

void star_sig_TimedGate_generate(void* signal) {
    struct star_sig_TimedGate* self = (struct star_sig_TimedGate*)
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

            self->samplesRemaining = self->durationSamps;

            if (self->parameters.resetOnTrigger > 0.0f &&
                self->samplesRemaining > 0) {
                // Gate is open and needs to be reset.
                // Close the gate for one sample,
                // and don't count down the duration
                // until next time.
                star_sig_TimedGate_outputLow(self, i);
            } else {
                star_sig_TimedGate_outputHigh(self, i);
            }
        } else if (self->samplesRemaining > 0) {
            star_sig_TimedGate_outputHigh(self, i);
        } else {
            star_sig_TimedGate_outputLow(self, i);
        }

        self->previousTrigger = currentTrigger;
    }
}

void star_sig_TimedGate_destroy(struct star_Allocator* allocator,
    struct star_sig_TimedGate* self) {
    star_sig_Signal_destroy(allocator, self);
}


void star_sig_ClockFreqDetector_init(
    struct star_sig_ClockFreqDetector* self,
    struct star_AudioSettings* settings,
    struct star_sig_ClockFreqDetector_Inputs* inputs,
    float_array_ptr output) {
    star_sig_Signal_init(self, settings, output,
        *star_sig_ClockFreqDetector_generate);

    struct star_sig_ClockFreqDetector_Parameters params = {
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

struct star_sig_ClockFreqDetector* star_sig_ClockFreqDetector_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_ClockFreqDetector_Inputs* inputs) {

    float_array_ptr output = star_AudioBlock_new(allocator, settings);
    struct star_sig_ClockFreqDetector* self =
        star_Allocator_malloc(allocator,
        sizeof(struct star_sig_ClockFreqDetector));
    star_sig_ClockFreqDetector_init(self, settings, inputs, output);

    return self;
}

static inline float star_sig_ClockFreqDetector_calcClockFreq(
    float sampleRate, uint32_t samplesSinceLastPulse,
    float prevFreq) {
    float freq = sampleRate / (float) samplesSinceLastPulse;
    // TODO: Is an LPF good, or is a moving average better?
    return star_filter_onepole(freq, prevFreq, 0.01f);
}

void star_sig_ClockFreqDetector_generate(void* signal) {
    struct star_sig_ClockFreqDetector* self =
        (struct star_sig_ClockFreqDetector*) signal;
    float_array_ptr source = self->inputs->source;
    float_array_ptr output = self->signal.output;

    float previousTrigger = self->previousTrigger;
    float clockFreq = self->clockFreq;
    bool isRisingEdge = self->isRisingEdge;
    uint32_t samplesSinceLastPulse = self->samplesSinceLastPulse;
    float sampleRate = self->signal.audioSettings->sampleRate;
    float threshold = self->parameters.threshold;
    float timeoutDuration = self->parameters.timeoutDuration;
    float pulseDurSamples = self->pulseDurSamples;

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
            clockFreq = star_sig_ClockFreqDetector_calcClockFreq(
                sampleRate, samplesSinceLastPulse, clockFreq);
            pulseDurSamples = samplesSinceLastPulse;
            samplesSinceLastPulse = 0;
            isRisingEdge = false;
        } else if (samplesSinceLastPulse > sampleRate * timeoutDuration) {
            // It's been too long since we've received a pulse.
            // Just reset everything.
            clockFreq = 0.0f;
            samplesSinceLastPulse = 0;
        } else if (samplesSinceLastPulse > pulseDurSamples) {
            // Tempo is slowing down; recalculate it.
            clockFreq = star_sig_ClockFreqDetector_calcClockFreq(
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

void star_sig_ClockFreqDetector_destroy(struct star_Allocator* allocator,
    struct star_sig_ClockFreqDetector* self) {
    star_sig_Signal_destroy(allocator, self);
}
