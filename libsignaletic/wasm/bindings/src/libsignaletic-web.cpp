#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <libsignaletic.h>

class Signals {
public:
    void evaluateSignals(struct sig_List* signalList) {
        return sig_dsp_evaluateSignals(signalList);
    }

    struct sig_dsp_Value* Value_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Value_new(allocator, context);
    }

    void Value_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Value* self) {
        return sig_dsp_Value_destroy(allocator, self);
    }

    struct sig_dsp_BinaryOp* Add_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Add_new(allocator, context);
    }

    void Add_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Add_destroy(allocator, self);
    }

    struct sig_dsp_BinaryOp* Mul_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Mul_new(allocator, context);
    }

    void Mul_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Mul_destroy(allocator, self);
    }

    struct sig_dsp_BinaryOp* Div_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Div_new(allocator, context);
    }

    void Div_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Div_destroy(allocator, self);
    }

    struct sig_dsp_Oscillator* Sine_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Sine_new(allocator, context);
    }

    void Sine_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Oscillator* self) {
        return sig_dsp_Sine_destroy(allocator, self);
    }

    struct sig_dsp_Oscillator* LFTriangle_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_LFTriangle_new(allocator, context);
    }

    void LFTriangle_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Oscillator* self) {
        return sig_dsp_LFTriangle_destroy(allocator, self);
    }


    struct sig_dsp_ClockDetector* ClockDetector_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_ClockDetector_new(allocator, context);
    }

    void ClockDetector_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_ClockDetector* self) {
        return sig_dsp_ClockDetector_destroy(allocator, self);
    }
};

class Signaletic {
public:

    const float PI = sig_PI;
    const float TWOPI = sig_PI;

    // TODO: How do we expose DEFAULT_AUDIO_SETTINGS
    // here as a pointer?

    Signals dsp;

    Signaletic() {}

    struct sig_Status* Status_new(struct sig_Allocator* allocator) {
        struct sig_Status* self = (struct sig_Status*) allocator->impl->malloc(
            allocator, sizeof(struct sig_Status));
        sig_Status_init(self);

        return self;
    }

    void Status_init(struct sig_Status* status) {
        return sig_Status_init(status);
    }

    void Status_reset(struct sig_Status* status) {
        return sig_Status_reset(status);
    }

    void Status_reportResult(struct sig_Status* status,
        enum sig_Result result) {
        return sig_Status_reportResult(status, result);
    }

    float fminf(float a, float b) {
        return sig_fminf(a, b);
    }

    float fmaxf(float a, float b) {
        return sig_fmaxf(a, b);
    }
    float clamp(float value, float min, float max) {
        return sig_clamp(value, min, max);
    }

    float midiToFreq(float midiNum) {
        return sig_midiToFreq(midiNum);
    }

    void fillWithValue(float_array_ptr array, size_t length,
        float value) {
        return sig_fillWithValue(array, length, value);
    }

    void fillWithSilence(float_array_ptr array, size_t length) {
        return sig_fillWithSilence(array, length);
    }

    float interpolate_linear(float idx, float_array_ptr table,
        size_t length) {
        return sig_interpolate_linear(idx, table, length);
    }

    float interpolate_cubic(float idx, float_array_ptr table,
        size_t length) {
        return sig_interpolate_cubic(idx, table, length);
    }

    float filter_smooth(float current, float previous, float coeff) {
        return sig_filter_smooth(current, previous, coeff);
    }

    float waveform_sine(float phase) {
        return sig_waveform_sine(phase);
    }

    float waveform_square(float phase) {
        return sig_waveform_square(phase);
    }

    float waveform_saw(float phase) {
        return sig_waveform_saw(phase);
    }

    float waveform_reverseSaw(float phase) {
        return sig_waveform_reverseSaw(phase);
    }

    float waveform_triangle(float phase) {
        return sig_waveform_triangle(phase);
    }

