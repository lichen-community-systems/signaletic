#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <libstar.h>

/**
 * Creates a new Allocator and heap of the specified size,
 * both of which are allocated using the platform's malloc().
 *
 * @param size the size in bytes of the Allocator's heap
 * @return a pointer to the new Allocator
 */
struct star_Allocator* star_Allocator_new(size_t heapSize) {
    char* heap = (char *) malloc(heapSize);
    struct star_Allocator* allocator = (struct star_Allocator*) malloc(
        sizeof(struct star_Allocator));
    allocator->heapSize = heapSize;
    allocator->heap = (void *) heap;

    return allocator;
}

/**
 * Destroys an Allocator and its underlying heap
 * using the platform's free().
 *
 * @param allocator the Allocator instance to destroy
 */
void star_Allocator_destroy(struct star_Allocator* allocator) {
    free(allocator->heap);
    free(allocator);
}

class Starlings {
public:

    const float PI = star_PI;
    const float TWOPI = star_PI;

    // TODO: How do we expose DEFAULT_AUDIO_SETTINGS
    // here as a pointer?

    Starlings() {}

    float fminf(float a, float b) {
        return star_fminf(a, b);
    }

    float fmaxf(float a, float b) {
        return star_fmaxf(a, b);
    }
    float clamp(float value, float min, float max) {
        return star_clamp(value, min, max);
    }

    float midiToFreq(float midiNum) {
        return star_midiToFreq(midiNum);
    }

    void fillWithValue(float_array_ptr array, size_t length,
        float value) {
        return star_fillWithValue(array, length, value);
    }

    void fillWithSilence(float_array_ptr array, size_t length) {
        return star_fillWithSilence(array, length);
    }

    float interpolate_linear(float idx, float_array_ptr table,
        size_t length) {
        return star_interpolate_linear(idx, table, length);
    }

    float interpolate_cubic(float idx, float_array_ptr table,
        size_t length) {
        return star_interpolate_cubic(idx, table, length);
    }

    float filter_onepole(float current, float previous, float coeff) {
        return star_filter_onepole(current, previous, coeff);
    }

    struct star_AudioSettings* AudioSettings_new(
        struct star_Allocator* allocator) {
        return star_AudioSettings_new(allocator);
    }

    void AudioSettings_destroy(struct star_Allocator* allocator,
        struct star_AudioSettings* self) {
        return star_AudioSettings_destroy(allocator, self);
    }

    struct star_Allocator* Allocator_new(size_t heapSize) {
        return star_Allocator_new(heapSize);
    }

    void Allocator_init(struct star_Allocator* allocator) {
        return star_Allocator_init(allocator);
    }

    void* Allocator_malloc(struct star_Allocator* allocator,
        size_t size) {
        return star_Allocator_malloc(allocator, size);
    }

    void Allocator_free(struct star_Allocator* allocator, void* obj) {
        return star_Allocator_free(allocator, obj);
    }

    struct star_Buffer* Buffer_new(struct star_Allocator* allocator,
        size_t length) {
        return star_Buffer_new(allocator, length);
    }

    void Buffer_fill(struct star_Buffer* buffer, float value) {
        return star_Buffer_fill(buffer, value);
    }

    void Buffer_fillWithSilence(struct star_Buffer* buffer) {
        return star_Buffer_fillWithSilence(buffer);
    }

    void Buffer_destroy(struct star_Allocator* allocator,
        struct star_Buffer* buffer) {
        return star_Buffer_destroy(allocator, buffer);
    }


    float_array_ptr AudioBlock_new(struct star_Allocator* allocator,
        struct star_AudioSettings* audioSettings) {
        return star_AudioBlock_new(allocator, audioSettings);
    }

    float_array_ptr AudioBlock_newWithValue(float value,
        struct star_Allocator* allocator,
        struct star_AudioSettings* audioSettings) {
            return star_AudioBlock_newWithValue(value, allocator,
                audioSettings);
    }

    ~Starlings() {}
};
