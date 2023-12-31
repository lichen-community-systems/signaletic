/*! \file libsignaletic.h
    \brief The core Signaletic library.

    Signaletic is music signal processing library designed for use
    in embedded environments and Web Assembly.
*/

#ifndef LIBSIGNALETIC_H
#define LIBSIGNALETIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> // For size_t
#include <stdbool.h>
#include <stdint.h> // For int32_t, uint32_t

// This typedef is necessary because Emscripten's
// WebIDL binder is unable to produce viable
// (i.e. performant and bug-free) bindings for
// array pointers. As a result, for Emscripten
// we handle float arrays as void pointers instead,
// so that we can do manual deferencing on the
// JavaScript side of the universe.
// As a result, all float* variables in Signaletic
// in public interface must use this type instead.
#ifdef __EMSCRIPTEN__
    typedef void* float_array_ptr;
    #define FLOAT_ARRAY(array) ((float*)array)
#else
    typedef float* float_array_ptr;
    #define FLOAT_ARRAY(array) array
#endif

static const float sig_PI = 3.14159265358979323846f;
static const float sig_TWOPI = 6.283185307179586f;
static const float sig_RECIP_TWOPI = 0.159154943091895f;
static const float sig_LOG0_001 = -6.907755278982137f;
static const float sig_LOG2 = 0.6931471805599453;
static const float sig_FREQ_C4 = 261.6256;

enum sig_Result {
    SIG_RESULT_NONE,
    SIG_RESULT_SUCCESS,
    SIG_ERROR_INDEX_OUT_OF_BOUNDS,
    SIG_ERROR_EXCEEDS_CAPACITY
};

struct sig_Status {
    enum sig_Result result;
};

void sig_Status_init(struct sig_Status* status);

void sig_Status_reset(struct sig_Status* status);

void sig_Status_reportResult(struct sig_Status* status,
    enum sig_Result result);

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
float sig_fminf(float a, float b);

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
float sig_fmaxf(float a, float b);

/**
 * Clamps the value between the specified minimum and maximum.
 *
 * @param value the sample to clamp
 * @param min the minimum value
 * @param max the maximum value
 * @return the clamped value
 */
float sig_clamp(float value, float min, float max);

/**
 * @brief Computes the remainder of the floored division of two arguments.
 * This is useful when implementing "through zero" wrap arounds.
 * See https://en.wikipedia.org/wiki/Modulo#Variants_of_the_definition
 * for more information.
 *
 * @param num the numerator
 * @param denom the denominator
 * @return float the remainder of the floored division operation
 */
float sig_flooredfmodf(float num, float denom);

/**
 * Generates a random float between 0.0 and 1.0.
 *
 * @return a random value
 */
float sig_randf();

/**
 * @brief A fast tanh approximation
 *
 * @param x floating point value representing a hyperbolic angle
 * @return float the hyperbolic tangent of x
 */
float sig_fastTanhf(float x);

/**
 * @brief Linearly maps a value from one range to another.
 * This implementation does not clamp the output if the value
 * is outside of the specified current range.
 *
 * @param value the value to map
 * @param fromMin the minimum of the current range
 * @param fromMax the maximum of the current range
 * @param toMin the minimum of the target range
 * @param toMax the maximum of the target range
 * @return float the mapped value
 */
float sig_linearMap(float value,
    float fromMin, float fromMax, float toMin, float toMax);

/**
 * Converts a unipolar floating point sample in the range 0.0 to 1.0
 * into to an unsigned 12-bit integer in the range 0 to 4095.
 *
 * This function does not clamp the sample.
 *
 * @param sample the floating point sample to convert
 * @return the sample converted to an unsigned 12-bit integer
 */
uint16_t sig_unipolarToUint12(float sample);

/**
 * Converts a bipolar floating point sample in the range -1.0 to 1.0
 * into to an unsigned 12-bit integer in the range 0 to 4095.
 *
 * This function does not clamp the sample.
 *
 * @param sample the floating point sample to convert
 * @return the sample converted to an unsigned 12-bit integer
 */
uint16_t sig_bipolarToUint12(float sample);

/**
 * Converts a bipolar floating point sample in the range -1.0 to 1.0
 * into to an unsigned 12-bit integer in the range 4095-0.
 *
 * This function does not clamp the sample.
 *
 * @param sample the floating point sample to convert
 * @return the sample converted to an unsigned 12-bit integer
 */
uint16_t sig_bipolarToInvUint12(float sample);

/**
 * Converts MIDI note numbers into frequencies in hertz.
 * This algorithm assumes A4 = 440 Hz = MIDI note #69.
 *
 * @param midiNum the MIDI note number to convert
 * @return the frequency in Hz of the note number
 */
float sig_midiToFreq(float midiNum);

/**
 * Converts frequencies in hertz to MIDI note numbers.
 * This algorithm assumes A4 = 440 Hz = MIDI note #69.
 *
 * @param frequency the frequency in hertz to convert
 * @return the MIDI note number corresponding to the frequency
 */
float sig_freqToMidi(float frequency);

/**
 * @brief Converts a floating point value that represents pitch in
 * a linear scale such as Eurorack-style volts/octave into a frequency in Hz.
 *
 * @param value the value to convert
 * @param middleFreq the frequency at the midpoint (e.g. 261.6256 for middle C4 at 0.0f)
 * @return float the frequency in hz
 */
float sig_linearToFreq(float value, float middleFreq);

/**
 * @brief Converts a frequency in Hz in to a linear volts per octave-style
 * floating point value.
 *
 * @param value the frequency to convert
 * @param middleFreq the frequency at the midpoint (e.g. 261.6256 for middle C4 at 0.0f)
 * @return float the linear value
 */
float sig_freqToLinear(float freq, float middleFreq);

/**
 * @brief Sums all values in an array.
 *
 * @param values an array of floating point values
 * @param length the length of the array
 * @return float the sum of all values
 */
float sig_sum(float_array_ptr values, size_t length);

/**
 * @brief Returns the index of the smallest value in an array.
 *
 * @param values an array of floating point values
 * @param length the length of the warray
 * @return size_t the index to the smallest item in the array
 */
size_t sig_indexOfMin(float_array_ptr values, size_t length);

/**
 * @brief Returns the index of the largest value in an array.
 *
 * @param values an array of floating point values
 * @param length the length of the warray
 * @return size_t the index to the largest item in the array

 */
size_t sig_indexOfMax(float_array_ptr values, size_t length);

/**
 * Type definition for array fill functions.
 *
 * @param i the current index of the array
 * @param array the array being filled
 */
typedef float (*sig_array_filler)(size_t i, float_array_ptr array);

/**
 * A fill function that returns random floats.
 *
 * @param i unused
 * @param array unused
 */
float sig_randomFill(size_t i, float_array_ptr array);

/**
 * Fills an array of floats using the specified filler function.
 *
 * @param array the array to fill
 * @param length the lengthg of the array
 * @param filler a pointer to a fill function
 */
void sig_fill(float_array_ptr array, size_t length,
    sig_array_filler filler);

/**
 * Fills an array of floats with the specified value.
 *
 * @param value the value to fill the array with
 * @param array the array to fill
 * @param length the length of the array to fill
 **/
void sig_fillWithValue(float_array_ptr array, size_t length,
    float value);

/**
 * Fills an array of floats with zeroes.
 *
 * @param array the array to fill with silence
 * @param length the length of the array to fill
 **/
void sig_fillWithSilence(float_array_ptr array, size_t length);

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
float sig_interpolate_linear(float idx, float_array_ptr table,
    size_t length);

