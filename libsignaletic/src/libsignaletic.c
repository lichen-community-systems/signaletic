#include <math.h>   // For powf, fmodf, sinf, roundf, fabsf
#include <stdlib.h> // For labs
#include <tlsf.h>   // Includes assert.h, limits.h, stddef.h
                    // stdio.h, stdlib.h, string.h (for errors etc.)
#include <libsignaletic.h>

extern inline float sig_fminf(float a, float b) {
    float r;
#ifdef __arm__
    asm("vminnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a < b) ? a : b;
#endif // __arm__
    return r;
}

extern inline float sig_fmaxf(float a, float b) {
    float r;
#ifdef __arm__
    asm("vmaxnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a > b) ? a : b;
#endif // __arm__
    return r;
}

extern inline float sig_clamp(float value, float min, float max) {
    return sig_fminf(sig_fmaxf(value, min), max);
}

extern inline float sig_fastMod1f(float x) {
    int32_t ix = (int32_t) x;
    float r = x - (float) ix;
    return r + (r < 0.0f);
}

extern inline float sig_flooredfmodf(float numer, float denom) {
    float remain = fmodf(numer, denom);
    if ((remain > 0.0f && denom < 0.0f) ||
        (remain < 0.0f && denom > 0.0f)) {
        remain = remain + denom;
    }

    return remain;
}

struct sig_Random* sig_Random_new(struct sig_Allocator* allocator,
    uint32_t seed) {
    struct sig_Random* self = sig_MALLOC(allocator,
        struct sig_Random);
    sig_Random_init(self, seed);

    return self;
}

void sig_Random_init(struct sig_Random* self, uint32_t seed) {
    sig_Random_seed(self, seed);
}

void sig_Random_seed(struct sig_Random* self, uint32_t seed) {
    self->seed = seed;
    // Expand and diffuse the seed into four state words
    // using SplitMix32.
    // Implementation from Kaito Udagawa's CC0 splitmix32.c
    // https://github.com/umireon/my-random-stuff/blob/master/xorshift/splitmix32.c
    for (size_t i = 0; i < 4; i++) {
        seed += 0x9e3779b9;
        uint32_t z = seed;
        z = (z ^ (z >> 16)) * 0x85ebca6b;
        z = (z ^ (z >> 13)) * 0xc2b2ae35;
        self->state[i] = z ^ (z >> 16);
    }
}

uint32_t sig_Random_nextU32(struct sig_Random* self) {
    // Directly from Black & Vigna's public domain implementation:
    // https://prng.di.unimi.it/xoshiro128plus.c
    uint32_t* s = self->state;
    const uint32_t result = s[0] + s[3];
	const uint32_t t = s[1] << 9;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];
	s[2] ^= t;
	s[3] = (s[3] << 11) | (s[3] >> (32 - 11));

	return result;
}

float sig_Random_nextFloat(struct sig_Random* self) {
    // Only take the top 24 bits:
    // 1. because xoshiro128+ has low linear complexity in
    // the lowest four bits.
    // 2. so that every output is an exact multiple
    // of 2^-24, avoiding float rounding artifacts.
    return (sig_Random_nextU32(self) >> 8) * (1.0f / 16777216.0f);
}

float sig_Random_generate(struct sig_Random* self) {
    return (sig_Random_nextU32(self) >> 8) * (1.0f / 16777215.0f);
}

void sig_Random_destroy(struct sig_Allocator* allocator,
    struct sig_Random* self) {
    allocator->impl->free(allocator, self);
}

// The global default generator used by sig_randf.
static struct sig_Random sig_Random_GLOBAL_GENERATOR = {
    .seed = 1,
    .state = {0x5E2D1772, 0x14E498F0, 0xD20EA1FD, 0xB382F339}
};

float sig_randf(void) {
    return sig_Random_generate(&sig_Random_GLOBAL_GENERATOR);
}

void sig_randf_seed(uint32_t seed) {
    sig_Random_seed(&sig_Random_GLOBAL_GENERATOR, seed);
}

extern inline float sig_fastTanhf(float x) {
    // From https://gist.github.com/ndonald2/534831b639b8c78d40279b5007e06e5b
    if (x > 3.0f) {
        return 1.0f;
    }

    if (x < -3.0f) {
        return -1.0f;
    }

    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

extern inline float sig_linearMap(float value,
    float fromMin, float fromMax, float toMin, float toMax) {
    if (fromMin == fromMax) {
        return toMin;
    }

    float clamped = sig_clamp(value, fromMin, fromMax);
    float mapped = (clamped - fromMin) * (toMax - toMin) /
        (fromMax - fromMin) + toMin;

    return mapped;
}

extern inline uint16_t sig_unipolarToUint12(float sample) {
    return (uint16_t) (sample * 4095.0f);
}

extern inline uint16_t sig_bipolarToUint12(float sample) {
    float normalized = sample * 0.5 + 0.5;
    return (uint16_t) (normalized * 4095.0f);
}

extern inline uint16_t sig_bipolarToInvUint12(float sample) {
    return sig_bipolarToUint12(-sample);
}

extern inline float sig_uint16ToBipolar(uint16_t sample) {
    float normalized = (float) (sample / 65535.0f);
    float scaled = normalized * 2.0f - 1.0f;

    return scaled;
}

extern inline float sig_uint16ToUnipolar(uint16_t sample) {
    float normalized = (float) (sample / 65535.0f);

    return normalized;
}

extern inline float sig_invUint16ToBipolar(uint16_t sample) {
    return -sig_uint16ToBipolar(sample);
}

float sig_midiToFreq(float midiNum) {
    return powf(2, (midiNum - 69.0f) / 12.0f) * 440.0f;
}

float sig_freqToMidi(float frequency) {
    return(12.0f * logf(frequency / 440.0f) / sig_LOG2) + 69.0f;
}

float sig_linearToFreq(float vOct, float middleFreq) {
    return middleFreq * powf(2, vOct);
}

float sig_freqToLinear(float freq, float middleFreq) {
    return (logf(freq / middleFreq) / sig_LOG2);
}

// TODO: Unit tests.
extern inline float sig_sum(float_array_ptr values,
    size_t length) {
    float sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += FLOAT_ARRAY(values)[i];
    }

    return sum;
}

// TODO: Unit tests.
extern inline size_t sig_indexOfMin(float_array_ptr values,
    size_t length) {
    size_t indexOfMin = 0;

    if (length < 1) {
        return indexOfMin;
    }

    float minValue = FLOAT_ARRAY(values)[0];
    for (size_t i = 1; i < length; i++) {
        float currentValue = FLOAT_ARRAY(values)[i];
        if (currentValue < minValue) {
            indexOfMin = i;
            minValue = currentValue;
        }
    }

    return indexOfMin;
}

// TODO: Unit tests.
extern inline size_t sig_indexOfMax(float_array_ptr values,
    size_t length) {
    size_t indexOfMax = 0;

    if (length < 1) {
        return indexOfMax;
    }

    float maxValue = FLOAT_ARRAY(values)[0];
    for (size_t i = 1; i < length; i++) {
        float currentValue = FLOAT_ARRAY(values)[i];
        if (currentValue > maxValue) {
            indexOfMax = i;
            maxValue = currentValue;
        }
    }

    return indexOfMax;
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
    size_t aIdx = (size_t) idxIntegral % length;
    float a = FLOAT_ARRAY(table)[aIdx];
    float b = FLOAT_ARRAY(table)[(aIdx + 1) % length];

    return a + (b - a) * idxFractional;
}

// TODO: Unit tests.
float sig_interpolate_cubic(float idx, float_array_ptr table,
    size_t length) {
    size_t idxIntegral = (size_t) idx;
    float idxFractional = idx - (float) idxIntegral;
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
extern inline float sig_filter_mean(float_array_ptr values,
    size_t length) {
    float sum = sig_sum(values, length);
    return sum / (float) length;
}

// TODO: Unit tests.
extern inline float sig_filter_meanExcludeMinMax(
    float_array_ptr values, size_t length) {
    if (length < 1) {
        return 0.0f;
    } else if (length < 3) {
        return sig_filter_mean(values, length);
    }

    float min = FLOAT_ARRAY(values)[sig_indexOfMin(values, length)];
    float max = FLOAT_ARRAY(values)[sig_indexOfMax(values, length)];
    float exclude = min + max;
    float sum = sig_sum(values, length) - exclude;
    return sum / (float) (length - 2);
}

// TODO: Unit tests.
extern inline float sig_filter_ema(float current, float previous,
    float a) {
    return (a * current) + (1 - a) * previous;
}

extern inline float sig_filter_onepole(float current,
    float previous, float b0, float a1) {
    return b0 * current - a1 * previous;
}

extern inline float sig_filter_onepole_HPF_calculateA1(
    float frequency, float sampleRate) {
    float fc = frequency / sampleRate;
    return -expf(-2.0f * sig_PI * (0.5f - fc));
}

extern inline float sig_filter_onepole_HPF_calculateB0(float a1) {
    return 1.0f + a1;
}

extern inline float sig_filter_onepole_LPF_calculateA1(
    float frequency, float sampleRate) {
    float fc = frequency / sampleRate;
    return expf(-2.0f * sig_PI * fc);
}

extern inline float sig_filter_onepole_LPF_calculateB0(float a1) {
    return 1.0f - a1;
}

extern inline float sig_filter_smooth(float current,
    float previous, float coeff) {
    return current + coeff * (previous - current);
}

extern inline float sig_filter_smooth_calculateCoefficient(
    float time, float sampleRate) {
    return expf(sig_LOG0_001 / (time * sampleRate));
}

extern inline float sig_filter_dcBlock(float current,
    float previousInput, float previousOutput, float r) {
    return current - previousInput + r * previousOutput;
}

extern inline float sig_filter_dcBlock_calculateCoefficient(
    float frequency, float sampleRate) {
    return 1.0f - (sig_TWOPI * frequency / sampleRate);
}

void sig_filter_Smooth_init(struct sig_filter_Smooth* self,
    float coeff) {
    self->coeff = coeff;
    self->previous = 0;
}

extern inline float sig_filter_Smooth_generate(
    struct sig_filter_Smooth* self, float value) {
    float smoothed = sig_filter_smooth(value, self->previous, self->coeff);
    self->previous = smoothed;

    return smoothed;
}

void sig_filter_DCBlock_init(struct sig_filter_DCBlock* self,
    float frequency, float sampleRate) {
    self->r = sig_filter_dcBlock_calculateCoefficient(frequency, sampleRate);
    self->previousInput = 0.0f;
    self->previousOutput = 0.0f;
}

extern inline float sig_filter_DCBlock_generate(
    struct sig_filter_DCBlock* self, float value) {
    float output = sig_filter_dcBlock(value,
        self->previousInput, self->previousOutput, self->r);
    self->previousInput = value;
    self->previousOutput = output;

    return output;
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

float sig_waveform_sineHarmonicGain(size_t harmonic) {
    return harmonic == 1 ? 1.0f : 0.0f;
}

float sig_waveform_triangleHarmonicGain(size_t harmonic) {
    float k = (float) harmonic;

    return harmonic % 2 == 1 ? 8.0f / (sig_PI * sig_PI * k * k) : 0.0f;
}

float sig_waveform_sawHarmonicGain(size_t harmonic) {
    return -2.0f / (sig_PI * (float) harmonic);
}

float sig_waveform_squareHarmonicGain(size_t harmonic) {
    return harmonic % 2 == 1 ? 4.0f / (sig_PI * (float) harmonic) : 0.0f;
}

float sig_waveform_reverseSawHarmonicGain(size_t harmonic) {
    return -sig_waveform_sawHarmonicGain(harmonic);
}

// Triangle harmonics are cosines, which start a quarter cycle into a sine.
const struct sig_WaveformShape
    sig_STANDARD_WAVEFORMS[sig_StandardWaveform_COUNT] = {
    {
        .generate = sig_waveform_sine,
        .harmonic = sig_waveform_sineHarmonicGain,
        .harmonicPhase = 0.0f
    },
    {
        .generate = sig_waveform_triangle,
        .harmonic = sig_waveform_triangleHarmonicGain,
        .harmonicPhase = 0.25f
    },
    {
        .generate = sig_waveform_saw,
        .harmonic = sig_waveform_sawHarmonicGain,
        .harmonicPhase = 0.0f
    },
    {
        .generate = sig_waveform_square,
        .harmonic = sig_waveform_squareHarmonicGain,
        .harmonicPhase = 0.0f
    },
    {
        .generate = sig_waveform_reverseSaw,
        .harmonic = sig_waveform_reverseSawHarmonicGain,
        .harmonicPhase = 0.0f
    }
};

float sig_waveform_harmonicGain(enum sig_StandardWaveform waveform,
    size_t harmonic) {
    return sig_STANDARD_WAVEFORMS[waveform].harmonic(harmonic);
}

float sig_waveform_harmonicPhase(enum sig_StandardWaveform waveform) {
    return sig_STANDARD_WAVEFORMS[waveform].harmonicPhase;
}

float sig_waveform_harmonicTaper(enum sig_HarmonicTaper taper,
    size_t harmonic, size_t numHarmonics) {
    if (taper != sig_HarmonicTaper_LANCZOS) {
        return 1.0f;
    }

    float x = sig_PI * (float) harmonic / (float) (numHarmonics + 1);

    return sinf(x) / x;
}


void sig_osc_Oscillator_init(struct sig_osc_Oscillator* self) {
    self->phaseAccumulator = 0.0f;
}

extern inline float sig_osc_Oscillator_eoc(float phase) {
    return phase < 0.0f || phase > 1.0f ? 1.0f : 0.0f;
}

extern inline float sig_osc_Oscillator_wrapPhase(float phase) {
    return sig_fastMod1f(phase);
}

extern inline void sig_osc_Oscillator_accumulatePhase(
    float* phaseAccumulator, float frequency, float sampleRate) {
    float phaseStep = frequency / sampleRate;
    float phase = *phaseAccumulator + phaseStep;

    *phaseAccumulator = sig_osc_Oscillator_wrapPhase(phase);
}

void sig_osc_Wavetable_init(struct sig_osc_Wavetable* self,
    struct sig_Buffer* wavetable) {
    self->phaseAccumulator = 0.0f;
    self->wavetable = wavetable;
}

extern inline float sig_osc_Wavetable_generate(
    struct sig_osc_Wavetable* self, float frequency,
    float phaseOffset, float sampleRate, float* eocOut) {
    float modulatedPhase = self->phaseAccumulator + phaseOffset;
    *eocOut = sig_osc_Oscillator_eoc(modulatedPhase);
    modulatedPhase = sig_osc_Oscillator_wrapPhase(modulatedPhase);
    float sample = sig_Buffer_readLinearAtPhase(self->wavetable,
        modulatedPhase);
    sig_osc_Oscillator_accumulatePhase(&self->phaseAccumulator,
        frequency, sampleRate);

    return sample;
}


void sig_osc_WavetableBank_init(struct sig_osc_WavetableBank* self,
    struct sig_WavetableBank* wavetables) {
    self->phaseAccumulator = 0.0f;
    self->wavetables = wavetables;
}

float sig_osc_WavetableBank_tableIndex(size_t table, size_t numTables) {
    return (float) table / (float) numTables;
}

extern inline float sig_osc_WavetableBank_generate(
    struct sig_osc_WavetableBank* self,
    float frequency, float phaseOffset, float tableIndex,
    float sampleRate, float* eocOut) {
        // TODO: Precompute this.
        float numTables = (float) self->wavetables->length;
        // TODO: Reduce duplication with sig_osc_Wavetable
        float modulatedPhase = self->phaseAccumulator + phaseOffset;
        *eocOut = sig_osc_Oscillator_eoc(modulatedPhase);
        modulatedPhase = sig_fastMod1f(modulatedPhase);
        float scaledTableIdx = tableIndex * numTables;
        float sample = sig_WavetableBank_readLinearAtPhase(
            self->wavetables, scaledTableIdx, modulatedPhase);
        sig_osc_Oscillator_accumulatePhase(&self->phaseAccumulator,
            frequency, sampleRate);

        return sample;
}

void sig_osc_AdditiveWavetableBank_init(
    struct sig_osc_AdditiveWavetableBank* self,
    struct sig_AdditiveWavetableBank* wavetables) {
    self->phaseAccumulator = 0.0f;
    self->wavetables = wavetables;
    self->previousFrequency = 0.0f;
    self->level = 0.0f;
}

extern inline float sig_osc_AdditiveWavetableBank_generate(
    struct sig_osc_AdditiveWavetableBank* self, float frequency,
    float phaseOffset, float tableIndex, float sampleRate, float* eocOut) {
    float waveIdx = tableIndex * (float) self->wavetables->numShapes;
    float modulatedPhase = self->phaseAccumulator + phaseOffset;
    *eocOut = sig_osc_Oscillator_eoc(modulatedPhase);
    modulatedPhase = sig_osc_Oscillator_wrapPhase(modulatedPhase);

    // Logarithms are expensive, so the level is only
    // recalculated when the frequency changes.
    if (frequency != self->previousFrequency) {
        self->level = sig_AdditiveWavetableBank_levelForFrequency(
            self->wavetables, frequency);
        self->previousFrequency = frequency;
    }

    float sample = sig_AdditiveWavetableBank_readLinearAtPhase(
        self->wavetables, self->level, waveIdx, modulatedPhase);
    sig_osc_Oscillator_accumulatePhase(&self->phaseAccumulator,
        frequency, sampleRate);

    return sample;
}


void sig_osc_FastLFSine_init(struct sig_osc_FastLFSine* self,
    float sampleRate) {
    self->sampleRate = sampleRate;
    self->sinZ = 0.0f;
    self->cosZ = 1.0f;
    sig_osc_FastLFSine_setFrequency(self, 1.0f);
}

extern inline void sig_osc_FastLFSine_setFrequency(
    struct sig_osc_FastLFSine* self, float frequency) {
    self->f = 2.0f * sinf(sig_PI * frequency / self->sampleRate);
}

extern inline void sig_osc_FastLFSine_setFrequencyFast(
    struct sig_osc_FastLFSine* self, float frequency) {
    self->f = sig_TWOPI * frequency / self->sampleRate;
}

extern inline void sig_osc_FastLFSine_generate(struct sig_osc_FastLFSine* self) {
    // TODO: Oversample for stability at higher frequencies.
    // Depending on the Q setting, but stability is roughly 1/6
    // of the sample rate.
    // A simple way to do this is to run the filter twice with the same input sample, and discard one output sample.
    self->sinZ = self->sinZ + self->f * self->cosZ;
    self->cosZ = self->cosZ - self->f * self->sinZ;
}



// TODO: Implement enough test coverage for sig_Allocator
// to support a switch from TLSF to another memory allocator
// implementation sometime in the future (gh-26).
void sig_TLSFAllocator_init(struct sig_Allocator* allocator) {
    tlsf_create_with_pool(allocator->heap->memory,
        allocator->heap->length);
}

void* sig_TLSFAllocator_malloc(struct sig_Allocator* allocator,
    size_t size) {
    // TODO: TLSF will return a null pointer if we're out of memory.
    // The Allocator API needs to be extended to always take a Status object
    // for any operation that can fail.
    return tlsf_malloc(allocator->heap->memory, size);
}

void sig_TLSFAllocator_free(struct sig_Allocator* allocator,
    void* obj) {
    tlsf_free(allocator->heap->memory, obj);
}

struct sig_AllocatorImpl sig_TLSFAllocatorImpl = {
    .init = sig_TLSFAllocator_init,
    .malloc = sig_TLSFAllocator_malloc,
    .free = sig_TLSFAllocator_free
};


struct sig_Status* sig_Status_new(struct sig_Allocator* allocator) {
    struct sig_Status* self = allocator->impl->malloc(allocator,
        sizeof(struct sig_Status));
    sig_Status_init(self);

    return self;
}

void sig_Status_init(struct sig_Status* status) {
    sig_Status_reset(status);
}

void sig_Status_reset(struct sig_Status* status) {
    status->result = SIG_RESULT_NONE;
}

extern inline void sig_Status_reportResult(
    struct sig_Status* status, enum sig_Result result) {
    if (status != NULL) {
        status->result = result;
    }
}


// TODO: Unit tests.
struct sig_List* sig_List_new(struct sig_Allocator* allocator,
    size_t capacity) {
    struct sig_List* self = allocator->impl->malloc(allocator,
        sizeof(struct sig_List));
    void** items = allocator->impl->malloc(allocator,
        sizeof(void*) * capacity);
    sig_List_init(self, items, capacity);

    return self;
}

// TODO: Unit tests.
void sig_List_init(struct sig_List* self, void** items,
    size_t capacity) {
    self->items = items;
    self->length = 0;
    self->capacity = capacity;
}

// TODO: Unit tests.
void sig_List_insert(struct sig_List* self,
    size_t index, void* item, struct sig_Status* status) {
    // We don't support sparse lists
    // or any kind of index wrapping.
    if (index > self->length || index < 0) {
        sig_Status_reportResult(status, SIG_ERROR_INDEX_OUT_OF_BOUNDS);
        return;
    }

    if (index == self->length) {
        sig_List_append(self, item, status);
        return;
    }

    if (self->length >= self->capacity) {
        sig_Status_reportResult(status, SIG_ERROR_EXCEEDS_CAPACITY);
        return;
    }

    for (size_t i = self->length - 1; i >= 0; i--) {
        self->items[i + 1] = self->items[i];
    }

    self->length++;

    sig_Status_reportResult(status, SIG_RESULT_SUCCESS);
}

// TODO: Unit tests.
void sig_List_append(struct sig_List* self, void* item,
    struct sig_Status* status) {
    size_t index = self->length;

    if (index >= self->capacity) {
        sig_Status_reportResult(status, SIG_ERROR_EXCEEDS_CAPACITY);
        return;
    }

    self->items[index] = item;
    self->length++;

    sig_Status_reportResult(status, SIG_RESULT_SUCCESS);
    return;
}

// TODO: Unit tests.
void* sig_List_pop(struct sig_List* self,
    struct sig_Status* status) {
    if (self->length < 1) {
        sig_Status_reportResult(status, SIG_ERROR_INDEX_OUT_OF_BOUNDS);
        return NULL;
    }

    size_t index = self->length - 1;
    void* item = self->items[index];

    self->items[index] = NULL;
    self->length--;

    sig_Status_reportResult(status, SIG_RESULT_SUCCESS);
    return item;
}

// TODO: Unit tests.
void* sig_List_remove(struct sig_List* self, size_t index,
    struct sig_Status* status) {
    if (index >= self->length || index < 1) {
        sig_Status_reportResult(status, SIG_ERROR_INDEX_OUT_OF_BOUNDS);
        return NULL;
    }

    if (index == self->length - 1) {
        return sig_List_pop(self, status);
    }

    void* removedItem = self->items[index];

    for (size_t i = index + 1; i < self->length - 1; i++) {
        void* item = self->items[i];
        self->items[i - 1] = item;
    }

    self->items[self->length - 1] = NULL;
    self->length--;

    sig_Status_reportResult(status, SIG_RESULT_SUCCESS);
    return removedItem;
}

// TODO: Unit tests.
void sig_List_destroy(struct sig_Allocator* allocator,
    struct sig_List* self) {
    allocator->impl->free(allocator, self->items);
    allocator->impl->free(allocator, self);
}


struct sig_AudioSettings* sig_AudioSettings_new(
    struct sig_Allocator* allocator) {
    struct sig_AudioSettings* settings =
        (struct sig_AudioSettings*)allocator->impl->malloc(
            allocator, sizeof(struct sig_AudioSettings));

    settings->sampleRate = sig_DEFAULT_AUDIOSETTINGS.sampleRate;
    settings->numChannels = sig_DEFAULT_AUDIOSETTINGS.numChannels;
    settings->blockSize = sig_DEFAULT_AUDIOSETTINGS.blockSize;

    return settings;
}

void sig_AudioSettings_destroy(struct sig_Allocator* allocator,
    struct sig_AudioSettings* self) {
    allocator->impl->free(allocator, self);
}



struct sig_SignalContext* sig_SignalContext_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings) {
    struct sig_SignalContext* self = sig_MALLOC(allocator,
        struct sig_SignalContext);

    self->audioSettings = audioSettings;

    struct sig_Buffer* emptyBuffer = sig_Buffer_new(allocator, 0);
    self->emptyBuffer = emptyBuffer;

    struct sig_DelayLine* oneSampleDelayLine = sig_DelayLine_new(allocator, 1);
    self->oneSampleDelayLine = oneSampleDelayLine;

    struct sig_dsp_ConstantValue* silence = sig_dsp_ConstantValue_new(
        allocator, self, 0.0f);
    self->silence = silence;

    struct sig_dsp_ConstantValue* unity = sig_dsp_ConstantValue_new(
        allocator, self, 1.0f);
    self->unity = unity;

    return self;
}

void sig_SignalContext_destroy(struct sig_Allocator* allocator,
    struct sig_SignalContext* self) {
    sig_Buffer_destroy(allocator, self->emptyBuffer);
    sig_DelayLine_destroy(allocator, self->oneSampleDelayLine);
    sig_dsp_ConstantValue_destroy(allocator, self->silence);
    sig_dsp_ConstantValue_destroy(allocator, self->unity);
    allocator->impl->free(allocator, self);
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
    return (float_array_ptr) allocator->impl->malloc(allocator,
        sizeof(float) * length);
}

// TODO: Does an AudioBlock type need to be introduced?
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

float_array_ptr sig_AudioBlock_newSilent(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings) {
    return sig_AudioBlock_newWithValue(allocator, audioSettings, 0.0f);
}

void sig_AudioBlock_destroy(struct sig_Allocator* allocator,
    float_array_ptr self) {
    allocator->impl->free(allocator, self);
}

struct sig_Buffer* sig_Buffer_new(struct sig_Allocator* allocator,
    size_t length) {
    struct sig_Buffer* self = (struct sig_Buffer*)
        allocator->impl->malloc(allocator, sizeof(struct sig_Buffer));
    self->length = length;
    self->samples = sig_samples_new(allocator, length);

    return self;
}

void sig_Buffer_fill(struct sig_Buffer* self,
    sig_array_filler filler) {
    sig_fill(self->samples, self->length, filler);
}

void sig_Buffer_fillWithValue(struct sig_Buffer* self,
    float value) {
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
        } else if (phase < 0.0f) {
            phase += sig_TWOPI;
        }
    }
}

