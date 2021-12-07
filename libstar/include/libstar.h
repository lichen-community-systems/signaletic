#ifndef LIBSTAR_H
#define LIBSTAR_H

#ifdef __cplusplus
extern "C" {
#endif

// TODO: Doesn't exist in a non-Emcripten-compiled wasm environment.
// Need to either switch to a faster custom approximation of sin()
// and other functions, or build the wasm version with Emscripten.
static const float star_PI = 3.14159265358979323846f;
static const float star_TWOPI = 6.28318530717958647693f;

/**
 * Converts MIDI note numbers into frequencies in hertz.
 * This algorithm assumes A4 = 440 Hz = MIDI note #69.
 */
// TODO: Inline?
float star_midiToFreq(float midiNum);

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

void star_Buffer_fill(float value, float* buffer, size_t numSamples);
void star_Buffer_fillSilence(float* buffer, size_t numSamples);
float* star_Buffer_new(struct star_Allocator* allocator, size_t numSamples);
float* star_AudioBlock_new(struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings);
float* star_AudioBlock_newWithValue(float value,
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings);

void star_sig_generateSilence(void* signal);

struct star_sig_Signal {
    struct star_AudioSettings* audioSettings;
    float* output;

    void (*generate)(void* signal);
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


#ifdef __cplusplus
}
#endif

#endif