/**
 * Interpolates a value from the specified lookup table
 * using Hermite cubic interpolation. This implementation will
 * wrap around at either boundary of the table if needed.
 *
 * Based on Laurent De Soras' implementation at:
 * http://www.musicdsp.org/showArchiveComment.php?ArchiveID=93
 *
 * @param idx an index into the table
 * @param table the table from which values around idx should be drawn and interpolated
 * @param length the length of the buffer
 * @return the interpolated value
 */
float sig_interpolate_cubic(float idx, float_array_ptr table, size_t length);

/**
 * @brief Returns the mean average of all values in the specified array.
 *
 * @param values an array of floating point values to average
 * @param length the number of values in the array
 * @return float the mean of all values
 */
float sig_filter_mean(float_array_ptr values, size_t length);

/**
 * @brief Returns the mean average of values in the array, after excluding the
 * largest and smallest values.
 *
 * @param values an array of floating point values to average
 * @param length the number of values in the array
 * @return float float the mean of all values
 */
float sig_filter_meanExcludeMinMax(float_array_ptr values, size_t length);

/**
 * @brief An exponential moving average filter that implements the formula
 * y[n] = a * x[n] + (1 - a) * y[n-1]
 *
 * @param current the current sample (i.e. x[n])
 * @param previous the previous output sample (i.e. y[n-1])
 * @param a the coefficient (between 0 and 1); values closer to 0 apply more filtering)
 * @return float the filtered sample (i.e. y[n])
 */
float sig_filter_ema(float current, float previous, float a);

/**
 * @brief A one pole filter that implements the formula
 * y[n] = b0 * x[n] + a1 * y[n-1].
 *
 * @param current the current sample (i.e. x[n])
 * @param previous the previous output sample (i.e. y[n-1])
 * @param b0 the forward coefficient
 * @param a1 the feedback coefficient
 * @return float the filtered sample (i.e. y[n])
 */
float sig_filter_onepole(float current, float previous, float b0,
    float a1);

/**
 * @brief Calculates the feedback coefficient for a high pass one pole filter.
 *
 * @param frequency the cutoff frequency of the filter
 * @param sampleRate the current sample rate
 * @return float the feedback coeffient (a1)
 */
float sig_filter_onepole_HPF_calculateA1(float frequency, float sampleRate);

/**
 * @brief Calculates the forward coefficient for a high pass one pole filter.
 *
 * @param a1 the feedback coefficient
 * @return float the forward coefficient (b0)
 */
float sig_filter_onepole_HPF_calculateB0(float a1);

/**
 * @brief Calculates the feedback coefficient for a low pass one pole filter.
 *
 * @param frequency the cutoff frequency of the filter
 * @param sampleRate the current sample rate
 * @return float the feedback coeffient (a1)
 */
float sig_filter_onepole_LPF_calculateA1(float frequency, float sampleRate);

/**
 * @brief Calculates the forward coefficient for a low pass one pole filter.
 *
 * @param a1 the feedback coefficient
 * @return float the forward coefficient (b0)
 */
float sig_filter_onepole_LPF_calculateB0(float a1);

/**
 * An optimized one pole low-pass filter,
 * typically used for smoothing parameter values.
 *
 * @param current the current sample (n)
 * @param previous the previous sample (n-1)
 * @param coeff the filter coefficient (a1)
 */
float sig_filter_smooth(float current, float previous, float coeff);

/**
 * @brief Calculates the coefficient from a 60 dB convergence time for
 * a low pass one pole filter.
 *
 * @param timeSecs the time (in seconds) for the filter to converge on the value
 * @param sampleRate the current sample rate
 * @return float the coefficient (a1)
 */
float sig_filter_smooth_calculateCoefficient(float timeSecs,
    float sampleRate);

/**
 * Type definition for a waveform generator function.
 *
 * @param phase the current phase
 * @return a sample for the current phase of the waveform
 */
typedef float (*sig_waveform_generator)(float phase);

/**
 * Generates one sample of a sine wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float sig_waveform_sine(float phase);

/**
 * Generates one sample of a square wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float sig_waveform_square(float phase);

/**
 * Generates one sample of a saw wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float sig_waveform_saw(float phase);

/**
 * Generates one sample of a reverse saw wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float sig_waveform_reverseSaw(float phase);

/**
 * Generates one sample of a triangle wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float sig_waveform_triangle(float phase);


/**
 * An AudioSettings structure holds key information
 * about the current configuration of the audio system,
 * including sample rate, number of channels, and the
 * audio block size.
 */
struct sig_AudioSettings {
    /**
     * The sampling rate in samples per second.
     */
    float sampleRate;

    // TODO: Should this just be an unsigned short? (gh-24)
    /**
     * The number of audio output channels.
     */
    size_t numChannels;

    // TODO: Should this also be an unsigned short? (gh-24)
    /**
     * The number of samples in an audio block.
     */
    size_t blockSize;
};

/**
 * The default audio settings used by Signaletic
 * for 48KHz mono audio with a block size of 48.
 */
static const struct sig_AudioSettings sig_DEFAULT_AUDIOSETTINGS = {
    .sampleRate = 48000.0f,
    .numChannels = 1,
    .blockSize = 48
};


/**
 * Converts a duration from seconds to number of samples.
 *
 * @param audioSettings a pointer to the audio settings
 * @param duration the duration in seconds
 * @return the duration in number of samples
 */
size_t sig_secondsToSamples(struct sig_AudioSettings* audioSettings,
    float duration);


/**
 * A memory heap for an  allocator.
 */
struct sig_AllocatorHeap {
    size_t length;
    void* memory;
};

struct sig_Allocator;

/**
 * Type definition for an Allocator init function.
 */
typedef void (*sig_Allocator_init)(struct sig_Allocator* allocator);

/**
 * Type definition for an Allocator malloc function.
 */
typedef void* (*sig_Allocator_malloc)(struct sig_Allocator* allocator,
    size_t size);

/**
 * Type definition for an Allocator free function.
 */
typedef void (*sig_Allocator_free)(struct sig_Allocator* allocator,
    void* obj);

/**
 * A memory allocator implementation that provides
 * function pointers for essential memory operations.
 */
struct sig_AllocatorImpl {
    sig_Allocator_init init;
    sig_Allocator_malloc malloc;
    sig_Allocator_free free;
};

struct sig_Allocator {
    struct sig_AllocatorImpl* impl;
    struct sig_AllocatorHeap* heap;
};

#define sig_MALLOC(allocator, T) (T*) allocator->impl->malloc(allocator, \
    sizeof(T));

/**
 * TLSF Allocator init function.
 */
void sig_TLSFAllocator_init(struct sig_Allocator* allocator);

/**
 * TLSF Allocator malloc function.
 */
void* sig_TLSFAllocator_malloc(struct sig_Allocator* allocator,
    size_t size);

/**
 * TLSF Allocator free function.
 */
void sig_TLSFAllocator_free(struct sig_Allocator* allocator,
    void* obj);

/**
 * A realtime-capable memory allocator based on
 * Matt Conte's TLSF library.
 */
extern struct sig_AllocatorImpl sig_TLSFAllocatorImpl;

/**
 * A mutable list, which stores pointers to any type of object ("items").
 */
struct sig_List {
    void** items;
    size_t capacity;
    size_t length;
};

/**
 * @brief Initializes a new List with the specified storage capacity.
 *
 * @param allocator the allocator to use
 * @param capacity the maximum number of entries that the list can hold
 * @return struct sig_List* the new List instance
 */
struct sig_List* sig_List_new(struct sig_Allocator* allocator,
    size_t capacity);

/**
 * @brief Initializes a List.
 *
 * @param self the sig_List struct
 * @param items an array of pointers to use to store list members
 * @param capacity the maximum number of entries that the list can hold
 */
