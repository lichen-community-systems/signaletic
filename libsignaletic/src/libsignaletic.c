#include <math.h>   // For powf, fmodf, sinf, roundf, fabsf, rand
#include <stdlib.h> // For RAND_MAX
#include <tlsf.h>   // Includes assert.h, limits.h, stddef.h
                    // stdio.h, stdlib.h, string.h (for errors etc.)
#include <libsignaletic.h>

void sig_Status_init(struct sig_Status* status) {
    sig_Status_reset(status);
}

void sig_Status_reset(struct sig_Status* status) {
    status->result = SIG_RESULT_NONE;
}

inline void sig_Status_reportResult(struct sig_Status* status,
    enum sig_Result result) {
    if (status != NULL) {
        status->result = result;
    }
}

inline float sig_fminf(float a, float b) {
    float r;
#ifdef __arm__
    asm("vminnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a < b) ? a : b;
#endif // __arm__
    return r;
}

inline float sig_fmaxf(float a, float b) {
    float r;
#ifdef __arm__
    asm("vmaxnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a > b) ? a : b;
#endif // __arm__
    return r;
}

// TODO: Unit tests
inline float sig_clamp(float value, float min, float max) {
    return sig_fminf(sig_fmaxf(value, min), max);
}

// TODO: Implement a fast fmodf
// See: https://github.com/electro-smith/DaisySP/blob/0cc02b37579e3619efde73be49a1fa01ffee5cf6/Source/Utility/dsp.h#L89-L95
// TODO: Unit tests
inline float sig_flooredfmodf(float numer, float denom) {
    float remain = fmodf(numer, denom);
    if ((remain > 0.0f && denom < 0.0f) ||
        (remain < 0.0f && denom > 0.0f)) {
        remain = remain + denom;
    }

    return remain;
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

inline float sig_fastTanhf(float x) {
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

// TODO: Unit tests.
inline float sig_linearMap(float value,
    float fromMin, float fromMax, float toMin, float toMax) {
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
inline float sig_sum(float_array_ptr values, size_t length) {
    float sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += FLOAT_ARRAY(values)[i];
    }

    return sum;
}

// TODO: Unit tests.
inline size_t sig_indexOfMin(float_array_ptr values, size_t length) {
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
inline size_t sig_indexOfMax(float_array_ptr values, size_t length) {
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
    float a = FLOAT_ARRAY(table)[idxIntegral];
    float b = FLOAT_ARRAY(table)[(idxIntegral + 1) % length];

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
inline float sig_filter_mean(float_array_ptr values, size_t length) {
    float sum = sig_sum(values, length);
    return sum / (float) length;
}

// TODO: Unit tests.
inline float sig_filter_meanExcludeMinMax(float_array_ptr values,
    size_t length) {
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
inline float sig_filter_ema(float current, float previous, float a) {
    return (a * current) + (1 - a) * previous;
}

inline float sig_filter_onepole(float current, float previous, float b0,
    float a1) {
    return b0 * current - a1 * previous;
}

inline float sig_filter_onepole_HPF_calculateA1(
    float frequency, float sampleRate) {
    float fc = frequency / sampleRate;
    return -expf(-2.0f * sig_PI * (0.5f - fc));
}

inline float sig_filter_onepole_HPF_calculateB0(float a1) {
    return 1.0f + a1;
}

inline float sig_filter_onepole_LPF_calculateA1(
    float frequency, float sampleRate) {
    float fc = frequency / sampleRate;
    return expf(-2.0f * sig_PI * fc);
}

inline float sig_filter_onepole_LPF_calculateB0(float a1) {
    return 1.0f - a1;
}

inline float sig_filter_smooth(float current, float previous, float coeff) {
    return current + coeff * (previous - current);
}

inline float sig_filter_smooth_calculateCoefficient(float time,
    float sampleRate) {
    return expf(sig_LOG0_001 / (time * sampleRate));
}


void sig_filter_Smooth_init(struct sig_filter_Smooth* self, float coeff) {
    self->coeff = coeff;
    self->previous = 0;
}

inline float sig_filter_Smooth_generate(struct sig_filter_Smooth* self,
    float value) {
    float smoothed = sig_filter_smooth(value, self->previous, self->coeff);
    self->previous = smoothed;

    return smoothed;
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



void sig_osc_Oscillator_init(struct sig_osc_Oscillator* self) {
    self->phaseAccumulator = 0.0f;
}

inline float sig_osc_Oscillator_eoc(float phase) {
    return phase < 0.0f || phase > 1.0f ? 1.0f : 0.0f;
}

inline float sig_osc_Oscillator_wrapPhase(float phase) {
    return sig_flooredfmodf(phase, 1.0f);
}

inline void sig_osc_Oscillator_accumulatePhase(float* phaseAccumulator,
    float frequency, float sampleRate) {
    float phaseStep = frequency / sampleRate;
    float phase = *phaseAccumulator + phaseStep;

    *phaseAccumulator = sig_osc_Oscillator_wrapPhase(phase);
}

void sig_osc_Wavetable_init(struct sig_osc_Wavetable* self,
    struct sig_Buffer* wavetable) {
    self->phaseAccumulator = 0.0f;
    self->wavetable = wavetable;
}

inline float sig_osc_Wavetable_generate(struct sig_osc_Wavetable* self,
    float frequency, float phaseOffset, float sampleRate, float* eocOut) {
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

inline float sig_osc_WavetableBank_generate(struct sig_osc_WavetableBank* self,
    float frequency, float phaseOffset, float tableIndex, float sampleRate,
    float* eocOut) {
        // TODO: Precompute this.
        float lastWaveTableIdx = (float) (self->wavetables->length - 1);
        // TODO: Reduce duplication with sig_osc_Wavetable
        float modulatedPhase = self->phaseAccumulator + phaseOffset;
        *eocOut = sig_osc_Oscillator_eoc(modulatedPhase);
        modulatedPhase = sig_flooredfmodf(modulatedPhase, 1.0f);
        float scaledTableIdx = tableIndex * lastWaveTableIdx;
        float sample = sig_WavetableBank_readLinearAtPhase(
            self->wavetables, scaledTableIdx, modulatedPhase);
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

inline void sig_osc_FastLFSine_setFrequency(struct sig_osc_FastLFSine* self,
    float frequency) {
    self->f = 2.0f * sinf(sig_PI * frequency / self->sampleRate);
}

inline void sig_osc_FastLFSine_setFrequencyFast(struct sig_osc_FastLFSine* self,
    float frequency) {
    self->f = sig_TWOPI * frequency / self->sampleRate;
}

inline void sig_osc_FastLFSine_generate(struct sig_osc_FastLFSine* self) {
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
void sig_List_init(struct sig_List* self, void** items, size_t capacity) {
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
void* sig_List_pop(struct sig_List* self, struct sig_Status* status) {
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
    struct sig_Allocator* allocator, struct sig_AudioSettings* audioSettings) {
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

inline float sig_Buffer_readLinearAtPhase(struct sig_Buffer* self,
    float phase) {
    size_t tableLength = self->length;
    float tableIdx = phase * tableLength;

    return sig_interpolate_linear(tableIdx, self->samples, tableLength);
}

float sig_Buffer_readCubic(struct sig_Buffer* self, float idx) {
    return sig_interpolate_cubic(idx, self->samples, self->length);
}

void sig_Buffer_destroy(struct sig_Allocator* allocator, struct sig_Buffer* self) {
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


struct sig_WavetableBank* sig_WavetableBank_new(struct sig_Allocator* allocator,
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

inline float sig_WavetableBank_readLinearAtPhase(struct sig_WavetableBank* self,
    float tableIdx, float phase) {
        int32_t tableIdxIntegral = (int32_t) tableIdx;
        float tableIdxFractional = tableIdx - (float) tableIdxIntegral;
        size_t aTableIndex = tableIdxIntegral % self->length;
        struct sig_Buffer* aTable = self->waves[aTableIndex];
        size_t bTableIndex = (tableIdxIntegral + 1) % self->length;
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

struct sig_DelayLine* sig_DelayLine_new(struct sig_Allocator* allocator,
    size_t maxDelayLength) {
    struct sig_DelayLine* self = sig_MALLOC(allocator, struct sig_DelayLine);
    self->buffer = sig_Buffer_new(allocator, maxDelayLength);
    sig_DelayLine_init(self);

    return self;
}

struct sig_DelayLine* sig_DelayLine_newSeconds(struct sig_Allocator* allocator,
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

inline float sig_DelayLine_readAt(struct sig_DelayLine* self, size_t readPos) {
    size_t idx = (self->writeIdx + readPos) % self->buffer->length;
    return FLOAT_ARRAY(self->buffer->samples)[idx];
}

inline float sig_DelayLine_linearReadAt(struct sig_DelayLine* self,
    float readPos) {
    size_t maxDelayLength = self->buffer->length;
    float* delayLineSamples = self->buffer->samples;
    int32_t integ = (int32_t) readPos;
    float frac = readPos - (float) integ;
    float a = delayLineSamples[(self->writeIdx + integ) % maxDelayLength];
    float b = delayLineSamples[(self->writeIdx + integ + 1) % maxDelayLength];

    return a + (b - a) * frac;
}

inline float sig_DelayLine_cubicReadAt(struct sig_DelayLine* self,
    float readPos) {
    size_t maxDelayLength = self->buffer->length;
    float* delayLineSamples = self->buffer->samples;

    int32_t integ = (int32_t) readPos;
    float frac = readPos - (float) integ;
    int32_t t = (self->writeIdx + integ + maxDelayLength);
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

inline float sig_DelayLine_allpassReadAt(struct sig_DelayLine* self,
    float readPos, float previousSample) {
    size_t maxDelayLength = self->buffer->length;
    float* delayLineSamples = self->buffer->samples;
    int32_t integ = (int32_t) readPos;
    float frac = readPos - (float) integ;
    float invFrac = 1.0f - frac;
    float a = delayLineSamples[(self->writeIdx + integ) % maxDelayLength];
    float b = delayLineSamples[(self->writeIdx + integ + 1) % maxDelayLength];

    return b + invFrac * a - invFrac * previousSample;
}

#define sig_DelayLine_readAtTime_IMPL(self, source, tapTime, sampleRate,\
    readFn)\
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

inline float sig_DelayLine_readAtTime(struct sig_DelayLine* self, float source,
    float tapTime, float sampleRate) {
    sig_DelayLine_readAtTime_IMPL(self, source, tapTime, sampleRate,
        sig_DelayLine_readAt);
}

inline float sig_DelayLine_linearReadAtTime(struct sig_DelayLine* self,
    float source, float tapTime, float sampleRate) {
    sig_DelayLine_readAtTime_IMPL(self, source, tapTime, sampleRate,
        sig_DelayLine_linearReadAt);
}

inline float sig_DelayLine_cubicReadAtTime(struct sig_DelayLine* self,
    float source, float tapTime, float sampleRate) {
    sig_DelayLine_readAtTime_IMPL(self, source, tapTime, sampleRate,
        sig_DelayLine_cubicReadAt);
}

inline float sig_DelayLine_allpassReadAtTime(struct sig_DelayLine* self,
    float source, float tapTime, float sampleRate, float previousSample) {
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

#define sig_DelayLine_readAtTimes_IMPL(self, source, tapTimes, tapGains,\
    numTaps, sampleRate, timeScale, readFn)\
    float tapSum = 0;\
    for (size_t i = 0; i < numTaps; i++) {\
        float tapTime = FLOAT_ARRAY(tapTimes)[i];\
        float scaledTapTime = tapTime * timeScale;\
        float gain = FLOAT_ARRAY(tapGains)[i];\
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

inline float sig_DelayLine_readAtTimes(struct sig_DelayLine* self,
    float source, float_array_ptr tapTimes, float_array_ptr tapGains,
    size_t numTaps, float sampleRate, float timeScale) {
    sig_DelayLine_readAtTimes_IMPL(self, source, tapTimes,
        tapGains, numTaps, sampleRate, timeScale, sig_DelayLine_readAt);
}

inline float sig_DelayLine_linearReadAtTimes(struct sig_DelayLine* self,
    float source, float_array_ptr tapTimes, float_array_ptr tapGains,
    size_t numTaps, float sampleRate, float timeScale) {
    sig_DelayLine_readAtTimes_IMPL(self, source, tapTimes,
        tapGains, numTaps, sampleRate, timeScale, sig_DelayLine_linearReadAt);
}

inline float sig_DelayLine_cubicReadAtTimes(struct sig_DelayLine* self,
    float source, float_array_ptr tapTimes, float_array_ptr tapGains,
    size_t numTaps, float sampleRate, float timeScale) {
    sig_DelayLine_readAtTimes_IMPL(self, source, tapTimes,
        tapGains,numTaps, sampleRate, timeScale, sig_DelayLine_cubicReadAt);
}

inline void sig_DelayLine_write(struct sig_DelayLine* self, float sample) {
    size_t maxDelayLength = self->buffer->length;
    FLOAT_ARRAY(self->buffer->samples)[self->writeIdx] = sample;
    self->writeIdx = (self->writeIdx - 1 + maxDelayLength) % maxDelayLength;
}

inline float sig_DelayLine_calcFeedbackGain(float delayTime, float decayTime) {
    // Convert 60dB time in secs to feedback gain (g) coefficient
    // (also why is the equation in Dodge and Jerse wrong?)
    if (delayTime <= 0.0f || decayTime <= 0.0f) {
        return 0.0f;
    }

    return expf(sig_LOG0_001 * delayTime / decayTime);
}

inline float sig_DelayLine_feedback(float sample, float read, float g) {
    return sample + (g * read);
}

#define sig_DelayLine_comb_IMPL(self, sample, readPos, g, readFn) \
    float read = readFn(self, readPos); \
    float toWrite = sig_DelayLine_feedback(sample, read, g); \
    sig_DelayLine_write(self, toWrite); \
    return read

inline float sig_DelayLine_comb(struct sig_DelayLine* self, float sample,
    size_t readPos, float g) {
    sig_DelayLine_comb_IMPL(self, sample, readPos, g, sig_DelayLine_readAt);
}

inline float sig_DelayLine_linearComb(struct sig_DelayLine* self, float sample,
    float readPos, float g) {
    sig_DelayLine_comb_IMPL(self, sample, readPos, g,
        sig_DelayLine_linearReadAt);
}

inline float sig_DelayLine_cubicComb(struct sig_DelayLine* self, float sample,
    float readPos, float g) {
    sig_DelayLine_comb_IMPL(self, sample, readPos, g,
        sig_DelayLine_cubicReadAt);
}

#define sig_DelayLine_allpass_IMPL(self, sample, readPos, g, readFn) \
    float read = readFn(self, readPos); \
    float toWrite = sample + (g * read); \
    sig_DelayLine_write(self, toWrite); \
    return read - (g * toWrite) \

inline float sig_DelayLine_allpass(struct sig_DelayLine* self, float sample,
    size_t readPos, float g) {
    sig_DelayLine_allpass_IMPL(self, sample, readPos, g, sig_DelayLine_readAt);
}

inline float sig_DelayLine_linearAllpass(struct sig_DelayLine* self,
    float sample, float readPos, float g) {
    sig_DelayLine_allpass_IMPL(self, sample, readPos, g,
        sig_DelayLine_linearReadAt);
}

inline float sig_DelayLine_cubicAllpass(struct sig_DelayLine* self,
    float sample, float readPos, float g) {
    sig_DelayLine_allpass_IMPL(self, sample, readPos, g,
        sig_DelayLine_cubicReadAt);
}

void sig_DelayLine_destroy(struct sig_Allocator* allocator,
    struct sig_DelayLine* self) {
    sig_Buffer_destroy(allocator, self->buffer);
    allocator->impl->free(allocator, self);
}

inline float sig_linearXFade(float left, float right, float mix) {
    float clipped = sig_clamp(mix, -1.0f, 1.0f);
    // At -1.0f, left gain should be 1.0 and right gain should be 0.0;
    // at 0.0, left and right gains should be 0.5;
    // At 1.0 left gain should be 0.0f and right gain should be 1.0.
    float gain = clipped * 0.5f + 0.5f;
    float sample = left + gain * (right - left);

    return sample;
}

inline float sig_sineWavefolder(float x, float gain, float factor) {
    float sample = x + gain * sinf(factor * x);
    return sample;
}

void sig_dsp_Signal_init(void* signal, struct sig_SignalContext* context,
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


inline void sig_dsp_evaluateSignals(struct sig_List* signalList) {
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
    struct sig_dsp_SignalListEvaluator* self, struct sig_List* signalList) {
    self->evaluate = sig_dsp_SignalListEvaluator_evaluate;
    self->signalList = signalList;
}

void sig_dsp_SignalListEvaluator_evaluate(
    struct sig_dsp_SignalEvaluator* evaluator) {
    struct sig_dsp_SignalListEvaluator* self =
        (struct sig_dsp_SignalListEvaluator*) evaluator;

    sig_dsp_evaluateSignals(self->signalList);
}

void sig_dsp_SignalListEvaluator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_SignalListEvaluator* self) {
    allocator->impl->free(allocator, self);
}

struct sig_dsp_Value* sig_dsp_Value_new(struct sig_Allocator* allocator,
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

void sig_dsp_Value_destroy(struct sig_Allocator* allocator, struct sig_dsp_Value* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}

void sig_dsp_Value_generate(void* signal) {
    struct sig_dsp_Value* self = (struct sig_dsp_Value*) signal;

    if (self->parameters.value == self->lastSample) {
        return;
    }

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = self->parameters.value;
    }

    self->lastSample = self->parameters.value;
}


struct sig_dsp_ConstantValue* sig_dsp_ConstantValue_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float rectified = fabsf(source);
        FLOAT_ARRAY(self->outputs.main)[i] = rectified;
    }
}

void sig_dsp_Abs_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Abs* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}



struct sig_dsp_Clamp* sig_dsp_Clamp_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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
    float min = self->parameters.min;
    float max = self->parameters.max;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float sample = sig_clamp(source, min, max);
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
    }
}

void sig_dsp_Clamp_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Clamp* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}



struct sig_dsp_ScaleOffset* sig_dsp_ScaleOffset_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        FLOAT_ARRAY(self->outputs.main)[i] = source * scale + offset;
    }
}

void sig_dsp_ScaleOffset_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ScaleOffset* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_dsp_Sine* sig_dsp_Sine_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        FLOAT_ARRAY(self->outputs.main)[i] = sinf(source);
    }
}

void sig_dsp_Sine_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Sine* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}


struct sig_dsp_BinaryOp* sig_dsp_BinaryOp_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context, sig_dsp_generateFn generate) {
    struct sig_dsp_BinaryOp* self = sig_MALLOC(allocator,
        struct sig_dsp_BinaryOp);
    sig_dsp_BinaryOp_init(self, context, generate);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_BinaryOp_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context, sig_dsp_generateFn generate) {
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    return sig_dsp_BinaryOp_new(allocator, context, *sig_dsp_Add_generate);
}

void sig_dsp_Add_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context) {
    sig_dsp_BinaryOp_init(self, context, *sig_dsp_Add_generate);
}

// TODO: Unit tests.
void sig_dsp_Add_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs.left)[i];
        float right = FLOAT_ARRAY(self->inputs.right)[i];
        float val = left + right;

        FLOAT_ARRAY(self->outputs.main)[i] = val;
    }
}

void sig_dsp_Add_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_BinaryOp_destroy(allocator, self);
}



struct sig_dsp_BinaryOp* sig_dsp_Sub_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    return sig_dsp_BinaryOp_new(allocator, context, *sig_dsp_Sub_generate);
}

