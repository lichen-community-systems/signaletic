#include <math.h>
#include <unity.h>
#include <tlsf.h>
#include <libsignaletic.h>
#include <stdlib.h> // TODO: Remove when a custom RNG is implemented.
#include <buffer-test-utils.h>

#define FLOAT_EPSILON powf(2, -23)
#define HEAP_SIZE 26214400 // 25 MB
#define BLOCK_SIZE 64

uint8_t heapMemory[HEAP_SIZE];

struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) heapMemory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

struct sig_AudioSettings mono441kAudioSettings = {
    .blockSize = BLOCK_SIZE,
    .numChannels = 1,
    .sampleRate = 44100.0
};

struct sig_AudioSettings* audioSettings;
struct sig_SignalContext* context;

void setUp(void) {
    allocator.impl->init(&allocator);
    audioSettings = sig_AudioSettings_new(&allocator);
    context = sig_SignalContext_new(&allocator, audioSettings);
}

void tearDown(void) {
    sig_AudioSettings_destroy(&allocator, audioSettings);
    sig_SignalContext_destroy(&allocator, context);
}

void test_sig_unipolarToUint12(void) {
    TEST_ASSERT_EQUAL_UINT16(2047, sig_unipolarToUint12(0.5f));
    TEST_ASSERT_EQUAL_UINT16(0, sig_unipolarToUint12(0.0f));
    TEST_ASSERT_EQUAL_UINT16(4095, sig_unipolarToUint12(1.0f));
}

void test_sig_bipolarToUint12(void) {
    TEST_ASSERT_EQUAL_UINT16(4095, sig_bipolarToUint12(1.0f));
    TEST_ASSERT_EQUAL_UINT16(3071, sig_bipolarToUint12(0.5f));
    TEST_ASSERT_EQUAL_UINT16(2047, sig_bipolarToUint12(0.0f));
    TEST_ASSERT_EQUAL_UINT16(1023, sig_bipolarToUint12(-0.5f));
    TEST_ASSERT_EQUAL_UINT16(0, sig_bipolarToUint12(-1.0f));
}

void test_sig_bipolarToInvUint12(void) {
    TEST_ASSERT_EQUAL_UINT16(0, sig_bipolarToInvUint12(1.0f));
    TEST_ASSERT_EQUAL_UINT16(1023, sig_bipolarToInvUint12(0.5f));
    TEST_ASSERT_EQUAL_UINT16(2047, sig_bipolarToInvUint12(0.0f));
    TEST_ASSERT_EQUAL_UINT16(3071, sig_bipolarToInvUint12(-0.5f));
    TEST_ASSERT_EQUAL_UINT16(4095, sig_bipolarToInvUint12(-1.0f));
}

void test_sig_randf(void) {
    size_t numSamples = 8192;

    // Values should be within 0.0 to 1.0
    struct sig_Buffer* firstRun = sig_Buffer_new(&allocator,
        numSamples);
    sig_Buffer_fill(firstRun, sig_randomFill);
    testAssertBufferValuesInRange(firstRun->samples, numSamples,
        0.0f, 1.0f);

    // Consecutive runs should contain different values.
    struct sig_Buffer* secondRun = sig_Buffer_new(&allocator,
        numSamples);
    sig_Buffer_fill(secondRun, sig_randomFill);
    testAssertBuffersNotEqual(firstRun->samples, secondRun->samples,
        numSamples);

    // Using the same seed for each run
    // should produce identical results.
    struct sig_Buffer* thirdRun = sig_Buffer_new(&allocator,
        numSamples);
    struct sig_Buffer* fourthRun = sig_Buffer_new(&allocator,
        numSamples);
    srand(1);
    sig_Buffer_fill(thirdRun, sig_randomFill);

    srand(1);
    sig_Buffer_fill(fourthRun, sig_randomFill);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(thirdRun->samples,
        fourthRun->samples, numSamples);
}

void test_sig_midiToFreq(void) {
    // 69 A 440
    float actual = sig_midiToFreq(69.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(FLOAT_EPSILON, 440.0f, actual,
        "MIDI A4 should be 440 Hz");

    // 60 Middle C 261.63
    actual = sig_midiToFreq(60.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, sig_FREQ_C4, actual,
        "MIDI C3 should be 261.63 Hz");

    // MIDI 0
    actual = sig_midiToFreq(0.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 8.176f, actual,
        "MIDI 0 should be 8.18 Hz");

    // Quarter tone
    actual = sig_midiToFreq(60.5f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 269.29f, actual,
        "A quarter tone above C3 should be 269.29 Hz");

    // Negative MIDI note numbers should return viable frequencies.
    actual = sig_midiToFreq(-60.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 0.2555, actual,
        "A negative MIDI number should return a negative frequency.");
}

void test_sig_freqToMidi(void) {
    // 69 A 440
    float actual = sig_freqToMidi(440.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(FLOAT_EPSILON, 69.0f, actual,
        "MIDI A4 should be 440 Hz");

    // 60 Middle C 261.63
    actual = sig_freqToMidi(sig_FREQ_C4);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005f, 60.0f, actual,
        "MIDI C3 should be 261.63 Hz");

    // MIDI 0
    actual = sig_freqToMidi(8.176f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005f, 0.0f, actual,
        "MIDI 0 should be 8.18 Hz");

    // Quarter tone
    actual = sig_freqToMidi(269.29f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005f, 60.5f, actual,
        "A quarter tone above C3 should be 269.29 Hz");

    // Negative MIDI note numbers should return viable frequencies.
    actual = sig_freqToMidi(0.2555f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005f, -60.0f, actual,
        "A negative MIDI number should return a negative frequency.");
}

void test_sig_sig_linearToFreq(void) {
    // 0.0f - 0V - Middle C4 - 261.626
    float actual = sig_linearToFreq(0.0f, sig_FREQ_C4);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(sig_FREQ_C4, actual,
        "0.0f should be middle C 261.626 Hz");

    // 0.75f - 0.75V - A4 440
    actual = sig_linearToFreq(0.75f, sig_FREQ_C4);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 440.0f, actual,
        "3/4 of a volt should be 440 Hz");

    // 2.5V - F#6 - 1479.98
    actual = sig_linearToFreq(2.5f, sig_FREQ_C4);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 1479.98f, actual,
        "2.5V should be F#5 739.99 Hz");

    // -1.25 - A2 - 110 Hz
    actual = sig_linearToFreq(-1.25f, sig_FREQ_C4);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 110.0f, actual,
        "-1.25V should be A2 110 Hz");

}

