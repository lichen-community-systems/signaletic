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

struct sig_AudioSettings* audioSettings;
float* silentBlock;

void setUp(void) {
    allocator.impl->init(&allocator);
    audioSettings = sig_AudioSettings_new(&allocator);
    silentBlock = sig_AudioBlock_newWithValue(&allocator,
        audioSettings, 0.0f);
}

void tearDown(void) {}

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
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 261.63f, actual,
        "MIDI C3 should be 261.63 Hz");

    // MIDI 0
    actual = sig_midiToFreq(0.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 8.18f, actual,
        "MIDI 0 should be 8.18 Hz");

    // Quarter tone
    actual = sig_midiToFreq(60.5f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 269.29f, actual,
        "A quarter tone above C3 should be 269.29 Hz");

    // Negative MIDI note numbers should return viable frequencies.
    actual = sig_midiToFreq(-60.0f);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.005, 0.255, actual,
        "A negative MIDI number should return a negative frequency.");
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
    struct sig_AudioSettings* settings,
    float value) {
    float* actual = sig_AudioBlock_newWithValue(localAlloc,
        settings, value);
    testAssertBufferContainsValueOnly(&allocator, value, actual,
        settings->blockSize);
    localAlloc->impl->free(localAlloc, actual);
}

void test_sig_AudioBlock_newWithValue(void) {
    char customMemory[262144];
    struct sig_AllocatorHeap customHeap = {
        .length = 262144,
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
    struct sig_dsp_Value* value = sig_dsp_Value_new(&allocator,
        audioSettings);
    value->parameters.value = 123.45f;

    // Output should contain the value parameter.
    value->signal.generate(value);
    testAssertBufferContainsValueOnly(&allocator,
        123.45f, value->signal.output, audioSettings->blockSize);

    // Output should contain the updated value parameter.
    value->parameters.value = 1.111f;
    value->signal.generate(value);
    testAssertBufferContainsValueOnly(&allocator,
        1.111f, value->signal.output, audioSettings->blockSize);

    // The lastSample member should have been updated.
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(FLOAT_EPSILON,
        1.111f, value->lastSample,
        "lastSample should have been updated.");

    // After multiple calls to generate(),
    // the output should continue to contain the value parameter.
    value->signal.generate(value);
    value->signal.generate(value);
    testAssertBufferContainsValueOnly(&allocator,
        1.111f, value->signal.output, audioSettings->blockSize);

    sig_dsp_Value_destroy(&allocator, value);
}

void test_sig_dsp_TimedTriggerCounter(void) {
    float halfBlockSecs = (audioSettings->blockSize / 2) /
        audioSettings->sampleRate;

    // A trigger right at the beginning of the input buffer.
    float* source = sig_AudioBlock_newWithValue(&allocator,
        audioSettings, 0.0f);
    source[0] = 1.0;

    struct sig_dsp_TimedTriggerCounter_Inputs inputs = {
        .source = source,
        .duration = sig_AudioBlock_newWithValue(&allocator,
            audioSettings, halfBlockSecs),
        .count = sig_AudioBlock_newWithValue(&allocator,
            audioSettings, 1.0f)
    };

    struct sig_dsp_TimedTriggerCounter* counter = sig_dsp_TimedTriggerCounter_new(&allocator, audioSettings,
        &inputs);

    // The output should contain a single trigger half block size.
    // (i.e. 24 samples after we received the
    // rising edge of the input trigger)
    counter->signal.generate(counter);
    float* expected = sig_AudioBlock_newWithValue(&allocator,
        audioSettings, 0.0f);
    expected[23] = 1.0f;
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, counter->signal.output,
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
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, counter->signal.output,
        audioSettings->blockSize);

    // When the input contains two triggers in the
    // specified duration, but we're looking for one,
    // no trigger should be output.
    sig_fillWithSilence(source, audioSettings->blockSize);
    source[1] = 1.0f;
    source[20] = 1.0f;
    counter->signal.generate(counter);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(silentBlock, counter->signal.output,
        audioSettings->blockSize);

    // When we're looking for two triggers and we get two triggers,
    // one trigger should be fired.
    sig_fillWithValue(counter->inputs->count, audioSettings->blockSize,
        2.0f);
    counter->signal.generate(counter);
    sig_fillWithSilence(expected, audioSettings->blockSize);
    expected[24] = 1.0f;
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, counter->signal.output,
        audioSettings->blockSize);

    // When we're looking for two triggers and
    // the second one comes too late,
    // No triggers should be fired.
    sig_fillWithSilence(source, audioSettings->blockSize);
    source[0] = 1.0f;
    source[25] = 1.0f;
    counter->signal.generate(counter);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(silentBlock, counter->signal.output,
        audioSettings->blockSize);
}