void sig_List_init(struct sig_List* self, void** items, size_t capacity);

/**
 * @brief Inserts an item into the list at the given position.
 *
 * @param self the List instance to insert into
 * @param index the position at which to insert the item
 * @param item the item to add
 * @param errorCode the
 */
void sig_List_insert(struct sig_List* self, size_t index, void* item,
    struct sig_Status* status);

/**
 * @brief Adds the item to the end of the list.
 *
 * @param self the list to append into
 * @param item the item to append
 */
void sig_List_append(struct sig_List* self, void* item,
    struct sig_Status* status);

/**
 * @brief Removes the last item from the list, and returns it.
 *
 * @param self the list from which to pop the last item
 * @return void* the item that was removed from the list
 */
void* sig_List_pop(struct sig_List* self, struct sig_Status* status);

/**
 * @brief Removes an item from the given index, and returns it.
 *
 * @param self the list from which to remove an item
 * @param index the index at which to remove the item
 * @return void* the item that was removed
 */
void* sig_List_remove(struct sig_List* self, size_t index,
    struct sig_Status* status);

/**
 * @brief Frees a List instance and its underlying item storage
 * (but not items themselves).
 *
 * @param allocator the allocator to use
 * @param self the List to destroy
 */
void sig_List_destroy(struct sig_Allocator* allocator,
    struct sig_List* self);



/**
 * Allocates a new AudioSettings instance with
 * the values from sig_DEFAULT_AUDIO_SETTINGS.
 *
 * @param allocator the memory allocator to use
 * @return the AudioSettings
 */
struct sig_AudioSettings* sig_AudioSettings_new(struct sig_Allocator* allocator);

/**
 * Destroys an AudioSettings instance.
 *
 * @param allocator the memory allocator to use
 * @param self the AudioSettings instance to destroy
 */
void sig_AudioSettings_destroy(struct sig_Allocator* allocator,
    struct sig_AudioSettings* self);



// TODO: Move to the DSP namespace.
struct sig_SignalContext {
    struct sig_AudioSettings* audioSettings;
    struct sig_Buffer* emptyBuffer;
    struct sig_DelayLine* oneSampleDelayLine;
    struct sig_dsp_ConstantValue* silence;
    struct sig_dsp_ConstantValue* unity;
};

struct sig_SignalContext* sig_SignalContext_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings);

void sig_SignalContext_destroy(
    struct sig_Allocator* allocator, struct sig_SignalContext* self);


/**
 * A Buffer holds an array of samples and its length.
 */
struct sig_Buffer {
    /**
     * The number of samples the buffer can store.
     */
    size_t length;

    /**
     * A pointer to the samples.
     */
    float_array_ptr samples;
};

/**
 * Allocates a new Buffer of the specified length.
 *
 * @param allocator the memory allocator to use
 * @param length the number of samples this Buffer should store
 */
struct sig_Buffer* sig_Buffer_new(struct sig_Allocator* allocator,
    size_t length);

/**
 * Fills a buffer using the specified array fill function.
 *
 * @param self the buffer to fill
 * @param filler a pointer to an array filler
 */
void sig_Buffer_fill(struct sig_Buffer* self, sig_array_filler filler);

/**
 * Fills a Buffer instance with a value.
 *
 * @param self the buffer instance to fill
 * @param value the value to which all samples will be set
 */
void sig_Buffer_fillWithValue(struct sig_Buffer* self, float value);

/**
 * Fills a Buffer with silence.
 *
 * @param self the buffer to fill with zeroes
 */
void sig_Buffer_fillWithSilence(struct sig_Buffer* self);

/**
 * Fills a Buffer with a waveform using the specified
 * waveform generator function.
 *
 * @param self the buffer to fill
 * @param generate a pointer to a waveform generator function
 * @param sampleRate the sample rate at which to generate the waveform
 * @param phase the initial phase of the waveform
 * @param freq the frequency of the waveform
 */
void sig_Buffer_fillWithWaveform(struct sig_Buffer* self,
    sig_waveform_generator generate, float sampleRate,
    float phase, float freq);

/**
 * Reads from the buffer, truncating the fractional portion of the index.
 *
 * @param self the buffer from which to read
 * @param idx the index (can be fractional) at which to read a sample
 * @return the linearly interpolated sample
 */
float sig_Buffer_read(struct sig_Buffer* self, float idx);

/**
 * Reads from the buffer using linear interpolation.
 *
 * @param self the buffer from which to read
 * @param idx the index (can be fractional) at which to read a sample
 * @return the linearly interpolated sample
 */
float sig_Buffer_readLinear(struct sig_Buffer* self, float idx);

/**
 * Reads from the buffer using cubic interpolation.
 *
 * @param self the buffer from which to read
 * @param idx the index (can be fractional) at which to read a sample
 * @return the cubic interpolated sample
 */
float sig_Buffer_readCubic(struct sig_Buffer* self, float idx);

/**
 * Destroys a Buffer and frees its memory.
 *
 * After calling this function, the pointer to
 * the buffer instance will no longer be usable.
 *
 * @param allocator the memory allocator to use
 * @param self the buffer instance to destroy
 */
void sig_Buffer_destroy(struct sig_Allocator* allocator, struct sig_Buffer* self);


/**
 * Creates a new Buffer that references a portion of another Buffer.
 * Be aware that destroying the parent buffer will
 * invalidate any BufferViews that have been created from it.
 *
 * @param allocator the allocator to use
 * @param buffer the parent buffer
 * @param startIdx the index in the parent buffer to start at
 * @param length the length of the subbuffer
 */
struct sig_Buffer* sig_BufferView_new(
    struct sig_Allocator* allocator, struct sig_Buffer* buffer,
    size_t startIdx, size_t length);

/**
 * Destroys a BufferView.
 * Note that this will not free the samples array,
 * since it is a shared object borrowed from another Buffer.
 *
 * @param allocator the allocator to use
 * @param self the subbuffer to destroy
 */
void sig_BufferView_destroy(struct sig_Allocator* allocator,
    struct sig_Buffer* self);


float_array_ptr sig_AudioBlock_new(struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings);
float_array_ptr sig_AudioBlock_newWithValue(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    float value);
float_array_ptr sig_AudioBlock_newSilent(struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings);
void sig_AudioBlock_destroy(struct sig_Allocator* allocator,
    float_array_ptr self);


/**
 * @brief A modulatable delay line
 * with support for comb and allpass configurations.
 *
 */
struct sig_DelayLine {
    struct sig_Buffer* buffer;
    size_t writeIdx;
};

struct sig_DelayLine* sig_DelayLine_new(struct sig_Allocator* allocator,
    size_t maxDelayLength);

struct sig_DelayLine* sig_DelayLine_newSeconds(struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings, float maxDelaySecs);

struct sig_DelayLine* sig_DelayLine_newWithTransferredBuffer(
    struct sig_Allocator* allocator, struct sig_Buffer* buffer);

void sig_DelayLine_init(struct sig_DelayLine* self);

typedef void (*sig_DelayLine_readFn)(void* signal);

float sig_DelayLine_readAt(struct sig_DelayLine* self, size_t readPos);

float sig_DelayLine_linearReadAt(struct sig_DelayLine* self, float readPos);

float sig_DelayLine_cubicReadAt(struct sig_DelayLine* self, float readPos);

float sig_DelayLine_readAtTime(struct sig_DelayLine* self, float source,
    float tapTime, float sampleRate);

float sig_DelayLine_linearReadAtTime(struct sig_DelayLine* self, float source,
    float tapTime, float sampleRate);

float sig_DelayLine_cubicReadAtTime(struct sig_DelayLine* self, float source,
    float tapTime, float sampleRate);