void test_sig_sig_freqToLinear(void) {
    // 0.0f - 0V - Middle C4 - 261.626 - 20Vpp
    float actual = sig_freqToLinear(sig_FREQ_C4, sig_FREQ_C4);
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f, actual,
        "0.0f should be middle C 261.626 Hz");

    // 0.75f - 0.75V - A4 440 - 1Vpp
    actual = sig_freqToLinear(440.0f, sig_FREQ_C4);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.001f, 0.75f, actual,
        "3/4 of a volt should be 440 Hz");

    // 2.5V - F#6 - 1479.98 - 10Vpp
    actual = sig_freqToLinear(1479.98f, sig_FREQ_C4);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 2.5f, actual,
        "2.5V should be F#5 739.99 Hz");

    // -1.25 - A2 - 110 Hz
    actual = sig_freqToLinear(110.0f, sig_FREQ_C4);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, -1.25f, actual,
        "-1.25V should be A2 110 Hz");
}

void test_sig_fillWithValue(void) {
    float actual[BLOCK_SIZE];
    float expected[BLOCK_SIZE];
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        expected[i] = 440.4f;
    };

    sig_fillWithValue(actual, BLOCK_SIZE, 440.4f);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, actual, BLOCK_SIZE);
}

void test_sig_fillWithSilence(void) {
    float buffer[16];
    sig_fillWithSilence(buffer, 16);
    testAssertBufferIsSilent(&allocator, buffer, 16);
}

void test_sig_AudioSettings_new() {
    struct sig_AudioSettings* s = sig_AudioSettings_new(&allocator);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(sig_DEFAULT_AUDIOSETTINGS.sampleRate,
        s->sampleRate, "The sample rate should be set to the default.");

    TEST_ASSERT_EQUAL_size_t_MESSAGE(sig_DEFAULT_AUDIOSETTINGS.numChannels,
        s->numChannels, "The channels should be set to the default.");

    TEST_ASSERT_EQUAL_size_t_MESSAGE(sig_DEFAULT_AUDIOSETTINGS.blockSize,
        s->blockSize, "The block size should be set to the default.");

    sig_AudioSettings_destroy(&allocator, s);
}

void test_sig_samplesToSeconds() {
    struct sig_AudioSettings* s = sig_AudioSettings_new(&allocator);

    size_t expected = 48000;
    size_t actual = sig_secondsToSamples(s, 1.0f);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(expected, actual,
        "One second should be the same as the sample rate.");

    expected = 16000;
    actual = sig_secondsToSamples(s, 1.0f/3.0f);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(expected, actual,
        "Even division of sample rate should as expected.");

    // Fractional samples should be rounded: 1/7" = 6857.142857142857143 samps
    expected = 6857;
    actual = sig_secondsToSamples(s, 1.0f/7.0f);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(expected, actual,
        "Fractional samples < 0.5 should be rounded down.");

    // 1/11" = 4363.636363636363757 samps
    expected = 4364;
    actual = sig_secondsToSamples(s, 1.0f/11.0f);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(expected, actual,
        "Fractional samples >= 0.5 should be rounded up.");
}

void test_sig_Audio_Block_newWithValue_testForValue(
    struct sig_Allocator* localAlloc,
    struct sig_AudioSettings* customSettings,
    float value) {
    float* actual = sig_AudioBlock_newWithValue(localAlloc,
        customSettings, value);
    testAssertBufferContainsValueOnly(&allocator, value, actual,
        customSettings->blockSize);
    sig_AudioBlock_destroy(localAlloc, actual);
}

void test_sig_AudioBlock_newWithValue(void) {
    // Note: This "heap" is actually stored here on the stack,
    // so it needs to stay small, or else has to be moved
    // onto the program's heap. It's here now so that this test
    // is more self-contained.
    char customMemory[32768];
    struct sig_AllocatorHeap customHeap = {
        .length = 32768,
        .memory = customMemory
    };
    struct sig_Allocator localAlloc = {
        .impl = &sig_TLSFAllocatorImpl,
        .heap = &customHeap
    };

    localAlloc.impl->init(&localAlloc);

    struct sig_AudioSettings* customSettings =
        sig_AudioSettings_new(&localAlloc);

    test_sig_Audio_Block_newWithValue_testForValue(&localAlloc,
        customSettings, 440.0);
    test_sig_Audio_Block_newWithValue_testForValue(&localAlloc,
        customSettings, 0.0);
    test_sig_Audio_Block_newWithValue_testForValue(&localAlloc,
        customSettings, 1.0);
    test_sig_Audio_Block_newWithValue_testForValue(&localAlloc,
        customSettings, 0.0);

    sig_AudioSettings_destroy(&localAlloc, customSettings);
}

void test_sig_Buffer(void) {
    size_t len = 1024;
    struct sig_Buffer* b = sig_Buffer_new(&allocator, len);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(len, b->length,
        "The buffer should be initialized with the correct length");

    float fillVal = 1.0f;
    sig_Buffer_fillWithValue(b, fillVal);
    testAssertBufferContainsValueOnly(&allocator, fillVal, b->samples,
        b->length);

    sig_Buffer_fillWithSilence(b);
    testAssertBufferContainsValueOnly(&allocator, 0.0f, b->samples, b->length);

    sig_Buffer_destroy(&allocator, b);
}

float fillWithIndices(size_t i, float_array_ptr array) {
    return (float) i;
}