void test_sig_dsp_Mul(void) {
    struct sig_dsp_BinaryOp_Inputs inputs = {
        .left = sig_AudioBlock_newWithValue(&allocator,
            audioSettings, 0.5f),
        .right = sig_AudioBlock_newWithValue(&allocator,
            audioSettings, 440.0f)
    };

    struct sig_dsp_BinaryOp* gain = sig_dsp_Mul_new(&allocator,
        audioSettings, &inputs);

    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(&allocator,
        220.0f, gain->signal.output, audioSettings->blockSize);

    sig_fillWithValue(inputs.left, audioSettings->blockSize, 0.0f);
    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(&allocator,
        0.0f, gain->signal.output, audioSettings->blockSize);

    sig_fillWithValue(inputs.left, audioSettings->blockSize, 2.0f);
    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(&allocator,
        880.0f, gain->signal.output, audioSettings->blockSize);

    sig_fillWithValue(inputs.left, audioSettings->blockSize, -1.0f);
    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(&allocator,
        -440.0f, gain->signal.output, audioSettings->blockSize);

    sig_dsp_Mul_destroy(&allocator, gain);
    allocator.impl->free(&allocator, inputs.left);
    allocator.impl->free(&allocator, inputs.right);
}

// TODO: Move into libsignaletic itself
// as a (semi?) generic input instantiator.
struct sig_dsp_Oscillator_Inputs* createSineInputs(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    float freq, float phaseOffset, float mul, float add) {

    struct sig_dsp_Oscillator_Inputs* inputs = (struct sig_dsp_Oscillator_Inputs*) allocator->impl->malloc(allocator,
        sizeof(struct sig_dsp_Oscillator_Inputs));

    inputs->freq = sig_AudioBlock_newWithValue(allocator,
        audioSettings, freq);
    inputs->phaseOffset = sig_AudioBlock_newWithValue(allocator,
        audioSettings, phaseOffset);
    inputs->mul = sig_AudioBlock_newWithValue(allocator,
        audioSettings, mul);
    inputs->add = sig_AudioBlock_newWithValue(allocator,
        audioSettings, add);

    return inputs;
}

// TODO: Move into libsignaletic itself
// as a (semi?) generic input instantiator.
void destroySineInputs(struct sig_Allocator* allocator,
    struct sig_dsp_Oscillator_Inputs* sineInputs) {
    allocator->impl->free(allocator, sineInputs->freq);
    allocator->impl->free(allocator, sineInputs->phaseOffset);
    allocator->impl->free(allocator, sineInputs->mul);
    allocator->impl->free(allocator, sineInputs->add);
    allocator->impl->free(allocator, sineInputs);
}

