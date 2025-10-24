#include <libsignaletic.h>
#include <math.h>

void testAssertBufferContainsValueOnly(struct sig_Allocator* allocator,
    float expectedValue, float* actual, size_t len);

void testAssertBuffersNotEqual(float* first, float* second, size_t len);

void testAssertBuffersNoValuesEqual(float* first, float* second, size_t len);

void testAssertBufferIsSilent(struct sig_Allocator* allocator,
    float* buffer, size_t len);

void testAssertBufferNotSilent(struct sig_Allocator* allocator,
    float* buffer, size_t len);

float toDecibels(float sample, float refLevel);

/**
 * @brief Asserts that all samples in the buffer are below
 * the specified dB level.
 *
 * @param buffer the buffer to test
 * @param maxDb the maximum dB level
 * @param refLevel the reference level for dB calculations
 * @param len the length of the buffer
 */
void testAssertSamplesBelowDB(float* buffer, float maxDb, float refLevel,
    size_t len);

/**
 * @brief Asserts that all samples in the buffer are above
 * the specified dB level.
 *
 * @param buffer the buffer to test
 * @param minDb the minimum dB level
 * @param refLevel the reference level for dB calculations
 * @param len the length of the buffer
 */
void testAssertSamplesAboveDB(float* buffer, float maxDb, float refLevel,
    size_t len);

void testAssertBufferValuesInRange(float* buffer, size_t len, float min,
    float max);

size_t countNonZeroSamples(float* buffer, size_t len);

int16_t countNonZeroSamplesGenerated(struct sig_dsp_Signal* signal,
    float_array_ptr output, int numRuns);

void testAssertBufferContainsNumZeroSamples(float* buffer,
    size_t len, int16_t expectedNumNonZero);

void testAssertGeneratedSignalContainsApproxNumNonZeroSamples(
    struct sig_dsp_Signal* signal, float_array_ptr output,
    int16_t expectedNumNonZero, double errorFactor, int numRuns);

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
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_Buffer* buffer;
    size_t currentSample;
};

void sig_test_BufferPlayer_generate(void* signal);

void sig_test_BufferPlayer_init(struct sig_test_BufferPlayer* self,
    struct sig_SignalContext* context, struct sig_Buffer* buffer);

struct sig_test_BufferPlayer* sig_test_BufferPlayer_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
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
    struct sig_test_BufferRecorder_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_Buffer* buffer;
    size_t currentSample;
};

void sig_test_BufferRecorder_generate(void* signal);

void sig_test_BufferRecorder_init(
    struct sig_test_BufferRecorder* self,
    struct sig_SignalContext* context,
    struct sig_Buffer* buffer);

struct sig_test_BufferRecorder* sig_test_BufferRecorder_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_Buffer* buffer);

void sig_test_BufferRecorder_destroy(struct sig_Allocator* allocator,
    struct sig_test_BufferRecorder* self);