void sig_dsp_Sub_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context) {
    sig_dsp_BinaryOp_init(self, context, *sig_dsp_Sub_generate);
}

// TODO: Unit tests.
void sig_dsp_Sub_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs.left)[i];
        float right = FLOAT_ARRAY(self->inputs.right)[i];
        float val = left - right;

        FLOAT_ARRAY(self->outputs.main)[i] = val;
    }
}

void sig_dsp_Sub_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_BinaryOp_destroy(allocator, self);
}



struct sig_dsp_BinaryOp* sig_dsp_Mul_new(struct sig_Allocator* allocator,
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs.left)[i];
        float right = FLOAT_ARRAY(self->inputs.right)[i];
        float val = left * right;
        FLOAT_ARRAY(self->outputs.main)[i] = val;
    }
}


struct sig_dsp_BinaryOp* sig_dsp_Div_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    return sig_dsp_BinaryOp_new(allocator, context, *sig_dsp_Div_generate);
}

void sig_dsp_Div_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context) {
    sig_dsp_BinaryOp_init(self, context, *sig_dsp_Div_generate);
}

void sig_dsp_Div_generate(void* signal) {
    struct sig_dsp_BinaryOp* self = (struct sig_dsp_BinaryOp*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs.left)[i];
        float right = FLOAT_ARRAY(self->inputs.right)[i];
        float val = left / right;
        FLOAT_ARRAY(self->outputs.main)[i] = val;
    }
}