void test_sig_dsp_Sine(void) {
    float expected[BLOCK_SIZE] = {
        0.0,0.06264832615852355957031250,0.12505052983760833740234375,0.18696144223213195800781250,0.24813786149024963378906250,0.30833938717842102050781250,0.36732956767082214355468750,0.42487663030624389648437500,0.48075449466705322265625000,0.53474360704421997070312500,0.58663195371627807617187500,0.63621556758880615234375000,0.68329972028732299804687500,0.72769939899444580078125000,0.76924020051956176757812500,0.80775886774063110351562500,0.84310418367385864257812500,0.87513720989227294921875000,0.90373212099075317382812500,0.92877656221389770507812500,0.95017212629318237304687500,0.96783477067947387695312500,0.98169511556625366210937500,0.99169868230819702148437500,0.99780619144439697265625000,0.99999368190765380859375000,0.99825245141983032226562500,0.99258947372436523437500000,0.98302686214447021484375000,0.96960228681564331054687500,0.95236849784851074218750000,0.93139308691024780273437500,0.90675866603851318359375000,0.87856185436248779296875000,0.84691345691680908203125000,0.81193780899047851562500000,0.77377235889434814453125000,0.73256701231002807617187500,0.68848365545272827148437500,0.64169549942016601562500000,0.59238636493682861328125000,0.54074990749359130859375000,0.48698899149894714355468750,0.43131488561630249023437500,0.37394630908966064453125000,0.31510862708091735839843750,0.25503295660018920898437500,0.19395537674427032470703125,0.13211579620838165283203125,0.06975717842578887939453125,0.00712451478466391563415527,-0.05553614348173141479492188,-0.11797861754894256591796875,-0.17995759844779968261718750,-0.24122957885265350341796875,-0.30155384540557861328125000,-0.36069342494010925292968750,-0.41841593384742736816406250,-0.47449466586112976074218750,-0.52870923280715942382812500,-0.58084672689437866210937500,-0.63070219755172729492187500,-0.67807990312576293945312500,-0.72279369831085205078125000
    };

    struct sig_AudioSettings audioSettings = {
        .blockSize = BLOCK_SIZE,
        .numChannels = 1,
        .sampleRate = 44100.0
    };

    struct sig_dsp_Oscillator_Inputs* inputs = createSineInputs(
        &allocator, &audioSettings, 440.0f, 0.0f, 1.0f, 0.0f);
    struct sig_dsp_Sine* sine = sig_dsp_Sine_new(&allocator,
        &audioSettings, inputs);

    sig_dsp_Sine_generate(sine);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(
        expected,
        sine->signal.output,
        sine->signal.audioSettings->blockSize);

    destroySineInputs(&allocator, inputs);
    sig_dsp_Sine_destroy(&allocator, sine);
}

void test_test_sig_dsp_Sine_isOffset(void) {
    float expected[BLOCK_SIZE] = {
        1.0,1.06264829635620117187500000,1.12505054473876953125000000,1.18696141242980957031250000,1.24813783168792724609375000,1.30833935737609863281250000,1.36732959747314453125000000,1.42487668991088867187500000,1.48075449466705322265625000,1.53474354743957519531250000,1.58663201332092285156250000,1.63621556758880615234375000,1.68329977989196777343750000,1.72769939899444580078125000,1.76924014091491699218750000,1.80775880813598632812500000,1.84310412406921386718750000,1.87513720989227294921875000,1.90373206138610839843750000,1.92877650260925292968750000,1.95017218589782714843750000,1.96783471107482910156250000,1.98169517517089843750000000,1.99169874191284179687500000,1.99780619144439697265625000,1.99999368190765380859375000,1.99825239181518554687500000,1.99258947372436523437500000,1.98302686214447021484375000,1.96960234642028808593750000,1.95236849784851074218750000,1.93139314651489257812500000,1.90675866603851318359375000,1.87856185436248779296875000,1.84691345691680908203125000,1.81193780899047851562500000,1.77377235889434814453125000,1.73256707191467285156250000,1.68848371505737304687500000,1.64169549942016601562500000,1.59238636493682861328125000,1.54074990749359130859375000,1.48698902130126953125000000,1.43131494522094726562500000,1.37394630908966064453125000,1.31510865688323974609375000,1.25503301620483398437500000,1.19395542144775390625000000,1.13211584091186523437500000,1.06975722312927246093750000,1.00712454319000244140625000,0.94446384906768798828125000,0.88202136754989624023437500,0.82004237174987792968750000,0.75877040624618530273437500,0.69844615459442138671875000,0.63930654525756835937500000,0.58158409595489501953125000,0.52550530433654785156250000,0.47129076719284057617187500,0.41915327310562133789062500,0.36929780244827270507812500,0.32192009687423706054687500,0.27720630168914794921875000
    };

    struct sig_AudioSettings audioSettings = {
        .blockSize = BLOCK_SIZE,
        .numChannels = 1,
        .sampleRate = 44100.0f
    };

    struct sig_dsp_Oscillator_Inputs* inputs = createSineInputs(
        &allocator, &audioSettings, 440.0f, 0.0f, 1.0f, 1.0f);
    struct sig_dsp_Sine* sine = sig_dsp_Sine_new(&allocator,
        &audioSettings, inputs);

    sig_dsp_Sine_generate(sine);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(
        expected,
        sine->signal.output,
        sine->signal.audioSettings->blockSize);

    destroySineInputs(&allocator, inputs);
    sig_dsp_Sine_destroy(&allocator, sine);
}

