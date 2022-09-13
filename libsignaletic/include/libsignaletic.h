/*! \file libsignaletic.h
    \brief The core Signaletic library.

    Signaletic is music signal processing library designed for use
    in embedded environments and Web Assembly.
*/

#ifndef LIBSIG_H
#define LIBSIG_H

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
static const float sig_TWOPI = 2.0f * sig_PI;
static const float sig_RECIP_TWOPI = 1.0f / sig_TWOPI;

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
 * Generates a random float between 0.0 and 1.0.
 *
 * @return a random value
 */
float sig_randf();

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
 * A one pole filter.
 *
 * @param current the current sample (n)
 * @param previous the previous sample (n-1)
 * @param coeff the filter coefficient
 */
float sig_filter_onepole(float current, float previous, float coeff);

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



// TODO: Should the signal argument at least be defined
// as a struct sig_dsp_Signal*, rather than void*?
// Either way, this is cast by the implementation to whatever
// concrete Signal type is appropriate.
typedef void (*sig_dsp_generateFn)(void* signal);

void sig_dsp_generateSilence(void* signal);

// TODO: Should the base Signal define a (void* ?) pointer
// to inputs, and provide an empty sig_dsp_Signal_Inputs struct
// as the default implementation?
struct sig_dsp_Signal {
    struct sig_AudioSettings* audioSettings;
    float_array_ptr output;
    sig_dsp_generateFn generate;
};

void sig_dsp_Signal_init(void* signal,
    struct sig_AudioSettings* settings,
    float_array_ptr output,
    sig_dsp_generateFn generate);
void sig_dsp_Signal_generate(void* signal);
void sig_dsp_Signal_destroy(struct sig_Allocator* allocator,
    void* signal);


struct sig_dsp_Value_Parameters {
    float value;
};

struct sig_dsp_Value {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Value_Parameters parameters;
    float lastSample;
};

struct sig_dsp_Value* sig_dsp_Value_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings);
void sig_dsp_Value_init(struct sig_dsp_Value* self,
    struct sig_AudioSettings* settings, float_array_ptr output);
void sig_dsp_Value_generate(void* signal);
void sig_dsp_Value_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Value* self);


struct sig_dsp_BinaryOp_Inputs {
    float_array_ptr left;
    float_array_ptr right;
};

struct sig_dsp_BinaryOp {
    struct sig_dsp_Signal signal;
    struct sig_dsp_BinaryOp_Inputs* inputs;
};


struct sig_dsp_BinaryOp* sig_dsp_Add_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs);
void sig_dsp_Add_init(struct sig_dsp_BinaryOp* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs,
    float_array_ptr output);
void sig_dsp_Add_generate(void* signal);
void sig_dsp_Add_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self);


void sig_dsp_Mul_init(struct sig_dsp_BinaryOp* self,
    struct sig_AudioSettings* settings, struct sig_dsp_BinaryOp_Inputs* inputs, float_array_ptr output);
struct sig_dsp_BinaryOp* sig_dsp_Mul_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs);
void sig_dsp_Mul_generate(void* signal);
void sig_dsp_Mul_destroy(struct sig_Allocator* allocator, struct sig_dsp_BinaryOp* self);


struct sig_dsp_BinaryOp* sig_dsp_Div_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs);
void sig_dsp_Div_init(struct sig_dsp_BinaryOp* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_BinaryOp_Inputs* inputs,
    float_array_ptr output);
void sig_dsp_Div_generate(void* signal);
void sig_dsp_Div_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_BinaryOp* self);


struct sig_dsp_Invert_Inputs {
    float_array_ptr source;
};

struct sig_dsp_Invert {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Invert_Inputs* inputs;
};

struct sig_dsp_Invert* sig_dsp_Invert_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Invert_Inputs* inputs);
void sig_dsp_Invert_init(struct sig_dsp_Invert* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Invert_Inputs* inputs,
    float_array_ptr output);
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
};