void sig_dsp_Div_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self) {
    sig_dsp_BinaryOp_destroy(allocator, self);
}


struct sig_dsp_Invert* sig_dsp_Invert_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float inSamp = FLOAT_ARRAY(self->inputs.source)[i];
        FLOAT_ARRAY(self->outputs.main)[i] = -inSamp;
    }
}

void sig_dsp_Invert_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Invert* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_Accumulate* sig_dsp_Accumulate_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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
    float wrap = self->parameters.wrap;
    float maxValue = self->parameters.maxValue;

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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = self->accumulator;
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        // TODO: MSVC compiler warning loss of precision.
        unsigned long durationSamps = (unsigned long)
            (FLOAT_ARRAY(self->inputs.duration)[i] *
            self->signal.audioSettings->sampleRate);
        float gate = FLOAT_ARRAY(self->inputs.gate)[i];

        if (gate > 0.0f) {
            // Gate is open.
            if (!self->hasFired ||
                FLOAT_ARRAY(self->inputs.loop)[i] > 0.0f) {
                self->timer++;
            }

            if (self->timer >= durationSamps) {
                // We reached the duration time.
                FLOAT_ARRAY(self->outputs.main)[i] = 1.0f;

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

        FLOAT_ARRAY(self->outputs.main)[i] = 0.0f;
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
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
                self->inputs.duration)[i] *
                self->signal.audioSettings->sampleRate);

            if (self->timer >= durSamps) {
                // Time's up.
                // Fire a trigger if we've the right number of
                // incoming triggers, otherwise just reset.
                if (self->numTriggers ==
                    (int) FLOAT_ARRAY(self->inputs.count)[i]) {
                    outputSample = 1.0f;
                }

                self->isTimerActive = false;
                self->numTriggers = 0;
                self->timer = 0;
            }
        }

        self->previousSource = source;
        FLOAT_ARRAY(self->outputs.main)[i] = outputSample;
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float trigger = FLOAT_ARRAY(self->inputs.trigger)[i];
        if (trigger > 0.0f && self->prevTrig <= 0.0f) {
            // Received a trigger, toggle the gate.
            self->isGateOpen = !self->isGateOpen;
        }

        FLOAT_ARRAY(self->outputs.main)[i] = (float) self->isGateOpen;

        self->prevTrig = trigger;
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
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
    struct sig_SignalContext* context, sig_dsp_generateFn generate) {
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

struct sig_dsp_Oscillator* sig_dsp_SineOscillator_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    return sig_dsp_Oscillator_new(allocator, context, *sig_dsp_SineOscillator_generate);
}

