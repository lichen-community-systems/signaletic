#ifndef LIBSTAR_H
#define LIBSTAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

static const float star_PI = 3.14159265358979323846f;
static const float star_TWOPI = 6.28318530717958647693f;

/**
 * Returns the smaller of two floating point arguments.
 *
 * ARM-optimized 32-bit floating point implementation
 * from DaisySP (Copyright 2020 Electrosmith, Corp),
 * written by Stephen McCaul.
 * https://github.com/electro-smith/DaisySP/blob/master/Source/Utility/dsp.h
 *
 * @param a the first value to compare
 * @param b the second value to compare
 * @return the samller of the two arguments
*/
float star_fminf(float a, float b);

/**
 * Returns the larger of two floating point arguments.
 *
 * ARM-optimized 32-bit floating point implementation
 * from DaisySP (Copyright 2020 Electrosmith, Corp),
 * written by Stephen McCaul.
 * https://github.com/electro-smith/DaisySP/blob/master/Source/Utility/dsp.h
 *
 * @param a the first value to compare
 * @param b the second value to compare
 * @return the larger of the two arguments
*/
float star_fmaxf(float a, float b);

/**
 * Clamps the value between the specified minimum and maximum.
 *
 * @param value the sample to clamp
 * @param min the minimum value
 * @param max the maximum value
 * @return the clamped value
 */
float star_clamp(float value, float min, float max);

/**
 * Converts MIDI note numbers into frequencies in hertz.
 * This algorithm assumes A4 = 440 Hz = MIDI note #69.
 *
 * @param midiNum the MIDI note number to convert
 * @return the frequency in Hz of the note number
 */
float star_midiToFreq(float midiNum);

/**
 * Fills an array of floats with the specified value.
 *
 * @param value the value to fill the array with
 * @param array the array to fill
 * @param length the length of the array to fill
 **/
void star_fillWithValue(float value, float* array, size_t length);

/**
 * Fills an array of floats with zeroes.
 *
 * @param array the array to fill with silence
 * @param length the length of the array to fill
 **/
void star_fillWithSilence(float* array, size_t length);

/**
 * Interpolates a value from the specified lookup table
 * using linear interpolation. This implementation will
 * wrap to the beginning of the table if needed.
 *
 * @param idx an index into the table
 * @param table the table from which values around idx should be drawn and interpolated
 * @param length the length of the buffer
 * @return the interpolated value
 */
float star_interpolate_linear(float idx, float* table,
    size_t length);

/**
 * Interpolates a value from the specified lookup table
 * using Hermite cubic interpolation. This implementation will
 * wrap around at either boundary of the table if needed.
 *
 * Based on Laurent De Soras' implementation at:
 * http://www.musicdsp.org/showArchiveComment.php?ArchiveID=93
 *
 * @param idx {float} an index into the table
 * @param table {float*} the table from which values around idx should be drawn and interpolated
 * @param length {size_t} the length of the buffer
 * @return {float} the interpolated value
 */
float star_interpolate_cubic(float idx, float* table, size_t length);

float star_filter_onepole(float current, float previous, float coeff);

struct star_AudioSettings {
    float sampleRate;
    size_t numChannels;
    size_t blockSize;
};

static const struct star_AudioSettings star_DEFAULT_AUDIOSETTINGS = {
    .sampleRate = 48000.0f,
    .numChannels = 1,
    .blockSize = 48
};

struct star_Allocator {
    size_t heapSize;
    void* heap;
};
void star_Allocator_init(struct star_Allocator* self);
void star_Allocator_destroy(struct star_Allocator* self);
void* star_Allocator_malloc(struct star_Allocator* self, size_t size);
void star_Allocator_free(struct star_Allocator* self, void* obj);

struct star_Buffer {
    size_t length;
    float* samples;
};
struct star_Buffer* star_Buffer_new(struct star_Allocator* allocator,
    size_t length);
void star_Buffer_fill(struct star_Buffer* self, float value);
void star_Buffer_fillWithSilence(struct star_Buffer* self);
void star_Buffer_destroy(struct star_Allocator* allocator, struct star_Buffer* buffer);