float sig_Buffer_read(struct sig_Buffer* self, float idx) {
    return FLOAT_ARRAY(self->samples)[(size_t) idx % self->length];
}

float sig_Buffer_readLinear(struct sig_Buffer* self, float idx) {
    return sig_interpolate_linear(idx, self->samples, self->length);
}

extern inline float sig_Buffer_readLinearAtPhase(
    struct sig_Buffer* self,
    float phase) {
    size_t tableLength = self->length;
    float tableIdx = phase * tableLength;

    return sig_interpolate_linear(tableIdx, self->samples, tableLength);
}

float sig_Buffer_readCubic(struct sig_Buffer* self, float idx) {
    return sig_interpolate_cubic(idx, self->samples, self->length);
}

void sig_Buffer_addHarmonic(struct sig_Buffer* self,
    struct sig_Buffer* sineTable, size_t harmonic, float gain, float phase) {
    const size_t length = self->length;
    float* SIG_RESTRICT samples = FLOAT_ARRAY(self->samples);
    const float* SIG_RESTRICT sine = FLOAT_ARRAY(sineTable->samples);
    size_t sineIdx = ((size_t) (phase * (float) length)) % length;

    for (size_t i = 0; i < length; i++) {
        samples[i] += gain * sine[sineIdx];
        sineIdx += harmonic;
        if (sineIdx >= length) {
            sineIdx -= length;
        }
    }
}

void sig_Buffer_fillWithHarmonics(struct sig_Buffer* self,
    struct sig_Buffer* sineTable, const struct sig_WaveformShape* shape,
    size_t numHarmonics, enum sig_HarmonicTaper taper) {
    const float phase = shape->harmonicPhase;

    sig_Buffer_fillWithSilence(self);
    for (size_t harmonic = 1; harmonic <= numHarmonics; harmonic++) {
        float gain = shape->harmonic(harmonic) *
            sig_waveform_harmonicTaper(taper, harmonic, numHarmonics);
        if (gain != 0.0f) {
            sig_Buffer_addHarmonic(self, sineTable, harmonic, gain, phase);
        }
    }
}

void sig_Buffer_destroy(struct sig_Allocator* allocator,
    struct sig_Buffer* self) {
    allocator->impl->free(allocator, self->samples);
    allocator->impl->free(allocator, self);
};


struct sig_Buffer* sig_BufferView_new(
    struct sig_Allocator* allocator,
    struct sig_Buffer* buffer, size_t startIdx, size_t length) {
    struct sig_Buffer* self = sig_MALLOC(allocator, struct sig_Buffer);

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
    allocator->impl->free(allocator, self);
}


struct sig_WavetableBank* sig_WavetableBank_new(
    struct sig_Allocator* allocator,
    size_t numTables, size_t tableLength) {
    struct sig_WavetableBank* self = sig_MALLOC(allocator,
        struct sig_WavetableBank);
    self->length = numTables;

    self->waves = (struct sig_Buffer**) allocator->impl->malloc(
        allocator, sizeof(struct sig_Buffer*) * numTables);

    for (size_t i = 0; i < numTables; i++) {
        self->waves[i] = sig_Buffer_new(allocator, tableLength);
    }

    return self;
}

extern inline float sig_WavetableBank_readLinearAtPhase(
    struct sig_WavetableBank* self,
    float tableIdx, float phase) {
        const size_t length = self->length;
        const float lengthF = (float) length;

        // Wrapping requires a division, so it is skipped for the
        // usual case where the index is already within the bank.
        if (tableIdx < 0.0f || tableIdx >= lengthF) {
            tableIdx = sig_flooredfmodf(tableIdx, lengthF);
        }

        size_t tableIdxIntegral = (size_t) tableIdx;
        float tableIdxFractional = tableIdx - (float) tableIdxIntegral;

        // Tiny negative indices can wrap to exactly the length,
        // which is the start of the bank.
        size_t aTableIndex = tableIdxIntegral % length;
        struct sig_Buffer* aTable = self->waves[aTableIndex];
        size_t bTableIndex = (aTableIndex + 1) % length;
        struct sig_Buffer* bTable = self->waves[bTableIndex];

        float a = sig_Buffer_readLinearAtPhase(aTable, phase);
        float b = sig_Buffer_readLinearAtPhase(bTable, phase);
        float sample = a + (b - a) * tableIdxFractional;

        return sample;
}

void sig_WavetableBank_destroy(struct sig_Allocator* allocator,
    struct sig_WavetableBank* self) {
    for (size_t i = 0; i < self->length; i++) {
        sig_Buffer_destroy(allocator, self->waves[i]);
        self->waves[i] = NULL;
    }

    allocator->impl->free(allocator, self->waves);
    self->waves = NULL;
    allocator->impl->free(allocator, self);
    self = NULL;
}

struct sig_AdditiveWavetableBank* sig_AdditiveWavetableBank_new(
    struct sig_Allocator* allocator, const struct sig_WaveformShape* shapes,
    size_t numShapes, size_t numLevels, size_t tableLength,
    float lowestFrequency, float maxHarmonicFrequency,
    enum sig_HarmonicTaper taper) {
    struct sig_AdditiveWavetableBank* self = sig_MALLOC(allocator,
        struct sig_AdditiveWavetableBank);
    self->shapes = (struct sig_WaveformShape*) allocator->impl->malloc(
        allocator, sizeof(struct sig_WaveformShape) * numShapes);
    for (size_t i = 0; i < numShapes; i++) {
        self->shapes[i] = shapes[i];
    }
    self->numShapes = numShapes;
    self->numLevels = numLevels;
    self->tableLength = tableLength;
    self->lowestFrequency = lowestFrequency;
    self->maxHarmonicFrequency = maxHarmonicFrequency;
    self->taper = taper;
    self->levels = (struct sig_WavetableBank**) allocator->impl->malloc(
        allocator, sizeof(struct sig_WavetableBank*) * numLevels);

    for (size_t level = 0; level < numLevels; level++) {
        self->levels[level] = sig_WavetableBank_new(allocator, numShapes,
            tableLength);
    }

    struct sig_Buffer* sineTable = sig_Buffer_new(allocator, tableLength);
    sig_Buffer_fillWithWaveform(sineTable, sig_waveform_sine,
        (float) tableLength, 0.0f, 1.0f);
    sig_AdditiveWavetableBank_fill(self, sineTable);
    sig_Buffer_destroy(allocator, sineTable);

    return self;
}

size_t sig_AdditiveWavetableBank_numHarmonics(
    struct sig_AdditiveWavetableBank* self, size_t level) {
    float topOfOctave = self->lowestFrequency *
        powf(2.0f, (float) (level + 1));
    size_t numHarmonics = (size_t) (self->maxHarmonicFrequency / topOfOctave);
    size_t maxHarmonics = self->tableLength / 2 - 1;

    if (numHarmonics < 1) {
        return 1;
    }

    return numHarmonics > maxHarmonics ? maxHarmonics : numHarmonics;
}

void sig_AdditiveWavetableBank_fill(
    struct sig_AdditiveWavetableBank* self, struct sig_Buffer* sineTable) {
    for (size_t level = 0; level < self->numLevels; level++) {
        size_t numHarmonics = sig_AdditiveWavetableBank_numHarmonics(self,
            level);
        for (size_t shape = 0; shape < self->numShapes; shape++) {
            sig_Buffer_fillWithHarmonics(self->levels[level]->waves[shape],
                sineTable, &self->shapes[shape], numHarmonics, self->taper);
        }
    }
}

extern inline float sig_AdditiveWavetableBank_levelForFrequency(
    struct sig_AdditiveWavetableBank* self, float frequency) {
    float highestLevel = (float) (self->numLevels - 1);
    float level = log2f(fabsf(frequency) / self->lowestFrequency);

    return sig_clamp(level, 0.0f, highestLevel);
}

extern inline float sig_AdditiveWavetableBank_readLinearAtPhase(
    struct sig_AdditiveWavetableBank* self, float level, float tableIdx,
    float phase) {
    size_t lowerLevel = (size_t) level;
    size_t upperLevel = lowerLevel + 1 < self->numLevels ?
        lowerLevel + 1 : lowerLevel;
    float levelMix = level - (float) lowerLevel;

    float lower = sig_WavetableBank_readLinearAtPhase(
        self->levels[lowerLevel], tableIdx, phase);
    float upper = sig_WavetableBank_readLinearAtPhase(
        self->levels[upperLevel], tableIdx, phase);

    return lower + (upper - lower) * levelMix;
}

