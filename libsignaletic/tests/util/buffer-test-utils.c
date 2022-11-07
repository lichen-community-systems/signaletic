#include <buffer-test-utils.h>
#include <unity.h>
#include <assert.h>

void testAssertBufferContainsValueOnly(struct sig_Allocator* allocator,
    float expectedValue, float* actual,
    size_t len) {
    float* expectedArray = (float*) allocator->impl->malloc(
        allocator, len * sizeof(float));
    sig_fillWithValue(expectedArray, len, expectedValue);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expectedArray, actual, len);
    allocator->impl->free(allocator, expectedArray);
}

void testAssertBuffersNotEqual(float* first, float* second, size_t len) {
    size_t numMatches = 0;

    for (size_t i = 0; i < len; i++) {
        if (first[i] == second[i]) {
            numMatches++;
        }
    }

    TEST_ASSERT_FALSE_MESSAGE(numMatches == len,
        "All values in both buffers were identical.");
}

void testAssertBuffersNoValuesEqual(float* first, float* second, size_t len) {
    int32_t failureIdx = -1;

    for (int32_t i = 0; i < len; i++) {
        if (first[i] == second[i]) {
            failureIdx = i;
            break;
        }
    }

    TEST_ASSERT_EQUAL_INT32_MESSAGE(-1, failureIdx,
        "Value at index was the same in both buffers.");
}

void testAssertBufferIsSilent(struct sig_Allocator* allocator,
    float* buffer, size_t len) {
    testAssertBufferContainsValueOnly(allocator, 0.0f, buffer, len);
}

void testAssertBufferNotSilent(struct sig_Allocator* allocator,
    float* buffer, size_t len) {
    float* silence = allocator->impl->malloc(allocator,
        sizeof(float) * len);
    sig_fillWithSilence(silence, len);
    testAssertBuffersNotEqual(silence, buffer, len);
}

void testAssertBufferValuesInRange(float* buffer, size_t len,
    float min, float max) {
    int32_t failureIdx = -1;

    for (int32_t i = 0; i < len; i++) {
        if (buffer[i] < min || buffer[i] > max) {
            failureIdx = i;
            break;
        }
    }

    TEST_ASSERT_EQUAL_INT32_MESSAGE(-1, failureIdx,
        "Value at index was not within the expected range.");
}

size_t countNonZeroSamples(float* buffer, size_t len) {
    size_t numNonZero = 0;
    for (size_t i = 0; i < len; i++) {
        if (buffer[i] != 0.0f) {
            numNonZero++;
        }
    }
    return numNonZero;
}

int16_t countNonZeroSamplesGenerated(struct sig_dsp_Signal* signal,
    int numRuns) {
    int16_t numNonZero = 0;
    for (int i = 0; i < numRuns; i++) {
        signal->generate(signal);
        numNonZero += countNonZeroSamples(signal->output,
            signal->audioSettings->blockSize);
    }

    return numNonZero;
}

void testAssertBufferContainsNumZeroSamples(float* buffer,
    size_t len, int16_t expectedNumNonZero) {
    int16_t numNonZero = countNonZeroSamples(buffer, len);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
        expectedNumNonZero,
        numNonZero,
        "An unexpected number of non-zero samples was found.");
}

void testAssertGeneratedSignalContainsApproxNumNonZeroSamples(
    struct sig_dsp_Signal* signal, int16_t expectedNumNonZero,
    double errorFactor, int numRuns) {
    double expectedNumNonZeroD = (double) expectedNumNonZero;
    double errorNumSamps = expectedNumNonZeroD * errorFactor;
    double high = expectedNumNonZeroD + errorNumSamps;
    double low = expectedNumNonZeroD - errorNumSamps;

    double avgNumNonZero = (double)
        countNonZeroSamplesGenerated(signal, numRuns) /
        (double) numRuns;
    double actualRoundedAvgNumNonZero = round(avgNumNonZero);
    TEST_ASSERT_TRUE_MESSAGE(actualRoundedAvgNumNonZero >= low &&
        actualRoundedAvgNumNonZero <= high,
        "The actual average number of non-zero samples was not within the expected range.");
}

void evaluateSignals(struct sig_AudioSettings* audioSettings,
    struct sig_dsp_Signal** signals, size_t numSignals, float duration) {
    size_t numBlocks = (size_t) (duration *
        (audioSettings->sampleRate / audioSettings->blockSize));

    for (size_t i = 0; i < numBlocks; i++) {
        for (size_t j = 0; j < numSignals; j++) {
            struct sig_dsp_Signal* signal = signals[j];
            signal->generate(signal);
        }
    }
}



void sig_test_BufferPlayer_generate(void* signal) {
    struct sig_test_BufferPlayer* self =
        (struct sig_test_BufferPlayer*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        if (self->currentSample >= self->buffer->length) {
            // Wrap around to the beginning of the buffer.
            self->currentSample = 0;
        }

        FLOAT_ARRAY(self->signal.output)[i] =
            FLOAT_ARRAY(self->buffer->samples)[self->currentSample];
        self->currentSample++;
    }
}

void sig_test_BufferPlayer_init(struct sig_test_BufferPlayer* self,
    struct sig_SignalContext* context, struct sig_Buffer* buffer,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, context, output,
        *sig_test_BufferPlayer_generate);
    self->buffer = buffer;
    self->currentSample = 0;
}

struct sig_test_BufferPlayer* sig_test_BufferPlayer_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
    struct sig_Buffer* buffer) {
    float_array_ptr output = sig_AudioBlock_new(allocator,
        context->audioSettings);
    struct sig_test_BufferPlayer* self = sig_MALLOC(allocator,
        struct sig_test_BufferPlayer);
    sig_test_BufferPlayer_init(self, context, buffer, output);

    return self;
}

void sig_test_BufferPlayer_destroy(struct sig_Allocator* allocator,
    struct sig_test_BufferPlayer* self) {
    sig_dsp_Signal_destroy(allocator, self);
}


void sig_test_BufferRecorder_generate(void* signal) {
    struct sig_test_BufferRecorder* self =
        (struct sig_test_BufferRecorder*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        // Blow up if we're still trying to record after the buffer is full.
        // This represents an error in the test.
        assert(self->currentSample < self->buffer->length);

        FLOAT_ARRAY(self->buffer->samples)[self->currentSample] =
            FLOAT_ARRAY(self->inputs.source)[i];

        self->currentSample++;
    }
}

void sig_test_BufferRecorder_init(
    struct sig_test_BufferRecorder* self,
    struct sig_SignalContext* context,
    struct sig_Buffer* buffer,
    float_array_ptr output) {
    sig_dsp_Signal_init(self, context, output,
        sig_test_BufferRecorder_generate);

    self->buffer = buffer;
    self->currentSample = 0;

    self->inputs.source = context->silence->signal.output;
}

struct sig_test_BufferRecorder* sig_test_BufferRecorder_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_Buffer* buffer) {
    float_array_ptr output = sig_AudioBlock_new(allocator,
        context->audioSettings);
    struct sig_test_BufferRecorder* self = sig_MALLOC(allocator,
        struct sig_test_BufferRecorder);
    sig_test_BufferRecorder_init(self, context, buffer, output);

    return self;
}

void sig_test_BufferRecorder_destroy(struct sig_Allocator* allocator,
    struct sig_test_BufferRecorder* self) {
    sig_dsp_Signal_destroy(allocator, self);
}
