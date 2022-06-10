#include <libsignaletic.h>

void testAssertBufferContainsValueOnly(struct sig_Allocator* allocator,
    float expectedValue, float* actual, size_t len);

void testAssertBuffersNotEqual(float* first, float* second, size_t len);

void testAssertBuffersNoValuesEqual(float* first, float* second, size_t len);

void testAssertBufferIsSilent(struct sig_Allocator* allocator,
    float* buffer, size_t len);

void testAssertBufferNotSilent(struct sig_Allocator* allocator,
    float* buffer, size_t len);

void testAssertBufferValuesInRange(float* buffer, size_t len, float min,
    float max);

size_t countNonZeroSamples(float* buffer, size_t len);

int16_t countNonZeroSamplesGenerated(struct sig_dsp_Signal* signal,
    int numRuns);

void testAssertBufferContainsNumZeroSamples(float* buffer,
    size_t len, int16_t expectedNumNonZero);

void testAssertGeneratedSignalContainsApproxNumNonZeroSamples(
    struct sig_dsp_Signal* signal, int16_t expectedNumNonZero,
    double errorFactor, int numRuns);

void evaluateSignals(struct sig_AudioSettings* audioSettings,
    struct sig_dsp_Signal** signals, size_t numSignals, float duration);

/**
 * A straight-through buffer player with no interpolation
 * or other fancy features. Primarily used for providing deterministic
 * inputs to signals for unit testing.
 *
 * This Signal should always only be for unit tests, so that
 * additional features aren't accidentally added to it that
 * might impact the determinism of the testing environment.
 */
struct sig_test_BufferPlayer {
    struct sig_dsp_Signal signal;
    struct sig_Buffer* buffer;
    size_t currentSample;
};

void sig_test_BufferPlayer_generate(void* signal);

void sig_test_BufferPlayer_init(struct sig_test_BufferPlayer* self,
    struct sig_AudioSettings* audioSettings,
    struct sig_Buffer* buffer,
    float_array_ptr output);

struct sig_test_BufferPlayer* sig_test_BufferPlayer_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_Buffer* buffer);

void sig_test_BufferPlayer_destroy(struct sig_Allocator* allocator,
    struct sig_test_BufferPlayer* self);


struct sig_test_BufferRecorder_Inputs {
    float_array_ptr source;
};


/**
 * A signal that records its source input into a buffer.
 * Primarily used for recording signal outputs for unit testing.
 *
 * This should always only be for unit tests, so that
 * additional features aren't accidentally added to it that
 * might impact the determinism of the testing environment.
 */
struct sig_test_BufferRecorder {
    struct sig_dsp_Signal signal;
    struct sig_test_BufferRecorder_Inputs* inputs;
    struct sig_Buffer* buffer;
    size_t currentSample;
};

void sig_test_BufferRecorder_generate(void* signal);

void sig_test_BufferRecorder_init(
    struct sig_test_BufferRecorder* self,
    struct sig_AudioSettings* audioSettings,
    struct sig_test_BufferRecorder_Inputs* inputs,
    struct sig_Buffer* buffer,
    float_array_ptr output);

struct sig_test_BufferRecorder* sig_test_BufferRecorder_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    struct sig_test_BufferRecorder_Inputs* inputs,
    struct sig_Buffer* buffer);

void sig_test_BufferRecorder_destroy(struct sig_Allocator* allocator,
    struct sig_test_BufferRecorder* self);