void sig_AdditiveWavetableBank_destroy(struct sig_Allocator* allocator,
    struct sig_AdditiveWavetableBank* self) {
    for (size_t level = 0; level < self->numLevels; level++) {
        sig_WavetableBank_destroy(allocator, self->levels[level]);
        self->levels[level] = NULL;
    }

    allocator->impl->free(allocator, self->levels);
    self->levels = NULL;
    allocator->impl->free(allocator, self->shapes);
    self->shapes = NULL;
    allocator->impl->free(allocator, self);
}

struct sig_DelayLine* sig_DelayLine_new(struct sig_Allocator* allocator,
    size_t maxDelayLength) {
    struct sig_DelayLine* self = sig_MALLOC(allocator, struct sig_DelayLine);
    self->buffer = sig_Buffer_new(allocator, maxDelayLength);
    sig_DelayLine_init(self);

    return self;
}

struct sig_DelayLine* sig_DelayLine_newSeconds(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings, float maxDelaySecs) {
    size_t maxDelayLength = (size_t) roundf(
        maxDelaySecs * audioSettings->sampleRate);

    return sig_DelayLine_new(allocator, maxDelayLength);
}

struct sig_DelayLine* sig_DelayLine_newWithTransferredBuffer(
    struct sig_Allocator* allocator, struct sig_Buffer* buffer) {
    struct sig_DelayLine* self = sig_MALLOC(allocator, struct sig_DelayLine);
    self->buffer = buffer;
    sig_DelayLine_init(self);

    return self;
}

void sig_DelayLine_init(struct sig_DelayLine* self) {
    self->writeIdx = 0;
    sig_Buffer_fillWithSilence(self->buffer); // Zero the delay line.
}

extern inline float sig_DelayLine_readAt(
    struct sig_DelayLine* self, size_t readPos) {
    size_t idx = (self->writeIdx + readPos) % self->buffer->length;
    return FLOAT_ARRAY(self->buffer->samples)[idx];
}

extern inline float sig_DelayLine_linearReadAt(
    struct sig_DelayLine* self, float readPos) {
    size_t maxDelayLength = self->buffer->length;
    float* delayLineSamples = self->buffer->samples;
    int32_t integ = (int32_t) readPos;
    float frac = readPos - (float) integ;
    float a = delayLineSamples[(self->writeIdx + integ) % maxDelayLength];
    float b = delayLineSamples[(self->writeIdx + integ + 1) % maxDelayLength];

    return a + (b - a) * frac;
}

extern inline float sig_DelayLine_cubicReadAt(
    struct sig_DelayLine* self, float readPos) {
    size_t maxDelayLength = self->buffer->length;
    float* delayLineSamples = self->buffer->samples;

    int32_t integ = (int32_t) readPos;
    float frac = readPos - (float) integ;
    int32_t t = (int32_t) (self->writeIdx + integ + maxDelayLength);
    float xm1 = delayLineSamples[(t - 1) % maxDelayLength];
    float x0 = delayLineSamples[t % maxDelayLength];
    float x1 = delayLineSamples[(t + 1) % maxDelayLength];
    float x2 = delayLineSamples[(t + 2) % maxDelayLength];
    float c = (x1 - xm1) * 0.5f;
    float v = x0 - x1;
    float w = c + v;
    float a = w + v + (x2 - x0) * 0.5f;
    float bNeg = w + a;

    return (((a * frac) - bNeg) * frac + c) * frac + x0;
}

extern inline float sig_DelayLine_allpassReadAt(
    struct sig_DelayLine* self, float readPos,
    float previousSample) {
    size_t maxDelayLength = self->buffer->length;
    float* delayLineSamples = self->buffer->samples;
    int32_t integ = (int32_t) readPos;
    float frac = readPos - (float) integ;
    float invFrac = 1.0f - frac;
    float a = delayLineSamples[(self->writeIdx + integ) % maxDelayLength];
    float b = delayLineSamples[(self->writeIdx + integ + 1) % maxDelayLength];

    return b + invFrac * a - invFrac * previousSample;
}

#define sig_DelayLine_readAtTime_IMPL(self, source, tapTime,\
    sampleRate, readFn)\
    float sample;\
    if (tapTime <= 0.0f) {\
        sample = source;\
    } else {\
        float readPos = tapTime * sampleRate;\
        float maxDelayLength = (float) self->buffer->length;\
        if (readPos >= maxDelayLength) {\
            readPos = maxDelayLength - 1;\
        }\
        sample = readFn(self, readPos);\
    }\
    return sample

extern inline float sig_DelayLine_readAtTime(
    struct sig_DelayLine* self, float source,
    float tapTime, float sampleRate) {
    sig_DelayLine_readAtTime_IMPL(self, source, tapTime, sampleRate,
        sig_DelayLine_readAt);
}

extern inline float sig_DelayLine_linearReadAtTime(
    struct sig_DelayLine* self, float source, float tapTime,
    float sampleRate) {
    sig_DelayLine_readAtTime_IMPL(self, source, tapTime, sampleRate,
        sig_DelayLine_linearReadAt);
}

extern inline float sig_DelayLine_cubicReadAtTime(
    struct sig_DelayLine* self, float source, float tapTime,
    float sampleRate) {
    sig_DelayLine_readAtTime_IMPL(self, source, tapTime, sampleRate,
        sig_DelayLine_cubicReadAt);
}

extern inline float sig_DelayLine_allpassReadAtTime(
    struct sig_DelayLine* self, float source, float tapTime,
    float sampleRate, float previousSample) {
    // TODO: Cut and pasted from the sig_DelayLine_readAtTime_IMPL macro above.
    float sample;
    if (tapTime <= 0.0f) {
        sample = source;
    } else {
        float readPos = tapTime * sampleRate;
        float maxDelayLength = (float) self->buffer->length;
        if (readPos >= maxDelayLength) {
            readPos = maxDelayLength - 1;
        }
        sample = sig_DelayLine_allpassReadAt(self, readPos, previousSample);
    }

    return sample;
}

#define sig_DelayLine_readAtTimes_IMPL(self, source, tapTimes,\
    tapGains, numTaps, sampleRate, timeScale, readFn)\
    const float* SIG_RESTRICT restrictTapTimes = FLOAT_ARRAY(tapTimes);\
    const float* SIG_RESTRICT restrictTapGains = FLOAT_ARRAY(tapGains);\
    float tapSum = 0;\
    for (size_t i = 0; i < numTaps; i++) {\
        float tapTime = restrictTapTimes[i];\
        float scaledTapTime = tapTime * timeScale;\
        float gain = restrictTapGains[i];\
        float sample;\
        if (tapTime <= 0.0f) {\
            sample = source;\
        } else {\
            float readPos = scaledTapTime * sampleRate;\
            float maxDelayLength = (float) self->buffer->length;\
            if (readPos >= maxDelayLength) {\
                readPos = maxDelayLength - 1;\
            }\
            sample = readFn(self, readPos);\
        }\
        tapSum += sample * gain;\
    }\
    return tapSum

extern inline float sig_DelayLine_readAtTimes(
    struct sig_DelayLine* self, float source,
    float_array_ptr SIG_RESTRICT tapTimes,
    float_array_ptr SIG_RESTRICT tapGains,
    size_t numTaps, float sampleRate, float timeScale) {
    sig_DelayLine_readAtTimes_IMPL(self, source, tapTimes,
        tapGains, numTaps, sampleRate, timeScale, sig_DelayLine_readAt);
}

extern inline float sig_DelayLine_linearReadAtTimes(
    struct sig_DelayLine* self, float source,
    float_array_ptr SIG_RESTRICT tapTimes,
    float_array_ptr SIG_RESTRICT tapGains,
    size_t numTaps, float sampleRate, float timeScale) {
    sig_DelayLine_readAtTimes_IMPL(self, source, tapTimes,
        tapGains, numTaps, sampleRate, timeScale, sig_DelayLine_linearReadAt);
}

extern inline float sig_DelayLine_cubicReadAtTimes(
    struct sig_DelayLine* self, float source,
    float_array_ptr SIG_RESTRICT tapTimes,
    float_array_ptr SIG_RESTRICT tapGains,
    size_t numTaps, float sampleRate, float timeScale) {
    sig_DelayLine_readAtTimes_IMPL(self, source, tapTimes,
        tapGains, numTaps, sampleRate, timeScale, sig_DelayLine_cubicReadAt);
}

extern inline void sig_DelayLine_write(struct sig_DelayLine* self,
    float sample) {
    size_t maxDelayLength = self->buffer->length;
    FLOAT_ARRAY(self->buffer->samples)[self->writeIdx] = sample;
    self->writeIdx = (self->writeIdx - 1 + maxDelayLength) % maxDelayLength;
}

extern inline float sig_DelayLine_calcFeedbackGain(float delayTime,
    float decayTime) {
    // Convert 60dB time in secs to feedback gain (g) coefficient
    // (also why is the equation in Dodge and Jerse wrong?)
    if (delayTime <= 0.0f || decayTime <= 0.0f) {
        return 0.0f;
    }

    return expf(sig_LOG0_001 * delayTime / decayTime);
}

extern inline float sig_DelayLine_feedback(float sample,
    float read, float g) {
    return sample + (g * read);
}

#define sig_DelayLine_comb_IMPL(self, sample, readPos, g, readFn) \
    float read = readFn(self, readPos); \
    float toWrite = sig_DelayLine_feedback(sample, read, g); \
    sig_DelayLine_write(self, toWrite); \
    return read

extern inline float sig_DelayLine_comb(struct sig_DelayLine* self,
    float sample, size_t readPos, float g) {
    sig_DelayLine_comb_IMPL(self, sample, readPos, g, sig_DelayLine_readAt);
}

extern inline float sig_DelayLine_linearComb(
    struct sig_DelayLine* self, float sample, float readPos,
    float g) {
    sig_DelayLine_comb_IMPL(self, sample, readPos, g,
        sig_DelayLine_linearReadAt);
}

extern inline float sig_DelayLine_cubicComb(
    struct sig_DelayLine* self, float sample, float readPos,
    float g) {
    sig_DelayLine_comb_IMPL(self, sample, readPos, g,
        sig_DelayLine_cubicReadAt);
}

#define sig_DelayLine_allpass_IMPL(self, sample, readPos, g,\
    readFn) \
    float read = readFn(self, readPos); \
    float toWrite = sample + (g * read); \
    sig_DelayLine_write(self, toWrite); \
    return read - (g * toWrite) \

extern inline float sig_DelayLine_allpass(
    struct sig_DelayLine* self, float sample, size_t readPos,
    float g) {
    sig_DelayLine_allpass_IMPL(self, sample, readPos, g, sig_DelayLine_readAt);
}

extern inline float sig_DelayLine_linearAllpass(
    struct sig_DelayLine* self, float sample, float readPos,
    float g) {
    sig_DelayLine_allpass_IMPL(self, sample, readPos, g,
        sig_DelayLine_linearReadAt);
}

extern inline float sig_DelayLine_cubicAllpass(
    struct sig_DelayLine* self, float sample, float readPos,
    float g) {
    sig_DelayLine_allpass_IMPL(self, sample, readPos, g,
        sig_DelayLine_cubicReadAt);
}

void sig_DelayLine_destroy(struct sig_Allocator* allocator,
    struct sig_DelayLine* self) {
    sig_Buffer_destroy(allocator, self->buffer);
    allocator->impl->free(allocator, self);
}

extern inline float sig_linearXFade(float left, float right,
    float mix) {
    float clipped = sig_clamp(mix, -1.0f, 1.0f);
    // At -1.0f, left gain should be 1.0 and right gain should be 0.0;
    // at 0.0, left and right gains should be 0.5;
    // At 1.0 left gain should be 0.0f and right gain should be 1.0.
    float gain = clipped * 0.5f + 0.5f;
    float sample = left + gain * (right - left);

    return sample;
}

extern inline float sig_sineWavefolder(float x, float gain,
    float factor) {
    float sample = x + gain * sinf(factor * x);
    return sample;
}

void sig_dsp_Signal_init(void* signal,
    struct sig_SignalContext* context,
    sig_dsp_generateFn generate) {
    struct sig_dsp_Signal* self = (struct sig_dsp_Signal*) signal;

    self->audioSettings = context->audioSettings;
    self->generate = generate;
}

void sig_dsp_Signal_noOp(void* signal) {};

// TODO: Should self be defined as a struct_dsp_Signal* in the signature?
void sig_dsp_Signal_destroy(struct sig_Allocator* allocator,
    void* self) {
    struct sig_dsp_Signal* signal = (struct sig_dsp_Signal*) self;
    allocator->impl->free(allocator, signal);
}


void sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_Signal_SingleMonoOutput* outputs) {
    outputs->main = sig_AudioBlock_newSilent(allocator, audioSettings);
}

void sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_Signal_SingleMonoOutput* outputs) {
    sig_AudioBlock_destroy(allocator, outputs->main);
}


extern inline void sig_dsp_evaluateSignals(
    struct sig_List* signalList) {
    for (size_t i = 0; i < signalList->length; i++) {
        struct sig_dsp_Signal* signal =
            (struct sig_dsp_Signal*) signalList->items[i];
        signal->generate(signal);
    }
}

struct sig_dsp_SignalListEvaluator* sig_dsp_SignalListEvaluator_new(
    struct sig_Allocator* allocator, struct sig_List* signalList) {
    struct sig_dsp_SignalListEvaluator* self = sig_MALLOC(allocator,
        struct sig_dsp_SignalListEvaluator);
    sig_dsp_SignalListEvaluator_init(self, signalList);

    return self;
}

void sig_dsp_SignalListEvaluator_init(
    struct sig_dsp_SignalListEvaluator* self,
    struct sig_List* signalList) {
    self->evaluate = sig_dsp_SignalListEvaluator_evaluate;
    self->signalList = signalList;
}

void sig_dsp_SignalListEvaluator_evaluate(
    struct sig_dsp_SignalEvaluator* evaluator) {
    struct sig_dsp_SignalListEvaluator* self =
        (struct sig_dsp_SignalListEvaluator*) evaluator;

    sig_dsp_evaluateSignals(self->signalList);
}

void sig_dsp_SignalListEvaluator_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_SignalListEvaluator* self) {
    allocator->impl->free(allocator, self);
}

struct sig_dsp_Value* sig_dsp_Value_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Value* self = sig_MALLOC(allocator, struct sig_dsp_Value);
    sig_dsp_Value_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Value_init(struct sig_dsp_Value* self,
    struct sig_SignalContext* context) {

    struct sig_dsp_Value_Parameters params = {
        .value = 1.0f
    };

    sig_dsp_Signal_init(self, context, *sig_dsp_Value_generate);

    self->parameters = params;
}

void sig_dsp_Value_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Value* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

void sig_dsp_Value_generate(void* signal) {
    struct sig_dsp_Value* self = (struct sig_dsp_Value*) signal;

    if (self->parameters.value == self->lastSample) {
        return;
    }

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float value = self->parameters.value;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = value;
    }

    self->lastSample = self->parameters.value;
}


struct sig_dsp_ConstantValue* sig_dsp_ConstantValue_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    float value) {
    struct sig_dsp_ConstantValue* self = sig_MALLOC(allocator,
        struct sig_dsp_ConstantValue);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    sig_dsp_ConstantValue_init(self, context, value);

    return self;
}

void sig_dsp_ConstantValue_init(struct sig_dsp_ConstantValue* self,
    struct sig_SignalContext* context, float value) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Signal_noOp);
    sig_fillWithValue(self->outputs.main, context->audioSettings->blockSize,
        value);
};

void sig_dsp_ConstantValue_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ConstantValue* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
};


struct sig_dsp_Abs* sig_dsp_Abs_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Abs* self = sig_MALLOC(allocator,
        struct sig_dsp_Abs);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    sig_dsp_Abs_init(self, context);

    return self;
}

void sig_dsp_Abs_init(struct sig_dsp_Abs* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Abs_generate);
    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_dsp_Abs_generate(void* signal) {
    struct sig_dsp_Abs* self = (struct sig_dsp_Abs*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = fabsf(in[i]);
    }
}

void sig_dsp_Abs_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Abs* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}



struct sig_dsp_Clamp* sig_dsp_Clamp_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Clamp* self = sig_MALLOC(allocator,
        struct sig_dsp_Clamp);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    sig_dsp_Clamp_init(self, context);

    return self;
}

void sig_dsp_Clamp_init(struct sig_dsp_Clamp* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Clamp_generate);

    self->parameters.min = 0.0f;
    self->parameters.max = 1.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_dsp_Clamp_generate(void* signal) {
    struct sig_dsp_Clamp* self = (struct sig_dsp_Clamp*) signal;
    const float min = self->parameters.min;
    const float max = self->parameters.max;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = sig_clamp(in[i], min, max);
    }
}

void sig_dsp_Clamp_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Clamp* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}



struct sig_dsp_ScaleOffset* sig_dsp_ScaleOffset_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_ScaleOffset* self = sig_MALLOC(allocator,
        struct sig_dsp_ScaleOffset);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    sig_dsp_ScaleOffset_init(self, context);

    return self;
}

