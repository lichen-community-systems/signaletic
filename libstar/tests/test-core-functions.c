#include <math.h>
#include <unity.h>
#include <libstar.h>

#define FLOAT_EPSILON powf(2, -23)

// TODO: Factor into a test utilities file.
void TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN_MESSAGE(
    float expected, float* buffer, size_t bufferLen) {
    for (size_t i = 0; i < bufferLen; i++) {
        float actual = buffer[i];
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(FLOAT_EPSILON, expected, actual,
            "Buffer should be filled with expected value.");
    }
}

void TEST_ASSERT_BUFFER_CONTAINS_SILENCE_MESSAGE(
    float* buffer, size_t bufferLen) {
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN_MESSAGE(
        0.0f, buffer, bufferLen);
}

void setUp(void) {}

void tearDown(void) {}

void test_star_midiToFreq(void) {
    // 69 A 440
    float actual = star_midiToFreq(69.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(FLOAT_EPSILON, 440.0f, actual,
        "MIDI A4 should be 440 Hz");

    // 60 Middle C 261.63
    actual = star_midiToFreq(60.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 261.63f, actual,
        "MIDI C3 should be 261.63 Hz");

    // MIDI 0
    actual = star_midiToFreq(0.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 8.18f, actual,
        "MIDI 0 should be 8.18 Hz");

    // Quarter tone
    actual = star_midiToFreq(60.5f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 269.29f, actual,
        "A quarter tone above C3 should be 269.29 Hz");

    // Negative MIDI note numbers should return viable frequencies.
    actual = star_midiToFreq(-60.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 0.255, actual,
        "A negative MIDI number should return a negative frequency.");
}

void test_star_Buffer_fill(void) {
    size_t blockSize = 64;
    float buffer[blockSize];
    star_Buffer_fill(440.4f, buffer, blockSize);
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN_MESSAGE(
        440.4f, buffer, blockSize);
}

void test_star_Buffer_fillSilence(void) {
    size_t blockSize = 16;
    float buffer[blockSize];
    star_Buffer_fillSilence(buffer, blockSize);
    TEST_ASSERT_BUFFER_CONTAINS_SILENCE_MESSAGE(buffer, blockSize);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_star_midiToFreq);
    RUN_TEST(test_star_Buffer_fill);
    RUN_TEST(test_star_Buffer_fillSilence);

    return UNITY_END();
}
