/*! \file libstar.h
    \brief The core Signaletic library.

    Signaletic is music signal processing library designed for use
    in embedded environments and Web Assembly.
*/

#ifndef LIBSTAR_H
#define LIBSTAR_H

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
// As a result, all float* variables in Starlings
// in public interface must use this type instead.
#ifdef __EMSCRIPTEN__
    typedef void* float_array_ptr;
    #define FLOAT_ARRAY(array) ((float*)array)
#else
    typedef float* float_array_ptr;
    #define FLOAT_ARRAY(array) array
#endif

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
 * Generates a random float between 0.0 and 1.0.
 *
 * @return a random value
 */
float star_randf();

/**
 * Converts a unipolar floating point sample in the range 0.0 to 1.0
 * into to an unsigned 12-bit integer in the range 0 to 4095.
 *
 * This function does not clamp the sample.
 *
 * @param sample the floating point sample to convert
 * @return the sample converted to an unsigned 12-bit integer
 */
uint16_t star_unipolarToUint12(float sample);

/**
 * Converts a bipolar floating point sample in the range -1.0 to 1.0
 * into to an unsigned 12-bit integer in the range 0 to 4095.
 *
 * This function does not clamp the sample.
 *
 * @param sample the floating point sample to convert
 * @return the sample converted to an unsigned 12-bit integer
 */
uint16_t star_bipolarToUint12(float sample);

/**
 * Converts a bipolar floating point sample in the range -1.0 to 1.0
 * into to an  unsigned 12-bit integer in the range 4095-0.
 *
 * This function does not clamp the sample.
 *
 * @param sample the floating point sample to convert
 * @return the sample converted to an unsigned 12-bit integer
 */
uint16_t star_bipolarToInvUint12(float sample);

/**
 * Converts MIDI note numbers into frequencies in hertz.
 * This algorithm assumes A4 = 440 Hz = MIDI note #69.
 *
 * @param midiNum the MIDI note number to convert
 * @return the frequency in Hz of the note number
 */
float star_midiToFreq(float midiNum);

/**
 * Type definition for array fill functions.
 *
 * @param i the current index of the array
 * @param array the array being filled
 */
typedef float (*star_array_filler)(size_t i, float_array_ptr array);

/**
 * A fill function that returns random floats.
 *
 * @param i unused
 * @param array unused
 */
float star_randomFill(size_t i, float_array_ptr array);

/**
 * Fills an array of floats using the specified filler function.
 *
 * @param array the array to fill
 * @param length the lengthg of the array
 * @param filler a pointer to a fill function
 */
void star_fill(float_array_ptr array, size_t length,
    star_array_filler filler);

/**
 * Fills an array of floats with the specified value.
 *
 * @param value the value to fill the array with
 * @param array the array to fill
 * @param length the length of the array to fill
 **/
void star_fillWithValue(float_array_ptr array, size_t length,
    float value);

/**
 * Fills an array of floats with zeroes.
 *
 * @param array the array to fill with silence
 * @param length the length of the array to fill
 **/