void sig_dsp_ScaleOffset_init(struct sig_dsp_ScaleOffset* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_ScaleOffset_generate);

    struct sig_dsp_ScaleOffset_Parameters parameters = {
        .scale = 1.0,
        .offset= 0.0f
    };
    self->parameters = parameters;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_dsp_ScaleOffset_generate(void* signal) {
    struct sig_dsp_ScaleOffset* self = (struct sig_dsp_ScaleOffset*) signal;

    const float scale = self->parameters.scale;
    const float offset = self->parameters.offset;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = in[i] * scale + offset;
    }
}

void sig_dsp_ScaleOffset_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ScaleOffset* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_dsp_Sine* sig_dsp_Sine_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Sine* self = sig_MALLOC(allocator,
        struct sig_dsp_Sine);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    sig_dsp_Sine_init(self, context);

    return self;
};

void sig_dsp_Sine_init(struct sig_dsp_Sine* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Sine_generate);

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_dsp_Sine_generate(void* signal) {
    struct sig_dsp_Sine* self = (struct sig_dsp_Sine*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = sinf(in[i]);
    }
}

void sig_dsp_Sine_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Sine* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_dsp_BinaryOp* sig_dsp_BinaryOp_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    sig_dsp_generateFn generate) {
    struct sig_dsp_BinaryOp* self = sig_MALLOC(allocator,
        struct sig_dsp_BinaryOp);
    sig_dsp_BinaryOp_init(self, context, generate);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_BinaryOp_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context,
    sig_dsp_generateFn generate) {
    sig_dsp_Signal_init(self, context, generate);

    sig_CONNECT_TO_SILENCE(self, left, context);
    sig_CONNECT_TO_SILENCE(self, right, context);
}

void sig_dsp_BinaryOp_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

struct sig_dsp_BinaryOp* sig_dsp_Add_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    return sig_dsp_BinaryOp_new(allocator, context, *sig_dsp_Add_generate);
}

void sig_dsp_Add_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context) {
    sig_dsp_BinaryOp_init(self, context, *sig_dsp_Add_generate);
}

// TODO: Unit tests.
void sig_dsp_Add_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT left = FLOAT_ARRAY(self->inputs.left);
    const float* SIG_RESTRICT right = FLOAT_ARRAY(self->inputs.right);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = left[i] + right[i];
    }
}

void sig_dsp_Add_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_BinaryOp_destroy(allocator, self);
}



struct sig_dsp_BinaryOp* sig_dsp_Sub_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    return sig_dsp_BinaryOp_new(allocator, context, *sig_dsp_Sub_generate);
}

void sig_dsp_Sub_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context) {
    sig_dsp_BinaryOp_init(self, context, *sig_dsp_Sub_generate);
}

// TODO: Unit tests.
void sig_dsp_Sub_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT left = FLOAT_ARRAY(self->inputs.left);
    const float* SIG_RESTRICT right = FLOAT_ARRAY(self->inputs.right);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = left[i] - right[i];
    }
}

void sig_dsp_Sub_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_BinaryOp_destroy(allocator, self);
}



struct sig_dsp_BinaryOp* sig_dsp_Mul_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    return sig_dsp_BinaryOp_new(allocator, context, *sig_dsp_Mul_generate);
}

void sig_dsp_Mul_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context) {
    sig_dsp_BinaryOp_init(self, context, *sig_dsp_Mul_generate);
};

void sig_dsp_Mul_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_BinaryOp_destroy(allocator, self);
};

void sig_dsp_Mul_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT left = FLOAT_ARRAY(self->inputs.left);
    const float* SIG_RESTRICT right = FLOAT_ARRAY(self->inputs.right);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = left[i] * right[i];
    }
}


struct sig_dsp_BinaryOp* sig_dsp_Div_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    return sig_dsp_BinaryOp_new(allocator, context, *sig_dsp_Div_generate);
}

void sig_dsp_Div_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context) {
    sig_dsp_BinaryOp_init(self, context, *sig_dsp_Div_generate);
}

void sig_dsp_Div_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT left = FLOAT_ARRAY(self->inputs.left);
    const float* SIG_RESTRICT right = FLOAT_ARRAY(self->inputs.right);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = left[i] / right[i];
    }
}

void sig_dsp_Div_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_BinaryOp_destroy(allocator, self);
}


struct sig_dsp_Invert* sig_dsp_Invert_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Invert* self = sig_MALLOC(allocator, struct sig_dsp_Invert);
    sig_dsp_Invert_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Invert_init(struct sig_dsp_Invert* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Invert_generate);
    self->inputs.source = context->silence->outputs.main;
}

// TODO: Unit tests.
void sig_dsp_Invert_generate(void* signal) {
    struct sig_dsp_Invert* self = (struct sig_dsp_Invert*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = -in[i];
    }
}

void sig_dsp_Invert_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Invert* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_Accumulate* sig_dsp_Accumulate_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Accumulate* self = sig_MALLOC(allocator,
        struct sig_dsp_Accumulate);
    sig_dsp_Accumulate_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    return self;
}

void sig_dsp_Accumulate_init(struct sig_dsp_Accumulate* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Accumulate_generate);

    struct sig_dsp_Accumulate_Parameters parameters = {
        .accumulatorStart = 1.0f,
        .wrap = 0.0f,
        .maxValue = 1.0f
    };
    self->parameters = parameters;

    // FIXME: This happens too early for users to
    // override the parameter, and thus the accumulator
    // is always initialized to 1.
    self->accumulator = parameters.accumulatorStart;
    self->previousReset = 0.0f;

    self->inputs.source = context->silence->outputs.main;
    self->inputs.reset = context->silence->outputs.main;
}

// TODO: Implement an audio rate version of this signal.
// TODO: Unit tests
void sig_dsp_Accumulate_generate(void* signal) {
    struct sig_dsp_Accumulate* self =
        (struct sig_dsp_Accumulate*) signal;

    self->accumulator += FLOAT_ARRAY(self->inputs.source)[0];

    float reset = FLOAT_ARRAY(self->inputs.reset)[0];
    const float wrap = self->parameters.wrap;
    const float maxValue = self->parameters.maxValue;

    // Reset the accumulator if we received a trigger
    // or if we're wrapping and we've gone past the min/max value.
    if (reset > 0.0f && self->previousReset <= 0.0f) {
        self->accumulator = self->parameters.accumulatorStart;
    } else if (wrap > 0.0f) {
        if (self->accumulator > maxValue) {
            self->accumulator = self->parameters.accumulatorStart;
        } else if (self->accumulator < self->parameters.accumulatorStart) {
            self->accumulator = self->parameters.maxValue;
        }
    }

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = self->accumulator;
    }

    self->previousReset = reset;
}

void sig_dsp_Accumulate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Accumulate* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_dsp_GatedTimer* sig_dsp_GatedTimer_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_GatedTimer* self = sig_MALLOC(allocator,
        struct sig_dsp_GatedTimer);
    sig_dsp_GatedTimer_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    return self;
}

void sig_dsp_GatedTimer_init(struct sig_dsp_GatedTimer* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_GatedTimer_generate);

    self->timer = 0;
    self->hasFired = false;
    self->prevGate = 0.0f;

    sig_CONNECT_TO_SILENCE(self, gate, context);
    sig_CONNECT_TO_SILENCE(self, duration, context);
    sig_CONNECT_TO_SILENCE(self, loop, context);
}

// TODO: Unit tests
void sig_dsp_GatedTimer_generate(void* signal) {
    struct sig_dsp_GatedTimer* self =
        (struct sig_dsp_GatedTimer*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT durationIn = FLOAT_ARRAY(self->inputs.duration);
    const float* SIG_RESTRICT gateIn = FLOAT_ARRAY(self->inputs.gate);
    const float* SIG_RESTRICT loopIn = FLOAT_ARRAY(self->inputs.loop);

    for (size_t i = 0; i < blockSize; i++) {
        // TODO: MSVC compiler warning loss of precision.
        unsigned long durationSamps = (unsigned long)
            (durationIn[i] *
            sampleRate);
        float gate = gateIn[i];

        if (gate > 0.0f) {
            // Gate is open.
            if (!self->hasFired ||
                loopIn[i] > 0.0f) {
                self->timer++;
            }

            if (self->timer >= durationSamps) {
                // We reached the duration time.
                out[i] = 1.0f;

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

        out[i] = 0.0f;
        self->prevGate = gate;
    }
}

void sig_dsp_GatedTimer_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_GatedTimer* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

struct sig_dsp_TimedTriggerCounter* sig_dsp_TimedTriggerCounter_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_TimedTriggerCounter* self = sig_MALLOC(allocator,
        struct sig_dsp_TimedTriggerCounter);
    sig_dsp_TimedTriggerCounter_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_TimedTriggerCounter_init(
    struct sig_dsp_TimedTriggerCounter* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_TimedTriggerCounter_generate);

    self->numTriggers = 0;
    self->timer = 0;
    self->isTimerActive = false;
    self->previousSource = 0.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, duration, context);
    sig_CONNECT_TO_SILENCE(self, count, context);
}

void sig_dsp_TimedTriggerCounter_generate(void* signal) {
    struct sig_dsp_TimedTriggerCounter* self =
        (struct sig_dsp_TimedTriggerCounter*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT sourceIn = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT durationIn = FLOAT_ARRAY(self->inputs.duration);
    const float* SIG_RESTRICT countIn = FLOAT_ARRAY(self->inputs.count);

    for (size_t i = 0; i < blockSize; i++) {
        float source = sourceIn[i];
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
            long durSamps = (long) (durationIn[i] *
                sampleRate);

            if (self->timer >= durSamps) {
                // Time's up.
                // Fire a trigger if we've the right number of
                // incoming triggers, otherwise just reset.
                if (self->numTriggers ==
                    (int) countIn[i]) {
                    outputSample = 1.0f;
                }

                self->isTimerActive = false;
                self->numTriggers = 0;
                self->timer = 0;
            }
        }

        self->previousSource = source;
        out[i] = outputSample;
    }
}

void sig_dsp_TimedTriggerCounter_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_TimedTriggerCounter* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_dsp_ToggleGate* sig_dsp_ToggleGate_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_ToggleGate* self = sig_MALLOC(allocator,
        struct sig_dsp_ToggleGate);
    sig_dsp_ToggleGate_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_ToggleGate_init(struct sig_dsp_ToggleGate* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_ToggleGate_generate);

    self->isGateOpen = false;
    self->prevTrig = 0.0f;

    sig_CONNECT_TO_SILENCE(self, trigger, context);
}

// TODO: Unit tests
void sig_dsp_ToggleGate_generate(void* signal) {
    struct sig_dsp_ToggleGate* self =
        (struct sig_dsp_ToggleGate*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT trigger = FLOAT_ARRAY(self->inputs.trigger);
    for (size_t i = 0; i < blockSize; i++) {
        float trig = trigger[i];
        if (trig > 0.0f && self->prevTrig <= 0.0f) {
            // Received a trigger, toggle the gate.
            self->isGateOpen = !self->isGateOpen;
        }

        out[i] = (float) self->isGateOpen;

        self->prevTrig = trig;
    }
}

void sig_dsp_ToggleGate_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_ToggleGate* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

void sig_dsp_Oscillator_Outputs_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_Oscillator_Outputs* outputs) {
    outputs->main = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->eoc = sig_AudioBlock_newSilent(allocator, audioSettings);
}

void sig_dsp_Oscillator_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator_Outputs* outputs) {
    sig_AudioBlock_destroy(allocator, outputs->main);
    outputs->main = NULL;
    sig_AudioBlock_destroy(allocator, outputs->eoc);
    outputs->eoc = NULL;
}

struct sig_dsp_Oscillator* sig_dsp_Oscillator_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    sig_dsp_generateFn generate) {
    struct sig_dsp_Oscillator* self = sig_MALLOC(allocator,
        struct sig_dsp_Oscillator);
    sig_dsp_Oscillator_init(self, context, generate);
    sig_dsp_Oscillator_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Oscillator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self) {
    sig_dsp_Oscillator_Outputs_destroyAudioBlocks(allocator, &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

void sig_dsp_Oscillator_init(struct sig_dsp_Oscillator* self,
    struct sig_SignalContext* context,
    sig_dsp_generateFn generate) {
    sig_dsp_Signal_init(self, context, generate);

    sig_CONNECT_TO_SILENCE(self, freq, context);
    sig_CONNECT_TO_SILENCE(self, phaseOffset, context);
    sig_CONNECT_TO_UNITY(self, mul, context);
    sig_CONNECT_TO_SILENCE(self, add, context);

    sig_osc_Oscillator_init(&self->state);
}

void sig_dsp_SineOscillator_init(struct sig_dsp_Oscillator* self,
    struct sig_SignalContext* context) {
    sig_dsp_Oscillator_init(self, context, *sig_dsp_SineOscillator_generate);
}

struct sig_dsp_Oscillator* sig_dsp_SineOscillator_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    return sig_dsp_Oscillator_new(allocator, context, *sig_dsp_SineOscillator_generate);
}

void sig_dsp_SineOscillator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self) {
    sig_dsp_Oscillator_destroy(allocator, self);
}

void sig_dsp_SineOscillator_generate(void* signal) {
    struct sig_dsp_Oscillator* self = (struct sig_dsp_Oscillator*) signal;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    const float* SIG_RESTRICT frequency = FLOAT_ARRAY(self->inputs.freq);
    const float* SIG_RESTRICT phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    const float* SIG_RESTRICT mul = FLOAT_ARRAY(self->inputs.mul);
    const float* SIG_RESTRICT add = FLOAT_ARRAY(self->inputs.add);
    float* SIG_RESTRICT mainOutput = FLOAT_ARRAY(self->outputs.main);
    float* SIG_RESTRICT eocOutput = FLOAT_ARRAY(self->outputs.eoc);

    for (size_t i = 0; i < blockSize; i++) {
        float modulatedPhase = self->state.phaseAccumulator +
            phaseOffset[i];
        float eoc = sig_osc_Oscillator_eoc(modulatedPhase);
        modulatedPhase = sig_osc_Oscillator_wrapPhase(modulatedPhase);

        float angularPhase = modulatedPhase * sig_TWOPI;
        float sample = sinf(angularPhase);
        float scaledSample = sample * mul[i] + add[i];

        mainOutput[i] = scaledSample;
        eocOutput[i] = eoc;

        sig_osc_Oscillator_accumulatePhase(&self->state.phaseAccumulator,
            frequency[i], sampleRate);
    }
}


void sig_dsp_FastLFSineOscillator_init(
    struct sig_dsp_FastLFSineOscillator* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_FastLFSineOscillator_generate);

    sig_CONNECT_TO_SILENCE(self, frequency, context);
    sig_CONNECT_TO_UNITY(self, mul, context);
    sig_CONNECT_TO_SILENCE(self, add, context);

    sig_osc_FastLFSine_init(&self->state,
        self->signal.audioSettings->sampleRate);
}

struct sig_dsp_FastLFSineOscillator* sig_dsp_FastLFSineOscillator_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_FastLFSineOscillator* self = sig_MALLOC(allocator,
        struct sig_dsp_FastLFSineOscillator);
    sig_dsp_FastLFSineOscillator_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_FastLFSineOscillator_generate(void* signal) {
    struct sig_dsp_FastLFSineOscillator* self =
        (struct sig_dsp_FastLFSineOscillator*) signal;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float* SIG_RESTRICT frequency = FLOAT_ARRAY(self->inputs.frequency);
    const float* SIG_RESTRICT mul = FLOAT_ARRAY(self->inputs.mul);
    const float* SIG_RESTRICT add = FLOAT_ARRAY(self->inputs.add);
    float* SIG_RESTRICT mainOutput = FLOAT_ARRAY(self->outputs.main);

    for (size_t i = 0; i < blockSize; i++) {
        sig_osc_FastLFSine_setFrequencyFast(&self->state, frequency[i]);
        sig_osc_FastLFSine_generate(&self->state);
        float sample = self->state.sinZ;
        float scaledSample = sample * mul[i] + add[i];
        mainOutput[i] = scaledSample;
    }
}

void sig_dsp_FastLFSineOscillator_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_FastLFSineOscillator* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
    self = NULL;
}


void sig_dsp_LFTriangle_init(struct sig_dsp_Oscillator* self,
    struct sig_SignalContext* context) {
    sig_dsp_Oscillator_init(self, context, *sig_dsp_LFTriangle_generate);
};

struct sig_dsp_Oscillator* sig_dsp_LFTriangle_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    return sig_dsp_Oscillator_new(allocator, context,
        *sig_dsp_LFTriangle_generate);
}

void sig_dsp_LFTriangle_generate(void* signal) {
    struct sig_dsp_Oscillator* self = (struct sig_dsp_Oscillator*) signal;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    const float* SIG_RESTRICT frequency = FLOAT_ARRAY(self->inputs.freq);
    const float* SIG_RESTRICT phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    const float* SIG_RESTRICT mul = FLOAT_ARRAY(self->inputs.mul);
    const float* SIG_RESTRICT add = FLOAT_ARRAY(self->inputs.add);
    float* SIG_RESTRICT mainOutput = FLOAT_ARRAY(self->outputs.main);
    float* SIG_RESTRICT eocOutput = FLOAT_ARRAY(self->outputs.eoc);

    for (size_t i = 0; i < blockSize; i++) {
        float modulatedPhase = self->state.phaseAccumulator +
            phaseOffset[i];
        float eoc = sig_osc_Oscillator_eoc(modulatedPhase);
        modulatedPhase = sig_osc_Oscillator_wrapPhase(modulatedPhase) *
            sig_TWOPI;

        float sample = -1.0f + (2.0f * (modulatedPhase * sig_RECIP_TWOPI));
        sample = 2.0f * (fabsf(sample) - 0.5f); // Rectify and scale/offset
        float scaledSample = sample * mul[i] + add[i];

        mainOutput[i] = scaledSample;
        eocOutput[i] = eoc;
        sig_osc_Oscillator_accumulatePhase(&self->state.phaseAccumulator,
            frequency[i], sampleRate);
    }
}