void sig_dsp_SineOscillator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self) {
    sig_dsp_Oscillator_destroy(allocator, self);
}

void sig_dsp_SineOscillator_generate(void* signal) {
    struct sig_dsp_Oscillator* self = (struct sig_dsp_Oscillator*) signal;
    float sampleRate = self->signal.audioSettings->sampleRate;
    float* frequency = FLOAT_ARRAY(self->inputs.freq);
    float* phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    float* mul = FLOAT_ARRAY(self->inputs.mul);
    float* add = FLOAT_ARRAY(self->inputs.add);
    float* mainOutput = FLOAT_ARRAY(self->outputs.main);
    float* eocOutput = FLOAT_ARRAY(self->outputs.eoc);

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
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
    float sampleRate = self->signal.audioSettings->sampleRate;
    float* frequency = FLOAT_ARRAY(self->inputs.freq);
    float* phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    float* mul = FLOAT_ARRAY(self->inputs.mul);
    float* add = FLOAT_ARRAY(self->inputs.add);
    float* mainOutput = FLOAT_ARRAY(self->outputs.main);
    float* eocOutput = FLOAT_ARRAY(self->outputs.eoc);

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
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


void sig_dsp_WavetableOscillator_init(struct sig_dsp_WavetableOscillator* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_WavetableOscillator_generate);

    sig_CONNECT_TO_SILENCE(self, freq, context);
    sig_CONNECT_TO_SILENCE(self, phaseOffset, context);
    sig_CONNECT_TO_UNITY(self, mul, context);
    sig_CONNECT_TO_SILENCE(self, add, context);

    sig_osc_Wavetable_init(&self->state, self->wavetable);
}

