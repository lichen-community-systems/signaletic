#ifndef LIBSTAR_H
#define LIBSTAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

// This typedef if necessary because Emscripten's
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
#else
    typedef float* float_array_ptr;
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
 * @param idx {float} an index into the table
 * @param table {float*} the table from which values around idx should be drawn and interpolated
 * @param length {size_t} the length of the buffer
 * @return {float} the interpolated value
 */
float star_interpolate_cubic(float idx, float_array_ptr table, size_t length);

float star_filter_onepole(float current, float previous, float coeff);

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

    // TODO: Should this just be an unsigned short?
    /**
     * The number of audio output channels.
     */
    size_t numChannels;

    // TODO: Should this also be an unsigned short?
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
 * Fills a Buffer instance with a value.
 *
 * @param self the buffer instance to fill
 * @param value the value to which all samples will be set
 */
void star_Buffer_fill(struct star_Buffer* self, float value);

/**
 * Fills a Buffer with silence.
 *
 * @param self the buffer to fill with zeroes
 */
void star_Buffer_fillWithSilence(struct star_Buffer* self);

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


float_array_ptr star_AudioBlock_new(struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings);
float_array_ptr star_AudioBlock_newWithValue(float value,
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings);

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

struct star_sig_Gain_Inputs {
    float_array_ptr gain;
    float_array_ptr source;
};

struct star_sig_Gain {
    struct star_sig_Signal signal;
    struct star_sig_Gain_Inputs* inputs;
};

void star_sig_Gain_init(struct star_sig_Gain* self,
    struct star_AudioSettings* settings, struct star_sig_Gain_Inputs* inputs, float_array_ptr output);
struct star_sig_Gain* star_sig_Gain_new(struct star_Allocator* allocator,
    struct star_AudioSettings* settings,
    struct star_sig_Gain_Inputs* inputs);
void star_sig_Gain_generate(void* signal);
void star_sig_Gain_destroy(struct star_Allocator* allocator, struct star_sig_Gain* self);


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


struct star_sig_Looper_Inputs {
    float_array_ptr source;
    float_array_ptr start;
    float_array_ptr length;
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

#ifdef __cplusplus
}
#endif

#endif