void sig_dsp_LFTriangle_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self) {
    sig_dsp_Oscillator_destroy(allocator, self);
}


void sig_dsp_WavetableOscillator_init(
    struct sig_dsp_WavetableOscillator* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_WavetableOscillator_generate);

    sig_CONNECT_TO_SILENCE(self, freq, context);
    sig_CONNECT_TO_SILENCE(self, phaseOffset, context);
    sig_CONNECT_TO_UNITY(self, mul, context);
    sig_CONNECT_TO_SILENCE(self, add, context);

    sig_osc_Wavetable_init(&self->state, self->wavetable);
}

struct sig_dsp_WavetableOscillator* sig_dsp_WavetableOscillator_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_WavetableOscillator* self = sig_MALLOC(allocator,
        struct sig_dsp_WavetableOscillator);
    sig_dsp_WavetableOscillator_init(self, context);
    sig_dsp_Oscillator_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_WavetableOscillator_generate(void* signal) {
    struct sig_dsp_WavetableOscillator* self =
        (struct sig_dsp_WavetableOscillator*) signal;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    const float* SIG_RESTRICT frequency = FLOAT_ARRAY(self->inputs.freq);
    const float* SIG_RESTRICT phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    const float* SIG_RESTRICT mul = FLOAT_ARRAY(self->inputs.mul);
    const float* SIG_RESTRICT add = FLOAT_ARRAY(self->inputs.add);
    float* SIG_RESTRICT eocOutput = FLOAT_ARRAY(self->outputs.eoc);
    float* SIG_RESTRICT mainOutput = FLOAT_ARRAY(self->outputs.main);

    // TODO: This has to be assigned because we don't know when
    // it is been updated by the user.
    self->state.wavetable = self->wavetable;

    for (size_t i = 0; i < blockSize; i++) {
        float sample = sig_osc_Wavetable_generate(&self->state,
            frequency[i], phaseOffset[i], sampleRate, &eocOutput[i]);
        float scaledSample = sample * mul[i] + add[i];

        mainOutput[i] = scaledSample;
    }
}

void sig_dsp_WavetableOscillator_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_WavetableOscillator* self) {
    sig_dsp_Oscillator_Outputs_destroyAudioBlocks(allocator, &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
    self = NULL;
}



void sig_dsp_WavetableBankOscillator_init(
    struct sig_dsp_WavetableBankOscillator* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_WavetableBankOscillator_generate);

    sig_CONNECT_TO_SILENCE(self, freq, context);
    sig_CONNECT_TO_SILENCE(self, phaseOffset, context);
    sig_CONNECT_TO_UNITY(self, mul, context);
    sig_CONNECT_TO_SILENCE(self, add, context);
    sig_CONNECT_TO_SILENCE(self, tableIndex, context);

    sig_osc_WavetableBank_init(&self->state, self->wavetables);
}

struct sig_dsp_WavetableBankOscillator* sig_dsp_WavetableBankOscillator_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_WavetableBankOscillator* self = sig_MALLOC(allocator,
        struct sig_dsp_WavetableBankOscillator);
    sig_dsp_WavetableBankOscillator_init(self, context);
    sig_dsp_Oscillator_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

extern inline void sig_dsp_WavetableBankOscillator_generate(
    void* signal) {
    struct sig_dsp_WavetableBankOscillator* self =
        (struct sig_dsp_WavetableBankOscillator*) signal;
    self->state.wavetables = self->wavetables;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    const float* SIG_RESTRICT frequency = FLOAT_ARRAY(self->inputs.freq);
    const float* SIG_RESTRICT phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    const float* SIG_RESTRICT tableIndex = FLOAT_ARRAY(self->inputs.tableIndex);
    const float* SIG_RESTRICT mul = FLOAT_ARRAY(self->inputs.mul);
    const float* SIG_RESTRICT add = FLOAT_ARRAY(self->inputs.add);
    float* SIG_RESTRICT eocOutput = FLOAT_ARRAY(self->outputs.eoc);
    float* SIG_RESTRICT mainOutput = FLOAT_ARRAY(self->outputs.main);
    for (size_t i = 0; i < blockSize; i++) {
        float sample = sig_osc_WavetableBank_generate(&self->state,
            frequency[i], phaseOffset[i], tableIndex[i], sampleRate,
            &eocOutput[i]);
        float scaledSample = sample * mul[i] + add[i];

        mainOutput[i] = scaledSample;
    }
}

void sig_dsp_WavetableBankOscillator_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_WavetableBankOscillator* self) {
    sig_dsp_Oscillator_Outputs_destroyAudioBlocks(allocator, &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
    self = NULL;
}

void sig_dsp_AdditiveWavetableBankOscillator_init(
    struct sig_dsp_AdditiveWavetableBankOscillator* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context,
        *sig_dsp_AdditiveWavetableBankOscillator_generate);
    self->wavetables = NULL;
    sig_osc_AdditiveWavetableBank_init(&self->state, self->wavetables);

    sig_CONNECT_TO_SILENCE(self, freq, context);
    sig_CONNECT_TO_SILENCE(self, phaseOffset, context);
    sig_CONNECT_TO_UNITY(self, mul, context);
    sig_CONNECT_TO_SILENCE(self, add, context);
    sig_CONNECT_TO_SILENCE(self, tableIndex, context);
}

struct sig_dsp_AdditiveWavetableBankOscillator*
    sig_dsp_AdditiveWavetableBankOscillator_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_AdditiveWavetableBankOscillator* self = sig_MALLOC(
        allocator, struct sig_dsp_AdditiveWavetableBankOscillator);
    sig_dsp_AdditiveWavetableBankOscillator_init(self, context);
    sig_dsp_Oscillator_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

extern inline void sig_dsp_AdditiveWavetableBankOscillator_generate(
    void* signal) {
    struct sig_dsp_AdditiveWavetableBankOscillator* self =
        (struct sig_dsp_AdditiveWavetableBankOscillator*) signal;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    const float* SIG_RESTRICT frequency = FLOAT_ARRAY(self->inputs.freq);
    const float* SIG_RESTRICT phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    const float* SIG_RESTRICT tableIndex = FLOAT_ARRAY(self->inputs.tableIndex);
    const float* SIG_RESTRICT mul = FLOAT_ARRAY(self->inputs.mul);
    const float* SIG_RESTRICT add = FLOAT_ARRAY(self->inputs.add);
    float* SIG_RESTRICT eocOutput = FLOAT_ARRAY(self->outputs.eoc);
    float* SIG_RESTRICT mainOutput = FLOAT_ARRAY(self->outputs.main);

    // The wavetables are assigned after construction,
    // so they have to be read on every block.
    self->state.wavetables = self->wavetables;

    for (size_t i = 0; i < blockSize; i++) {
        float sample = sig_osc_AdditiveWavetableBank_generate(&self->state,
            frequency[i], phaseOffset[i], tableIndex[i], sampleRate,
            &eocOutput[i]);
        mainOutput[i] = sample * mul[i] + add[i];
    }
}

void sig_dsp_AdditiveWavetableBankOscillator_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_AdditiveWavetableBankOscillator* self) {
    sig_dsp_Oscillator_Outputs_destroyAudioBlocks(allocator, &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


void sig_dsp_Smooth_init(struct sig_dsp_Smooth* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Smooth_generate);
    self->previousTime = -1.0f;
    self->previousSample = 0.0f;
    self->parameters.time = 0.01f;
    self->a1 = 0.0f;
    sig_CONNECT_TO_SILENCE(self, source, context);
}

struct sig_dsp_Smooth* sig_dsp_Smooth_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Smooth* self = sig_MALLOC(allocator,
        struct sig_dsp_Smooth);
    sig_dsp_Smooth_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

// TODO: Unit tests
void sig_dsp_Smooth_generate(void* signal) {
    struct sig_dsp_Smooth* self = (struct sig_dsp_Smooth*) signal;

    float previousSample = self->previousSample;
    float a1 = self->a1;
    float time = self->parameters.time;

    if (self->previousTime != time) {
        self->a1 = a1 = sig_filter_smooth_calculateCoefficient(time,
            self->signal.audioSettings->sampleRate);
        self->previousTime = time;
    }

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = previousSample =
            sig_filter_smooth(
                in[i], previousSample, a1);
    }

    self->previousSample = previousSample;
}

void sig_dsp_Smooth_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Smooth* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


void sig_dsp_DCBlock_init(struct sig_dsp_DCBlock* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_DCBlock_generate);
    self->previousFrequency = 0.0f;
    self->parameters.frequency = 2.0f;
    sig_filter_DCBlock_init(&self->state, self->parameters.frequency,
        context->audioSettings->sampleRate);

    sig_CONNECT_TO_SILENCE(self, source, context);
}

struct sig_dsp_DCBlock* sig_dsp_DCBlock_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_DCBlock* self = sig_MALLOC(allocator,
        struct sig_dsp_DCBlock);
    sig_dsp_DCBlock_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_DCBlock_generate(void* signal) {
    struct sig_dsp_DCBlock* self = (struct sig_dsp_DCBlock*) signal;

    if (self->parameters.frequency != self->previousFrequency) {
        self->state.r = sig_filter_dcBlock_calculateCoefficient(
            self->parameters.frequency,
            self->signal.audioSettings->sampleRate);
        self->previousFrequency = self->parameters.frequency;
    }

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        float outputSample = sig_filter_DCBlock_generate(&self->state,
            in[i]);

        out[i] = outputSample;
    }
}

void sig_dsp_DCBlock_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_DCBlock* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}



void sig_dsp_OnePole_init(struct sig_dsp_OnePole* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_OnePole_generate);
    self->parameters.mode = sig_dsp_OnePole_Mode_LOW_PASS;
    self->a1 = 0.0f;
    self->b0 = 1.0f;
    self->previousFrequency = 0.0f;
    self->previousMode = sig_dsp_OnePole_Mode_NOT_SPECIFIED;
    self->previousSample = 0.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, frequency, context);
}

struct sig_dsp_OnePole* sig_dsp_OnePole_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_OnePole* self = sig_MALLOC(allocator,
        struct sig_dsp_OnePole);
    sig_dsp_OnePole_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

extern inline void sig_dsp_OnePole_recalculateCoefficients(
    struct sig_dsp_OnePole* self, float frequency) {
    float sampleRate = self->signal.audioSettings->sampleRate;
    enum sig_dsp_OnePole_Mode mode = self->parameters.mode;

    self->previousFrequency = frequency;
    self->previousMode = mode;

    if (mode == sig_dsp_OnePole_Mode_HIGH_PASS) {
        self->a1 = sig_filter_onepole_HPF_calculateA1(frequency, sampleRate);
        self->b0 = sig_filter_onepole_HPF_calculateB0(self->a1);
    } else {
        self->a1 = sig_filter_onepole_LPF_calculateA1(frequency, sampleRate);
        self->b0 = sig_filter_onepole_LPF_calculateB0(self->a1);
    }
}

void sig_dsp_OnePole_generate(void* signal) {
    struct sig_dsp_OnePole* self = (struct sig_dsp_OnePole*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT freqIn = FLOAT_ARRAY(self->inputs.frequency);
    float previousSample = self->previousSample;
    for (size_t i = 0; i < blockSize; i++) {
        float frequency = freqIn[i];
        if (self->previousFrequency != frequency ||
            self->previousMode != self->parameters.mode) {
            sig_dsp_OnePole_recalculateCoefficients(self, frequency);
        }

        float sample = sig_filter_onepole(in[i],
            previousSample, self->b0, self->a1);
        out[i] = sample;
        previousSample = sample;

    }
    self->previousSample = previousSample;
}

void sig_dsp_OnePole_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_OnePole* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}



void sig_dsp_EMA_init(struct sig_dsp_EMA* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_EMA_generate);
    self->parameters.alpha = 0.1f;
    self->previousSample = 0.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

struct sig_dsp_EMA* sig_dsp_EMA_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_EMA* self = sig_MALLOC(allocator,
        struct sig_dsp_EMA);
    sig_dsp_EMA_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;

}
void sig_dsp_EMA_generate(void* signal) {
    struct sig_dsp_EMA* self = (struct sig_dsp_EMA*) signal;
    float previousSample = self->previousSample;
    const float alpha = self->parameters.alpha;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        float sample = sig_filter_ema(in[i], previousSample, alpha);
        out[i] = sample;
        previousSample = sample;
    }

    self->previousSample = previousSample;
}

void sig_dsp_EMA_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_EMA* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}



void sig_dsp_Tanh_init(struct sig_dsp_Tanh* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Tanh_generate);

    sig_CONNECT_TO_SILENCE(self, source, context);
}

struct sig_dsp_Tanh* sig_dsp_Tanh_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Tanh* self = sig_MALLOC(allocator, struct sig_dsp_Tanh);
    sig_dsp_Tanh_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

// TODO: Unit tests.
void sig_dsp_Tanh_generate(void* signal) {
    struct sig_dsp_Tanh* self = (struct sig_dsp_Tanh*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = tanhf(in[i]);
    }
}

void sig_dsp_Tanh_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Tanh* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_Dust_init(struct sig_dsp_Dust* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Dust_generate);

    struct sig_dsp_Dust_Parameters parameters = {
        .bipolar = 0.0
    };

    self->parameters = parameters;
    self->sampleDuration = 1.0 / context->audioSettings->sampleRate;
    self->previousDensity = 0.0;
    self->threshold = 0.0;

    sig_CONNECT_TO_SILENCE(self, density, context);
}

struct sig_dsp_Dust* sig_dsp_Dust_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {

    struct sig_dsp_Dust* self = sig_MALLOC(allocator, struct sig_dsp_Dust);
    sig_dsp_Dust_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Dust_generate(void* signal) {
    struct sig_dsp_Dust* self = (struct sig_dsp_Dust*) signal;

    const float scaleDiv = self->parameters.bipolar > 0.0f ? 2.0f : 1.0f;
    const float scaleSub = self->parameters.bipolar > 0.0f ? 1.0f : 0.0f;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT densityIn = FLOAT_ARRAY(self->inputs.density);

    for (size_t i = 0; i < blockSize; i++) {
        float density = densityIn[i];

        if (density != self->previousDensity) {
            self->previousDensity = density;
            self->threshold = density * self->sampleDuration;
            self->scale = self->threshold > 0.0f ?
                scaleDiv / self->threshold : 0.0f;
        }

        float rand = sig_randf();
        float val = rand < self->threshold ?
            rand * self->scale - scaleSub : 0.0f;
        out[i] = val;
    }
}

void sig_dsp_Dust_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Dust* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_TimedGate_init(struct sig_dsp_TimedGate* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_TimedGate_generate);

    struct sig_dsp_TimedGate_Parameters parameters = {
        .resetOnTrigger = 0.0,
        .bipolar = 0.0
    };

    self->parameters = parameters;
    self->previousTrigger = 0.0f;
    self->previousDuration = 0.0f;
    self->gateValue = 0.0f;
    self->durationSamps = 0;
    self->samplesRemaining = 0;

    sig_CONNECT_TO_SILENCE(self, trigger, context);
    sig_CONNECT_TO_SILENCE(self, duration, context);
}

struct sig_dsp_TimedGate* sig_dsp_TimedGate_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_TimedGate* self = sig_MALLOC(allocator,
        struct sig_dsp_TimedGate);
    sig_dsp_TimedGate_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

static inline void sig_dsp_TimedGate_outputHigh(
    struct sig_dsp_TimedGate* self, float* SIG_RESTRICT out,
    size_t index) {
    out[index] = self->gateValue;
    self->samplesRemaining--;
}

static inline void sig_dsp_TimedGate_outputLow(
    struct sig_dsp_TimedGate* self, float* SIG_RESTRICT out,
    size_t index) {
    out[index] = 0.0f;
}

void sig_dsp_TimedGate_generate(void* signal) {
    struct sig_dsp_TimedGate* self = (struct sig_dsp_TimedGate*)
        signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT triggerIn = FLOAT_ARRAY(self->inputs.trigger);
    const float* SIG_RESTRICT durationIn = FLOAT_ARRAY(self->inputs.duration);

    for (size_t i = 0; i < blockSize; i++) {
        float currentTrigger = triggerIn[i];
        float duration = durationIn[i];

        if ((currentTrigger > 0.0f && self->previousTrigger <= 0.0f) ||
            (self->parameters.bipolar > 0.0 && currentTrigger < 0.0f && self->previousTrigger >= 0.0f)) {
            // A new trigger was received.
            self->gateValue = currentTrigger;

            if (duration != self->previousDuration) {
                // The duration input has changed.
                self->durationSamps = lroundf(duration *
                    sampleRate);
                self->previousDuration = duration;
            }

            if (self->parameters.resetOnTrigger > 0.0f &&
                self->samplesRemaining > 0) {
                // Gate is open and needs to be reset.
                // Close the gate for one sample,
                // and don't count down the duration
                // until next time.
                sig_dsp_TimedGate_outputLow(self, out, i);
                self->samplesRemaining = self->durationSamps;
            } else {
                self->samplesRemaining = self->durationSamps;
                sig_dsp_TimedGate_outputHigh(self, out, i);
            }
        } else if (self->samplesRemaining > 0) {
            sig_dsp_TimedGate_outputHigh(self, out, i);
        } else {
            sig_dsp_TimedGate_outputLow(self, out, i);
        }

        self->previousTrigger = currentTrigger;
    }
}