struct sig_dsp_WavetableOscillator* sig_dsp_WavetableOscillator_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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
    float sampleRate = self->signal.audioSettings->sampleRate;
    float* frequency = FLOAT_ARRAY(self->inputs.freq);
    float* phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    float* mul = FLOAT_ARRAY(self->inputs.mul);
    float* add = FLOAT_ARRAY(self->inputs.add);
    float* eocOutput = FLOAT_ARRAY(self->outputs.eoc);
    float* mainOutput = FLOAT_ARRAY(self->outputs.main);

    // TODO: This has to be assigned because we don't know when
    // it is been updated by the user.
    self->state.wavetable = self->wavetable;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float sample = sig_osc_Wavetable_generate(&self->state,
            frequency[i], phaseOffset[i], sampleRate, &eocOutput[i]);
        float scaledSample = sample * mul[i] + add[i];

        mainOutput[i] = scaledSample;
    }
}

void sig_dsp_WavetableOscillator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_WavetableOscillator* self) {
    sig_dsp_Oscillator_Outputs_destroyAudioBlocks(allocator, &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
    self = NULL;
}



void sig_dsp_WavetableBankOscillator_init(struct sig_dsp_WavetableBankOscillator* self,
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_WavetableBankOscillator* self = sig_MALLOC(allocator,
        struct sig_dsp_WavetableBankOscillator);
    sig_dsp_WavetableBankOscillator_init(self, context);
    sig_dsp_Oscillator_Outputs_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

inline void sig_dsp_WavetableBankOscillator_generate(void* signal) {
    struct sig_dsp_WavetableBankOscillator* self =
        (struct sig_dsp_WavetableBankOscillator*) signal;
    self->state.wavetables = self->wavetables;
    float sampleRate = self->signal.audioSettings->sampleRate;
    float* frequency = FLOAT_ARRAY(self->inputs.freq);
    float* phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    float* tableIndex = FLOAT_ARRAY(self->inputs.tableIndex);
    float* mul = FLOAT_ARRAY(self->inputs.mul);
    float* add = FLOAT_ARRAY(self->inputs.add);
    float* eocOutput = FLOAT_ARRAY(self->outputs.eoc);
    float* mainOutput = FLOAT_ARRAY(self->outputs.main);
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float sample = sig_osc_WavetableBank_generate(&self->state,
            frequency[i], phaseOffset[i], tableIndex[i], sampleRate,
            &eocOutput[i]);
        float scaledSample = sample * mul[i] + add[i];

        mainOutput[i] = scaledSample;
    }
}

void sig_dsp_WavetableBankOscillator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_WavetableBankOscillator* self) {
    sig_dsp_Oscillator_Outputs_destroyAudioBlocks(allocator, &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
    self = NULL;
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

struct sig_dsp_Smooth* sig_dsp_Smooth_new(struct sig_Allocator* allocator,
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = previousSample =
            sig_filter_smooth(
                FLOAT_ARRAY(self->inputs.source)[i], previousSample, a1);
    }

    self->previousSample = previousSample;
}

void sig_dsp_Smooth_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Smooth* self) {
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

struct sig_dsp_OnePole* sig_dsp_OnePole_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_OnePole* self = sig_MALLOC(allocator,
        struct sig_dsp_OnePole);
    sig_dsp_OnePole_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

inline void sig_dsp_OnePole_recalculateCoefficients(
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

    float previousSample = self->previousSample;
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float frequency = FLOAT_ARRAY(self->inputs.frequency)[i];
        if (self->previousFrequency != frequency ||
            self->previousMode != self->parameters.mode) {
            sig_dsp_OnePole_recalculateCoefficients(self, frequency);
        }

        float sample = sig_filter_onepole(FLOAT_ARRAY(self->inputs.source)[i],
            previousSample, self->b0, self->a1);
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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
    float alpha = self->parameters.alpha;
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float sample = sig_filter_ema(source, previousSample, alpha);
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
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

struct sig_dsp_Tanh* sig_dsp_Tanh_new(struct sig_Allocator* allocator,
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float inSamp = FLOAT_ARRAY(self->inputs.source)[i];
        float outSamp = tanhf(inSamp);
        FLOAT_ARRAY(self->outputs.main)[i] = outSamp;
    }
}

void sig_dsp_Tanh_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Tanh* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_dsp_Looper_init(struct sig_dsp_Looper* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Looper_generate);

    struct sig_dsp_Looper_Loop loop = {
        .buffer = NULL,
        .startIdx = 0,
        .length = 0,
        .isEmpty = true
    };

    self->loop = loop;
    self->playbackPos = 0.0f;
    self->previousRecord = 0.0f;
    self->previousClear = 0.0f;

    sig_dsp_Looper_setBuffer(self, context->emptyBuffer);

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, start, context);
    sig_CONNECT_TO_UNITY(self, end, context);
    sig_CONNECT_TO_UNITY(self, speed, context);
    sig_CONNECT_TO_SILENCE(self, record, context);
    sig_CONNECT_TO_SILENCE(self, clear, context);
}

struct sig_dsp_Looper* sig_dsp_Looper_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Looper* self = sig_MALLOC(allocator,
        struct sig_dsp_Looper);
    sig_dsp_Looper_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

// TODO: Should buffers just be required at initialization? (Probably, yes.)
void sig_dsp_Looper_setBuffer(struct sig_dsp_Looper* self,
    struct sig_Buffer* buffer) {
    self->loop.buffer = buffer;
    self->loopLastIdx = self->loop.buffer->length - 1;
}

static inline float sig_dsp_Looper_record(struct sig_dsp_Looper* self,
    float startPos, float endPos, size_t i) {
    struct sig_dsp_Looper_Loop* loop = &self->loop;
    float* recordBuffer = FLOAT_ARRAY(loop->buffer->samples);
    size_t playbackIdx = (size_t) self->playbackPos;

    // Start with the current input sample.
    float sample = FLOAT_ARRAY(self->inputs.source)[i];

    if (loop->isEmpty) {
        if (self->previousRecord <= 0.0f) {
            // We're going forward; reset to the beginning of the buffer.
            self->playbackPos = 0.0f;
            loop->startIdx = 0;
        }

        if (loop->length < loop->buffer->length) {
            // TODO: Add support for recording in reverse
            // on the first overdub.
            loop->length++;
        }
    } else {
        // We're overdubbing.
        // Reduce the volume of the previously recorded audio by 10%
        float previousRecordedSample = recordBuffer[playbackIdx] * 0.9;

        // Mix it in with the input.
        sample += previousRecordedSample;
    }

    // Add a little distortion/limiting.
    sample = tanhf(sample);

    // Replace the audio in the buffer with the new mix.
    recordBuffer[playbackIdx] = sample;

    return sample;
}