    /**
     * Creates a new TLSFAllocator and heap of the specified size,
     * both of which are allocated using the platform's malloc().
     *
     * @param size the size in bytes of the Allocator's heap
     * @return a pointer to the new Allocator
     */
    struct sig_Allocator* TLSFAllocator_new(size_t size) {
        void* memory = malloc(size);
        struct sig_AllocatorHeap* heap = (struct sig_AllocatorHeap*)
            malloc(sizeof(struct sig_AllocatorHeap));
        heap->length = size;
        heap->memory = memory;

        struct sig_Allocator* allocator = (struct sig_Allocator*)
            malloc(sizeof(struct sig_Allocator));
        allocator->impl = &sig_TLSFAllocatorImpl;
        allocator->heap = heap;

        allocator->impl->init(allocator);

        return allocator;
    }

    /**
     * Destroys an TLSFAllocator and its underlying heap
     * using the platform's free().
     *
     * @param allocator the Allocator instance to destroy
     */
    void TLSFAllocator_destroy(struct sig_Allocator* allocator) {
        free(allocator->heap->memory);
        free(allocator->heap);
        // Note that we don't delete the impl because
        // we didn't create it (it's a global singleton).
        free(allocator);
    }


    struct sig_List* List_new(struct sig_Allocator* allocator,
        size_t capacity) {
        return sig_List_new(allocator, capacity);
    }

    void List_insert(struct sig_List* self, size_t index, void* item,
        struct sig_Status* status) {
        return sig_List_insert(self, index, item, status);
    }

    void List_append(struct sig_List* self, void* item,
        struct sig_Status* status) {
        return sig_List_append(self, item, status);
    }

    void* List_pop(struct sig_List* self, struct sig_Status* status) {
        return sig_List_pop(self, status);
    }

    void* List_remove(struct sig_List* self, size_t index,
        struct sig_Status* status) {
        return sig_List_remove(self, index, status);
    }

    void List_destroy(struct sig_Allocator* allocator,
        struct sig_List* self) {
        return sig_List_destroy(allocator, self);
    }


    struct sig_AudioSettings* AudioSettings_new(
        struct sig_Allocator* allocator) {
        return sig_AudioSettings_new(allocator);
    }

    void AudioSettings_destroy(struct sig_Allocator* allocator,
        struct sig_AudioSettings* self) {
        return sig_AudioSettings_destroy(allocator, self);
    }


    struct sig_SignalContext* SignalContext_new(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings) {
        return sig_SignalContext_new(allocator, audioSettings);
    }

    void SignalContext_destroy(struct sig_Allocator* allocator,
        struct sig_SignalContext* self) {
        sig_SignalContext_destroy(allocator, self);
    }


    struct sig_Buffer* Buffer_new(struct sig_Allocator* allocator,
        size_t length) {
        return sig_Buffer_new(allocator, length);
    }

    void Buffer_fill(struct sig_Buffer* buffer, float value) {
        return sig_Buffer_fillWithValue(buffer, value);
    }

    void Buffer_fillWithSilence(struct sig_Buffer* buffer) {
        return sig_Buffer_fillWithSilence(buffer);
    }

    float Buffer_read(struct sig_Buffer* buffer, float idx) {
        return sig_Buffer_read(buffer, idx);
    }

    float Buffer_readLinear(struct sig_Buffer* buffer, float idx) {
        return sig_Buffer_readLinear(buffer, idx);
    }

    float Buffer_readCubic(struct sig_Buffer* buffer, float idx) {
        return sig_Buffer_readCubic(buffer, idx);
    }

    void Buffer_destroy(struct sig_Allocator* allocator,
        struct sig_Buffer* buffer) {
        return sig_Buffer_destroy(allocator, buffer);
    }


    float_array_ptr AudioBlock_new(struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings) {
        return sig_AudioBlock_new(allocator, audioSettings);
    }

    float_array_ptr AudioBlock_newWithValue(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        float value) {
            return sig_AudioBlock_newWithValue(allocator,
                audioSettings, value);
    }

    void AudioBlock_destroy(struct sig_Allocator* allocator,
        float_array_ptr self) {
        return sig_AudioBlock_destroy(allocator, self);
    }

    ~Signaletic() {}
};