void star_fillWithSilence(float_array_ptr array, size_t length);

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
float star_interpolate_linear(float idx, float_array_ptr table,
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
float star_interpolate_cubic(float idx, float_array_ptr table, size_t length);

/**
 * A one pole filter.
 *
 * @param current the current sample (n)
 * @param previous the previous sample (n-1)
 * @param coeff the filter coefficient
 */
float star_filter_onepole(float current, float previous, float coeff);

/**
 * Type definition for a waveform generator function.
 *
 * @param phase the current phase
 * @return a sample for the current phase of the waveform
 */
typedef float (*star_waveform_generator)(float phase);

/**
 * Generates one sample of a sine wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float star_waveform_sine(float phase);

/**
 * Generates one sample of a square wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float star_waveform_square(float phase);

/**
 * Generates one sample of a saw wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float star_waveform_saw(float phase);

/**
 * Generates one sample of a reverse saw wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float star_waveform_reverseSaw(float phase);

/**
 * Generates one sample of a triangle wave at the specified phase.
 *
 * @param phase the current phase of the waveform
 * @return the generated sample
 */
float star_waveform_triangle(float phase);


/**
 * An AudioSettings structure holds key information
 * about the current configuration of the audio system,
 * including sample rate, number of channels, and the
 * audio block size.
 */
struct star_AudioSettings {
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
 * The default audio settings used by Starlings
 * for 48KHz mono audio with a block size of 48.
 */
static const struct star_AudioSettings star_DEFAULT_AUDIOSETTINGS = {
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
size_t star_secondsToSamples(struct star_AudioSettings* audioSettings,
    float duration);

/**
 * A realtime-capable memory allocator.
 */
struct star_Allocator {
    size_t heapSize;
    void* heap;
};
void star_Allocator_init(struct star_Allocator* self);
void* star_Allocator_malloc(struct star_Allocator* self, size_t size);
void star_Allocator_free(struct star_Allocator* self, void* obj);


/**
 * Allocates a new AudioSettings instance with
 * the values from star_DEFAULT_AUDIO_SETTINGS.
 *
 * @param allocator the memory allocator to use
 * @return the AudioSettings
 */
struct star_AudioSettings* star_AudioSettings_new(struct star_Allocator* allocator);

/**
 * Destroys an AudioSettings instance.
 *
 * @param allocator the memory allocator to use
 * @param self the AudioSettings instance to destroy
 */
void star_AudioSettings_destroy(struct star_Allocator* allocator,
    struct star_AudioSettings* self);

/**
 * A Buffer holds an array of samples and its length.
 */
struct star_Buffer {
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
struct star_Buffer* star_Buffer_new(struct star_Allocator* allocator,
    size_t length);

/**
 * Fills a buffer using the specified array fill function.
 *
 * @param self the buffer to fill
 * @param filler a pointer to an array filler
 */
void star_Buffer_fill(struct star_Buffer* self, star_array_filler filler);

/**
 * Fills a Buffer instance with a value.
 *
 * @param self the buffer instance to fill
 * @param value the value to which all samples will be set
 */
void star_Buffer_fillWithValue(struct star_Buffer* self, float value);

/**
 * Fills a Buffer with silence.
 *
 * @param self the buffer to fill with zeroes
 */
void star_Buffer_fillWithSilence(struct star_Buffer* self);

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
void star_Buffer_fillWithWaveform(struct star_Buffer* self,
    star_waveform_generator generate, float sampleRate,
    float phase, float freq);

/**
 * Destroys a Buffer and frees its memory.
 *
 * After calling this function, the pointer to
 * the buffer instance will no longer be usable.
 *
 * @param allocator the memory allocator to use
 * @param self the buffer instance to destroy
 */
void star_Buffer_destroy(struct star_Allocator* allocator, struct star_Buffer* self);


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
struct star_Buffer* star_BufferView_new(
    struct star_Allocator* allocator, struct star_Buffer* buffer,
    size_t startIdx, size_t length);

/**
 * Destroys a BufferView.
 * Note that this will not free the samples array,
 * since it is a shared object borrowed from another Buffer.
 *
 * @param allocator the allocator to use
 * @param self the subbuffer to destroy
 */
void star_BufferView_destroy(struct star_Allocator* allocator,
    struct star_Buffer* self);


float_array_ptr star_AudioBlock_new(struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings);
float_array_ptr star_AudioBlock_newWithValue(
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings,
    float value);



// TODO: Should the signal argument at least be defined
// as a struct star_sig_Signal*, rather than void*?
// Either way, this is cast by the implementation to whatever
// concrete Signal type is appropriate.
typedef void (*star_sig_generateFn)(void* signal);

void star_sig_generateSilence(void* signal);

// TODO: Should the base Signal define a (void* ?) pointer
// to inputs, and provide an empty star_sig_Signal_Inputs struct
// as the default implementation?
struct star_sig_Signal {
    struct star_AudioSettings* audioSettings;
    float_array_ptr output;
    star_sig_generateFn generate;
};

void star_sig_Signal_init(void* signal, struct star_AudioSettings* settings,
    float_array_ptr output, star_sig_generateFn generate);
void star_sig_Signal_generate(void* signal);
void star_sig_Signal_destroy(struct star_Allocator* allocator, void* signal);


struct star_sig_Value_Parameters {
    float value;
};

struct star_sig_Value {
    struct star_sig_Signal signal;
    struct star_sig_Value_Parameters parameters;
    float lastSample;
};

struct star_sig_Value* star_sig_Value_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings);
void star_sig_Value_init(struct star_sig_Value* self,
    struct star_AudioSettings* settings, float_array_ptr output);
void star_sig_Value_generate(void* signal);
void star_sig_Value_destroy(struct star_Allocator* allocator,
    struct star_sig_Value* self);


struct star_sig_BinaryOp_Inputs {
    float_array_ptr left;
    float_array_ptr right;
};

struct star_sig_BinaryOp {
    struct star_sig_Signal signal;
    struct star_sig_BinaryOp_Inputs* inputs;
};


struct star_sig_BinaryOp* star_sig_Add_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs);
void star_sig_Add_init(struct star_sig_BinaryOp* self,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs,
    float_array_ptr output);
void star_sig_Add_generate(void* signal);
void star_sig_Add_destroy(struct star_Allocator* allocator,
    struct star_sig_BinaryOp* self);


void star_sig_Mul_init(struct star_sig_BinaryOp* self,
    struct star_AudioSettings* settings, struct star_sig_BinaryOp_Inputs* inputs, float_array_ptr output);
struct star_sig_BinaryOp* star_sig_Mul_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs);
void star_sig_Mul_generate(void* signal);
void star_sig_Mul_destroy(struct star_Allocator* allocator, struct star_sig_BinaryOp* self);


struct star_sig_BinaryOp* star_sig_Div_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs);
void star_sig_Div_init(struct star_sig_BinaryOp* self,
    struct star_AudioSettings* settings,
    struct star_sig_BinaryOp_Inputs* inputs,
    float_array_ptr output);