void test_sig_BufferView(void) {
    size_t superLen = 1024;
    size_t viewLen = 256;
    size_t viewStart = 256;

    struct sig_Buffer* b = sig_Buffer_new(&allocator, superLen);
    sig_Buffer_fill(b, fillWithIndices);

    struct sig_Buffer* expectedViewContents =
        sig_Buffer_new(&allocator, viewLen);
    for (size_t i = 0; i < expectedViewContents->length; i++) {
        FLOAT_ARRAY(expectedViewContents->samples)[i] =
            (float) i + viewStart;
    }

    struct sig_Buffer* view = sig_BufferView_new(&allocator,
        b, viewStart, viewLen);

    TEST_ASSERT_EQUAL_size_t_MESSAGE(viewLen, view->length,
        "The BufferView should have the correct length.");

    TEST_ASSERT_EQUAL_FLOAT_ARRAY_MESSAGE(
        expectedViewContents->samples,
        view->samples,
        viewLen,
        "The subarray should contain the correct values.");
}

void test_sig_dsp_Value(void) {
    struct sig_dsp_Value* value = sig_dsp_Value_new(&allocator, context);
    value->parameters.value = 123.45f;

    // Output should contain the value parameter.
    value->signal.generate(value);
    testAssertBufferContainsValueOnly(&allocator,
        123.45f, value->outputs.main, audioSettings->blockSize);

    // Output should contain the updated value parameter.
    value->parameters.value = 1.111f;
    value->signal.generate(value);
    testAssertBufferContainsValueOnly(&allocator,
        1.111f, value->outputs.main, audioSettings->blockSize);

    // The lastSample member should have been updated.
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(FLOAT_EPSILON,
        1.111f, value->lastSample,
        "lastSample should have been updated.");

    // After multiple calls to generate(),
    // the output should continue to contain the value parameter.
    value->signal.generate(value);
    value->signal.generate(value);
    testAssertBufferContainsValueOnly(&allocator,
        1.111f, value->outputs.main, audioSettings->blockSize);

    sig_dsp_Value_destroy(&allocator, value);
}

void test_sig_dsp_ConstantValue(void) {
    struct sig_dsp_ConstantValue* constVal = sig_dsp_ConstantValue_new(
        &allocator, context, 42.0f);

    // Output should contain the value, even prior to being evaluated.
    testAssertBufferContainsValueOnly(&allocator, 42.0f,
        constVal->outputs.main, audioSettings->blockSize);

    // The output should not change.
    constVal->signal.generate(constVal);
    testAssertBufferContainsValueOnly(&allocator, 42.0f,
        constVal->outputs.main, audioSettings->blockSize);
}

void test_sig_dsp_TimedTriggerCounter(void) {
    float halfBlockSecs = (audioSettings->blockSize / 2) /
        audioSettings->sampleRate;

    // A trigger right at the beginning of the input buffer.
    float* source = sig_AudioBlock_newWithValue(&allocator,
        audioSettings, 0.0f);
    source[0] = 1.0;

    struct sig_dsp_TimedTriggerCounter* counter = sig_dsp_TimedTriggerCounter_new(&allocator, context);
    counter->inputs.source = source;
    counter->inputs.duration = sig_AudioBlock_newWithValue(&allocator,
        audioSettings, halfBlockSecs);
    counter->inputs.count = sig_AudioBlock_newWithValue(&allocator,
        audioSettings, 1.0f);

    // The output should contain a single trigger half block size.
    // (i.e. 24 samples after we received the
    // rising edge of the input trigger)
    counter->signal.generate(counter);
    float* expected = sig_AudioBlock_newWithValue(&allocator,
        audioSettings, 0.0f);
    expected[23] = 1.0f;
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, counter->outputs.main,
        audioSettings->blockSize);

    // The first trigger can happen later after the
    // "duration" has elapsed and it will be recognized
    // because it shouldn't start counting until a trigger is
    // received.
    source[0] = 0.0f;
    source[24] = 1.0f;
    counter->signal.generate(counter);
    sig_fillWithSilence(expected, audioSettings->blockSize);
    expected[47] = 1.0f;
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, counter->outputs.main,
        audioSettings->blockSize);

    // When the input contains two triggers in the
    // specified duration, but we're looking for one,
    // no trigger should be output.
    sig_fillWithSilence(source, audioSettings->blockSize);
    source[1] = 1.0f;
    source[20] = 1.0f;
    counter->signal.generate(counter);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(
        context->silence->outputs.main,
        counter->outputs.main,
        audioSettings->blockSize);

    // When we're looking for two triggers and we get two triggers,
    // one trigger should be fired.
    sig_fillWithValue(counter->inputs.count, audioSettings->blockSize,
        2.0f);
    counter->signal.generate(counter);
    sig_fillWithSilence(expected, audioSettings->blockSize);
    expected[24] = 1.0f;
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, counter->outputs.main,
        audioSettings->blockSize);

    // When we're looking for two triggers and
    // the second one comes too late,
    // No triggers should be fired.
    sig_fillWithSilence(source, audioSettings->blockSize);
    source[0] = 1.0f;
    source[25] = 1.0f;
    counter->signal.generate(counter);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(
        context->silence->outputs.main,
        counter->outputs.main,
        audioSettings->blockSize);
}

void test_sig_dsp_Mul(void) {
    struct sig_dsp_BinaryOp* gain = sig_dsp_Mul_new(&allocator, context);
    gain->inputs.left = sig_AudioBlock_newWithValue(&allocator, audioSettings,
        0.5f);
    gain->inputs.right = sig_AudioBlock_newWithValue(&allocator, audioSettings,
        440.0f);

    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(&allocator,
        220.0f, gain->outputs.main, audioSettings->blockSize);

    sig_fillWithValue(gain->inputs.left, audioSettings->blockSize, 0.0f);
    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(&allocator,
        0.0f, gain->outputs.main, audioSettings->blockSize);

    sig_fillWithValue(gain->inputs.left, audioSettings->blockSize, 2.0f);
    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(&allocator,
        880.0f, gain->outputs.main, audioSettings->blockSize);

    sig_fillWithValue(gain->inputs.left, audioSettings->blockSize, -1.0f);
    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(&allocator,
        -440.0f, gain->outputs.main, audioSettings->blockSize);

    sig_AudioBlock_destroy(&allocator, gain->inputs.left);
    sig_AudioBlock_destroy(&allocator, gain->inputs.right);
    sig_dsp_Mul_destroy(&allocator, gain);
}

