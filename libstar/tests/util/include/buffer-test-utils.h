#include <libstar.h>

void testAssertBufferContainsValueOnly(struct star_Allocator* allocator,
    float expectedValue, float* actual, size_t len);

void testAssertBuffersNotEqual(float* first, float* second, size_t len);

void testAssertBuffersNoValuesEqual(float* first, float* second, size_t len);

void testAssertBufferIsSilent(struct star_Allocator* allocator,
    float* buffer, size_t len);

void testAssertBufferNotSilent(struct star_Allocator* allocator,
    float* buffer, size_t len);

void testAssertBufferValuesInRange(float* buffer, size_t len, float min,
    float max);

size_t countNonZeroSamples(float* buffer, size_t len);

int16_t countNonZeroSamplesGenerated(struct star_sig_Signal* signal,
    int numRuns);

void testAssertBufferContainsNumZeroSamples(float* buffer,
    size_t len, int16_t expectedNumNonZero);

void testAssertGeneratedSignalContainsApproxNumNonZeroSamples(
    struct star_sig_Signal* signal, int16_t expectedNumNonZero,
    double errorFactor, int numRuns);

void evaluateSignals(struct star_AudioSettings* audioSettings,
    struct star_sig_Signal** signals, size_t numSignals, float duration);

/**
 * A straight-through buffer player with no interpolation
 * or other fancy features. Primarily used for providing deterministic
 * inputs to signals for unit testing.
 *
 * This Signal should always only be for unit tests, so that
 * additional features aren't accidentally added to it that
 * might impact the determinism of the testing environment.
 */
struct star_test_BufferPlayer {
    struct star_sig_Signal signal;
    struct star_Buffer* buffer;
    size_t currentSample;
};

void star_test_BufferPlayer_generate(void* signal);

void star_test_BufferPlayer_init(struct star_test_BufferPlayer* self,
    struct star_AudioSettings* audioSettings,
    struct star_Buffer* buffer,
    float_array_ptr output);

struct star_test_BufferPlayer* star_test_BufferPlayer_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings,
    struct star_Buffer* buffer);

void star_test_BufferPlayer_destroy(struct star_Allocator* allocator,
    struct star_test_BufferPlayer* self);


struct star_test_BufferRecorder_Inputs {
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
struct star_test_BufferRecorder {
    struct star_sig_Signal signal;
    struct star_test_BufferRecorder_Inputs* inputs;
    struct star_Buffer* buffer;
    size_t currentSample;
};

void star_test_BufferRecorder_generate(void* signal);

void star_test_BufferRecorder_init(
    struct star_test_BufferRecorder* self,
    struct star_AudioSettings* audioSettings,
    struct star_test_BufferRecorder_Inputs* inputs,
    struct star_Buffer* buffer,
    float_array_ptr output);

struct star_test_BufferRecorder* star_test_BufferRecorder_new(
    struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings,
    struct star_test_BufferRecorder_Inputs* inputs,
    struct star_Buffer* buffer);

void star_test_BufferRecorder_destroy(struct star_Allocator* allocator,
    struct star_test_BufferRecorder* self);
