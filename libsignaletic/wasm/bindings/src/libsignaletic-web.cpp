#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <libsignaletic.h>

class Signals {
public:
    struct sig_dsp_Value* Value_new(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings) {
        return sig_dsp_Value_new(allocator, audioSettings);
    }

    void Value_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Value* self) {
        return sig_dsp_Value_destroy(allocator, self);
    }

    // TODO: Address duplication with other Input_new functions.
    struct sig_dsp_BinaryOp_Inputs* BinaryOp_Inputs_new(
        struct sig_Allocator* allocator,
        float_array_ptr left, float_array_ptr right) {
        struct sig_dsp_BinaryOp_Inputs* inputs =
            (struct sig_dsp_BinaryOp_Inputs*)
                allocator->impl->malloc(allocator,
                    sizeof(sig_dsp_BinaryOp_Inputs));

        inputs->left = left;
        inputs->right = right;

        return inputs;
    }

    void BinaryOp_Inputs_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp_Inputs* self) {
        allocator->impl->free(allocator, self);
    }

    struct sig_dsp_BinaryOp* Add_new(struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_BinaryOp_Inputs* inputs) {
        return sig_dsp_Add_new(allocator, audioSettings,
            inputs);
    }

    void Add_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Add_destroy(allocator, self);
    }

    struct sig_dsp_BinaryOp* Mul_new(struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_BinaryOp_Inputs* inputs) {
        return sig_dsp_Mul_new(allocator, audioSettings,
            inputs);
    }

    void Mul_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Mul_destroy(allocator, self);
    }

    struct sig_dsp_BinaryOp* Div_new(struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_BinaryOp_Inputs* inputs) {
        return sig_dsp_Div_new(allocator, audioSettings,
            inputs);
    }

    void Div_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Div_destroy(allocator, self);
    }

    struct sig_dsp_Sine* Sine_new(struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_Sine_Inputs* inputs) {
        return sig_dsp_Sine_new(allocator, audioSettings,
            inputs);
    }

    void Sine_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Sine* self) {
        return sig_dsp_Sine_destroy(allocator, self);
    }

    // TODO: Should some version of this go directly into
    // libsignaletic, or is the only purpose of this function to
    // provide a means for creating sig_dsp_Sine_Input objects
    // from JavaScript?
    struct sig_dsp_Sine_Inputs* Sine_Inputs_new(
        struct sig_Allocator* allocator,
        float_array_ptr freq, float_array_ptr phaseOffset,
        float_array_ptr mul, float_array_ptr add) {
        struct sig_dsp_Sine_Inputs* inputs =
            (struct sig_dsp_Sine_Inputs*) allocator->impl->malloc(
                allocator, sizeof(sig_dsp_Sine_Inputs));

        inputs->freq = freq;
        inputs->phaseOffset = phaseOffset;
        inputs->mul = mul;
        inputs->add = add;

        return inputs;
    }

    void Sine_Inputs_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Sine_Inputs* self) {
        allocator->impl->free(allocator, self);
    }


    struct sig_dsp_ClockFreqDetector* ClockFreqDetector_new(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_ClockFreqDetector_Inputs* inputs) {
        return sig_dsp_ClockFreqDetector_new(allocator, audioSettings, inputs);
    }

    void ClockFreqDetector_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_ClockFreqDetector* self) {
        return sig_dsp_ClockFreqDetector_destroy(allocator, self);
    }

    struct sig_dsp_ClockFreqDetector_Inputs* ClockFreqDetector_Inputs_new(
        struct sig_Allocator* allocator, float_array_ptr source) {
        struct sig_dsp_ClockFreqDetector_Inputs* inputs = (struct sig_dsp_ClockFreqDetector_Inputs*) allocator->impl->malloc(
            allocator, sizeof(sig_dsp_ClockFreqDetector_Inputs));
        inputs->source = source;

        return inputs;
    }

    void ClockFreqDetector_Inputs_destroy(
        struct sig_Allocator* allocator,
        struct sig_dsp_ClockFreqDetector_Inputs* self) {
        allocator->impl->free(allocator, self);
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

    float filter_onepole(float current, float previous, float coeff) {
        return sig_filter_onepole(current, previous, coeff);
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

    struct sig_AudioSettings* AudioSettings_new(
        struct sig_Allocator* allocator) {
        return sig_AudioSettings_new(allocator);
    }

    void AudioSettings_destroy(struct sig_Allocator* allocator,
        struct sig_AudioSettings* self) {
        return sig_AudioSettings_destroy(allocator, self);
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

    ~Signaletic() {}
};