void test_sig_dsp_Sine_accumulatesPhase(void) {
    struct sig_dsp_Oscillator_Inputs* inputs = createSineInputs(
        &allocator, audioSettings, 440.0f, 0.0f, 1.0f, 0.0f);
    struct sig_dsp_Sine* sine = sig_dsp_Sine_new(&allocator,
        audioSettings, inputs);

    // 440 Hz frequency at 48 KHz sample rate.
    float phaseStep = 0.05759586393833160400390625f;

    sig_dsp_Sine_generate(sine);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
        0.000001,
        phaseStep * 48.0,
        sine->phaseAccumulator,
        "The phase accumulator should have been incremented for each sample in the block."
    );

    sig_dsp_Sine_generate(sine);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
        0.000001,
        phaseStep * 96.0,
        sine->phaseAccumulator,
        "The phase accumulator should have continued to be incremented when generating a second block."
    );

    destroySineInputs(&allocator, inputs);
    sig_dsp_Sine_destroy(&allocator, sine);
}

void test_sig_dsp_Sine_phaseWrapsAt2PI(void) {
    struct sig_dsp_Oscillator_Inputs* inputs = createSineInputs(
        &allocator, audioSettings, 440.0f, 0.0f, 1.0f, 0.0f);
    struct sig_dsp_Sine* sine = sig_dsp_Sine_new(&allocator,
        audioSettings, inputs);

    sig_dsp_Sine_generate(sine);
    sig_dsp_Sine_generate(sine);
    sig_dsp_Sine_generate(sine);

    TEST_ASSERT_TRUE_MESSAGE(
        sine->phaseAccumulator <= sig_TWOPI &&
        sine->phaseAccumulator >= 0.0,
        "The phase accumulator should wrap around when it is greater than 2*PI."
    );

    destroySineInputs(&allocator, inputs);
    sig_dsp_Sine_destroy(&allocator, sine);
}

void testDust(struct sig_dsp_Dust* dust,
    float min, float max, int16_t expectedNumDustPerBlock) {
    dust->signal.generate(dust);

    testAssertBufferNotSilent(&allocator, dust->signal.output,
        audioSettings->blockSize);
    testAssertBufferValuesInRange(dust->signal.output,
        audioSettings->blockSize, min, max);
    // Signal should generate a number of non-zero samples
    // within 5% of the expected number (since it's random).
    testAssertGeneratedSignalContainsApproxNumNonZeroSamples(
        &dust->signal, expectedNumDustPerBlock, 0.005, 1500);
}

void test_sig_dsp_Dust(void) {
    int32_t expectedNumDustPerBlock = 5;
    float density = (audioSettings->sampleRate /
        audioSettings->blockSize) * expectedNumDustPerBlock;

    struct sig_dsp_Dust_Inputs inputs = {
        .density = sig_AudioBlock_newWithValue(&allocator,
            audioSettings, density)
    };

    struct sig_dsp_Dust* dust = sig_dsp_Dust_new(&allocator,
        audioSettings, &inputs);

    // Unipolar output.
    testDust(dust, 0.0f, 1.0f, expectedNumDustPerBlock);

    // Bipolar output.
    dust->parameters.bipolar = 1.0f;
    testDust(dust, -1.0f, 1.0f, expectedNumDustPerBlock);

    allocator.impl->free(&allocator, inputs.density);
    sig_dsp_Dust_destroy(&allocator, dust);
}