void sig_dsp_TimedGate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_TimedGate* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_DustGate* sig_dsp_DustGate_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_DustGate* self = sig_MALLOC(allocator,
        struct sig_dsp_DustGate);

    self->reciprocalDensity = sig_dsp_Div_new(allocator, context);
    self->densityDurationMultiplier = sig_dsp_Mul_new(allocator, context);
    self->dust = sig_dsp_Dust_new(allocator, context);
    self->gate = sig_dsp_TimedGate_new(allocator, context);
    sig_dsp_DustGate_init(self, context);

    return self;
}


void sig_dsp_DustGate_init(struct sig_dsp_DustGate* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_DustGate_generate);
    self->reciprocalDensity->inputs.left = context->unity->outputs.main;
    self->densityDurationMultiplier->inputs.left =
        self->reciprocalDensity->outputs.main;
    self->gate->inputs.trigger = self->dust->outputs.main;
    self->gate->inputs.duration =
        self->densityDurationMultiplier->outputs.main;
    self->outputs.main = self->gate->outputs.main;

    sig_CONNECT_TO_SILENCE(self, density, context);
    sig_CONNECT_TO_SILENCE(self, durationPercentage, context);
}

void sig_dsp_DustGate_generate(void* signal) {
    struct sig_dsp_DustGate* self = (struct sig_dsp_DustGate*) signal;

    // Bind all parameters (remove this when we have change events).
    self->dust->inputs.density = self->inputs.density;
    self->reciprocalDensity->inputs.right = self->inputs.density;
    self->densityDurationMultiplier->inputs.right =
        self->inputs.durationPercentage;
    self->dust->parameters.bipolar = self->parameters.bipolar;
    self->gate->parameters.bipolar = self->parameters.bipolar;

    // Evaluate all signals.
    self->reciprocalDensity->signal.generate(self->reciprocalDensity);
    self->densityDurationMultiplier->signal.generate(
        self->densityDurationMultiplier);
    self->dust->signal.generate(self->dust);
    self->gate->signal.generate(self->gate);
}


void sig_dsp_DustGate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_DustGate* self) {
    sig_dsp_TimedGate_destroy(allocator, self->gate);

    sig_dsp_Mul_destroy(allocator, self->densityDurationMultiplier);
    sig_AudioBlock_destroy(allocator, self->reciprocalDensity->inputs.left);
    sig_dsp_Div_destroy(allocator, self->reciprocalDensity);

    sig_dsp_Dust_destroy(allocator, self->dust);

    // We don't call sig_dsp_Signal_destroy
    // because our output is borrowed from self->gate,
    // and it was already freed in TimeGate's destructor.
    allocator->impl->free(allocator, self);
}


void sig_dsp_ClockSource_init(struct sig_dsp_ClockSource* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_ClockSource_generate);

    struct sig_dsp_Clock_Parameters params = {
        .threshold = 0.04f, // 0.2V, assuming 10Vpp
    };
    self->parameters = params;

    self->previousTapValue = 0.0f;
    self->tapCount = 0;
    self->tempoDurationSamples = 0;
    self->samplesSinceLastTap = 0;
    self->isLatching = false;
    self->latchedTapSamplesRemaining = 0;

    sig_CONNECT_TO_SILENCE(self, pulse, context);
    sig_CONNECT_TO_SILENCE(self, tap, context);
}

struct sig_dsp_ClockSource* sig_dsp_ClockSource_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_ClockSource* self = sig_MALLOC(allocator,
        struct sig_dsp_ClockSource);
    sig_dsp_ClockSource_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

static inline void updateTempo(struct sig_dsp_ClockSource* self,
    uint32_t tempoDuration) {
    self->tempoDurationSamples = tempoDuration;
    self->numHighSamples = (uint32_t) (
        (float) self->tempoDurationSamples * 0.25f);
}

static inline void handleTap(struct sig_dsp_ClockSource* self) {
    self->tapCount++;
    if (self->tapCount == 2) {
        // Two taps is enough to output a tempo clock.
        updateTempo(self, self->samplesSinceLastTap);
        self->latchedTapSamplesRemaining = self->tempoDurationSamples;
        self->isLatching = true;
    } else if (self->tapCount > 2) {
        // The second duration should be averaged with the first,
        // assuming it's within 50%.
        uint32_t shortestDuration = (uint32_t) (
            (float) self->tempoDurationSamples / 1.5f);
        uint32_t longestDuration = (uint32_t) (
            (float) self->tempoDurationSamples * 1.5f);

        if (self->samplesSinceLastTap >= shortestDuration &&
            self->samplesSinceLastTap <= longestDuration) {
            float prevTempoDuration = self->tempoDurationSamples;
            uint32_t averagedTempoDuration = (uint32_t) (
                (float) (prevTempoDuration + self->samplesSinceLastTap) / 2.0f);
            updateTempo(self, averagedTempoDuration);

            // Recalculate the tap duration.
            if (self->tempoDurationSamples < prevTempoDuration) {
                if (self->latchedTapSamplesRemaining >
                    self->tempoDurationSamples) {
                    // The current duration is shorter than
                    // the previous time remaining.
                    self->latchedTapSamplesRemaining = self->tempoDurationSamples;
                }
            } else if (self->tempoDurationSamples > prevTempoDuration) {
                // Add the additional time to the duration remaining.
                self->latchedTapSamplesRemaining += (
                    self->tempoDurationSamples - prevTempoDuration);
            }

            self->tapCount = 0;
        } else {
            self->tapCount = 1;
        }
    }

    self->samplesSinceLastTap = 0;
}

void sig_dsp_ClockSource_generate(void* signal) {
    struct sig_dsp_ClockSource* self = (struct sig_dsp_ClockSource*) signal;
    const float threshold = self->parameters.threshold;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT pulse = FLOAT_ARRAY(self->inputs.pulse);
    const float* SIG_RESTRICT tap = FLOAT_ARRAY(self->inputs.tap);

    for (size_t i = 0; i < blockSize; i++) {
        float pulseValue = pulse[i];
        float tapValue = tap[i];

        float sample = 0.0f;
        if (pulseValue >= threshold) {
            self->isLatching = false;
            self->tapCount = 0;
            sample = 1.0f;
        } else if (pulseValue < threshold) {
            if (self->tapCount > 0) {
                self->samplesSinceLastTap++;
            }

            if (tapValue >= threshold && self->previousTapValue < threshold) {
                handleTap(self);
            }

            if (self->isLatching) {
                if (self->latchedTapSamplesRemaining <= self->numHighSamples) {
                    sample = 1.0f;
                }

                self->latchedTapSamplesRemaining--;
                if (self->latchedTapSamplesRemaining <= 0) {
                    self->latchedTapSamplesRemaining =
                        self->tempoDurationSamples;
                }
            }
        }

        out[i] = sample;
        self->previousTapValue = tapValue;
    }
}

void sig_dsp_ClockSource_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ClockSource* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
    self = NULL;
}



void sig_dsp_ClockDetector_Outputs_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_ClockDetector_Outputs* outputs) {
    outputs->main = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->bpm = sig_AudioBlock_newSilent(allocator, audioSettings);
}

void sig_dsp_ClockDetector_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_ClockDetector_Outputs* outputs) {

    sig_AudioBlock_destroy(allocator, outputs->main);
    sig_AudioBlock_destroy(allocator, outputs->bpm);
}

void sig_dsp_ClockDetector_init(struct sig_dsp_ClockDetector* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_ClockDetector_generate);

    struct sig_dsp_Clock_Parameters params = {
        .threshold = 0.04f // 0.2V, assuming 10Vpp
    };

    self->parameters = params;
    self->previousTrigger = 0.0f;
    self->isRisingEdge = false;
    self->numPulsesDetected = 0;
    self->samplesSinceLastPulse = (uint32_t)
        self->signal.audioSettings->sampleRate;
    self->clockFreq = 0.0f;
    self->pulseDurSamples = 0;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

struct sig_dsp_ClockDetector* sig_dsp_ClockDetector_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_ClockDetector* self = sig_MALLOC(allocator,
        struct sig_dsp_ClockDetector);
    sig_dsp_ClockDetector_init(self, context);
    sig_dsp_ClockDetector_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

static inline float sig_dsp_ClockDetector_calcClockFreq(
    float sampleRate, uint32_t samplesSinceLastPulse,
    float prevFreq) {
    float freq = sampleRate / (float) samplesSinceLastPulse;

    return freq;
}

void sig_dsp_ClockDetector_generate(void* signal) {
    struct sig_dsp_ClockDetector* self =
        (struct sig_dsp_ClockDetector*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    const float threshold = self->parameters.threshold;
    float* SIG_RESTRICT mainOut = FLOAT_ARRAY(self->outputs.main);
    float* SIG_RESTRICT bpmOut = FLOAT_ARRAY(self->outputs.bpm);
    const float* SIG_RESTRICT source = FLOAT_ARRAY(self->inputs.source);

    float previousTrigger = self->previousTrigger;
    float clockFreq = self->clockFreq;
    bool isRisingEdge = self->isRisingEdge;
    uint32_t numPulsesDetected = self->numPulsesDetected;
    uint32_t samplesSinceLastPulse = self->samplesSinceLastPulse;
    uint32_t pulseDurSamples = self->pulseDurSamples;

    for (size_t i = 0; i < blockSize; i++) {
        samplesSinceLastPulse++;

        float sourceSamp = source[i];
        if (sourceSamp > 0.0f && previousTrigger < threshold) {
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
            numPulsesDetected++;
            if (numPulsesDetected > 1) {
                // Need to have found at least two pulses before
                // the frequency can be reliably determined.
                clockFreq = sig_dsp_ClockDetector_calcClockFreq(
                    sampleRate, samplesSinceLastPulse, clockFreq);
                numPulsesDetected = 1;
            }
            pulseDurSamples = samplesSinceLastPulse;
            samplesSinceLastPulse = 0;
            isRisingEdge = false;
        }

        mainOut[i] = clockFreq;
        bpmOut[i] = clockFreq * 60.0f;

        previousTrigger = sourceSamp;
    }

    self->previousTrigger = previousTrigger;
    self->clockFreq = clockFreq;
    self->isRisingEdge = isRisingEdge;
    self->numPulsesDetected = numPulsesDetected;
    self->samplesSinceLastPulse = samplesSinceLastPulse;
    self->pulseDurSamples = pulseDurSamples;
}

void sig_dsp_ClockDetector_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ClockDetector* self) {
    sig_dsp_ClockDetector_Outputs_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_LinearToFreq* sig_dsp_LinearToFreq_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_LinearToFreq* self = sig_MALLOC(allocator,
        struct sig_dsp_LinearToFreq);
    sig_dsp_LinearToFreq_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_LinearToFreq_init(struct sig_dsp_LinearToFreq* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_LinearToFreq_generate);
    self->parameters.middleFreq = sig_FREQ_C4;
    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_dsp_LinearToFreq_generate(void* signal) {
    struct sig_dsp_LinearToFreq* self = (struct sig_dsp_LinearToFreq*) signal;
    const float middleFreq = self->parameters.middleFreq;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);

    for (size_t i = 0; i < blockSize; i++) {
        out[i] = sig_linearToFreq(in[i], middleFreq);
    }
}

void sig_dsp_LinearToFreq_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_LinearToFreq* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_Branch* sig_dsp_Branch_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Branch* self = sig_MALLOC(allocator, struct sig_dsp_Branch);
    sig_dsp_Branch_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Branch_init(struct sig_dsp_Branch* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Branch_generate);
    sig_CONNECT_TO_SILENCE(self, off, context);
    sig_CONNECT_TO_SILENCE(self, on, context);
    sig_CONNECT_TO_SILENCE(self, condition, context);
}

void sig_dsp_Branch_generate(void* signal) {
    struct sig_dsp_Branch* self = (struct sig_dsp_Branch*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT condition = FLOAT_ARRAY(self->inputs.condition);
    const float* SIG_RESTRICT off = FLOAT_ARRAY(self->inputs.off);
    const float* SIG_RESTRICT on = FLOAT_ARRAY(self->inputs.on);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = condition[i] > 0.0f ? on[i] : off[i];
    }
}

void sig_dsp_Branch_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Branch* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_List_Outputs_newAudioBlocks(struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_List_Outputs* outputs) {
    outputs->main = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->index = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->length = sig_AudioBlock_newSilent(allocator, audioSettings);
}

void sig_dsp_List_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_List_Outputs* outputs) {
    sig_AudioBlock_destroy(allocator, outputs->main);
    sig_AudioBlock_destroy(allocator, outputs->index);
    sig_AudioBlock_destroy(allocator, outputs->length);
}

struct sig_dsp_List* sig_dsp_List_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_List* self = sig_MALLOC(allocator, struct sig_dsp_List);
    sig_dsp_List_init(self, context);
    sig_dsp_List_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_List_init(struct sig_dsp_List* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_List_generate);
    self->parameters.wrap = 1.0f;
    self->parameters.normalizeIndex = 1.0f;
    self->parameters.interpolate = 0.0f;
    sig_CONNECT_TO_SILENCE(self, index, context);
}

extern inline float sig_dsp_List_constrain(bool shouldWrap,
    float index, float lastIndex, float listLength) {
    if (shouldWrap) {
        while (index < 0.0f) {
            index = listLength + index;
        }

        while (index > lastIndex) {
            index -= lastIndex;
        }
    } else {
        index = sig_clamp(index, 0.0f, lastIndex);
    }

    return index;
}

void sig_dsp_List_generate(void* signal) {
    struct sig_dsp_List* self = (struct sig_dsp_List*) signal;
    struct sig_Buffer* list = self->list;
    const size_t blockSize = self->signal.audioSettings->blockSize;

    // List buffers can only be updated at control rate.
    // TODO: Cache these and only recalculate when
    // the list buffer changes.
    size_t listLength = self->list != NULL ? list->length : 0;
    float listLengthF = (float) listLength;
    size_t lastIndex = listLength - 1;
    float lastIndexF = (float) lastIndex;
    bool shouldWrap = self->parameters.wrap > 0.0f;
    bool shouldInterpolate = self->parameters.interpolate > 0.0f;
    bool shouldNormalize = self->parameters.normalizeIndex > 0.0f;

    float* SIG_RESTRICT mainOut = FLOAT_ARRAY(self->outputs.main);
    float* SIG_RESTRICT indexOut = FLOAT_ARRAY(self->outputs.index);
    float* SIG_RESTRICT lengthOut = FLOAT_ARRAY(self->outputs.length);
    const float* SIG_RESTRICT indexIn = FLOAT_ARRAY(self->inputs.index);

    if (listLength < 1) {
        // There's nothing in the list; just output silence.
        for (size_t i = 0; i < blockSize; i++) {
            mainOut[i] = 0.0f;
            indexOut[i] = -1.0f;
            lengthOut[i] = listLengthF;
        }
    } else {
        for (size_t i = 0; i < blockSize; i++) {
            float index = indexIn[i];
            float sample = 0.0f;

            if (shouldNormalize) {
                index = index * lastIndexF;
            }

            if (shouldInterpolate) {
                index = sig_dsp_List_constrain(shouldWrap, index, lastIndexF,
                    listLengthF);
                sample = sig_interpolate_linear(index, list->samples,
                    listLength);
            } else {
                index = roundf(index);
                index = sig_dsp_List_constrain(shouldWrap, index, lastIndexF,
                    listLengthF);
                sample = FLOAT_ARRAY(list->samples)[(size_t) index];
            }

            mainOut[i] = sample;
            indexOut[i] = index;
            lengthOut[i] = listLengthF;
        }
    }
}

void sig_dsp_List_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_List* self) {
    sig_dsp_List_Outputs_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



void sig_dsp_Sequencer_Outputs_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_Sequencer_Outputs* outputs) {
    outputs->main = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->index = sig_AudioBlock_newSilent(allocator, audioSettings);
}

void sig_dsp_Sequencer_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_Sequencer_Outputs* outputs) {
    sig_AudioBlock_destroy(allocator, outputs->main);
    sig_AudioBlock_destroy(allocator, outputs->index);
}

struct sig_dsp_Sequencer* sig_dsp_Sequencer_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Sequencer* self = sig_MALLOC(allocator,
        struct sig_dsp_Sequencer);

    sig_dsp_Sequencer_init(self, context);
    sig_dsp_Sequencer_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Sequencer_init(struct sig_dsp_Sequencer* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Sequencer_generate);
    self->parameters.loop = 1.0f;
    self->parameters.resetOnNext = 0.0f;
    self->parameters.holdLastValue = 0.0f;
    self->samplesRemaining = 0;
    self->stepIndex = -1;
    self->lastValue = 0.0f;

    sig_CONNECT_TO_UNITY(self, mul, context);
    sig_CONNECT_TO_SILENCE(self, add, context);
}