void createOscInputs(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator* osc,
    float freq, float phaseOffset, float mul, float add) {

    osc->inputs.freq = sig_AudioBlock_newWithValue(allocator,
        osc->signal.audioSettings, freq);
    osc->inputs.phaseOffset = sig_AudioBlock_newWithValue(allocator,
        osc->signal.audioSettings, phaseOffset);
    osc->inputs.mul = sig_AudioBlock_newWithValue(allocator,
        osc->signal.audioSettings, mul);
    osc->inputs.add = sig_AudioBlock_newWithValue(allocator,
        osc->signal.audioSettings, add);
}

void destroyOscInputs(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator_Inputs* sineInputs) {
    allocator->impl->free(allocator, sineInputs->freq);
    allocator->impl->free(allocator, sineInputs->phaseOffset);
    allocator->impl->free(allocator, sineInputs->mul);
    allocator->impl->free(allocator, sineInputs->add);
}

void test_sig_dsp_Sine(void) {
    // Generated from this program:
    /*
        #include <stddef.h>
        #include <math.h>
        #include <stdio.h>

        const float PI = 3.14159265358979323846f;
        const float TWOPI = 2.0f * PI;

        int main(int argc, char *argv[]) {
            float sr = 44100.0f;
            float freq = 440.0f;
            float phaseStep = freq / sr * TWOPI;
            float phase = 0.0f;
            for (size_t i = 0; i < 64; i++) {
                float sample = sinf(phase);
                phase = phase + phaseStep;
                printf("%.8ff, ", sample);
            }
        }
    */
    float expected[BLOCK_SIZE] = {
        0.00000000f, 0.06264833f, 0.12505053f, 0.18696144f,
        0.24813786f, 0.30833939f, 0.36732957f, 0.42487663f,
        0.48075449f, 0.53474361f, 0.58663195f, 0.63621557f,
        0.68329972f, 0.72769940f, 0.76924020f, 0.80775887f,
        0.84310418f, 0.87513721f, 0.90373212f, 0.92877656f,
        0.95017213f, 0.96783477f, 0.98169512f, 0.99169868f,
        0.99780619f, 0.99999368f, 0.99825245f, 0.99258947f,
        0.98302686f, 0.96960229f, 0.95236850f, 0.93139309f,
        0.90675867f, 0.87856185f, 0.84691346f, 0.81193781f,
        0.77377236f, 0.73256701f, 0.68848366f, 0.64169550f,
        0.59238636f, 0.54074991f, 0.48698899f, 0.43131489f,
        0.37394631f, 0.31510863f, 0.25503296f, 0.19395538f,
        0.13211580f, 0.06975718f, 0.00712451f, -0.05553614f,
        -0.11797862f, -0.17995760f, -0.24122958f, -0.30155385f,
        -0.36069342f, -0.41841593f, -0.47449467f, -0.52870923f,
        -0.58084673f, -0.63070220f, -0.67807990f, -0.72279370f
    };
    struct sig_SignalContext* mono441kContext = sig_SignalContext_new(
        &allocator, &mono441kAudioSettings);

    struct sig_dsp_Oscillator* sine = sig_dsp_Sine_new(&allocator,
        mono441kContext);
    createOscInputs(&allocator, sine, 440.0f, 0.0f, 1.0f, 0.0f);

    sine->signal.generate(sine);
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        TEST_ASSERT_FLOAT_WITHIN(0.00001,
            FLOAT_ARRAY(sine->outputs.main)[i],
            FLOAT_ARRAY(expected)[i]);
    }

    destroyOscInputs(&allocator, &sine->inputs);
    sig_dsp_Sine_destroy(&allocator, sine);
    sig_SignalContext_destroy(&allocator, mono441kContext);
}

void test_test_sig_dsp_Sine_isOffset(void) {
    float expected[BLOCK_SIZE] = {
        1.00000000f, 1.06264830f, 1.12505054f, 1.18696141f,
        1.24813783f, 1.30833936f, 1.36732960f, 1.42487669f,
        1.48075449f, 1.53474355f, 1.58663201f, 1.63621557f,
        1.68329978f, 1.72769940f, 1.76924014f, 1.80775881f,
        1.84310412f, 1.87513721f, 1.90373206f, 1.92877650f,
        1.95017219f, 1.96783471f, 1.98169518f, 1.99169874f,
        1.99780619f, 1.99999368f, 1.99825239f, 1.99258947f,
        1.98302686f, 1.96960235f, 1.95236850f, 1.93139315f,
        1.90675867f, 1.87856185f, 1.84691346f, 1.81193781f,
        1.77377236f, 1.73256707f, 1.68848372f, 1.64169550f,
        1.59238636f, 1.54074991f, 1.48698902f, 1.43131495f,
        1.37394631f, 1.31510866f, 1.25503302f, 1.19395542f,
        1.13211584f, 1.06975722f, 1.00712454f, 0.94446385f,
        0.88202137f, 0.82004237f, 0.75877041f, 0.69844615f,
        0.63930655f, 0.58158410f, 0.52550530f, 0.47129077f,
        0.41915327f, 0.36929780f, 0.32192010f, 0.27720630f
    };

    struct sig_SignalContext* mono441kContext = sig_SignalContext_new(
        &allocator, &mono441kAudioSettings);

    struct sig_dsp_Oscillator* sine = sig_dsp_Sine_new(&allocator,
        mono441kContext);
    createOscInputs(&allocator, sine, 440.0f, 0.0f, 1.0f, 1.0f);

    sine->signal.generate(sine);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(
        expected,
        sine->outputs.main,
        sine->signal.audioSettings->blockSize);

    destroyOscInputs(&allocator, &sine->inputs);
    sig_dsp_Sine_destroy(&allocator, sine);
    sig_SignalContext_destroy(&allocator, mono441kContext);
}