struct sig_test_BufferPlayer* WaveformPlayer_new(
    sig_waveform_generator waveform,
    float sampleRate, float freq, float duration) {
    struct sig_Buffer* waveformBuffer = sig_Buffer_new(&allocator,
        (size_t) audioSettings->sampleRate * duration);

    sig_Buffer_fillWithWaveform(waveformBuffer, waveform,
        sampleRate, 0.0f, freq);

    return sig_test_BufferPlayer_new(&allocator, audioSettings,
        waveformBuffer);
}

void WaveformPlayer_destroy(struct sig_test_BufferPlayer* player) {
    sig_Buffer_destroy(&allocator, player->buffer);
    sig_test_BufferPlayer_destroy(&allocator, player);
}

void testClockDetector(struct sig_test_BufferPlayer* clockPlayer,
    float duration, float expectedFreq) {
    struct sig_dsp_ClockFreqDetector_Inputs inputs = {
        .source = clockPlayer->signal.output
    };

    struct sig_dsp_ClockFreqDetector* det = sig_dsp_ClockFreqDetector_new(
        &allocator, audioSettings, &inputs);

    struct sig_dsp_Signal* signals[2] = {&clockPlayer->signal, &det->signal};
    evaluateSignals(audioSettings, signals, 2, duration);

    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.0002,
        expectedFreq,
        FLOAT_ARRAY(det->signal.output)[0],
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
        &allocator, audioSettings, waveformBuffer);

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
        &allocator, audioSettings, waveformBuffer);

    testClockDetector(clockPlayer, bufferDuration, 0.0f);

    sig_BufferView_destroy(&allocator, silentSection);
    sig_BufferView_destroy(&allocator, clockSection);
    sig_test_BufferPlayer_destroy(&allocator, clockPlayer);
    sig_Buffer_destroy(&allocator, waveformBuffer);
}

void runTimedGate(struct sig_test_BufferPlayer* triggerPlayer,
    struct sig_Buffer* recBuffer, float recDuration, float gateDuration,
    float resetOnTrigger, float bipolar) {
    struct sig_dsp_TimedGate_Inputs gateInputs = {
        .duration = sig_AudioBlock_newWithValue(&allocator,
            audioSettings, gateDuration),
        .trigger = triggerPlayer->signal.output
    };

    struct sig_dsp_TimedGate* timedGate = sig_dsp_TimedGate_new(
        &allocator, audioSettings, &gateInputs);
    timedGate->parameters.resetOnTrigger = resetOnTrigger;
    timedGate->parameters.bipolar = bipolar;

    struct sig_test_BufferRecorder_Inputs recInputs = {
        .source = timedGate->signal.output
    };

    struct sig_test_BufferRecorder* recorder = sig_test_BufferRecorder_new(
        &allocator, audioSettings, &recInputs, recBuffer);

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
        &allocator, audioSettings, triggerBuffer);

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
        &allocator, audioSettings, triggerBuffer);

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
        &allocator, audioSettings, triggerBuffer);

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

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sig_unipolarToUint12);
    RUN_TEST(test_sig_bipolarToUint12);
    RUN_TEST(test_sig_bipolarToInvUint12);
    RUN_TEST(test_sig_randf);
    RUN_TEST(test_sig_midiToFreq);
    RUN_TEST(test_sig_fillWithValue);
    RUN_TEST(test_sig_fillWithSilence);
    RUN_TEST(test_sig_AudioSettings_new);
    RUN_TEST(test_sig_samplesToSeconds);
    RUN_TEST(test_sig_AudioBlock_newWithValue);
    RUN_TEST(test_sig_Buffer);
    RUN_TEST(test_sig_BufferView);
    RUN_TEST(test_sig_dsp_Value);
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

    return UNITY_END();
}