float sig_DelayLine_readAtTimes(struct sig_DelayLine* self, float source,
    float* tapTimes, float* tapGains, size_t numTaps,
    float sampleRate, float timeScale);

float sig_DelayLine_linearReadAtTimes(struct sig_DelayLine* self,
    float source, float* tapTimes, float* tapGains, size_t numTaps,
    float sampleRate, float timeScale);

float sig_DelayLine_cubicReadAtTimes(struct sig_DelayLine* self,
    float source, float* tapTimes, float* tapGains, size_t numTaps,
    float sampleRate, float timeScale);

void sig_DelayLine_write(struct sig_DelayLine* self, float sample);

float sig_DelayLine_calcFeedbackGain(float delayTime, float decayTime);

float sig_DelayLine_feedback(float sample, float read, float g);

float sig_DelayLine_comb(struct sig_DelayLine* self, float sample,
    size_t readPos, float g);

float sig_DelayLine_cubicComb(struct sig_DelayLine* self, float sample,
    float readPos, float g);

float sig_DelayLine_allpass(struct sig_DelayLine* self, float sample,
    size_t readPos, float g);

float sig_DelayLine_cubicAllpass(struct sig_DelayLine* self, float sample,
    float readPos, float g);

void sig_DelayLine_destroy(struct sig_Allocator* allocator,
    struct sig_DelayLine* self);

float sig_linearXFade(float left, float right, float mix);



// TODO: Should the signal argument at least be defined
// as a struct sig_dsp_Signal*, rather than void*?
// Either way, this is cast by the implementation to whatever
// concrete Signal type is appropriate.
typedef void (*sig_dsp_generateFn)(void* signal);

struct sig_dsp_Signal {
    struct sig_AudioSettings* audioSettings;
    sig_dsp_generateFn generate;
};

// TODO: Signal initializer will need to take some additional information:
//  - type information about its inputs
//  - a pointer to its input container
//  - type information about its outputs
//  - a pointer to its output container
// This will allow for the implementation of a generic connector API,
// which will connect different signals together appropriately. Maybe?
void sig_dsp_Signal_init(void* signal, struct sig_SignalContext* context,
    sig_dsp_generateFn generate);
void sig_dsp_Signal_generate(void* signal);
void sig_dsp_Signal_destroy(struct sig_Allocator* allocator,
    void* signal);

#define sig_CONNECT_TO_SILENCE(signal, inputName, context)\
    signal->inputs.inputName = context->silence->outputs.main;

#define sig_CONNECT_TO_UNITY(signal, inputName, context)\
    signal->inputs.inputName = context->unity->outputs.main;

struct sig_dsp_Signal_SingleMonoOutput {
    float_array_ptr main;
};

void sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_Signal_SingleMonoOutput* outputs);
void sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_Signal_SingleMonoOutput* outputs);


void sig_dsp_evaluateSignals(struct sig_List* signalList);

struct sig_dsp_SignalEvaluator;

typedef void (*sig_dsp_SignalEvaluator_evaluate)(
    struct sig_dsp_SignalEvaluator* self);

struct sig_dsp_SignalEvaluator {
    sig_dsp_SignalEvaluator_evaluate evaluate;
};

struct sig_dsp_SignalListEvaluator {
    sig_dsp_SignalEvaluator_evaluate evaluate;
    struct sig_List* signalList;
};

struct sig_dsp_SignalListEvaluator* sig_dsp_SignalListEvaluator_new(
    struct sig_Allocator* allocator, struct sig_List* signalList);

void sig_dsp_SignalListEvaluator_init(
    struct sig_dsp_SignalListEvaluator* self, struct sig_List* signalList);

void sig_dsp_SignalListEvaluator_evaluate(
    struct sig_dsp_SignalEvaluator* self);

void sig_dsp_SignalListEvaluator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_SignalListEvaluator* self);


struct sig_dsp_Value_Parameters {
    float value;
};

/**
 * @brief A signal that outputs a value.
 * This is intended to be used for values that may change periodically as a
 * result of user input.
 */
struct sig_dsp_Value {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Value_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    float lastSample;
};

struct sig_dsp_Value* sig_dsp_Value_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_dsp_Value_init(struct sig_dsp_Value* self,
    struct sig_SignalContext* context);
void sig_dsp_Value_generate(void* signal);
void sig_dsp_Value_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Value* self);

// TODO: Could this be replaced with a sig_dsp_Value that is
// only generated once, at instantiation time?
/**
 * @brief A signal whose value is constant throughout its lifetime.
 */
struct sig_dsp_ConstantValue {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

struct sig_dsp_ConstantValue* sig_dsp_ConstantValue_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
    float value);
void sig_dsp_ConstantValue_init(struct sig_dsp_ConstantValue* self,
    struct sig_SignalContext* context, float value);
void sig_dsp_ConstantValue_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ConstantValue* self);



struct sig_dsp_Abs_Inputs {
    float_array_ptr source;
};

struct sig_dsp_Abs {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Abs_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

struct sig_dsp_Abs* sig_dsp_Abs_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Abs_init(struct sig_dsp_Abs* self,
    struct sig_SignalContext* context);
void sig_dsp_Abs_generate(void* signal);
void sig_dsp_Abs_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Abs* self);



struct sig_dsp_ScaleOffset_Inputs {
    float_array_ptr source;
};

struct sig_dsp_ScaleOffset_Parameters {
    float scale;
    float offset;
};

struct sig_dsp_ScaleOffset {
    struct sig_dsp_Signal signal;
    struct sig_dsp_ScaleOffset_Inputs inputs;
    struct sig_dsp_ScaleOffset_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

struct sig_dsp_ScaleOffset* sig_dsp_ScaleOffset_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_ScaleOffset_init(struct sig_dsp_ScaleOffset* self,
    struct sig_SignalContext* context);
void sig_dsp_ScaleOffset_generate(void* signal);
void sig_dsp_ScaleOffset_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ScaleOffset* self);



struct sig_dsp_BinaryOp_Inputs {
    float_array_ptr left;
    float_array_ptr right;
};

struct sig_dsp_BinaryOp {
    struct sig_dsp_Signal signal;
    struct sig_dsp_BinaryOp_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

struct sig_dsp_BinaryOp* sig_dsp_BinaryOp_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context, sig_dsp_generateFn generate);
void sig_dsp_BinaryOp_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context, sig_dsp_generateFn generate);

struct sig_dsp_BinaryOp* sig_dsp_Add_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Add_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context);
void sig_dsp_Add_generate(void* signal);
void sig_dsp_Add_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self);

struct sig_dsp_BinaryOp* sig_dsp_Sub_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Sub_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context);
void sig_dsp_Sub_generate(void* signal);
void sig_dsp_Sub_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self);

struct sig_dsp_BinaryOp* sig_dsp_Mul_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Mul_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context);
void sig_dsp_Mul_generate(void* signal);
void sig_dsp_Mul_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self);

struct sig_dsp_BinaryOp* sig_dsp_Div_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Div_init(struct sig_dsp_BinaryOp* self,
    struct sig_SignalContext* context);
void sig_dsp_Div_generate(void* signal);
void sig_dsp_Div_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self);


struct sig_dsp_Invert_Inputs {
    float_array_ptr source;
};

struct sig_dsp_Invert {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Invert_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

struct sig_dsp_Invert* sig_dsp_Invert_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Invert_init(struct sig_dsp_Invert* self,
    struct sig_SignalContext* context);
void sig_dsp_Invert_generate(void* signal);
void sig_dsp_Invert_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Invert* self);


/**
 * The inputs for an Accumulator Signal.
 */