void test_sig_dsp_Sine_accumulatesPhase(void) {
    struct sig_dsp_Oscillator* sine = sig_dsp_Sine_new(&allocator,
        context);
    createOscInputs(&allocator, sine, 440.0f, 0.0f, 1.0f, 0.0f);

    // 440 Hz frequency at 48 KHz sample rate.
    float phaseStep = 0.00916667f;

    sine->signal.generate(sine);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
        0.000001,
        phaseStep * 48.0,
        sine->phaseAccumulator,
        "The phase accumulator should have been incremented for each sample in the block."
    );

    sine->signal.generate(sine);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
        0.000001,
        phaseStep * 96.0,
        sine->phaseAccumulator,
        "The phase accumulator should have continued to be incremented when generating a second block."
    );

    destroyOscInputs(&allocator, &sine->inputs);
    sig_dsp_Sine_destroy(&allocator, sine);
}

void test_sig_dsp_Sine_phaseWrapsAt2PI(void) {
    struct sig_dsp_Oscillator* sine = sig_dsp_Sine_new(&allocator,
        context);
    createOscInputs(&allocator, sine, 440.0f, 0.0f, 1.0f, 0.0f);

    sine->signal.generate(sine);
    sine->signal.generate(sine);
    sine->signal.generate(sine);

    TEST_ASSERT_TRUE_MESSAGE(
        sine->phaseAccumulator <= 1.0 &&
        sine->phaseAccumulator >= 0.0,
        "The phase accumulator should wrap around when it is greater than 1.0 (i.e. a normalized equivalent to 2*PI)."
    );

    destroyOscInputs(&allocator, &sine->inputs);
    sig_dsp_Sine_destroy(&allocator, sine);
}

void testDust(struct sig_dsp_Dust* dust,
    float min, float max, int16_t expectedNumDustPerBlock) {
    dust->signal.generate(dust);

    testAssertBufferNotSilent(&allocator, dust->outputs.main,
        audioSettings->blockSize);
    testAssertBufferValuesInRange(dust->outputs.main,
        audioSettings->blockSize, min, max);
    // Signal should generate a number of non-zero samples
    // within 0.5% of the expected number (since it's random).
    testAssertGeneratedSignalContainsApproxNumNonZeroSamples(
        &dust->signal, dust->outputs.main, expectedNumDustPerBlock,
        0.005, 1500);
}

void test_sig_dsp_Dust(void) {
    int32_t expectedNumDustPerBlock = 5;
    float density = (audioSettings->sampleRate /
        audioSettings->blockSize) * expectedNumDustPerBlock;

    struct sig_dsp_Dust* dust = sig_dsp_Dust_new(&allocator, context);
    dust->inputs.density = sig_AudioBlock_newWithValue(&allocator,
        audioSettings, density);

    // Unipolar output.
    testDust(dust, 0.0f, 1.0f, expectedNumDustPerBlock);

    // Bipolar output.
    dust->parameters.bipolar = 1.0f;
    testDust(dust, -1.0f, 1.0f, expectedNumDustPerBlock);

    allocator.impl->free(&allocator, dust->inputs.density);
    sig_dsp_Dust_destroy(&allocator, dust);
}


struct sig_test_BufferPlayer* WaveformPlayer_new(
    sig_waveform_generator waveform,
    float sampleRate, float freq, float duration) {
    struct sig_Buffer* waveformBuffer = sig_Buffer_new(&allocator,
        (size_t) audioSettings->sampleRate * duration);

    sig_Buffer_fillWithWaveform(waveformBuffer, waveform,
        sampleRate, 0.0f, freq);

    return sig_test_BufferPlayer_new(&allocator, context,
        waveformBuffer);
}

void WaveformPlayer_destroy(struct sig_test_BufferPlayer* player) {
    sig_Buffer_destroy(&allocator, player->buffer);
    sig_test_BufferPlayer_destroy(&allocator, player);
}

void testClockDetector(struct sig_test_BufferPlayer* clockPlayer,
    float duration, float expectedFreq) {
    struct sig_dsp_ClockFreqDetector* det = sig_dsp_ClockFreqDetector_new(
        &allocator, context);
    det->inputs.source = clockPlayer->outputs.main;

    struct sig_dsp_Signal* signals[2] = {&clockPlayer->signal, &det->signal};
    evaluateSignals(audioSettings, signals, 2, duration);

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0002,
        expectedFreq,
        FLOAT_ARRAY(det->outputs.main)[0],
        "The clock's frequency should have been detected correctly.");

    sig_dsp_ClockFreqDetector_destroy(&allocator, det);
}

void testClockDetector_SingleWaveform(sig_waveform_generator waveform,
    float bufferDuration, float clockFreq) {
    struct sig_test_BufferPlayer* clockPlayer =
        WaveformPlayer_new(waveform, audioSettings->sampleRate,
            clockFreq, bufferDuration);

    testClockDetector(clockPlayer, bufferDuration, clockFreq);

    WaveformPlayer_destroy(clockPlayer);
}

void test_sig_dsp_ClockFreqDetector_square() {
    // Square wave, 2Hz for two seconds.
    testClockDetector_SingleWaveform(sig_waveform_square, 2.0f, 2.0f);
}

void test_sig_dsp_ClockFreqDetector_sine() {
    // Sine wave, 10 Hz.
    testClockDetector_SingleWaveform(sig_waveform_sine, 2.0f, 10.0f);
}

void test_sig_dsp_ClockFreqDetector_slowDown() {
    float bufferDuration = 2.0f;
    size_t bufferLen = (size_t) audioSettings->sampleRate * bufferDuration;
    size_t halfBufferLen = bufferLen / 2;
    float fastSpeed = 10.0f;
    float slowSpeed = 2.0f;

    struct sig_Buffer* waveformBuffer = sig_Buffer_new(&allocator, bufferLen);

    struct sig_Buffer* fastSection = sig_BufferView_new(&allocator,
        waveformBuffer, 0, halfBufferLen);
    sig_Buffer_fillWithWaveform(fastSection, sig_waveform_square,
        audioSettings->sampleRate, 0.0f, fastSpeed);

    struct sig_Buffer* slowSection = sig_BufferView_new(&allocator,
        waveformBuffer, halfBufferLen, halfBufferLen);
    sig_Buffer_fillWithWaveform(slowSection, sig_waveform_square,
        audioSettings->sampleRate, 0.0f, slowSpeed);

    struct sig_test_BufferPlayer* clockPlayer = sig_test_BufferPlayer_new(
        &allocator, context, waveformBuffer);

    testClockDetector(clockPlayer, bufferDuration, slowSpeed);

    sig_BufferView_destroy(&allocator, fastSection);
    sig_BufferView_destroy(&allocator, slowSection);
    sig_Buffer_destroy(&allocator, waveformBuffer);
    sig_test_BufferPlayer_destroy(&allocator, clockPlayer);
}