static inline void sig_dsp_Looper_clearBuffer(struct sig_dsp_Looper* self) {
    // For performance reasons, the buffer is never actually zeroed,
    // it's just marked as empty.
    // TODO: Fade out before clearing the buffer (gh-28).
    self->loop.length = 0;
    self->loop.isEmpty = true;
    self->loopLastIdx = self->loop.buffer->length - 1;
}

static inline float sig_dsp_Looper_play(struct sig_dsp_Looper* self, size_t i) {
    struct sig_dsp_Looper_Loop* loop = &self->loop;
    if (self->previousRecord > 0.0f && loop->isEmpty) {
        loop->startIdx = 0;
        self->loopLastIdx = loop->length - 1;
        loop->isEmpty = false;
    }

    if (FLOAT_ARRAY(self->inputs.clear)[i] > 0.0f &&
        self->previousClear == 0.0f) {
        sig_dsp_Looper_clearBuffer(self);
    }

    float outputSample = FLOAT_ARRAY(self->inputs.source)[i];

    if (!loop->isEmpty) {
        // Only read from the record buffer if it has something in it
        // (because we don't actually erase its contents when clearing).

        // TODO: The sig_interpolate_linear implementation
        // may wrap around inappropriately to the beginning of
        // the buffer (not to the startPos) if we're right at
        // the end of the buffer.
        outputSample += sig_Buffer_readLinear(
            loop->buffer, self->playbackPos);
    }

    return outputSample;
}

static inline float sig_dsp_Looper_nextPosition(float currentPos,
    float startPos, float endPos, float speed) {
    float nextPlaybackPos = currentPos + speed;
    if (nextPlaybackPos > endPos) {
        nextPlaybackPos = startPos + (nextPlaybackPos - endPos);
    } else if (nextPlaybackPos < startPos) {
        nextPlaybackPos = endPos - (startPos - nextPlaybackPos);
    }

    return nextPlaybackPos;
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float speed = FLOAT_ARRAY(self->inputs.speed)[i];

        // Clamp the start and end inputs to within 0..1.
        float start = sig_clamp(FLOAT_ARRAY(self->inputs.start)[i],
            0.0, 1.0);
        float end = sig_clamp(FLOAT_ARRAY(self->inputs.end)[i],
            0.0, 1.0);

        // Flip the start and end points if they're reversed.
        if (start > end) {
            float temp = start;
            start = end;
            end = temp;
        }

        float startPos = roundf(self->loopLastIdx * start);
        float endPos = roundf(self->loopLastIdx * end);

        float outputSample = 0.0f;
        if ((endPos - startPos) <= fabsf(speed)) {
            // The loop size is too small to play at this speed.
            outputSample = FLOAT_ARRAY(self->inputs.source)[i];
        } else if (FLOAT_ARRAY(self->inputs.record)[i] > 0.0f) {
            // Playback has to be at regular speed
            // while recording, so ignore any modulation
            // and only use its direction.
            // Also, the first recording has to be in forward.
            // TODO: Add support for cool tape-style effects
            // when overdubbing at different speeds.
            // Note: Naively omitting this will work,
            // but introduces lots pitched resampling artifacts.
            speed = self->loop.isEmpty ? 1.0 : speed >=0 ? 1.0 : -1.0;
            outputSample = sig_dsp_Looper_record(self, startPos, endPos, i);
        } else {
            outputSample = sig_dsp_Looper_play(self, i);
        }

        FLOAT_ARRAY(self->outputs.main)[i] = outputSample;

        self->playbackPos = sig_dsp_Looper_nextPosition(self->playbackPos,
            startPos, endPos, speed);

        self->previousRecord = FLOAT_ARRAY(self->inputs.record)[i];
        self->previousClear = FLOAT_ARRAY(self->inputs.clear)[i];
    }
}