void star_sig_Div_generate(void* signal);
void star_sig_Div_destroy(struct star_Allocator* allocator,
    struct star_sig_BinaryOp* self);


struct star_sig_Invert_Inputs {
    float_array_ptr source;
};

struct star_sig_Invert {
    struct star_sig_Signal signal;
    struct star_sig_Invert_Inputs* inputs;
};

struct star_sig_Invert* star_sig_Invert_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Invert_Inputs* inputs);
void star_sig_Invert_init(struct star_sig_Invert* self,
    struct star_AudioSettings* settings,
    struct star_sig_Invert_Inputs* inputs,
    float_array_ptr output);
void star_sig_Invert_generate(void* signal);
void star_sig_Invert_destroy(struct star_Allocator* allocator,
    struct star_sig_Invert* self);


/**
 * The inputs for an Accumulator Signal.
 */
struct star_sig_Accumulate_Inputs {
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

struct star_sig_Accumulate_Parameters {
    float accumulatorStart;
};

/**
 * A Signal that accumulates its "source" input.
 *
 * Note: while Starlings doesn't have a formal
 * concept of a control rate, this Signal
 * currently only read the first sample from each block
 * of its source input, and so effectively runs at kr.
 */
struct star_sig_Accumulate {
    struct star_sig_Signal signal;
    struct star_sig_Accumulate_Inputs* inputs;
    struct star_sig_Accumulate_Parameters parameters;
    float accumulator;
    float previousReset;
};

struct star_sig_Accumulate* star_sig_Accumulate_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Accumulate_Inputs* inputs);
void star_sig_Accumulate_init(
    struct star_sig_Accumulate* self,
    struct star_AudioSettings* settings,
    struct star_sig_Accumulate_Inputs* inputs,
    float_array_ptr output);
void star_sig_Accumulate_generate(void* signal);
void star_sig_Accumulate_destroy(struct star_Allocator* allocator,
    struct star_sig_Accumulate* self);


/**
 * Inputs for a GatedTimer.
 */