void test_sig_dsp_ClockFreqDetector_stop() {
    float bufferDuration = 122.0f;
    float clockDuration = 1.0f;
    size_t bufferLen = (size_t) audioSettings->sampleRate * bufferDuration;
    size_t clockSectionLen = (size_t) audioSettings->sampleRate * clockDuration;
    size_t silentSectionLen = (size_t)
        audioSettings->sampleRate * (bufferDuration - clockDuration);
    float clockFreq = 10.0f;

    struct sig_Buffer* waveformBuffer = sig_Buffer_new(&allocator, bufferLen);

    struct sig_Buffer* clockSection = sig_BufferView_new(&allocator,
        waveformBuffer, 0, clockSectionLen);
    sig_Buffer_fillWithWaveform(clockSection, sig_waveform_square,
        audioSettings->sampleRate, 0.0f, clockFreq);

    struct sig_Buffer* silentSection = sig_BufferView_new(&allocator,
        waveformBuffer, clockSectionLen, silentSectionLen);
    sig_Buffer_fillWithSilence(silentSection);

    struct sig_test_BufferPlayer* clockPlayer = sig_test_BufferPlayer_new(
        &allocator, context, waveformBuffer);

    testClockDetector(clockPlayer, bufferDuration, 0.0f);

    sig_BufferView_destroy(&allocator, silentSection);
    sig_BufferView_destroy(&allocator, clockSection);
    sig_test_BufferPlayer_destroy(&allocator, clockPlayer);
    sig_Buffer_destroy(&allocator, waveformBuffer);
}

void runTimedGate(struct sig_test_BufferPlayer* triggerPlayer,
    struct sig_Buffer* recBuffer, float recDuration, float gateDuration,
    float resetOnTrigger, float bipolar) {
    struct sig_dsp_TimedGate* timedGate = sig_dsp_TimedGate_new(&allocator,
        context);
    timedGate->inputs.duration = sig_AudioBlock_newWithValue(&allocator,
        audioSettings, gateDuration);
    timedGate->inputs.trigger = triggerPlayer->outputs.main;

    timedGate->parameters.resetOnTrigger = resetOnTrigger;
    timedGate->parameters.bipolar = bipolar;

    struct sig_test_BufferRecorder* recorder = sig_test_BufferRecorder_new(
        &allocator, context, recBuffer);
    recorder->inputs.source = timedGate->outputs.main;

    struct sig_dsp_Signal* signals[3] = {
        &triggerPlayer->signal, &timedGate->signal, &recorder->signal};

    evaluateSignals(audioSettings, signals, 3, recDuration);
}


void test_sig_dsp_TimedGate_unipolar(void) {
    float recDuration = 1.0f;
    float recDurationSamples = sig_secondsToSamples(audioSettings,
        recDuration);
    float gateDuration = 0.1f;
    float gateDurationSamples = sig_secondsToSamples(audioSettings,
        gateDuration);

    // One trigger, after 10 samples have elapsed.
    struct sig_Buffer* triggerBuffer = sig_Buffer_new(&allocator,
        recDurationSamples);
    sig_Buffer_fillWithSilence(triggerBuffer);
    FLOAT_ARRAY(triggerBuffer->samples)[10] = 1.0f;
    struct sig_test_BufferPlayer* triggerPlayer = sig_test_BufferPlayer_new(
        &allocator, context, triggerBuffer);

    struct sig_Buffer* recBuffer = sig_Buffer_new(&allocator,
        recDurationSamples);

    runTimedGate(triggerPlayer, recBuffer, recDuration, gateDuration,
        0.0f, 0.0f);

    // First 9 samples, prior to the trigger, should be silent.
    struct sig_Buffer* startSection = sig_BufferView_new(&allocator,
        recBuffer, 0, 10);
    testAssertBufferIsSilent(&allocator,
        startSection->samples, startSection->length);

    // Gate should be high for the specified duration.
    struct sig_Buffer* gateSection = sig_BufferView_new(&allocator,
        recBuffer, 10, gateDurationSamples);
    testAssertBufferContainsValueOnly(&allocator, 1.0,
        gateSection->samples, gateSection->length);

    // All samples after the gate should be silent.
    size_t endSectionIdx = 10 + gateDurationSamples;
    struct sig_Buffer* endSection = sig_BufferView_new(&allocator,
        recBuffer, endSectionIdx, recBuffer->length - endSectionIdx);
    testAssertBufferIsSilent(&allocator, endSection->samples,
        endSection->length);
}

