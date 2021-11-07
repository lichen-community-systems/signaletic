#ifndef LIBSTAR_H
#define LIBSTAR_H

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Doesn't exist in a non-Emcripten-compiled wasm environment.
// Need to either switch to a faster custom approximation or
// Build the wasm version with Emscripten.
#include <math.h>

static const float star_TWOPI = M_PI * 2.0f;

/**
 * Converts MIDI note numbers into frequencies in hertz.
 * This algorithm assumes A4 = 440 Hz = MIDI note #69.
 */
// TODO: Inline?
float star_midiToFreq(float midiNum);

struct star_AudioSettings {
    float sampleRate;
    int numChannels;
    unsigned int blockSize;
};

static const struct star_AudioSettings star_DEFAULT_AUDIOSETTINGS = {
    .sampleRate = 48000.0f,
    .numChannels = 1,
    .blockSize = 48
};

void star_Buffer_fill(float value, float* buffer, int blockSize);

void star_Buffer_fillSilence(float* buffer, int blockSize);

struct star_sig_Signal {
    struct star_AudioSettings* audioSettings;
    float* output;

    void (*generate)(void* self);
};

struct star_sig_Value_Parameters {
    float value;
};

struct star_sig_Value {
    struct star_sig_Signal signal;
    struct star_sig_Value_Parameters parameters;
    float lastSample;
};

void star_sig_Value_init(void* self,
    struct star_AudioSettings* settings, float* output);
void star_sig_Value_generate(void* self);

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

void star_sig_Sine_init(void* self,
    struct star_AudioSettings* settings, struct star_sig_Sine_Inputs* inputs, float* output);
void star_sig_Sine_generate(void* self);


struct star_sig_Gain_Inputs {
    float* gain;
    float* source;
};

struct star_sig_Gain {
    struct star_sig_Signal signal;
    struct star_sig_Gain_Inputs* inputs;
};

void star_sig_Gain_init(void* self,
    struct star_AudioSettings* settings, struct star_sig_Gain_Inputs* inputs, float* output);

void star_sig_Gain_generate(void* self);

#ifdef __cplusplus
}
#endif

#endif