struct sig_dsp_Accumulate_Inputs {
    /**
     * The source input.
     * Only the first value of each block will be read.
     */
    float_array_ptr source;

    /**
     * When a positive trigger is received from this input,
     * the accumulator will be reset to zero.
     */
    float_array_ptr reset;
};

struct sig_dsp_Accumulate_Parameters {
    float accumulatorStart;
    float wrap;
    float maxValue;
};

/**
 * A Signal that accumulates its "source" input.
 *
 * Note: while Signaletic doesn't have a formal
 * concept of a control rate, this Signal
 * currently only reads the first sample from each block
 * of its source input.
 */
struct sig_dsp_Accumulate {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Accumulate_Inputs inputs;
    struct sig_dsp_Accumulate_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    float accumulator;
    float previousReset;
};

struct sig_dsp_Accumulate* sig_dsp_Accumulate_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Accumulate_init(struct sig_dsp_Accumulate* self,
    struct sig_SignalContext* context);
void sig_dsp_Accumulate_generate(void* signal);
void sig_dsp_Accumulate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Accumulate* self);


/**
 * Inputs for a GatedTimer.
 */
struct sig_dsp_GatedTimer_Inputs {
    /**
     * A gate signal that, when open, causes the timer to run.
     */
    float_array_ptr gate;

    /**
     * The duration, in seconds, that the timer should run for.
     */
    float_array_ptr duration;

    /**
     * A boolean signal specifying whether the time should run again
     * after it has finished. Values >0.0 denote true.
     */
    float_array_ptr loop;
};

/**
 * GatedTimer is a Signal that fires a trigger after
 * a specified duration has elapsed.
 *
 * The timer will only run while the gate is open,
 * and will reset whenever the gate closes.
 */
struct sig_dsp_GatedTimer {
    struct sig_dsp_Signal signal;
    struct sig_dsp_GatedTimer_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    unsigned long timer;
    bool hasFired;
    float prevGate;
};

struct sig_dsp_GatedTimer* sig_dsp_GatedTimer_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_GatedTimer_init(struct sig_dsp_GatedTimer* self,
    struct sig_SignalContext* context);
void sig_dsp_GatedTimer_generate(void* signal);
void sig_dsp_GatedTimer_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_GatedTimer* self);


/**
 * The inputs for a TimedTriggerCounter Signal.
 */
struct sig_dsp_TimedTriggerCounter_Inputs {
    /**
     * The source input, from which incoming triggers will be counted.
     */
    float_array_ptr source;

    /**
     * The time (in seconds) before which the triggers
     * should have fired.
     */
    float_array_ptr duration;

    /**
     * The number of triggers to count within the specified duration
     * before firing an output trigger.
     * Decimal points will be truncated and ignored.
     */
    float_array_ptr count;
};

/**
 * A signal that counts incoming triggers.
 *
 * This signal will fire trigger if the expected number
 * of triggers are received within the specified duration.
 */
struct sig_dsp_TimedTriggerCounter {
    struct sig_dsp_Signal signal;
    struct sig_dsp_TimedTriggerCounter_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    int numTriggers;
    long timer;
    bool isTimerActive;
    float previousSource;
};

struct sig_dsp_TimedTriggerCounter* sig_dsp_TimedTriggerCounter_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_TimedTriggerCounter_init(
    struct sig_dsp_TimedTriggerCounter* self,
    struct sig_SignalContext* context);
void sig_dsp_TimedTriggerCounter_generate(void* signal);
void sig_dsp_TimedTriggerCounter_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_TimedTriggerCounter* self);


/**
 * The inputs for a ToggleGate Signal.
 */
struct sig_dsp_ToggleGate_Inputs {

    /**
     * The incoming trigger signal used to
     * toggle the gate on and off.
     *
     * Any transition from a zero or negative value
     * to a positive value will be interpreted as a trigger.
     */
    float_array_ptr trigger;
};

/**
 * A ToggleGate Signal outputs a positive gate
 * when it receives a trigger to open the gate,
 * and will close it when another trigger is received.
 */
struct sig_dsp_ToggleGate {
    struct sig_dsp_Signal signal;
    struct sig_dsp_ToggleGate_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    bool isGateOpen;
    float prevTrig;
};

struct sig_dsp_ToggleGate* sig_dsp_ToggleGate_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_ToggleGate_init(struct sig_dsp_ToggleGate* self,
    struct sig_SignalContext* context);
void sig_dsp_ToggleGate_generate(void* signal);
void sig_dsp_ToggleGate_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_ToggleGate* self);

// TODO: Rename mul and add to scale and offset.
struct sig_dsp_Oscillator_Inputs {
    float_array_ptr freq;
    float_array_ptr phaseOffset;
    float_array_ptr mul;
    float_array_ptr add;
};

struct sig_dsp_Oscillator_Outputs {
    float_array_ptr main;
    float_array_ptr eoc;
};

void sig_dsp_Oscillator_Outputs_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_Oscillator_Outputs* outputs);
void sig_dsp_Oscillator_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator_Outputs* outputs);

struct sig_dsp_Oscillator {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Oscillator_Inputs inputs;
    struct sig_dsp_Oscillator_Outputs outputs;
    float phaseAccumulator;
};

struct sig_dsp_Oscillator* sig_dsp_Oscillator_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
    sig_dsp_generateFn generate);
void sig_dsp_Oscillator_init(struct sig_dsp_Oscillator* self,
    struct sig_SignalContext* context, sig_dsp_generateFn generate);
float sig_dsp_Oscillator_eoc(float phase);
float sig_dsp_Oscillator_wrapPhase(float phase);
void sig_dsp_Oscillator_accumulatePhase(struct sig_dsp_Oscillator* self,
    size_t i);
void sig_dsp_Oscillator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self);


void sig_dsp_Sine_init(struct sig_dsp_Oscillator* self,
    struct sig_SignalContext* context);
struct sig_dsp_Oscillator* sig_dsp_Sine_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_dsp_Sine_generate(void* signal);
void sig_dsp_Sine_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self);

void sig_dsp_LFTriangle_init(struct sig_dsp_Oscillator* self,
    struct sig_SignalContext* context);
struct sig_dsp_Oscillator* sig_dsp_LFTriangle_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_LFTriangle_generate(void* signal);
void sig_dsp_LFTriangle_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self);


struct sig_dsp_Smooth_Inputs {
    float_array_ptr source;
};

struct sig_dsp_Smooth_Parameters {
    float time;
};

struct sig_dsp_Smooth {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Smooth_Inputs inputs;
    struct sig_dsp_Smooth_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    float a1;
    float previousTime;
    float previousSample;
};

void sig_dsp_Smooth_init(struct sig_dsp_Smooth* self,
    struct sig_SignalContext* context);
struct sig_dsp_Smooth* sig_dsp_Smooth_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Smooth_generate(void* signal);
void sig_dsp_Smooth_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Smooth* self);



struct sig_dsp_EMA_Inputs {
    float_array_ptr source;
};

struct sig_dsp_EMA_Parameters {
    float alpha;
};

struct sig_dsp_EMA {
    struct sig_dsp_Signal signal;
    struct sig_dsp_EMA_Inputs inputs;
    struct sig_dsp_EMA_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    float previousSample;
};

void sig_dsp_EMA_init(struct sig_dsp_EMA* self,
    struct sig_SignalContext* context);
struct sig_dsp_EMA* sig_dsp_EMA_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_EMA_generate(void* signal);
void sig_dsp_EMA_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_EMA* self);



enum sig_dsp_OnePole_Mode {
    sig_dsp_OnePole_Mode_HIGH_PASS,
    sig_dsp_OnePole_Mode_LOW_PASS,
    sig_dsp_OnePole_Mode_NOT_SPECIFIED
};