void test_sig_dsp_TimedGate_resetOnTrigger(void) {
    float recDuration = 0.3f;
    size_t recDurationSamples = sig_secondsToSamples(audioSettings,
        recDuration);
    float gateDuration = 0.1f;
    size_t gateDurationSamples = sig_secondsToSamples(audioSettings,
        gateDuration);
    size_t triggerIndices[2] = {0, (size_t) gateDurationSamples / 2};

    struct sig_Buffer* triggerBuffer = sig_Buffer_new(&allocator,
        recDurationSamples);
    sig_Buffer_fillWithSilence(triggerBuffer);
    FLOAT_ARRAY(triggerBuffer->samples)[triggerIndices[0]] = 1.0f;
    FLOAT_ARRAY(triggerBuffer->samples)[triggerIndices[1]] = 1.0f;

    struct sig_test_BufferPlayer* triggerPlayer = sig_test_BufferPlayer_new(
        &allocator, context, triggerBuffer);

    struct sig_Buffer* recBuffer = sig_Buffer_new(&allocator,
        recDurationSamples);

    runTimedGate(triggerPlayer, recBuffer, recDuration, gateDuration,
        1.0f, 0.0f);

    // The gate should open right away.
    // Halfway through the first gate, the signal should drop for one sample
    // and then reopen, staying high for gateDurationSamples.
    struct sig_Buffer* firstGate = sig_BufferView_new(&allocator,
        recBuffer, 0, triggerIndices[1]);

    testAssertBufferContainsValueOnly(&allocator, 1.0,
        firstGate->samples, firstGate->length);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0.0f,
        FLOAT_ARRAY(recBuffer->samples)[triggerIndices[1]],
        "When the second trigger fires, the gate should drop to zero for one sample.");

    // The second gate should be high for the full gate duration.
    size_t secondGateStartIdx = triggerIndices[1] + 1;
    size_t secondGateLen = gateDurationSamples;
    struct sig_Buffer* secondGate = sig_BufferView_new(&allocator,
        recBuffer, secondGateStartIdx, secondGateLen);
    testAssertBufferContainsValueOnly(&allocator, 1.0,
        secondGate->samples, secondGate->length);

    // Afterwards we should have only silence.
    size_t silenceStartIdx = secondGateStartIdx + secondGateLen;
    size_t silenceLength = recBuffer->length - silenceStartIdx;
    struct sig_Buffer* silence = sig_BufferView_new(&allocator,
        recBuffer, silenceStartIdx, silenceLength);
    testAssertBufferIsSilent(&allocator, silence->samples, silence->length);
}

void test_sig_dsp_TimedGate_bipolar(void) {
    float recDuration = 0.3f;
    size_t recDurationSamples = sig_secondsToSamples(audioSettings,
        recDuration);
    float gateDuration = 0.1f;
    size_t gateDurationSamples = sig_secondsToSamples(audioSettings,
        gateDuration);

    struct sig_Buffer* triggerBuffer = sig_Buffer_new(&allocator,
        recDurationSamples);
    sig_Buffer_fillWithSilence(triggerBuffer);
    FLOAT_ARRAY(triggerBuffer->samples)[0] = 0.5f;
    FLOAT_ARRAY(triggerBuffer->samples)[gateDurationSamples] = -0.5f;
    FLOAT_ARRAY(triggerBuffer->samples)[gateDurationSamples * 2] = 0.75f;

    struct sig_test_BufferPlayer* triggerPlayer = sig_test_BufferPlayer_new(
        &allocator, context, triggerBuffer);

    struct sig_Buffer* recBuffer = sig_Buffer_new(&allocator,
        recDurationSamples);

    runTimedGate(triggerPlayer, recBuffer, recDuration, gateDuration,
        0.0f, 1.0f);

    // The gate should open right away and be positive,
    // and match the amplitude of the trigger.
    struct sig_Buffer* firstGate = sig_BufferView_new(&allocator,
        recBuffer, 0, gateDurationSamples);
    testAssertBufferContainsValueOnly(&allocator, 0.5f,
        firstGate->samples, firstGate->length);

    // The second gate should be negative and also match the amplitude
    // of the trigger.
    size_t secondGateStartIdx = gateDurationSamples;
    size_t secondGateLen = gateDurationSamples;
    struct sig_Buffer* secondGate = sig_BufferView_new(&allocator,
        recBuffer, secondGateStartIdx, secondGateLen);
    testAssertBufferContainsValueOnly(&allocator, -0.5f,
        secondGate->samples, secondGate->length);

    // The third gate should also match the gate value.
    size_t thirdGateStartIdx = gateDurationSamples * 2;
    size_t thirdGateLen = gateDurationSamples;
    struct sig_Buffer* thirdGate = sig_BufferView_new(&allocator,
        recBuffer, thirdGateStartIdx, thirdGateLen);
    testAssertBufferContainsValueOnly(&allocator, 0.75f,
        thirdGate->samples, thirdGate->length);
}

void generateAndTestListIndex(struct sig_dsp_Value* idx, float idxValue,
    struct sig_dsp_List* list, float expectedListValue) {
    idx->parameters.value = idxValue;
    idx->signal.generate(idx);
    list->signal.generate(list);
    testAssertBufferContainsValueOnly(&allocator, expectedListValue,
        list->outputs.main, audioSettings->blockSize);
}

