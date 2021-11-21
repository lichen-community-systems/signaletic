#include <math.h>
#include <unity.h>
#include <libstar.h>

#define FLOAT_EPSILON powf(2, -23)

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_star_midiToFreq);
    return UNITY_END();
}