struct sig_dsp_OnePole_Parameters {
    enum sig_dsp_OnePole_Mode mode;
};

struct sig_dsp_OnePole_Inputs {
    float_array_ptr source;
    float_array_ptr frequency;
};

struct sig_dsp_OnePole {
    struct sig_dsp_Signal signal;
    struct sig_dsp_OnePole_Inputs inputs;
    struct sig_dsp_OnePole_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    float b0;
    float a1;
    enum sig_dsp_OnePole_Mode previousMode;
    float previousFrequency;
    float previousSample;
};

void sig_dsp_OnePole_init(struct sig_dsp_OnePole* self,
    struct sig_SignalContext* context);
struct sig_dsp_OnePole* sig_dsp_OnePole_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_dsp_OnePole_recalculateCoefficients(struct sig_dsp_OnePole* self,
    float frequency);
void sig_dsp_OnePole_generate(void* signal);
void sig_dsp_OnePole_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_OnePole* self);


struct sig_dsp_Tanh_Inputs {
    float_array_ptr source;
};

struct sig_dsp_Tanh {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Tanh_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

void sig_dsp_Tanh_init(struct sig_dsp_Tanh* self,
    struct sig_SignalContext* context);
struct sig_dsp_Tanh* sig_dsp_Tanh_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Tanh_generate(void* signal);
void sig_dsp_Tanh_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Tanh* self);


struct sig_dsp_Looper_Inputs {
    float_array_ptr source;
    float_array_ptr start;
    float_array_ptr end;
    float_array_ptr speed;
    float_array_ptr record;
    float_array_ptr clear;
};

struct sig_dsp_Looper_Loop {
    struct sig_Buffer* buffer;
    size_t startIdx;
    size_t length;
    bool isEmpty;
};

struct sig_dsp_Looper {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Looper_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_dsp_Looper_Loop loop;
    size_t loopLastIdx;
    float playbackPos;
    float previousRecord;
    float previousClear;
};

void sig_dsp_Looper_init(struct sig_dsp_Looper* self,
    struct sig_SignalContext* context);
struct sig_dsp_Looper* sig_dsp_Looper_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Looper_setBuffer(struct sig_dsp_Looper* self,
    struct sig_Buffer* buffer);
void sig_dsp_Looper_generate(void* signal);
void sig_dsp_Looper_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Looper* self);

struct sig_dsp_Dust_Parameters {
    float bipolar;
};

struct sig_dsp_Dust_Inputs {
    float_array_ptr density;
};

/**
 * Generates random impulses.
 *
 * If the bipolar parameter is set to 0 (the default),
 * the output will be in the range of 0 to 1.
 * If bipolar >0, impulses between -1 and 1 will be generated.
 *
 * Inputs:
 * - density: the number of impulses per second
 *
 * Parameters:
 * - bipolar: the output will be unipolar if <=0, bipolar if >0
 */
struct sig_dsp_Dust {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Dust_Inputs inputs;
    struct sig_dsp_Dust_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;

    // TODO: Should this be part of AudioSettings?
    float sampleDuration;

    float previousDensity;
    float threshold;
    float scale;
};

void sig_dsp_Dust_init(struct sig_dsp_Dust* self,
    struct sig_SignalContext* context);
struct sig_dsp_Dust* sig_dsp_Dust_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Dust_generate(void* signal);
void sig_dsp_Dust_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Dust* self);



struct sig_dsp_TimedGate_Parameters {
    float resetOnTrigger;
    float bipolar;
};

struct sig_dsp_TimedGate_Inputs {
    float_array_ptr trigger;
    float_array_ptr duration;
};

/**
 * A triggerable timed gate.
 *
 * When triggered, this signal will output a gate that matches the
 * amplitude and polarity of the trigger, for the specified duration.
 *
 * Inputs:
 *  - duration: the duration (in seconds) to remain open
 *  - trigger: a rising edge that will cause the gate to open
 *
 * Parameters:
 * - resetOnTrigger: when >0, the gate will close and then reopen
 *                   if a trigger is received while the gate is open
 * - bipolar: when >0, the gate will open when the trigger is
 *            either positive or negative. Otherwise only positive
 *            zero-crossings will cause the gate to open.
 *
 * Output: the gate's amplitude will reflect the value and
 *         polarity of the trigger
 */
struct sig_dsp_TimedGate {
    struct sig_dsp_Signal signal;
    struct sig_dsp_TimedGate_Inputs inputs;
    struct sig_dsp_TimedGate_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;

    float previousTrigger;
    float gateValue;
    float previousDuration;
    long durationSamps;
    long samplesRemaining;
};

void sig_dsp_TimedGate_init(struct sig_dsp_TimedGate* self,
    struct sig_SignalContext* context);
struct sig_dsp_TimedGate* sig_dsp_TimedGate_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_TimedGate_generate(void* signal);
void sig_dsp_TimedGate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_TimedGate* self);


struct sig_dsp_DustGate_Inputs {
    float_array_ptr density;
    float_array_ptr durationPercentage;
};

struct sig_dsp_DustGate_Parameters {
    float bipolar;
};

struct sig_dsp_DustGate {
    struct sig_dsp_Signal signal;
    struct sig_dsp_DustGate_Inputs inputs;
    struct sig_dsp_DustGate_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_dsp_Dust* dust;
    struct sig_dsp_BinaryOp* reciprocalDensity;
    struct sig_dsp_BinaryOp* densityDurationMultiplier;
    struct sig_dsp_TimedGate* gate;
};

struct sig_dsp_DustGate* sig_dsp_DustGate_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context);

void sig_dsp_DustGate_init(struct sig_dsp_DustGate* self,
    struct sig_SignalContext* context);

void sig_dsp_DustGate_generate(void* signal);

void sig_dsp_DustGate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_DustGate* self);


/**
 * @brief Inputs for a ClockDetector.
 */
struct sig_dsp_ClockDetector_Inputs {
    /**
     * @brief The incoming clock signal.
     */
    float_array_ptr source;
};

struct sig_dsp_ClockDetector_Outputs {
    /**
     * @brief The detected clock frequency in pulses per second (i.e. Hz).
     */
    float_array_ptr main;

    /**
     * @brief The detected clock frequency in beats per minute.
     */
    float_array_ptr bpm;
};

void sig_dsp_ClockDetector_Outputs_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_ClockDetector_Outputs* outputs);

void sig_dsp_ClockDetector_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_ClockDetector_Outputs* outputs);

struct sig_dsp_ClockDetector_Parameters {
    /**
     * @brief The minimum value that a trigger must reach to be detected
     * as a clock pulse.
     *
     */
    float threshold;
};

/**
 * Detects the number of pulses per second of an incoming clock signal
 * (i.e. any signal that outputs a rising edge to denote clock pulses)
 * and outputs the clock's tempo in pulses per second.
 *
 * Inputs:
 *  - source the incoming clock signal
 */
struct sig_dsp_ClockDetector {
    struct sig_dsp_Signal signal;
    struct sig_dsp_ClockDetector_Inputs inputs;
    struct sig_dsp_ClockDetector_Parameters parameters;
    struct sig_dsp_ClockDetector_Outputs outputs;

    float previousTrigger;
    bool isRisingEdge;
    uint8_t numPulsesDetected;
    uint32_t samplesSinceLastPulse;
    float clockFreq;
    uint32_t pulseDurSamples;
};

void sig_dsp_ClockDetector_init(
    struct sig_dsp_ClockDetector* self,
    struct sig_SignalContext* context);
struct sig_dsp_ClockDetector* sig_dsp_ClockDetector_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_ClockDetector_generate(void* signal);
void sig_dsp_ClockDetector_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ClockDetector* self);