void test_sig_dsp_List_wrapping(void) {
    struct sig_dsp_Value* idx = sig_dsp_Value_new(&allocator,
        context);

    float listItems[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    struct sig_Buffer listBuffer = {
        .length = 5,
        .samples = listItems
    };

    struct sig_dsp_List* list = sig_dsp_List_new(&allocator, context);
    list->list = &listBuffer;
    list->inputs.index = idx->outputs.main;

    // No index rounding required.
    generateAndTestListIndex(idx, 0.0f, list, 1.0f);
    testAssertBufferContainsValueOnly(&allocator, 5.0f,
        list->outputs.length, audioSettings->blockSize);

    // Index is normalized between 0.0->1.0f,
    // so there are values at 0.0, 0.25 0.5 0.75, 1.0
    // Fractional indexes less than halfway should be rounded down.
    generateAndTestListIndex(idx, 0.12f, list, 1.0f);

    // Fractional indexes >= half should be rounded up.
    generateAndTestListIndex(idx, 0.125f, list, 2.0f);

    // Another even index.
    generateAndTestListIndex(idx, 0.5f, list, 3.0f);

    // Nearly the last index.
    generateAndTestListIndex(idx, 0.99f, list, 5.0f);

    // Just past the first index.
    generateAndTestListIndex(idx, 0.00001f, list, 1.0f);

    // Exactly the last index.
    generateAndTestListIndex(idx, 1.0f, list, 5.0f);

    // A slightly out of bounds index should round down to the last index.
    generateAndTestListIndex(idx, 1.1f, list, 5.0f);

    // But a larger index should round up and
    // wrap around to return the first value.
    generateAndTestListIndex(idx, 1.25f, list, 1.0f);

    // Larger indices should wrap around past the beginning.
    generateAndTestListIndex(idx, 1.5f, list, 2.0f);

    // Very large indices should wrap around the beginning again.
    generateAndTestListIndex(idx, 3.0f, list, 3.0f);

    // Negative indices should wrap around the end.
    // Note the index "shift" here: -0.25 should point
    // to the last value in the list.
    generateAndTestListIndex(idx, -0.25f, list, 5.0f);

    // But not if they're not negative enough.
    generateAndTestListIndex(idx, -0.12f, list, 1.0f);

    // Very negative indices should keep wrapping around.
    generateAndTestListIndex(idx, -1.5f, list, 5.0f);

    // Small lists should work.
    float small[2] = {1.0f, 2.0f};
    listBuffer.length = 2;
    listBuffer.samples = small;
    generateAndTestListIndex(idx, 0.0f, list, 1.0f);
    generateAndTestListIndex(idx, 1.0f, list, 2.0f);
    generateAndTestListIndex(idx, 0.5f, list, 2.0f);
    generateAndTestListIndex(idx, 0.25f, list, 1.0f);
    generateAndTestListIndex(idx, -1.0f, list, 2.0f);
    generateAndTestListIndex(idx, -0.25f, list, 1.0f);
    generateAndTestListIndex(idx, -0.5f, list, 2.0f);

    // Very small lists should work.
    float verySmall[1] = {1.0f};
    listBuffer.length = 1;
    listBuffer.samples = verySmall;
    generateAndTestListIndex(idx, 1.0f, list, 1.0f);
    generateAndTestListIndex(idx, 0.0f, list, 1.0f);
    generateAndTestListIndex(idx, 0.5f, list, 1.0f);
    generateAndTestListIndex(idx, -0.5f, list, 1.0f);
    generateAndTestListIndex(idx, -1.15f, list, 1.0f);
}

void test_sig_dsp_List_clamping(void) {
    struct sig_dsp_Value* idx = sig_dsp_Value_new(&allocator,
        context);

    float listItems[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    struct sig_Buffer listBuffer = {
        .length = 5,
        .samples = listItems
    };

    struct sig_dsp_List* list = sig_dsp_List_new(&allocator, context);
    list->parameters.wrap = 0.0f;
    list->list = &listBuffer;
    list->inputs.index = idx->outputs.main;

    generateAndTestListIndex(idx, 0.0f, list, 1.0f);
    generateAndTestListIndex(idx, 0.00001f, list, 1.0f);
    generateAndTestListIndex(idx, 0.99f, list, 5.0f);
    generateAndTestListIndex(idx, 1.0f, list, 5.0f);
    generateAndTestListIndex(idx, 1.001f, list, 5.0f);
    generateAndTestListIndex(idx, 1.125f, list, 5.0f);
    generateAndTestListIndex(idx, 1.25f, list, 5.0f);
    generateAndTestListIndex(idx, 3.0f, list, 5.0f);
    generateAndTestListIndex(idx, -0.00001f, list, 1.0f);
    generateAndTestListIndex(idx, -0.99f, list, 1.0f);
    generateAndTestListIndex(idx, -1.5f, list, 1.0f);
}

void test_sig_dsp_List_noList(void) {
    struct sig_dsp_Value* idx = sig_dsp_Value_new(&allocator,
        context);

    float empty[1] = {42.0f};  // Not so empty, because MSVC.
    struct sig_Buffer listBuffer = {
        .length = 0,
        .samples = empty
    };

    struct sig_dsp_List* list = sig_dsp_List_new(&allocator, context);
    list->inputs.index = idx->outputs.main;

    // A NULL list should return silence.
    list->signal.generate(list);
    testAssertBufferIsSilent(&allocator, list->outputs.main,
        audioSettings->blockSize);
    testAssertBufferIsSilent(&allocator, list->outputs.length,
        audioSettings->blockSize);

    // An empty list should return silence.
    list->list = &listBuffer;
    list->signal.generate(list);
    testAssertBufferIsSilent(&allocator, list->outputs.main,
        audioSettings->blockSize);
    testAssertBufferIsSilent(&allocator, list->outputs.length,
        audioSettings->blockSize);

}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sig_unipolarToUint12);
    RUN_TEST(test_sig_bipolarToUint12);
    RUN_TEST(test_sig_bipolarToInvUint12);
    RUN_TEST(test_sig_randf);
    RUN_TEST(test_sig_midiToFreq);
    RUN_TEST(test_sig_freqToMidi);
    RUN_TEST(test_sig_sig_linearToFreq);
    RUN_TEST(test_sig_sig_freqToLinear);
    RUN_TEST(test_sig_fillWithValue);
    RUN_TEST(test_sig_fillWithSilence);
    RUN_TEST(test_sig_AudioSettings_new);
    RUN_TEST(test_sig_samplesToSeconds);
    RUN_TEST(test_sig_AudioBlock_newWithValue);
    RUN_TEST(test_sig_Buffer);
    RUN_TEST(test_sig_BufferView);
    RUN_TEST(test_sig_dsp_Value);
    RUN_TEST(test_sig_dsp_ConstantValue);
    RUN_TEST(test_sig_dsp_TimedTriggerCounter);
    RUN_TEST(test_sig_dsp_Mul);
    RUN_TEST(test_sig_dsp_Sine);
    RUN_TEST(test_sig_dsp_Sine_accumulatesPhase);
    RUN_TEST(test_sig_dsp_Sine_phaseWrapsAt2PI);
    RUN_TEST(test_test_sig_dsp_Sine_isOffset);
    RUN_TEST(test_sig_dsp_Dust);
    RUN_TEST(test_sig_dsp_ClockFreqDetector_square);
    RUN_TEST(test_sig_dsp_ClockFreqDetector_sine);
    RUN_TEST(test_sig_dsp_ClockFreqDetector_slowDown);
    RUN_TEST(test_sig_dsp_ClockFreqDetector_stop);
    RUN_TEST(test_sig_dsp_TimedGate_unipolar);
    RUN_TEST(test_sig_dsp_TimedGate_resetOnTrigger);
    RUN_TEST(test_sig_dsp_TimedGate_bipolar);
    RUN_TEST(test_sig_dsp_List_wrapping);
    RUN_TEST(test_sig_dsp_List_clamping);
    RUN_TEST(test_sig_dsp_List_noList);

    return UNITY_END();
}