/**
 * A Signal that accumulates its "source" input.
 *
 * Note: while Signaletic doesn't have a formal
 * concept of a control rate, this Signal
 * currently only read the first sample from each block
 * of its source input, and so effectively runs at kr.
 */
struct sig_dsp_Accumulate {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Accumulate_Inputs* inputs;
    struct sig_dsp_Accumulate_Parameters parameters;
    float accumulator;
    float previousReset;
};

struct sig_dsp_Accumulate* sig_dsp_Accumulate_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Accumulate_Inputs* inputs);
void sig_dsp_Accumulate_init(
    struct sig_dsp_Accumulate* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Accumulate_Inputs* inputs,
    float_array_ptr output);
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
    struct sig_dsp_GatedTimer_Inputs* inputs;
    unsigned long timer;
    bool hasFired;
    float prevGate;
};

struct sig_dsp_GatedTimer* sig_dsp_GatedTimer_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_GatedTimer_Inputs* inputs);
void sig_dsp_GatedTimer_init(struct sig_dsp_GatedTimer* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_GatedTimer_Inputs* inputs,
    float_array_ptr output);
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
    struct sig_dsp_TimedTriggerCounter_Inputs* inputs;
    int numTriggers;
    long timer;
    bool isTimerActive;
    float previousSource;
};

struct sig_dsp_TimedTriggerCounter* sig_dsp_TimedTriggerCounter_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_TimedTriggerCounter_Inputs* inputs);
void sig_dsp_TimedTriggerCounter_init(
    struct sig_dsp_TimedTriggerCounter* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_TimedTriggerCounter_Inputs* inputs,
    float_array_ptr output);
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
    struct sig_dsp_ToggleGate_Inputs* inputs;
    bool isGateOpen;
    float prevTrig;
};

struct sig_dsp_ToggleGate* sig_dsp_ToggleGate_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_ToggleGate_Inputs* inputs);
void sig_dsp_ToggleGate_init(
    struct sig_dsp_ToggleGate* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_ToggleGate_Inputs* inputs,
    float_array_ptr output);
void sig_dsp_ToggleGate_generate(void* signal);
void sig_dsp_ToggleGate_destroy(
    struct sig_Allocator* allocator,
    struct sig_dsp_ToggleGate* self);

struct sig_dsp_Oscillator_Inputs {
    float_array_ptr freq;
    float_array_ptr phaseOffset;
    float_array_ptr mul;
    float_array_ptr add;
};

struct sig_dsp_Oscillator {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Oscillator_Inputs* inputs;
    float phaseAccumulator;
};

void sig_dsp_Oscillator_init(struct sig_dsp_Oscillator* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Oscillator_Inputs* inputs, float_array_ptr output);

void sig_dsp_Sine_init(struct sig_dsp_Oscillator* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Oscillator_Inputs* inputs, float_array_ptr output);
struct sig_dsp_Oscillator* sig_dsp_Sine_new(struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Oscillator_Inputs* inputs);
void sig_dsp_Sine_generate(void* signal);
void sig_dsp_Sine_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self);


struct sig_dsp_LFTriangle {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Oscillator_Inputs* inputs;
    float phaseAccumulator;
};

void sig_dsp_LFTriangle_init(struct sig_dsp_Oscillator* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Oscillator_Inputs* inputs, float_array_ptr output);
struct sig_dsp_Oscillator* sig_dsp_LFTriangle_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Oscillator_Inputs* inputs);
void sig_dsp_LFTriangle_generate(void* signal);
void sig_dsp_LFTriangle_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* self);


struct sig_dsp_OnePole_Inputs {
    float_array_ptr source;
    float_array_ptr coefficient;
};

struct sig_dsp_OnePole {
    struct sig_dsp_Signal signal;
    struct sig_dsp_OnePole_Inputs* inputs;
    float previousSample;
};