extern inline float sig_dsp_Sequencer_startStage(
    struct sig_dsp_Sequencer* self) {
    float duration = FLOAT_ARRAY(self->durations->samples)[self->stepIndex];
    self->samplesRemaining = (size_t) floor(duration *
        self->signal.audioSettings->sampleRate);
    self->samplesRemaining--;

    return self->parameters.resetOnNext ?
        0.0f : FLOAT_ARRAY(self->values->samples)[self->stepIndex];
};

void sig_dsp_Sequencer_generate(void* signal) {
    struct sig_dsp_Sequencer* self = (struct sig_dsp_Sequencer*) signal;
    const float* SIG_RESTRICT mul = FLOAT_ARRAY(self->inputs.mul);
    const float* SIG_RESTRICT add = FLOAT_ARRAY(self->inputs.add);
    struct sig_Buffer* durations = self->durations;
    struct sig_Buffer* values = self->values;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const size_t valuesLength = values->length;
    const size_t durationsLength = durations->length;
    const bool hasValidSequence = valuesLength == durationsLength &&
        durationsLength > 0 && valuesLength > 0;
    float* SIG_RESTRICT mainOut = FLOAT_ARRAY(self->outputs.main);
    float* SIG_RESTRICT indexOut = FLOAT_ARRAY(self->outputs.index);

    for (size_t i = 0; i < blockSize; i++) {
        float sample = 0.0f;
        if (hasValidSequence) {
            if (self->samplesRemaining == 0) {
                // We're at the start or end of a stage.
                if (self->stepIndex < 0 ||
                    (size_t)self->stepIndex < durationsLength - 1) {
                    // Continue to the next value/duration pair.
                    self->stepIndex++;
                    sample = sig_dsp_Sequencer_startStage(self);
                } else if (self->parameters.loop > 0.0f) {
                    // Loop back to the first value/duration pair.
                    self->stepIndex = 0;
                    sample = sig_dsp_Sequencer_startStage(self);
                } else {
                    // Nothing left to do.
                    sample = self->parameters.holdLastValue ?
                        self->lastValue : 0.0;
                }
            } else {
                // Still in the midst of a stage.
                sample = FLOAT_ARRAY(values->samples)[self->stepIndex];
                self->samplesRemaining--;
            }
        }

        mainOut[i] = self->lastValue = sample *
            mul[i] + add[i];
        indexOut[i] = (float) self->stepIndex;
    }
}

void sig_dsp_Sequencer_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Sequencer* self) {
    sig_dsp_Sequencer_Outputs_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_LinearMap* sig_dsp_LinearMap_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_LinearMap* self = sig_MALLOC(allocator,
        struct sig_dsp_LinearMap);
    sig_dsp_LinearMap_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}
void sig_dsp_LinearMap_init(struct sig_dsp_LinearMap* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_LinearMap_generate);
    self->parameters.fromMin = -1.0f;
    self->parameters.fromMax = 1.0f;
    self->parameters.toMin = -5.0f;
    self->parameters.toMax = 5.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_dsp_LinearMap_generate(void* signal) {
    struct sig_dsp_LinearMap* self = (struct sig_dsp_LinearMap*) signal;

    const float fromMin = self->parameters.fromMin;
    const float fromMax = self->parameters.fromMax;
    const float toMin = self->parameters.toMin;
    const float toMax = self->parameters.toMax;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = sig_linearMap(in[i], fromMin, fromMax, toMin, toMax);
    }
}

void sig_dsp_LinearMap_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_LinearMap* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_TwoOpFM_Outputs_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_TwoOpFM_Outputs* outputs) {
    outputs->main = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->carrierEOC = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->modulator = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->modulatorEOC = sig_AudioBlock_newSilent(allocator,
        audioSettings);
}

void sig_dsp_TwoOpFM_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_TwoOpFM_Outputs* outputs) {
    sig_AudioBlock_destroy(allocator, outputs->main);
    outputs->main = NULL;
    sig_AudioBlock_destroy(allocator, outputs->carrierEOC);
    outputs->carrierEOC = NULL;
    sig_AudioBlock_destroy(allocator, outputs->modulator);
    outputs->modulator = NULL;
    sig_AudioBlock_destroy(allocator, outputs->modulatorEOC);
    outputs->modulatorEOC = NULL;
}

struct sig_dsp_TwoOpFM* sig_dsp_TwoOpFM_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_TwoOpFM* self = sig_MALLOC(allocator,
        struct sig_dsp_TwoOpFM);
    sig_dsp_TwoOpFM_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    self->sineTable = sig_Buffer_new(allocator, 8192);
    self->feedbackDelay.buffer = sig_Buffer_new(allocator, 2);

    sig_dsp_TwoOpFM_init(self, context);

    return self;
}

void sig_dsp_TwoOpFM_init(struct sig_dsp_TwoOpFM* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_TwoOpFM_generate);

    sig_Buffer_fillWithWaveform(self->sineTable, sig_waveform_sine,
        self->sineTable->length, 0.0f, 1.0f);
    sig_osc_Wavetable_init(&self->carrierState, self->sineTable);
    sig_osc_Wavetable_init(&self->modulatorState, self->sineTable);
    sig_DelayLine_init(&self->feedbackDelay);

    sig_CONNECT_TO_SILENCE(self, frequency, context);
    sig_CONNECT_TO_SILENCE(self, index, context);
    sig_CONNECT_TO_SILENCE(self, ratio, context);
    sig_CONNECT_TO_SILENCE(self, phaseOffset, context);
    sig_CONNECT_TO_SILENCE(self, modulatorPhaseOffset, context);
    sig_CONNECT_TO_SILENCE(self, feedbackGain, context);
}

void sig_dsp_TwoOpFM_generate(void* signal) {
    struct sig_dsp_TwoOpFM* self = (struct sig_dsp_TwoOpFM*) signal;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    const float* SIG_RESTRICT frequency = FLOAT_ARRAY(self->inputs.frequency);
    const float* SIG_RESTRICT index = FLOAT_ARRAY(self->inputs.index);
    const float* SIG_RESTRICT ratio = FLOAT_ARRAY(self->inputs.ratio);
    const float* SIG_RESTRICT feedbackGain = FLOAT_ARRAY(self->inputs.feedbackGain);
    const float* SIG_RESTRICT phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    const float* SIG_RESTRICT modulatorPhaseOffset = FLOAT_ARRAY(
        self->inputs.modulatorPhaseOffset);
    float* SIG_RESTRICT modulatorEOCOut = FLOAT_ARRAY(self->outputs.modulatorEOC);
    float* SIG_RESTRICT carrierEOCOut = FLOAT_ARRAY(self->outputs.carrierEOC);
    float* SIG_RESTRICT mainOut = FLOAT_ARRAY(self->outputs.main);
    float* SIG_RESTRICT modulatorOut = FLOAT_ARRAY(self->outputs.modulator);
    struct sig_Buffer* feedbackBuffer = self->feedbackDelay.buffer;

    for (size_t i = 0; i < blockSize; i++) {
        float fundamental = frequency[i];
        float feedback = sig_filter_mean(feedbackBuffer->samples,
            feedbackBuffer->length) * feedbackGain[i];
        float modulatorFrequency = fundamental * ratio[i];
        float modulatorSample = sig_osc_Wavetable_generate(
            &self->modulatorState, modulatorFrequency,
            feedback + modulatorPhaseOffset[i], sampleRate,
            &modulatorEOCOut[i]);
        float modulatorSampleScaled = modulatorSample * index[i];
        float carrierPhaseOffset = modulatorSampleScaled + phaseOffset[i];
        float carrierSample = sig_osc_Wavetable_generate(&self->carrierState,
            fundamental, carrierPhaseOffset, sampleRate, &carrierEOCOut[i]);

        sig_DelayLine_write(&self->feedbackDelay, carrierSample);
        mainOut[i] = carrierSample;
        modulatorOut[i] = modulatorSample;
    }
}

void sig_dsp_TwoOpFM_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_TwoOpFM* self) {
    sig_dsp_TwoOpFM_Outputs_destroyAudioBlocks(allocator, &self->outputs);
    sig_Buffer_destroy(allocator, self->sineTable);
    self->sineTable = NULL;
    sig_Buffer_destroy(allocator, self->feedbackDelay.buffer);
    self->feedbackDelay.buffer = NULL;
    sig_dsp_Signal_destroy(allocator, self);
}



void sig_dsp_FourPoleFilter_Outputs_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_FourPoleFilter_Outputs* outputs) {
    outputs->main = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->twoPole = sig_AudioBlock_newSilent(allocator, audioSettings);
}

void sig_dsp_FourPoleFilter_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_FourPoleFilter_Outputs* outputs) {
    sig_AudioBlock_destroy(allocator, outputs->main);
    sig_AudioBlock_destroy(allocator, outputs->twoPole);
}


struct sig_dsp_Ladder* sig_dsp_Ladder_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Ladder* self = sig_MALLOC(allocator,
        struct sig_dsp_Ladder);
    sig_dsp_Ladder_init(self, context);
    sig_dsp_FourPoleFilter_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Ladder_init(
    struct sig_dsp_Ladder* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Ladder_generate);

    struct sig_dsp_Ladder_Parameters parameters = {
        .passbandGain = 0.5f
    };
    self->parameters = parameters;

    self->interpolation = 4;
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
}

extern inline void sig_dsp_Ladder_calcCoefficients(
    struct sig_dsp_Ladder* self, float freq) {
    float sampleRate = self->signal.audioSettings->sampleRate;
    freq = sig_clamp(freq, 5.0f, sampleRate * 0.425f);
    float wc = freq * (float) (sig_TWOPI /
        ((float)self->interpolation * sampleRate));
    float wc2 = wc * wc;
    self->alpha = 0.9892f * wc - 0.4324f *
        wc2 + 0.1381f * wc * wc2 - 0.0202f * wc2 * wc2;
    self->qAdjust = 1.006f + 0.0536f * wc - 0.095f * wc2 - 0.05f * wc2 * wc2;
}

extern inline float sig_dsp_Ladder_calcStage(
    struct sig_dsp_Ladder* self, float s, uint8_t i) {
    float ft = s * (1.0f/1.3f) + (0.3f/1.3f) * self->z0[i] - self->z1[i];
    ft = ft * self->alpha + self->z1[i];
    self->z1[i] = ft;
    self->z0[i] = s;
    return ft;
}

void sig_dsp_Ladder_generate(void* signal) {
    struct sig_dsp_Ladder* self =
        (struct sig_dsp_Ladder*) signal;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float interpolationRecip = self->interpolationRecip;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    float* SIG_RESTRICT twoPoleOut = FLOAT_ARRAY(self->outputs.twoPole);
    float* SIG_RESTRICT fourPoleOut = FLOAT_ARRAY(self->outputs.fourPole);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT freqIn = FLOAT_ARRAY(self->inputs.frequency);
    const float* SIG_RESTRICT resIn = FLOAT_ARRAY(self->inputs.resonance);
    const float* SIG_RESTRICT inputGainIn = FLOAT_ARRAY(self->inputs.inputGain);
    const float* SIG_RESTRICT pole1GainIn = FLOAT_ARRAY(self->inputs.pole1Gain);
    const float* SIG_RESTRICT pole2GainIn = FLOAT_ARRAY(self->inputs.pole2Gain);
    const float* SIG_RESTRICT pole3GainIn = FLOAT_ARRAY(self->inputs.pole3Gain);
    const float* SIG_RESTRICT pole4GainIn = FLOAT_ARRAY(self->inputs.pole4Gain);

    for (size_t i = 0; i < blockSize; i++) {
        float input = in[i];
        float frequency = freqIn[i];
        float resonance = resIn[i];
        float totals[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        float interp = 0.0f;

        // Recalculate coefficients if the frequency has changed.
        if (frequency != self->prevFrequency) {
            sig_dsp_Ladder_calcCoefficients(self, frequency);
            self->prevFrequency = frequency;
        }

        self->k = 4.0f * resonance;

        for (size_t os = 0; os < self->interpolation; os++) {
            float inInterp = interp * self->prevInput + (1.0f - interp) * input;
            float u = inInterp - (self->z1[3] - self->parameters.passbandGain *
                inInterp) * self->k * self->qAdjust;
            u = sig_fastTanhf(u);
            totals[0] = u;
            float stage1 = sig_dsp_Ladder_calcStage(self, u, 0);
            totals[1] += stage1 * interpolationRecip;
            float stage2 = sig_dsp_Ladder_calcStage(self, stage1, 1);
            totals[2] += stage2 * interpolationRecip;
            float stage3 = sig_dsp_Ladder_calcStage(self, stage2, 2);
            totals[3] += stage3 * interpolationRecip;
            float stage4 = sig_dsp_Ladder_calcStage(self, stage3, 3);
            totals[4] += stage4 * interpolationRecip;
            interp += interpolationRecip;
        }
        self->prevInput = input;
        out[i] =
            (totals[0] * inputGainIn[i]) +
            (totals[1] * pole1GainIn[i]) +
            (totals[2] * pole2GainIn[i]) +
            (totals[3] * pole3GainIn[i]) +
            (totals[4] * pole4GainIn[i]);
        twoPoleOut[i] = totals[2];
        fourPoleOut[i] = totals[4];
    }
}

void sig_dsp_Ladder_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Ladder* self) {
    sig_dsp_FourPoleFilter_Outputs_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_TiltEQ* sig_dsp_TiltEQ_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_TiltEQ* self = sig_MALLOC(allocator,
        struct sig_dsp_TiltEQ);
    sig_dsp_TiltEQ_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_TiltEQ_init(struct sig_dsp_TiltEQ* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_TiltEQ_generate);
    self->sr3 = 3.0f * self->signal.audioSettings->sampleRate;
    self->lpOut = 0.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, frequency, context);
    sig_CONNECT_TO_SILENCE(self, gain, context);
}

void sig_dsp_TiltEQ_generate(void* signal) {
    struct sig_dsp_TiltEQ* self = (struct sig_dsp_TiltEQ*) signal;
    const float amp = 8.656170f; // 6.0f / log(2)
    const float gfactor = 5.0f; // Proportional gain.
    const float sr3 = self->sr3;
    float lpOut = self->lpOut;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT freqIn = FLOAT_ARRAY(self->inputs.frequency);
    const float* SIG_RESTRICT gainIn = FLOAT_ARRAY(self->inputs.gain);

    for (size_t i = 0; i < blockSize; i++) {
        float source = in[i];
        float f0 = freqIn[i];
        float gain = gainIn[i] * 6.0f; // Convert to dB.
        float g1, g2;

        if (gain > 0.0f) {
            g1 = -gfactor * gain;
            g2 = gain;
        } else {
            g1 = -gain;
            g2 = gfactor * gain;
        }

        float lgain = exp(g1 / amp) - 1.0f;
        float hgain = exp(g2 / amp) - 1.0f;

        float omega = 2.0f * sig_PI * f0;
        float n = 1.0 / (sr3 + omega);
        float a0 = 2.0 * omega * n;
        float b1 = (sr3 - omega) * n;

        lpOut = a0 * source + b1 * lpOut;
        out[i] = source + lgain * lpOut +
            hgain * (source - lpOut);
    }

    self->lpOut = lpOut;
}

void sig_dsp_TiltEQ_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_TiltEQ* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_Delay* sig_dsp_Delay_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Delay* self = sig_MALLOC(allocator, struct sig_dsp_Delay);
    // TODO: Improve buffer management throughout Signaletic.
    self->delayLine = context->oneSampleDelayLine;
    sig_dsp_Delay_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Delay_init(struct sig_dsp_Delay* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Delay_generate);

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, delayTime, context);
}

extern inline void sig_dsp_Delay_read(struct sig_dsp_Delay* self,
    float source, size_t i) {
    float delayTime = FLOAT_ARRAY(self->inputs.delayTime)[i];

    FLOAT_ARRAY(self->outputs.main)[i] = sig_DelayLine_cubicReadAtTime(
        self->delayLine,
        source,
        delayTime,
        self->signal.audioSettings->sampleRate);
}