void sig_dsp_Looper_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Looper* self) {
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

struct sig_dsp_Dust* sig_dsp_Dust_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {

    struct sig_dsp_Dust* self = sig_MALLOC(allocator, struct sig_dsp_Dust);
    sig_dsp_Dust_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_Dust_generate(void* signal) {
    struct sig_dsp_Dust* self = (struct sig_dsp_Dust*) signal;

    float scaleDiv = self->parameters.bipolar > 0.0f ? 2.0f : 1.0f;
    float scaleSub = self->parameters.bipolar > 0.0f ? 1.0f : 0.0f;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float density = FLOAT_ARRAY(self->inputs.density)[i];

        if (density != self->previousDensity) {
            self->previousDensity = density;
            self->threshold = density * self->sampleDuration;
            self->scale = self->threshold > 0.0f ?
                scaleDiv / self->threshold : 0.0f;
        }

        float rand = sig_randf();
        float val = rand < self->threshold ?
            rand * self->scale - scaleSub : 0.0f;
        FLOAT_ARRAY(self->outputs.main)[i] = val;
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_TimedGate* self = sig_MALLOC(allocator,
        struct sig_dsp_TimedGate);
    sig_dsp_TimedGate_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

static inline void sig_dsp_TimedGate_outputHigh(struct sig_dsp_TimedGate* self,
    size_t index) {
    FLOAT_ARRAY(self->outputs.main)[index] = self->gateValue;
    self->samplesRemaining--;
}

static inline void sig_dsp_TimedGate_outputLow(struct sig_dsp_TimedGate* self,
    size_t index) {
    FLOAT_ARRAY(self->outputs.main)[index] = 0.0f;
}

void sig_dsp_TimedGate_generate(void* signal) {
    struct sig_dsp_TimedGate* self = (struct sig_dsp_TimedGate*)
        signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float currentTrigger = FLOAT_ARRAY(self->inputs.trigger)[i];
        float duration = FLOAT_ARRAY(self->inputs.duration)[i];

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
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_DustGate* sig_dsp_DustGate_new(struct sig_Allocator* allocator,
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


void sig_dsp_ClockSource_init(
    struct sig_dsp_ClockSource* self,
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_ClockSource* self = sig_MALLOC(allocator,
        struct sig_dsp_ClockSource);
    sig_dsp_ClockSource_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

static inline void updateTempo(struct sig_dsp_ClockSource* self,
    size_t tempoDuration) {
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
    float_array_ptr pulse = self->inputs.pulse;
    float_array_ptr tap = self->inputs.tap;
    float threshold = self->parameters.threshold;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float pulseValue = FLOAT_ARRAY(pulse)[i];
        float tapValue = FLOAT_ARRAY(tap)[i];

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

        FLOAT_ARRAY(self->outputs.main)[i] = sample;
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
    float_array_ptr source = self->inputs.source;

    float previousTrigger = self->previousTrigger;
    float clockFreq = self->clockFreq;
    bool isRisingEdge = self->isRisingEdge;
    uint32_t numPulsesDetected = self->numPulsesDetected;
    uint32_t samplesSinceLastPulse = self->samplesSinceLastPulse;
    float sampleRate = self->signal.audioSettings->sampleRate;
    float threshold = self->parameters.threshold;
    uint32_t pulseDurSamples = self->pulseDurSamples;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        samplesSinceLastPulse++;

        float sourceSamp = FLOAT_ARRAY(source)[i];
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

        FLOAT_ARRAY(self->outputs.main)[i] = clockFreq;
        FLOAT_ARRAY(self->outputs.bpm)[i] = clockFreq * 60.0f;

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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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
    float_array_ptr source = self->inputs.source;
    float_array_ptr output = self->outputs.main;
    float middleFreq = self->parameters.middleFreq;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float inputSample = FLOAT_ARRAY(source)[i];
        FLOAT_ARRAY(output)[i] = sig_linearToFreq(inputSample, middleFreq);
    }
}

void sig_dsp_LinearToFreq_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_LinearToFreq* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_Branch* sig_dsp_Branch_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float condition = FLOAT_ARRAY(self->inputs.condition)[i];
        float off = FLOAT_ARRAY(self->inputs.off)[i];
        float on = FLOAT_ARRAY(self->inputs.on)[i];

        FLOAT_ARRAY(self->outputs.main)[i] = condition > 0.0f ? on : off;
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

void sig_dsp_List_Outputs_destroyAudioBlocks(struct sig_Allocator* allocator,
    struct sig_dsp_List_Outputs* outputs) {
    sig_AudioBlock_destroy(allocator, outputs->main);
    sig_AudioBlock_destroy(allocator, outputs->index);
    sig_AudioBlock_destroy(allocator, outputs->length);
}

struct sig_dsp_List* sig_dsp_List_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

inline float sig_dsp_List_constrain(bool shouldWrap, float index,
    float lastIndex, float listLength) {
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
    size_t blockSize = self->signal.audioSettings->blockSize;

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

    if (listLength < 1) {
        // There's nothing in the list; just output silence.
        for (size_t i = 0; i < blockSize; i++) {
            FLOAT_ARRAY(self->outputs.main)[i] = 0.0f;
            FLOAT_ARRAY(self->outputs.index)[i] = -1.0f;
            FLOAT_ARRAY(self->outputs.length)[i] = listLengthF;
        }
    } else {
        for (size_t i = 0; i < blockSize; i++) {
            float index = FLOAT_ARRAY(self->inputs.index)[i];
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

            FLOAT_ARRAY(self->outputs.main)[i] = sample;
            FLOAT_ARRAY(self->outputs.index)[i] = index;
            FLOAT_ARRAY(self->outputs.length)[i] = listLengthF;
        }
    }
}

void sig_dsp_List_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_List* self) {
    sig_dsp_List_Outputs_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_LinearMap* sig_dsp_LinearMap_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = sig_linearMap(
            FLOAT_ARRAY(self->inputs.source)[i],
            self->parameters.fromMin,
            self->parameters.fromMax,
            self->parameters.toMin,
            self->parameters.toMax);
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

struct sig_dsp_TwoOpFM* sig_dsp_TwoOpFM_new(struct sig_Allocator* allocator,
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
    float* frequency = FLOAT_ARRAY(self->inputs.frequency);
    float* index = FLOAT_ARRAY(self->inputs.index);
    float* ratio = FLOAT_ARRAY(self->inputs.ratio);
    float* feedbackGain = FLOAT_ARRAY(self->inputs.feedbackGain);
    float* phaseOffset = FLOAT_ARRAY(self->inputs.phaseOffset);
    float* modulatorPhaseOffset = FLOAT_ARRAY(
        self->inputs.modulatorPhaseOffset);
    float sampleRate = self->signal.audioSettings->sampleRate;
    float* modulatorEOCOut = FLOAT_ARRAY(self->outputs.modulatorEOC);
    float* carrierEOCOut = FLOAT_ARRAY(self->outputs.carrierEOC);
    float* mainOut = FLOAT_ARRAY(self->outputs.main);
    float* modulatorOut = FLOAT_ARRAY(self->outputs.modulator);
    struct sig_Buffer* feedbackBuffer = self->feedbackDelay.buffer;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
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


struct sig_dsp_Ladder* sig_dsp_Ladder_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

inline void sig_dsp_Ladder_calcCoefficients(
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

inline float sig_dsp_Ladder_calcStage(
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
    float interpolationRecip = self->interpolationRecip;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float input = FLOAT_ARRAY(self->inputs.source)[i];
        float frequency = FLOAT_ARRAY(self->inputs.frequency)[i];
        float resonance = FLOAT_ARRAY(self->inputs.resonance)[i];
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
        FLOAT_ARRAY(self->outputs.main)[i] =
            (totals[0] * FLOAT_ARRAY(self->inputs.inputGain)[i]) +
            (totals[1] * FLOAT_ARRAY(self->inputs.pole1Gain)[i]) +
            (totals[2] * FLOAT_ARRAY(self->inputs.pole2Gain)[i]) +
            (totals[3] * FLOAT_ARRAY(self->inputs.pole3Gain)[i]) +
            (totals[4] * FLOAT_ARRAY(self->inputs.pole4Gain)[i]);
        FLOAT_ARRAY(self->outputs.twoPole)[i] = totals[2];
        FLOAT_ARRAY(self->outputs.fourPole)[i] = totals[4];
    }
}

void sig_dsp_Ladder_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Ladder* self) {
    sig_dsp_FourPoleFilter_Outputs_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_TiltEQ* sig_dsp_TiltEQ_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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
    float amp = 8.656170f; // 6.0f / log(2)
    float gfactor = 5.0f; // Proportional gain.
    float sr3 = self->sr3;
    float lpOut = self->lpOut;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float f0 = FLOAT_ARRAY(self->inputs.frequency)[i];
        float gain = FLOAT_ARRAY(self->inputs.gain)[i] * 6.0f; // Convert to dB.
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
        FLOAT_ARRAY(self->outputs.main)[i] = source + lgain * lpOut +
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

inline void sig_dsp_Delay_read(struct sig_dsp_Delay* self, float source,
    size_t i) {
    float delayTime = FLOAT_ARRAY(self->inputs.delayTime)[i];

    FLOAT_ARRAY(self->outputs.main)[i] = sig_DelayLine_cubicReadAtTime(
        self->delayLine,
        source,
        delayTime,
        self->signal.audioSettings->sampleRate);
}

void sig_dsp_Delay_generate(void* signal) {
    struct sig_dsp_Delay* self = (struct sig_dsp_Delay*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        sig_dsp_Delay_read(self, source, i);
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        sig_dsp_Delay_read(self, source, i);
    }
}

void sig_dsp_DelayTap_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Delay* self) {
    // Don't destroy the delay line; it isn't owned.
    sig_dsp_Delay_destroy(allocator, self);
}



struct sig_dsp_DelayWrite* sig_dsp_DelayWrite_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        sig_DelayLine_write(self->delayLine, source);
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float maxDelayLength = (float) self->delayLine->buffer->length;
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float feedbackGain = FLOAT_ARRAY(self->inputs.feedbackGain)[i];
        float delayTime = FLOAT_ARRAY(self->inputs.delayTime)[i];
        float lpfCoefficient = FLOAT_ARRAY(self->inputs.lpfCoefficient)[i];
        float readPos = (delayTime * self->signal.audioSettings->sampleRate);
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
        FLOAT_ARRAY(self->outputs.main)[i] = outputSample;
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float maxDelayLength = (float) self->delayLine->buffer->length;
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float delayTime = FLOAT_ARRAY(self->inputs.delayTime)[i];
        float g = FLOAT_ARRAY(self->inputs.g)[i];
        float readPos = (delayTime * self->signal.audioSettings->sampleRate);
        if (readPos >= maxDelayLength) {
            readPos = maxDelayLength - 1;
        }

        if (delayTime <= 0.0f || g <= 0.0f) {
            FLOAT_ARRAY(self->outputs.main)[i] = source;
        } else {
            FLOAT_ARRAY(self->outputs.main)[i] = sig_DelayLine_linearAllpass(
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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float delayTime = FLOAT_ARRAY(self->inputs.delayTime)[i];
        float speed = FLOAT_ARRAY(self->inputs.speed)[i];
        float width = FLOAT_ARRAY(self->inputs.width)[i];
        float feedbackGain = FLOAT_ARRAY(self->inputs.feedbackGain)[i];
        float feedforwardGain = FLOAT_ARRAY(self->inputs.feedforwardGain)[i];
        float blend = FLOAT_ARRAY(self->inputs.blend)[i];

        sig_osc_FastLFSine_setFrequencyFast(&self->modulator, speed);
        sig_osc_FastLFSine_generate(&self->modulator);
        FLOAT_ARRAY(self->outputs.modulator)[i] = self->modulator.sinZ;

        // TODO: Add one pole low pass filters in both the feedback and
        // feedforward lines to support echo.

        float fixedRead = sig_DelayLine_allpassReadAtTime(self->delayLine,
            source, delayTime, self->signal.audioSettings->sampleRate,
            self->previousFixedOutput);
        self->previousFixedOutput = fixedRead;
        float toWrite = source - (fixedRead * feedbackGain);
        sig_DelayLine_write(self->delayLine, toWrite);

        float modulatedDelayTime = self->modulator.sinZ * width + delayTime;
        float modulatedRead = sig_DelayLine_allpassReadAtTime(self->delayLine,
            source, modulatedDelayTime, self->signal.audioSettings->sampleRate,
            self->previousModulatedOutput);
        self->previousModulatedOutput = modulatedRead;
        float feedforwardSample = modulatedRead * feedforwardGain;
        float output = (toWrite * blend) + feedforwardSample;

        // TODO: What kind of gain staging should we do here?
        // It seems likely that we can have up to 3x gain depending on
        // the values of feedbackGain, feedforwardGain, and blend.
        FLOAT_ARRAY(self->outputs.main)[i] = tanhf(output / 3.0f);
    }
}

void sig_dsp_Chorus_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Chorus* self) {
    // Don't destroy the delay line; it isn't owned.
    sig_dsp_Chorus_Outputs_destroyAudioBlocks(allocator, &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_LinearXFade* sig_dsp_LinearXFade_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float left = FLOAT_ARRAY(self->inputs.left)[i];
        float right = FLOAT_ARRAY(self->inputs.right)[i];
        float mix = FLOAT_ARRAY(self->inputs.mix)[i];
        FLOAT_ARRAY(self->outputs.main)[i] = sig_linearXFade(left, right, mix);
    }
}

void sig_dsp_LinearXFade_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_LinearXFade* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}



struct sig_dsp_Calibrator* sig_dsp_Calibrator_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_Calibrator* self = sig_MALLOC(allocator,
        struct sig_dsp_Calibrator);
    sig_dsp_Calibrator_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

inline void sig_dsp_Calibrator_Node_init(
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

inline size_t sig_dsp_Calibrator_locateIntervalForValue(float x,
    struct sig_dsp_Calibrator_Node* nodes, size_t numNodes) {
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

inline float sig_dsp_Calibrator_fitValueToCalibrationData(float x,
    struct sig_dsp_Calibrator_Node* nodes, size_t numNodes) {
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

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float gate = FLOAT_ARRAY(self->inputs.gate)[i];
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
        FLOAT_ARRAY(self->outputs.main)[i] = px;

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
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_SineWavefolder* self = sig_MALLOC(allocator,
        struct sig_dsp_SineWavefolder);
    sig_dsp_SineWavefolder_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_SineWavefolder_init(struct sig_dsp_SineWavefolder* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_SineWavefolder_generate);

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, gain, context);
    sig_CONNECT_TO_SILENCE(self, factor, context);
}

void sig_dsp_SineWavefolder_generate(void* signal) {
    struct sig_dsp_SineWavefolder* self =
        (struct sig_dsp_SineWavefolder*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float gain = FLOAT_ARRAY(self->inputs.gain)[i];
        float factor = FLOAT_ARRAY(self->inputs.factor)[i];

        float sample = sig_sineWavefolder(source, gain, factor);

        FLOAT_ARRAY(self->outputs.main)[i] = sample;
    }
}

void sig_dsp_SineWavefolder_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_SineWavefolder* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}


struct sig_dsp_NoiseGate* sig_dsp_NoiseGate_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
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

    float scale = self->parameters.scale;
    float offset = self->parameters.offset;
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float threshold = FLOAT_ARRAY(self->inputs.threshold)[i];

        float sample = source >= threshold ? source * scale + offset : 0.0f;
        FLOAT_ARRAY(self->outputs.main)[i] = sample;
    }
}

void sig_dsp_NoiseGate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_NoiseGate* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}
