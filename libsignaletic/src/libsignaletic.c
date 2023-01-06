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

float sig_freqToMidi(float frequency) {
    return(12.0f * logf(frequency / 440.0f) / sig_LOG2) + 69.0f;
}

float sig_linearToFreq(float vOct, float middleFreq) {
    return middleFreq * powf(2, vOct);
}

float sig_freqToLinear(float freq, float middleFreq) {
    return (logf(freq / middleFreq) / sig_LOG2);
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
void sig_TLSFAllocator_init(struct sig_Allocator* allocator) {
    tlsf_create_with_pool(allocator->heap->memory,
        allocator->heap->length);
}

void* sig_TLSFAllocator_malloc(struct sig_Allocator* allocator,
    size_t size) {
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
        return sig_List_append(self, item, status);
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
        } else if (phase < 0.0) {
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
    struct sig_Buffer* self = (struct sig_Buffer*)
        allocator->impl->malloc(allocator,
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
    allocator->impl->free(allocator, self);
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
    outputs->main = sig_AudioBlock_new(allocator, audioSettings);
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
    self->inputs.left = context->silence->outputs.main;
    self->inputs.right = context->silence->outputs.main;
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
        .accumulatorStart = 1.0
    };
    self->parameters = parameters;

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

    float reset = FLOAT_ARRAY(self->inputs.reset)[0];
    if (reset > 0.0f && self->previousReset <= 0.0f) {
        // Reset the accumulator if we received a trigger.
        self->accumulator = self->parameters.accumulatorStart;
    }

    self->accumulator += FLOAT_ARRAY(self->inputs.source)[0];

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
            FLOAT_ARRAY(self->inputs.duration)[i] *
            self->signal.audioSettings->sampleRate;
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
    outputs->main = sig_AudioBlock_new(allocator, audioSettings);
    outputs->eoc = sig_AudioBlock_new(allocator, audioSettings);
}

void sig_dsp_Oscillator_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator_Outputs* outputs) {
    sig_AudioBlock_destroy(allocator, outputs->main);
    sig_AudioBlock_destroy(allocator, outputs->eoc);
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

    self->phaseAccumulator = 0.0f;
}

inline void sig_dsp_Oscillator_accumulatePhase(
    struct sig_dsp_Oscillator* self, size_t i) {
    float phaseStep = FLOAT_ARRAY(self->inputs.freq)[i] /
        self->signal.audioSettings->sampleRate * sig_TWOPI;

    self->phaseAccumulator += phaseStep;

    float eocValue = 0.0;
    if (self->phaseAccumulator > sig_TWOPI) {
        self->phaseAccumulator -= sig_TWOPI;
        eocValue = 1.0;
    }
    FLOAT_ARRAY(self->outputs.eoc)[i] = eocValue;
}

void sig_dsp_Sine_init(struct sig_dsp_Oscillator* self,
    struct sig_SignalContext* context) {
    sig_dsp_Oscillator_init(self, context, *sig_dsp_Sine_generate);
}

struct sig_dsp_Oscillator* sig_dsp_Sine_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    return sig_dsp_Oscillator_new(allocator, context, *sig_dsp_Sine_generate);
}

void sig_dsp_Sine_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self) {
    sig_dsp_Oscillator_destroy(allocator, self);
}

void sig_dsp_Sine_generate(void* signal) {
    struct sig_dsp_Oscillator* self = (struct sig_dsp_Oscillator*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        // TODO: Negative offsets will fail here.
        float modulatedPhase = fmodf(self->phaseAccumulator +
            FLOAT_ARRAY(self->inputs.phaseOffset)[i], sig_TWOPI);

        FLOAT_ARRAY(self->outputs.main)[i] = sinf(modulatedPhase) *
            FLOAT_ARRAY(self->inputs.mul)[i] +
            FLOAT_ARRAY(self->inputs.add)[i];

        sig_dsp_Oscillator_accumulatePhase(self, i);
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

// TODO: Address duplication with other oscillator types.
void sig_dsp_LFTriangle_generate(void* signal) {
    struct sig_dsp_Oscillator* self = (struct sig_dsp_Oscillator*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        // TODO: Negative offsets will fail here.
        float modulatedPhase = fmodf(self->phaseAccumulator +
            FLOAT_ARRAY(self->inputs.phaseOffset)[i], sig_TWOPI);

        float val = -1.0f + (2.0f * (modulatedPhase * sig_RECIP_TWOPI));
        val = 2.0f * (fabsf(val) - 0.5f); // Rectify and scale/offset

        FLOAT_ARRAY(self->outputs.main)[i] = val *
            FLOAT_ARRAY(self->inputs.mul)[i] +
            FLOAT_ARRAY(self->inputs.add)[i];

        sig_dsp_Oscillator_accumulatePhase(self, i);
    }
}

void sig_dsp_LFTriangle_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self) {
    sig_dsp_Oscillator_destroy(allocator, self);
}


void sig_dsp_OnePole_init(struct sig_dsp_OnePole* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_OnePole_generate);
    self->previousSample = 0.0f;

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, coefficient, context);
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

// TODO: Unit tests
void sig_dsp_OnePole_generate(void* signal) {
    struct sig_dsp_OnePole* self = (struct sig_dsp_OnePole*) signal;

    float previousSample = self->previousSample;
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        FLOAT_ARRAY(self->outputs.main)[i] = previousSample =
            sig_filter_onepole(
                FLOAT_ARRAY(self->inputs.source)[i], previousSample,
                FLOAT_ARRAY(self->inputs.coefficient)[i]);
    }
    self->previousSample = previousSample;
}

void sig_dsp_OnePole_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_OnePole* self) {
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


void sig_dsp_ClockFreqDetector_init(struct sig_dsp_ClockFreqDetector* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_ClockFreqDetector_generate);

    struct sig_dsp_ClockFreqDetector_Parameters params = {
        .threshold = 0.1f,
        .timeoutDuration = 120.0f
    };

    self->parameters = params;
    self->previousTrigger = 0.0f;
    self->samplesSinceLastPulse = 0;
    self->clockFreq = 0.0f;
    self->pulseDurSamples = 0;

    sig_CONNECT_TO_SILENCE(self, source, context);
}

struct sig_dsp_ClockFreqDetector* sig_dsp_ClockFreqDetector_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_ClockFreqDetector* self = sig_MALLOC(allocator,
        struct sig_dsp_ClockFreqDetector);
    sig_dsp_ClockFreqDetector_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

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
    float_array_ptr source = self->inputs.source;
    float_array_ptr output = self->outputs.main;

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
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
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