struct star_sig_GatedTimer_Inputs {
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
struct star_sig_GatedTimer {
    struct star_sig_Signal signal;
    struct star_sig_GatedTimer_Inputs* inputs;
    unsigned long timer;
    bool hasFired;
    float prevGate;
};

struct star_sig_GatedTimer* star_sig_GatedTimer_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_GatedTimer_Inputs* inputs);
void star_sig_GatedTimer_init(struct star_sig_GatedTimer* self,
    struct star_AudioSettings* settings,
    struct star_sig_GatedTimer_Inputs* inputs,
    float_array_ptr output);
void star_sig_GatedTimer_generate(void* signal);
void star_sig_GatedTimer_destroy(struct star_Allocator* allocator,
    struct star_sig_GatedTimer* self);


/**
 * The inputs for a TimedTriggerCounter Signal.
 */
struct star_sig_TimedTriggerCounter_Inputs {
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
struct star_sig_TimedTriggerCounter {
    struct star_sig_Signal signal;
    struct star_sig_TimedTriggerCounter_Inputs* inputs;
    int numTriggers;
    long timer;
    bool isTimerActive;
    float previousSource;
};

struct star_sig_TimedTriggerCounter* star_sig_TimedTriggerCounter_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_TimedTriggerCounter_Inputs* inputs);
void star_sig_TimedTriggerCounter_init(
    struct star_sig_TimedTriggerCounter* self,
    struct star_AudioSettings* settings,
    struct star_sig_TimedTriggerCounter_Inputs* inputs,
    float_array_ptr output);
void star_sig_TimedTriggerCounter_generate(void* signal);
void star_sig_TimedTriggerCounter_destroy(
    struct star_Allocator* allocator,
    struct star_sig_TimedTriggerCounter* self);


/**
 * The inputs for a ToggleGate Signal.
 */
struct star_sig_ToggleGate_Inputs {

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
struct star_sig_ToggleGate {
    struct star_sig_Signal signal;
    struct star_sig_ToggleGate_Inputs* inputs;
    bool isGateOpen;
    float prevTrig;
};

struct star_sig_ToggleGate* star_sig_ToggleGate_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_ToggleGate_Inputs* inputs);
void star_sig_ToggleGate_init(
    struct star_sig_ToggleGate* self,
    struct star_AudioSettings* settings,
    struct star_sig_ToggleGate_Inputs* inputs,
    float_array_ptr output);
void star_sig_ToggleGate_generate(void* signal);
void star_sig_ToggleGate_destroy(
    struct star_Allocator* allocator,
    struct star_sig_ToggleGate* self);

struct star_sig_Sine_Inputs {
    float_array_ptr freq;
    float_array_ptr phaseOffset;
    float_array_ptr mul;
    float_array_ptr add;
};

struct star_sig_Sine {
    struct star_sig_Signal signal;
    struct star_sig_Sine_Inputs* inputs;
    float phaseAccumulator;
};

void star_sig_Sine_init(struct star_sig_Sine* self,
    struct star_AudioSettings* settings, struct star_sig_Sine_Inputs* inputs, float_array_ptr output);
struct star_sig_Sine* star_sig_Sine_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Sine_Inputs* inputs);
void star_sig_Sine_generate(void* signal);
void star_sig_Sine_destroy(struct star_Allocator* allocator, struct star_sig_Sine* self);


struct star_sig_OnePole_Inputs {
    float_array_ptr source;
    float_array_ptr coefficient;
};

struct star_sig_OnePole {
    struct star_sig_Signal signal;
    struct star_sig_OnePole_Inputs* inputs;
    float previousSample;
};

void star_sig_OnePole_init(struct star_sig_OnePole* self,
    struct star_AudioSettings* settings,
    struct star_sig_OnePole_Inputs* inputs,
    float_array_ptr output);
struct star_sig_OnePole* star_sig_OnePole_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_OnePole_Inputs* inputs);
void star_sig_OnePole_generate(void* signal);
void star_sig_OnePole_destroy(struct star_Allocator* allocator,
    struct star_sig_OnePole* self);


struct star_sig_Tanh_Inputs {
    float_array_ptr source;
};