struct sig_dsp_LinearToFreq_Inputs {
    float_array_ptr source;
};

struct sig_dsp_LinearToFreq_Parameters {
    float middleFreq;
};

struct sig_dsp_LinearToFreq {
    struct sig_dsp_Signal signal;
    struct sig_dsp_LinearToFreq_Inputs inputs;
    struct sig_dsp_LinearToFreq_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

struct sig_dsp_LinearToFreq* sig_dsp_LinearToFreq_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_LinearToFreq_init(struct sig_dsp_LinearToFreq* self,
    struct sig_SignalContext* context);
void sig_dsp_LinearToFreq_generate(void* signal);
void sig_dsp_LinearToFreq_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_LinearToFreq* self);


struct sig_dsp_Branch_Inputs {
    float_array_ptr off;
    float_array_ptr on;
    float_array_ptr condition;
};

struct sig_dsp_Branch {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Branch_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

struct sig_dsp_Branch* sig_dsp_Branch_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Branch_init(struct sig_dsp_Branch* self,
    struct sig_SignalContext* context);
void sig_dsp_Branch_generate(void* signal);
void sig_dsp_Branch_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Branch* self);


struct sig_dsp_List_Inputs {
    float_array_ptr index;
};

struct sig_dsp_List_Outputs {
    float_array_ptr main;
    float_array_ptr index;
    float_array_ptr length;
};

void sig_dsp_List_Outputs_newAudioBlocks(struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_List_Outputs* outputs);

void sig_dsp_List_Outputs_destroyAudioBlocks(struct sig_Allocator* allocator,
    struct sig_dsp_List_Outputs* outputs);


struct sig_dsp_List_Parameters {
    float wrap;
    float normalizeIndex;
};

struct sig_dsp_List {
    struct sig_dsp_Signal signal;
    struct sig_dsp_List_Inputs inputs;
    struct sig_dsp_List_Parameters parameters;
    struct sig_dsp_List_Outputs outputs;
    struct sig_Buffer* list;
};

struct sig_dsp_List* sig_dsp_List_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_List_init(struct sig_dsp_List* self,
    struct sig_SignalContext* context);
void sig_dsp_List_generate(void* signal);
void sig_dsp_List_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_List* self);


struct sig_dsp_LinearMap_Inputs {
    float_array_ptr source;
};

struct sig_dsp_LinearMap_Parameters {
    float fromMin;
    float fromMax;
    float toMin;
    float toMax;
};

struct sig_dsp_LinearMap {
    struct sig_dsp_Signal signal;
    struct sig_dsp_LinearMap_Inputs inputs;
    struct sig_dsp_LinearMap_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

struct sig_dsp_LinearMap* sig_dsp_LinearMap_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_LinearMap_init(struct sig_dsp_LinearMap* self,
    struct sig_SignalContext* context);
void sig_dsp_LinearMap_generate(void* signal);
void sig_dsp_LinearMap_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_LinearMap* self);


struct sig_dsp_TwoOpFM_Inputs {
    float_array_ptr frequency;
    float_array_ptr index;
    float_array_ptr ratio;
    float_array_ptr phaseOffset;
};

struct sig_dsp_TwoOpFM_Outputs {
    float_array_ptr main;
    float_array_ptr modulator;
};

struct sig_dsp_TwoOpFM {
    struct sig_dsp_Signal signal;
    struct sig_dsp_TwoOpFM_Inputs inputs;
    struct sig_dsp_TwoOpFM_Outputs outputs;
    struct sig_dsp_BinaryOp* modulatorFrequency;
    struct sig_dsp_BinaryOp* carrierPhaseOffset;
    struct sig_dsp_Oscillator* modulator;
    struct sig_dsp_Oscillator* carrier;
};

struct sig_dsp_TwoOpFM* sig_dsp_TwoOpFM_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_dsp_TwoOpFM_init(struct sig_dsp_TwoOpFM* self,
    struct sig_SignalContext* context);
void sig_dsp_TwoOpFM_generate(void* signal);
void sig_dsp_TwoOpFM_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_TwoOpFM* self);


struct sig_dsp_FourPoleFilter_Inputs {
    float_array_ptr source;
    float_array_ptr frequency;
    float_array_ptr resonance;
    float_array_ptr inputGain;
    float_array_ptr pole1Gain;
    float_array_ptr pole2Gain;
    float_array_ptr pole3Gain;
    float_array_ptr pole4Gain;
};


struct sig_dsp_FourPoleFilter_Outputs {
    /**
     * @brief A mix of all filter stages based on the stage mix inputs.
     */
    float_array_ptr main;

    /**
     * @brief The full two pole 12 dB output of the filter.
     */
    float_array_ptr twoPole;

    /**
     * @brief The full four pole 24 dB output from the filter.
     */
    float_array_ptr fourPole;
};

void sig_dsp_FourPoleFilter_Outputs_newAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_dsp_FourPoleFilter_Outputs* outputs);

void sig_dsp_FourPoleFilter_Outputs_destroyAudioBlocks(
    struct sig_Allocator* allocator,
    struct sig_dsp_FourPoleFilter_Outputs* outputs);


/**
 * @brief Miller Pucket's 24dB Moog-style ladder low pass filter.
 * Imitates a Moog resonant filter by Runge-Kutte numerical integration
 * of a differential equation approximately describing the dynamics of
 * the circuit.
 *
 * The differential equations are:
 *	y1' = k * (S(x - r * y4) - S(y1))
 *	y2' = k * (S(y1) - S(y2))
 *	y3' = k * (S(y2) - S(y3))
 *	y4' = k * (S(y3) - S(y4))
 * where k controls the cutoff frequency,
 * r is feedback (<= 4 for stability),
 * and S(x) is a saturation function.
 *
 * Adapted from Dimitri Diakopoulos' version of
 * Pure Data's BSD-licensed ~bob implementation.
 *
 * https://github.com/ddiakopoulos/MoogLadders/blob/9cef8fac86a35c40f4a99bcc9b52b8874ee5ff2b/src/RKSimulationModel.h
 * https://github.com/pure-data/pure-data/blob/3b4be8c6e228397b27615b279451578e71cabfcf/extra/bob~/bob~.c
 *
 * Inputs:
 *  - source: the input to filter
 *  - frequency: the cutoff frequency in Hz
 *  - resonance: the resonance; values > 4 lead to instability
 */
struct sig_dsp_Bob {
    struct sig_dsp_Signal signal;
    struct sig_dsp_FourPoleFilter_Inputs inputs;
    // TODO: Add outputs for the other filter poles.
    struct sig_dsp_FourPoleFilter_Outputs outputs;

    float state[4];
    float deriv1[4];
    float deriv2[4];
    float deriv3[4];
    float deriv4[4];
    float tempState[4];

    float saturation;
    float saturationInv;
    uint8_t oversample;
    float stepSize;
};

struct sig_dsp_Bob* sig_dsp_Bob_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Bob_init(struct sig_dsp_Bob* self,
    struct sig_SignalContext* context);
float sig_dsp_Bob_clip(float value, float saturation,
    float saturationInv);
void sig_dsp_Bob_generate(void* signal);
void sig_dsp_Bob_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Bob* self);



struct sig_dsp_Ladder_Parameters {
    /**
     * @brief The passband gain of the filter (best between 0.0-0.5),
     * higher values will amplify the lower frequencies when resonance is high.
     */
    float passbandGain;

    /**
     * @brief Input gain applied prior to the filter,
     * allowing for greater saturation levels (best between 0.0-4.0)
     */
    float overdrive;
};