void sig_dsp_OnePole_init(struct sig_dsp_OnePole* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_OnePole_Inputs* inputs,
    float_array_ptr output);
struct sig_dsp_OnePole* sig_dsp_OnePole_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_OnePole_Inputs* inputs);
void sig_dsp_OnePole_generate(void* signal);
void sig_dsp_OnePole_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_OnePole* self);


struct sig_dsp_Tanh_Inputs {
    float_array_ptr source;
};

struct sig_dsp_Tanh {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Tanh_Inputs* inputs;
};

void sig_dsp_Tanh_init(struct sig_dsp_Tanh* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Tanh_Inputs* inputs,
    float_array_ptr output);
struct sig_dsp_Tanh* sig_dsp_Tanh_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Tanh_Inputs* inputs);
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
    struct sig_dsp_Looper_Inputs* inputs;
    struct sig_dsp_Looper_Loop loop;
    size_t loopLastIdx;
    float playbackPos;
    float previousRecord;
    float previousClear;
};

void sig_dsp_Looper_init(struct sig_dsp_Looper* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Looper_Inputs* inputs,
    float_array_ptr output);
struct sig_dsp_Looper* sig_dsp_Looper_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Looper_Inputs* inputs);
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
    struct sig_dsp_Dust_Inputs* inputs;
    struct sig_dsp_Dust_Parameters parameters;

    // TODO: Should this be part of AudioSettings?
    float sampleDuration;

    float previousDensity;
    float threshold;
    float scale;
};

void sig_dsp_Dust_init(struct sig_dsp_Dust* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Dust_Inputs* inputs,
    float_array_ptr output);
struct sig_dsp_Dust* sig_dsp_Dust_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_Dust_Inputs* inputs);
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
    struct sig_dsp_TimedGate_Inputs* inputs;
    struct sig_dsp_TimedGate_Parameters parameters;

    float previousTrigger;
    float gateValue;
    float previousDuration;
    long durationSamps;
    long samplesRemaining;
};

void sig_dsp_TimedGate_init(struct sig_dsp_TimedGate* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_TimedGate_Inputs* inputs,
    float_array_ptr output);
struct sig_dsp_TimedGate* sig_dsp_TimedGate_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_TimedGate_Inputs* inputs);
void sig_dsp_TimedGate_generate(void* signal);
void sig_dsp_TimedGate_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_TimedGate* self);



struct sig_dsp_ClockFreqDetector_Inputs {
    float_array_ptr source;
};

struct sig_dsp_ClockFreqDetector_Parameters {
    float threshold;
    float timeoutDuration;
};

/**
 * Detects the number of pulses per second of an incoming clock signal
 * (i.e. any signal that outputs a rising edge to denote clock pulses)
 * and outputs the clock's tempo in pulses per second.
 *
 * Inputs:
 *  - source the incoming clock signal
 */
struct sig_dsp_ClockFreqDetector {
    struct sig_dsp_Signal signal;
    struct sig_dsp_ClockFreqDetector_Inputs* inputs;
    struct sig_dsp_ClockFreqDetector_Parameters parameters;

    float previousTrigger;
    bool isRisingEdge;
    uint32_t samplesSinceLastPulse;
    float clockFreq;
    uint32_t pulseDurSamples;
};

void sig_dsp_ClockFreqDetector_init(
    struct sig_dsp_ClockFreqDetector* self,
    struct sig_AudioSettings* settings,
    struct sig_dsp_ClockFreqDetector_Inputs* inputs,
    float_array_ptr output);
struct sig_dsp_ClockFreqDetector* sig_dsp_ClockFreqDetector_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_dsp_ClockFreqDetector_Inputs* inputs);
void sig_dsp_ClockFreqDetector_generate(void* signal);
void sig_dsp_ClockFreqDetector_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_ClockFreqDetector* self);


#ifdef __cplusplus
}
#endif

#endif