struct star_sig_Tanh {
    struct star_sig_Signal signal;
    struct star_sig_Tanh_Inputs* inputs;
};

void star_sig_Tanh_init(struct star_sig_Tanh* self,
    struct star_AudioSettings* settings,
    struct star_sig_Tanh_Inputs* inputs,
    float_array_ptr output);
struct star_sig_Tanh* star_sig_Tanh_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Tanh_Inputs* inputs);
void star_sig_Tanh_generate(void* signal);
void star_sig_Tanh_destroy(struct star_Allocator* allocator,
    struct star_sig_Tanh* self);


struct star_sig_Looper_Inputs {
    float_array_ptr source;
    float_array_ptr start;
    float_array_ptr end;
    float_array_ptr speed;
    float_array_ptr record;
    float_array_ptr clear;
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
    float_array_ptr output);
struct star_sig_Looper* star_sig_Looper_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Looper_Inputs* inputs);
void star_sig_Looper_generate(void* signal);
void star_sig_Looper_destroy(struct star_Allocator* allocator,
    struct star_sig_Looper* self);

struct star_sig_Dust_Parameters {
    float bipolar;
};

struct star_sig_Dust_Inputs {
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
struct star_sig_Dust {
    struct star_sig_Signal signal;
    struct star_sig_Dust_Inputs* inputs;
    struct star_sig_Dust_Parameters parameters;

    // TODO: Should this be part of AudioSettings?
    float sampleDuration;

    float previousDensity;
    float threshold;
    float scale;
};

void star_sig_Dust_init(struct star_sig_Dust* self,
    struct star_AudioSettings* settings,
    struct star_sig_Dust_Inputs* inputs,
    float_array_ptr output);
struct star_sig_Dust* star_sig_Dust_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Dust_Inputs* inputs);
void star_sig_Dust_generate(void* signal);
void star_sig_Dust_destroy(struct star_Allocator* allocator,
    struct star_sig_Dust* self);



struct star_sig_TimedGate_Parameters {
    float resetOnTrigger;
    float bipolar;
};

struct star_sig_TimedGate_Inputs {
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
struct star_sig_TimedGate {
    struct star_sig_Signal signal;
    struct star_sig_TimedGate_Inputs* inputs;
    struct star_sig_TimedGate_Parameters parameters;

    float previousTrigger;
    float gateValue;
    float previousDuration;
    long durationSamps;
    long samplesRemaining;
};

void star_sig_TimedGate_init(struct star_sig_TimedGate* self,
    struct star_AudioSettings* settings,
    struct star_sig_TimedGate_Inputs* inputs,
    float_array_ptr output);
struct star_sig_TimedGate* star_sig_TimedGate_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_TimedGate_Inputs* inputs);
void star_sig_TimedGate_generate(void* signal);
void star_sig_TimedGate_destroy(struct star_Allocator* allocator,
    struct star_sig_TimedGate* self);



struct star_sig_ClockFreqDetector_Inputs {
    float_array_ptr source;
};

struct star_sig_ClockFreqDetector_Parameters {
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
struct star_sig_ClockFreqDetector {
    struct star_sig_Signal signal;
    struct star_sig_ClockFreqDetector_Inputs* inputs;
    struct star_sig_ClockFreqDetector_Parameters parameters;

    float previousTrigger;
    bool isRisingEdge;
    uint32_t samplesSinceLastPulse;
    float clockFreq;
    uint32_t pulseDurSamples;
};

void star_sig_ClockFreqDetector_init(
    struct star_sig_ClockFreqDetector* self,
    struct star_AudioSettings* settings,
    struct star_sig_ClockFreqDetector_Inputs* inputs,
    float_array_ptr output);
struct star_sig_ClockFreqDetector* star_sig_ClockFreqDetector_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_ClockFreqDetector_Inputs* inputs);
void star_sig_ClockFreqDetector_generate(void* signal);
void star_sig_ClockFreqDetector_destroy(struct star_Allocator* allocator,
    struct star_sig_ClockFreqDetector* self);


#ifdef __cplusplus
}
#endif

#endif