float* star_AudioBlock_new(struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings);
float* star_AudioBlock_newWithValue(float value,
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings);

// TODO: Should the signal argument at least be defined
// as a struct star_sig_Signal*, rather than void*?
// Either way, this is cast by the implementation to whatever
// concrete Signal type is appropriate.
typedef void (*star_sig_generateFn)(void* signal);

void star_sig_generateSilence(void* signal);

struct star_sig_Signal {
    struct star_AudioSettings* audioSettings;
    float* output;
    star_sig_generateFn generate;
};

struct star_sig_Value_Parameters {
    float value;
};

struct star_sig_Value {
    struct star_sig_Signal signal;
    struct star_sig_Value_Parameters parameters;
    float lastSample;
};

void star_sig_Value_init(struct star_sig_Value* self,
    struct star_AudioSettings* settings, float* output);
struct star_sig_Value* star_sig_Value_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings);
void star_sig_Value_destroy(struct star_Allocator* allocator, struct star_sig_Value* self);

void star_sig_Value_generate(void* signal);

struct star_sig_Sine_Inputs {
    float* freq;
    float* phaseOffset;
    float* mul;
    float* add;
};

struct star_sig_Sine {
    struct star_sig_Signal signal;
    struct star_sig_Sine_Inputs* inputs;
    float phaseAccumulator;
};

void star_sig_Sine_init(struct star_sig_Sine* self,
    struct star_AudioSettings* settings, struct star_sig_Sine_Inputs* inputs, float* output);
struct star_sig_Sine* star_sig_Sine_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Sine_Inputs* inputs);
void star_sig_Sine_generate(void* signal);
void star_sig_Sine_destroy(struct star_Allocator* allocator, struct star_sig_Sine* self);

struct star_sig_Gain_Inputs {
    float* gain;
    float* source;
};

struct star_sig_Gain {
    struct star_sig_Signal signal;
    struct star_sig_Gain_Inputs* inputs;
};

void star_sig_Gain_init(struct star_sig_Gain* self,
    struct star_AudioSettings* settings, struct star_sig_Gain_Inputs* inputs, float* output);
struct star_sig_Gain* star_sig_Gain_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Gain_Inputs* inputs);
void star_sig_Gain_generate(void* signal);
void star_sig_Gain_destroy(struct star_Allocator* allocator, struct star_sig_Gain* self);


struct star_sig_OnePole_Inputs {
    float* source;
    float* coefficient;
};

struct star_sig_OnePole {
    struct star_sig_Signal signal;
    struct star_sig_OnePole_Inputs* inputs;
    float previousSample;
};

void star_sig_OnePole_init(struct star_sig_OnePole* self,
    struct star_AudioSettings* settings,
    struct star_sig_OnePole_Inputs* inputs,
    float* output);
struct star_sig_OnePole* star_sig_OnePole_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_OnePole_Inputs* inputs);
void star_sig_OnePole_generate(void* signal);
void star_sig_OnePole_destroy(struct star_Allocator* allocator,
    struct star_sig_OnePole* self);


struct star_sig_Looper_Inputs {
    float* source;
    float* start;
    float* length;
    float* speed;
    float* record;
    float* clear;
};

struct star_sig_Looper {
    struct star_sig_Signal signal;
    struct star_sig_Looper_Inputs* inputs;
    struct star_Buffer* buffer;
    float playbackPos;
    bool isBufferEmpty;
    float previousRecord;
    float previousClear;
};

void star_sig_Looper_init(struct star_sig_Looper* self,
    struct star_AudioSettings* settings,
    struct star_sig_Looper_Inputs* inputs,
    float* output);
struct star_sig_Looper* star_sig_Looper_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Looper_Inputs* inputs);
void star_sig_Looper_generate(void* signal);
void star_sig_Looper_destroy(struct star_Allocator* allocator,
    struct star_sig_Looper* self);

#ifdef __cplusplus
}
#endif

#endif