void sig_dsp_Delay_generate(void* signal) {
    struct sig_dsp_Delay* self = (struct sig_dsp_Delay*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT delayTime = FLOAT_ARRAY(self->inputs.delayTime);
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float sampleRate = self->signal.audioSettings->sampleRate;

    for (size_t i = 0; i < blockSize; i++) {
        float source = in[i];
        out[i] = sig_DelayLine_cubicReadAtTime(
            self->delayLine,
            source,
            delayTime[i],
            sampleRate);
        sig_DelayLine_write(self->delayLine, source);
    }
}

void sig_dsp_Delay_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Delay* self) {
    // Don't destroy the delay line; it isn't owned.
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_Delay* sig_dsp_DelayTap_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    // TODO: Copy-pasted from sig_dsp_Delay but we have a different
    // initialization function we need to call here. Seems like it's time to
    // decompose Signals into more function pointers!
    struct sig_dsp_Delay* self = sig_MALLOC(allocator, struct sig_dsp_Delay);
    // TODO: Improve buffer management throughout Signaletic.
    self->delayLine = context->oneSampleDelayLine;

    sig_dsp_Delay_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_DelayTap_init(struct sig_dsp_Delay* self,
    struct sig_SignalContext* context) {
    sig_dsp_Delay_init(self, context);
    sig_dsp_Signal_init(self, context, *sig_dsp_DelayTap_generate);
}

void sig_dsp_DelayTap_generate(void* signal) {
    struct sig_dsp_Delay* self = (struct sig_dsp_Delay*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT delayTime = FLOAT_ARRAY(self->inputs.delayTime);
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float sampleRate = self->signal.audioSettings->sampleRate;

    for (size_t i = 0; i < blockSize; i++) {
        float source = in[i];
        out[i] = sig_DelayLine_cubicReadAtTime(
            self->delayLine,
            source,
            delayTime[i],
            sampleRate);
    }
}

void sig_dsp_DelayTap_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Delay* self) {
    // Don't destroy the delay line; it isn't owned.
    sig_dsp_Delay_destroy(allocator, self);
}



struct sig_dsp_DelayWrite* sig_dsp_DelayWrite_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_DelayWrite* self = sig_MALLOC(allocator,
        struct sig_dsp_DelayWrite);
    // TODO: Improve buffer management throughout Signaletic.
    self->delayLine = context->oneSampleDelayLine;

    sig_dsp_DelayWrite_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_DelayWrite_init(struct sig_dsp_DelayWrite* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_DelayWrite_generate);

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_dsp_DelayWrite_generate(void* signal) {
    struct sig_dsp_DelayWrite* self = (struct sig_dsp_DelayWrite*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);

    for (size_t i = 0; i < blockSize; i++) {
        sig_DelayLine_write(self->delayLine, in[i]);
    }

}

void sig_dsp_DelayWrite_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_DelayWrite* self) {
    // Don't destroy the delay line; it isn't owned.
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



// TODO: Resolve duplication with sig_dsp_Delay
struct sig_dsp_Comb* sig_dsp_Comb_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Comb* self = sig_MALLOC(allocator, struct sig_dsp_Comb);
     // TODO: Improve buffer management throughout Signaletic.
    self->delayLine = context->oneSampleDelayLine;

    sig_dsp_Comb_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Comb_init(struct sig_dsp_Comb* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Comb_generate);
    self->previousSample = 0.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, delayTime, context);
    sig_CONNECT_TO_SILENCE(self, feedbackGain, context);
    sig_CONNECT_TO_SILENCE(self, lpfCoefficient, context);
}

void sig_dsp_Comb_generate(void* signal) {
    struct sig_dsp_Comb* self = (struct sig_dsp_Comb*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float maxDelayLength = (float) self->delayLine->buffer->length;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT feedbackGainIn = FLOAT_ARRAY(self->inputs.feedbackGain);
    const float* SIG_RESTRICT delayTimeIn = FLOAT_ARRAY(self->inputs.delayTime);
    const float* SIG_RESTRICT lpfCoefficientIn = FLOAT_ARRAY(self->inputs.lpfCoefficient);

    for (size_t i = 0; i < blockSize; i++) {
        float source = in[i];
        float feedbackGain = feedbackGainIn[i];
        float delayTime = delayTimeIn[i];
        float lpfCoefficient = lpfCoefficientIn[i];
        float readPos = (delayTime * sampleRate);
        if (readPos >= maxDelayLength) {
            readPos = maxDelayLength - 1;
        }

        delayTime = sig_fmaxf(delayTime, 0.00001f); // Delay time can't be zero.
        float read = sig_DelayLine_linearReadAt(self->delayLine, readPos);
        float outputSample = sig_filter_smooth(read, self->previousSample,
            lpfCoefficient);
        float toWrite = sig_DelayLine_feedback(source, outputSample,
            feedbackGain);
        sig_DelayLine_write(self->delayLine, toWrite);
        out[i] = outputSample;
        self->previousSample = outputSample;
    }
}

void sig_dsp_Comb_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Comb* self) {
    // Don't destroy the delay line; it isn't owned.
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



// TODO: Resolve duplication with sig_dsp_Delay and Comb
struct sig_dsp_Allpass* sig_dsp_Allpass_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Allpass* self = sig_MALLOC(allocator,
        struct sig_dsp_Allpass);
     // TODO: Improve buffer management throughout Signaletic.
    self->delayLine = context->oneSampleDelayLine;

    sig_dsp_Allpass_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Allpass_init(struct sig_dsp_Allpass* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Allpass_generate);

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, delayTime, context);
    sig_CONNECT_TO_SILENCE(self, g, context);
}

void sig_dsp_Allpass_generate(void* signal) {
    struct sig_dsp_Allpass* self = (struct sig_dsp_Allpass*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float maxDelayLength = (float) self->delayLine->buffer->length;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT delayTimeIn = FLOAT_ARRAY(self->inputs.delayTime);
    const float* SIG_RESTRICT gIn = FLOAT_ARRAY(self->inputs.g);

    for (size_t i = 0; i < blockSize; i++) {
        float source = in[i];
        float delayTime = delayTimeIn[i];
        float g = gIn[i];
        float readPos = (delayTime * sampleRate);
        if (readPos >= maxDelayLength) {
            readPos = maxDelayLength - 1;
        }

        if (delayTime <= 0.0f || g <= 0.0f) {
            out[i] = source;
        } else {
            out[i] = sig_DelayLine_linearAllpass(
                self->delayLine, source, readPos, g);
        }
    }
}

void sig_dsp_Allpass_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Allpass* self) {
    // Don't destroy the delay line; it isn't owned.
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_Chorus_Outputs_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_Chorus_Outputs* outputs) {
    outputs->main = sig_AudioBlock_newSilent(allocator, audioSettings);
    outputs->modulator = sig_AudioBlock_newSilent(allocator, audioSettings);
}

void sig_dsp_Chorus_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_Chorus_Outputs* outputs) {
    sig_AudioBlock_destroy(allocator, outputs->main);
    sig_AudioBlock_destroy(allocator, outputs->modulator);
}

struct sig_dsp_Chorus* sig_dsp_Chorus_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Chorus* self = sig_MALLOC(allocator,
        struct sig_dsp_Chorus);
     // TODO: Improve buffer management throughout Signaletic.
    self->delayLine = context->oneSampleDelayLine;

    sig_dsp_Chorus_init(self, context);
    sig_dsp_Chorus_Outputs_newAudioBlocks(allocator, context->audioSettings,
        &self->outputs);

    return self;
}

void sig_dsp_Chorus_init(struct sig_dsp_Chorus* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Chorus_generate);

    sig_osc_FastLFSine_init(&self->modulator,
        self->signal.audioSettings->sampleRate);

    self->previousFixedOutput = 0.0f;
    self->previousModulatedOutput = 0.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, delayTime, context);
    sig_CONNECT_TO_SILENCE(self, speed, context);
    sig_CONNECT_TO_SILENCE(self, width, context);
    sig_CONNECT_TO_SILENCE(self, feedbackGain, context);
    sig_CONNECT_TO_SILENCE(self, feedforwardGain, context);
    sig_CONNECT_TO_SILENCE(self, blend, context);
}

void sig_dsp_Chorus_generate(void* signal) {
    struct sig_dsp_Chorus* self = (struct sig_dsp_Chorus*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    const float sampleRate = self->signal.audioSettings->sampleRate;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    float* SIG_RESTRICT modulatorOut = FLOAT_ARRAY(self->outputs.modulator);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT delayTimeIn = FLOAT_ARRAY(self->inputs.delayTime);
    const float* SIG_RESTRICT speedIn = FLOAT_ARRAY(self->inputs.speed);
    const float* SIG_RESTRICT widthIn = FLOAT_ARRAY(self->inputs.width);
    const float* SIG_RESTRICT feedbackGainIn = FLOAT_ARRAY(self->inputs.feedbackGain);
    const float* SIG_RESTRICT feedforwardGainIn = FLOAT_ARRAY(self->inputs.feedforwardGain);
    const float* SIG_RESTRICT blendIn = FLOAT_ARRAY(self->inputs.blend);

    for (size_t i = 0; i < blockSize; i++) {
        float source = in[i];
        float delayTime = delayTimeIn[i];
        float speed = speedIn[i];
        float width = widthIn[i];
        float feedbackGain = feedbackGainIn[i];
        float feedforwardGain = feedforwardGainIn[i];
        float blend = blendIn[i];

        sig_osc_FastLFSine_setFrequencyFast(&self->modulator, speed);
        sig_osc_FastLFSine_generate(&self->modulator);
        modulatorOut[i] = self->modulator.sinZ;

        // TODO: Add one pole low pass filters in both the feedback and
        // feedforward lines to support echo.

        float fixedRead = sig_DelayLine_allpassReadAtTime(self->delayLine,
            source, delayTime, sampleRate,
            self->previousFixedOutput);
        self->previousFixedOutput = fixedRead;
        float toWrite = source - (fixedRead * feedbackGain);
        sig_DelayLine_write(self->delayLine, toWrite);

        float modulatedDelayTime = self->modulator.sinZ * width + delayTime;
        float modulatedRead = sig_DelayLine_allpassReadAtTime(self->delayLine,
            source, modulatedDelayTime, sampleRate,
            self->previousModulatedOutput);
        self->previousModulatedOutput = modulatedRead;
        float feedforwardSample = modulatedRead * feedforwardGain;
        float output = (toWrite * blend) + feedforwardSample;

        // TODO: What kind of gain staging should we do here?
        // It seems likely that we can have up to 3x gain depending on
        // the values of feedbackGain, feedforwardGain, and blend.
        out[i] = sig_fastTanhf(output / 3.0f);
    }
}

void sig_dsp_Chorus_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Chorus* self) {
    // Don't destroy the delay line; it isn't owned.
    sig_dsp_Chorus_Outputs_destroyAudioBlocks(allocator, &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_LinearXFade* sig_dsp_LinearXFade_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_LinearXFade* self = sig_MALLOC(allocator,
        struct sig_dsp_LinearXFade);
    sig_dsp_LinearXFade_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_LinearXFade_init(struct sig_dsp_LinearXFade* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_LinearXFade_generate);

    sig_CONNECT_TO_SILENCE(self, left, context);
    sig_CONNECT_TO_SILENCE(self, right, context);
    sig_CONNECT_TO_SILENCE(self, mix, context);
}

void sig_dsp_LinearXFade_generate(void* signal) {
    struct sig_dsp_LinearXFade* self = (struct sig_dsp_LinearXFade*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT left = FLOAT_ARRAY(self->inputs.left);
    const float* SIG_RESTRICT right = FLOAT_ARRAY(self->inputs.right);
    const float* SIG_RESTRICT mix = FLOAT_ARRAY(self->inputs.mix);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = sig_linearXFade(left[i], right[i], mix[i]);
    }
}

void sig_dsp_LinearXFade_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_LinearXFade* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_Calibrator* sig_dsp_Calibrator_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Calibrator* self = sig_MALLOC(allocator,
        struct sig_dsp_Calibrator);
    sig_dsp_Calibrator_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

extern inline void sig_dsp_Calibrator_Node_init(
    struct sig_dsp_Calibrator_Node* nodes,
    float_array_ptr targetValues, size_t numNodes) {
    for (size_t i = 0; i < numNodes; i++) {
        struct sig_dsp_Calibrator_Node* node = &nodes[i];
        node->target = node->avg = FLOAT_ARRAY(targetValues)[i];
        node->numSamplesRecorded = 0;
        node->min = INFINITY;
        node->max = -INFINITY;
        node->sum = 0.0f;
        node->diff = 0.0f;
    }
}

void sig_dsp_Calibrator_init(struct sig_dsp_Calibrator* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Calibrator_generate);
    self->previousGate = 0.0f;
    self->stage = 0;

    float targetValues[sig_dsp_Calibrator_NUM_STAGES] =
        sig_dsp_Calibrator_TARGET_VALUES;
    sig_dsp_Calibrator_Node_init(self->nodes,
        targetValues, sig_dsp_Calibrator_NUM_STAGES);

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, gate, context);
}

extern inline size_t sig_dsp_Calibrator_locateIntervalForValue(
    float x, struct sig_dsp_Calibrator_Node* nodes,
    size_t numNodes) {
    size_t lastNodeIdx = numNodes - 1;
    size_t intervalEndIdx = lastNodeIdx;
    for (size_t i = 0; i < lastNodeIdx - 1; i++) {
        size_t nextIdx = i + 1;
        struct sig_dsp_Calibrator_Node nextState = nodes[nextIdx];
        if (x < nextState.avg) {
            intervalEndIdx = nextIdx;
            break;
        }
    }

    return intervalEndIdx;
}

extern inline float sig_dsp_Calibrator_fitValueToCalibrationData(
    float x, struct sig_dsp_Calibrator_Node* nodes,
    size_t numNodes) {
        // Calibrate using piecewise linear fit from
        // readings sampled at 0.0, 1.0, 2.0, 3.0, 4.0 and 4.75V.
        // The ADC tops out slightly before 5 volts,
        // So 4.75V is 9 semitones above the fourth octave.

        // Find the segment in which the current value is located.
        size_t intervalEndIdx = sig_dsp_Calibrator_locateIntervalForValue(x,
            nodes, numNodes);
        struct sig_dsp_Calibrator_Node start = nodes[intervalEndIdx - 1];
        struct sig_dsp_Calibrator_Node end = nodes[intervalEndIdx];

        // y is the interval values measured during the calibration process.
        float yk = start.avg;
        float ykplus1 = end.avg;

        // t is the target interval values.
        float tk = start.target;
        float tkplus1 = end.target;

        // Interpolate the calibrated value.
        return tk + ((tkplus1 - tk) / (ykplus1 - yk)) * (x - yk);
};

void sig_dsp_Calibrator_generate(void* signal) {
    struct sig_dsp_Calibrator* self = (struct sig_dsp_Calibrator*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT sourceIn = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT gateIn = FLOAT_ARRAY(self->inputs.gate);

    for (size_t i = 0; i < blockSize; i++) {
        float source = sourceIn[i];
        float gate = gateIn[i];
        size_t stage = self->stage;
        struct sig_dsp_Calibrator_Node* node = &self->nodes[stage];

        if (gate <= 0.0f && self->previousGate > 0.0f) {
            // Gate is low; recording has just stopped.
            // Calculate offset by discarding the highest and lowest values,
            // and then averaging the rest.
            node->sum -= node->min;
            node->sum -= node->max;
            node->avg = node->sum /
                (node->numSamplesRecorded - 2);
            node->diff = -(node->target - node->avg);
            self->stage = (stage + 1) % sig_dsp_Calibrator_NUM_STAGES;
        } else if (gate > 0.0f) {
            // Gate is high; we're recording.
            node->sum += source;
            node->numSamplesRecorded++;

            if (source < node->min) {
                node->min = source;
            }

            if (source > node->max) {
                node->max = source;
            }
        }

        float px = sig_dsp_Calibrator_fitValueToCalibrationData(source,
            self->nodes, sig_dsp_Calibrator_NUM_STAGES);
        out[i] = px;

        self->previousGate = gate;
    }
}

void sig_dsp_Calibrator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Calibrator* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}

struct sig_dsp_SineWavefolder* sig_dsp_SineWavefolder_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_SineWavefolder* self = sig_MALLOC(allocator,
        struct sig_dsp_SineWavefolder);
    sig_dsp_SineWavefolder_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_SineWavefolder_init(
    struct sig_dsp_SineWavefolder* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_SineWavefolder_generate);

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, gain, context);
    sig_CONNECT_TO_SILENCE(self, factor, context);
}

void sig_dsp_SineWavefolder_generate(void* signal) {
    struct sig_dsp_SineWavefolder* self =
        (struct sig_dsp_SineWavefolder*) signal;

    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT gain = FLOAT_ARRAY(self->inputs.gain);
    const float* SIG_RESTRICT factor = FLOAT_ARRAY(self->inputs.factor);
    for (size_t i = 0; i < blockSize; i++) {
        out[i] = sig_sineWavefolder(in[i], gain[i], factor[i]);
    }
}

void sig_dsp_SineWavefolder_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_SineWavefolder* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_NoiseGate* sig_dsp_NoiseGate_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_NoiseGate* self = sig_MALLOC(allocator,
        struct sig_dsp_NoiseGate);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    sig_dsp_NoiseGate_init(self, context);

    return self;
}

void sig_dsp_NoiseGate_init(struct sig_dsp_NoiseGate* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_NoiseGate_generate);

    struct sig_dsp_ScaleOffset_Parameters parameters = {
        .scale = 1.0,
        .offset= 0.0f
    };
    self->parameters = parameters;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

void sig_dsp_NoiseGate_generate(void* signal) {
    struct sig_dsp_NoiseGate* self = (struct sig_dsp_NoiseGate*) signal;

    const float scale = self->parameters.scale;
    const float offset = self->parameters.offset;
    const size_t blockSize = self->signal.audioSettings->blockSize;
    float* SIG_RESTRICT out = FLOAT_ARRAY(self->outputs.main);
    const float* SIG_RESTRICT in = FLOAT_ARRAY(self->inputs.source);
    const float* SIG_RESTRICT threshold = FLOAT_ARRAY(self->inputs.threshold);
    for (size_t i = 0; i < blockSize; i++) {
        float source = in[i];

        float sample = source >= threshold[i] ? source * scale + offset : 0.0f;
        out[i] = sample;
    }
}

void sig_dsp_NoiseGate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_NoiseGate* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}