/**
 * @brief Valimaki and Huovilainen's Moog-style Ladder Filter
 * Ported from Infrasonic Audio LLC's adaptation of
 * Richard Van Hoesel's implementation from the Teensy Audio Library.
 * https://gist.github.com/ndonald2/534831b639b8c78d40279b5007e06e5b
 *
 * Inputs:
 *  - source: the input to filter
 *  - frequency: the cutoff frequency in Hz
 *  - resonance: the resonance (stable values in the range 0 - 1.8)
 */
struct sig_dsp_Ladder {
    struct sig_dsp_Signal signal;
    struct sig_dsp_FourPoleFilter_Inputs inputs;
    struct sig_dsp_Ladder_Parameters parameters;
    struct sig_dsp_FourPoleFilter_Outputs outputs;

    uint8_t interpolation;
    float interpolationRecip;
    float alpha;
    float beta[4];
    float z0[4];
    float z1[4];
    float k;
    float fBase;
    float qAdjust;
    float prevFrequency;
    float prevInput;
};

struct sig_dsp_Ladder* sig_dsp_Ladder_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Ladder_init(
    struct sig_dsp_Ladder* self,
    struct sig_SignalContext* context);
void sig_dsp_Ladder_calcCoefficients(
    struct sig_dsp_Ladder* self, float freq);
float sig_dsp_Ladder_calcStage(
    struct sig_dsp_Ladder* self, float s, uint8_t i);
void sig_dsp_Ladder_generate(void* signal);
void sig_dsp_Ladder_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Ladder* self);



struct sig_dsp_TiltEQ_Inputs {
    float_array_ptr source;
    float_array_ptr frequency; // 20-20000 Hz
    float_array_ptr gain; // -6 to +6 dB, normalized to -1.0 to 1.0
};

/**
 * @brief A Tilt Equalizer.
 * Boosts a range of the spectrum above or below the centre frequency,
 * while attunuating the opposite range. More extreme settings will
 * turn the filter into first order low-pass or high-pass.
 * This is achieved with greater gain factors
 * (ex: +6db for high pass, or -6db for low pass)
 *
 * This implementation is written by moc.liamg@321tiloen
 * from the musicdsp.org archive. It is an emulation
 * of the "Niveau" filter from the "Elysia mPressor" compressor plugin.
 *
 * https://www.musicdsp.org/en/latest/Filters/267-simple-tilt-equalizer.html
 *
 *  Inputs:
 *  - source: the input to filter
 *  - frequency: the centre frequency in Hz (20-20000 Hz)
 *  - gain: the EQ gain, in dB (-6.0 to +6.0 dB)
 */
struct sig_dsp_TiltEQ {
    struct sig_dsp_Signal signal;
    struct sig_dsp_TiltEQ_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;

    float sr3;
    float lpOut;
};

struct sig_dsp_TiltEQ* sig_dsp_TiltEQ_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_TiltEQ_init(struct sig_dsp_TiltEQ* self,
    struct sig_SignalContext* context);
void sig_dsp_TiltEQ_generate(void* signal);
void sig_dsp_TiltEQ_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_TiltEQ* self);



struct sig_dsp_Delay_Inputs {
    float_array_ptr source;
    float_array_ptr delayTime;
};

struct sig_dsp_Delay {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Delay_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;

    struct sig_DelayLine* delayLine;
};

struct sig_dsp_Delay* sig_dsp_Delay_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Delay_init(struct sig_dsp_Delay* self,
    struct sig_SignalContext* context);
void sig_dsp_Delay_read(struct sig_dsp_Delay* self, float source,
    size_t i);
void sig_dsp_Delay_generate(void* signal);
void sig_dsp_Delay_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Delay* self);

struct sig_dsp_Delay* sig_dsp_DelayTap_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_DelayTap_init(struct sig_dsp_Delay* self,
    struct sig_SignalContext* context);
void sig_dsp_DelayTap_generate(void* signal);
void sig_dsp_DelayTap_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Delay* self);

struct sig_dsp_DelayWrite_Inputs {
    float_array_ptr source;
};

struct sig_dsp_DelayWrite {
    struct sig_dsp_Signal signal;
    struct sig_dsp_DelayWrite_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;

    struct sig_DelayLine* delayLine;
};

struct sig_dsp_DelayWrite* sig_dsp_DelayWrite_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_DelayWrite_init(struct sig_dsp_DelayWrite* self,
    struct sig_SignalContext* context);
void sig_dsp_DelayWrite_generate(void* signal);
void sig_dsp_DelayWrite_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_DelayWrite* self);


struct sig_dsp_Comb_Inputs {
    float_array_ptr source;
    float_array_ptr delayTime;
    float_array_ptr feedbackGain;
    float_array_ptr lpfCoefficient;
};

struct sig_dsp_Comb {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Comb_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;

    struct sig_DelayLine* delayLine;
    float previousSample;
};

struct sig_dsp_Comb* sig_dsp_Comb_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Comb_init(struct sig_dsp_Comb* self,
    struct sig_SignalContext* context);
void sig_dsp_Comb_generate(void* signal);
void sig_dsp_Comb_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Comb* self);



struct sig_dsp_Allpass_Inputs {
    float_array_ptr source;
    float_array_ptr delayTime;
    float_array_ptr g;
};

struct sig_dsp_Allpass {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Allpass_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;

    struct sig_DelayLine* delayLine;
};

struct sig_dsp_Allpass* sig_dsp_Allpass_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Allpass_init(struct sig_dsp_Allpass* self,
    struct sig_SignalContext* context);
void sig_dsp_Allpass_generate(void* signal);
void sig_dsp_Allpass_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Allpass* self);



struct sig_dsp_LinearXFade_Inputs {
    float_array_ptr left;
    float_array_ptr right;
    float_array_ptr mix;
};


struct sig_dsp_LinearXFade {
    struct sig_dsp_Signal signal;
    struct sig_dsp_LinearXFade_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

struct sig_dsp_LinearXFade* sig_dsp_LinearXFade_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_LinearXFade_init(struct sig_dsp_LinearXFade* self,
    struct sig_SignalContext* context);
void sig_dsp_LinearXFade_generate(void* signal);
void sig_dsp_LinearXFade_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_LinearXFade* self);



// TODO: Don't hardcode these.
#define sig_dsp_Calibrator_NUM_STAGES 6
#define sig_dsp_Calibrator_TARGET_VALUES {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 4.75f}

struct sig_dsp_Calibrator_Inputs {
    float_array_ptr source;
    float_array_ptr gate;
};

struct sig_dsp_Calibrator_Node {
    float target;
    size_t numSamplesRecorded;
    float min;
    float max;
    float sum;
    float avg;
    float diff;
};

void sig_dsp_Calibrator_Node_init(struct sig_dsp_Calibrator_Node* nodes,
    float* targetValues, size_t numNodes);

size_t sig_dsp_Calibrator_locateIntervalForValue(float x,
    struct sig_dsp_Calibrator_Node* nodes, size_t numNodes);

float sig_dsp_Calibrator_fitValueToCalibrationData(float x,
    struct sig_dsp_Calibrator_Node* nodes, size_t numNodes);

struct sig_dsp_Calibrator {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Calibrator_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;

    struct sig_dsp_Calibrator_Node nodes[sig_dsp_Calibrator_NUM_STAGES];
    float previousGate;
    size_t stage;
};

struct sig_dsp_Calibrator* sig_dsp_Calibrator_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Calibrator_init(struct sig_dsp_Calibrator* self,
    struct sig_SignalContext* context);
void sig_dsp_Calibrator_generate(void* signal);
void sig_dsp_Calibrator_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Calibrator* self);

#ifdef __cplusplus
}
#endif

#endif /* LIBSIGNALETIC_H */
