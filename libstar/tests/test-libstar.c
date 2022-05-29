#include <math.h>
#include <unity.h>
#include <tlsf.h>
#include <libstar.h>
#include <stdlib.h> // TODO: Remove when a custom RNG is implemented.

#define FLOAT_EPSILON powf(2, -23)
#define HEAP_SIZE 1048576 // 1 MB
#define BLOCK_SIZE 64

char heap[HEAP_SIZE];
struct star_Allocator allocator = {
    .heapSize = HEAP_SIZE,
    .heap = (void*) heap
};

struct star_AudioSettings* audioSettings;
float* silentBlock;

// TODO: Factor into a test utilities file.
void testAssertBufferContainsValueOnly(
    float expectedValue, float* actual, size_t len) {
    float* expectedArray = (float*) star_Allocator_malloc(&allocator,
        len * sizeof(float));
    star_fillWithValue(expectedArray, len, expectedValue);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expectedArray, actual, len);
    star_Allocator_free(&allocator, expectedArray);
}

void testAssertBuffersNotEqual(float* first, float* second,
    size_t len) {
    size_t numMatches = 0;

    for (size_t i = 0; i < len; i++) {
        if (first[i] == second[i]) {
            numMatches++;
        }
    }

    TEST_ASSERT_FALSE_MESSAGE(numMatches == len,
        "All values in both buffers were identical.");
}

void testAssertBuffersNoValuesEqual(float* first, float* second,
    size_t len) {
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

void testAssertBufferIsSilent(
    float* buffer, size_t len) {
    testAssertBufferContainsValueOnly(0.0f, buffer, len);
}

void testAssertBufferNotSilent(float* buffer, size_t len) {
    float* silence = star_Allocator_malloc(&allocator,
        sizeof(float) * len);
    star_fillWithSilence(silence, len);
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

int16_t countNonZeroSamplesGenerated(struct star_sig_Signal* signal,
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
    struct star_sig_Signal* signal, int16_t expectedNumNonZero,
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

void setUp(void) {
    star_Allocator_init(&allocator);
    audioSettings = star_AudioSettings_new(&allocator);
    silentBlock = star_AudioBlock_newWithValue(&allocator,
        audioSettings, 0.0f);
}

void tearDown(void) {}

void test_star_unipolarToUint12(void) {
    TEST_ASSERT_EQUAL_UINT16(2047, star_unipolarToUint12(0.5f));
    TEST_ASSERT_EQUAL_UINT16(0, star_unipolarToUint12(0.0f));
    TEST_ASSERT_EQUAL_UINT16(4095, star_unipolarToUint12(1.0f));
}

void test_star_bipolarToUint12(void) {
    TEST_ASSERT_EQUAL_UINT16(4095, star_bipolarToUint12(1.0f));
    TEST_ASSERT_EQUAL_UINT16(3071, star_bipolarToUint12(0.5f));
    TEST_ASSERT_EQUAL_UINT16(2047, star_bipolarToUint12(0.0f));
    TEST_ASSERT_EQUAL_UINT16(1023, star_bipolarToUint12(-0.5f));
    TEST_ASSERT_EQUAL_UINT16(0, star_bipolarToUint12(-1.0f));
}

void test_star_bipolarToInvUint12(void) {
    TEST_ASSERT_EQUAL_UINT16(0, star_bipolarToInvUint12(1.0f));
    TEST_ASSERT_EQUAL_UINT16(1023, star_bipolarToInvUint12(0.5f));
    TEST_ASSERT_EQUAL_UINT16(2047, star_bipolarToInvUint12(0.0f));
    TEST_ASSERT_EQUAL_UINT16(3071, star_bipolarToInvUint12(-0.5f));
    TEST_ASSERT_EQUAL_UINT16(4095, star_bipolarToInvUint12(-1.0f));
}

void fillBufferRandom(float* buffer, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buffer[i] = star_randf();
    }
}

void test_star_randf(void) {
    size_t numSamples = 8192;

    // Values should be within 0.0 to 1.0
    struct star_Buffer* firstRun = star_Buffer_new(&allocator,
        numSamples);
    fillBufferRandom(firstRun->samples, numSamples);
    testAssertBufferValuesInRange(firstRun->samples, numSamples,
        0.0f, 1.0f);

    // Consecutive runs should contain different values.
    struct star_Buffer* secondRun = star_Buffer_new(&allocator,
        numSamples);
    fillBufferRandom(secondRun->samples, numSamples);
    testAssertBuffersNotEqual(firstRun->samples, secondRun->samples,
        numSamples);

    // Using the same seed for each run
    // should produce identical results.
    struct star_Buffer* thirdRun = star_Buffer_new(&allocator,
        numSamples);
    struct star_Buffer* fourthRun = star_Buffer_new(&allocator,
        numSamples);
    srand(1);
    fillBufferRandom(thirdRun->samples, numSamples);
    srand(1);
    fillBufferRandom(fourthRun->samples, numSamples);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(thirdRun->samples,
        fourthRun->samples, numSamples);
}

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

void test_star_fillWithValue(void) {
    float actual[BLOCK_SIZE];
    float expected[BLOCK_SIZE];
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        expected[i] = 440.4f;
    };

    star_fillWithValue(actual, BLOCK_SIZE, 440.4f);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, actual, BLOCK_SIZE);
}

void test_star_fillWithSilence(void) {
    float buffer[16];
    star_fillWithSilence(buffer, 16);
    testAssertBufferIsSilent(buffer, 16);
}

void test_star_AudioSettings_new() {
    struct star_AudioSettings* s = star_AudioSettings_new(&allocator);

    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(star_DEFAULT_AUDIOSETTINGS.sampleRate,
        s->sampleRate, "The sample rate should be set to the default.");

    TEST_ASSERT_EQUAL_size_t_MESSAGE(star_DEFAULT_AUDIOSETTINGS.numChannels,
        s->numChannels, "The channels should be set to the default.");

    TEST_ASSERT_EQUAL_size_t_MESSAGE(star_DEFAULT_AUDIOSETTINGS.blockSize,
        s->blockSize, "The block size should be set to the default.");

    star_AudioSettings_destroy(&allocator, s);
}

void test_star_Audio_Block_newWithValue_testForValue(
    struct star_Allocator* alloc,
    struct star_AudioSettings* settings,
    float value) {
    float* actual = star_AudioBlock_newWithValue(alloc,
        settings, value);
    testAssertBufferContainsValueOnly(value, actual,
        settings->blockSize);
    star_Allocator_free(alloc, actual);
}

void test_star_AudioBlock_newWithValue(void) {
    char customHeap[262144];
    struct star_Allocator customAlloc = {
        .heap = (void*) customHeap,
        .heapSize = 262144
    };
    star_Allocator_init(&customAlloc);

    struct star_AudioSettings* customSettings =
        star_AudioSettings_new(&customAlloc);

    test_star_Audio_Block_newWithValue_testForValue(&customAlloc,
        customSettings, 440.0);
    test_star_Audio_Block_newWithValue_testForValue(&customAlloc,
        customSettings, 0.0);
    test_star_Audio_Block_newWithValue_testForValue(&customAlloc,
        customSettings, 1.0);
    test_star_Audio_Block_newWithValue_testForValue(&customAlloc,
        customSettings, 0.0);
}

void test_star_Buffer(void) {
    size_t len = 1024;
    struct star_Buffer* b = star_Buffer_new(&allocator, len);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(len, b->length,
        "The buffer should be initialized with the correct length");

    float fillVal = 1.0f;
    star_Buffer_fill(b, fillVal);
    testAssertBufferContainsValueOnly(fillVal, b->samples, b->length);

    star_Buffer_fillWithSilence(b);
    testAssertBufferContainsValueOnly(0.0f, b->samples, b->length);

    star_Buffer_destroy(&allocator, b);
}

void test_star_sig_Value(void) {
    struct star_sig_Value* value = star_sig_Value_new(&allocator,
        audioSettings);
    value->parameters.value = 123.45f;

    // Output should contain the value parameter.
    value->signal.generate(value);
    testAssertBufferContainsValueOnly(
        123.45f, value->signal.output, audioSettings->blockSize);

    // Output should contain the updated value parameter.
    value->parameters.value = 1.111f;
    value->signal.generate(value);
    testAssertBufferContainsValueOnly(
        1.111f, value->signal.output, audioSettings->blockSize);

    // The lastSample member should have been updated.
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(FLOAT_EPSILON,
        1.111f, value->lastSample,
        "lastSample should have been updated.");

    // After multiple calls to generate(),
    // the output should continue to contain the value parameter.
    value->signal.generate(value);
    value->signal.generate(value);
    testAssertBufferContainsValueOnly(
        1.111f, value->signal.output, audioSettings->blockSize);

    star_sig_Value_destroy(&allocator, value);
}

void test_star_sig_TimedTriggerCounter(void) {
    float halfBlockSecs = (audioSettings->blockSize / 2) /
        audioSettings->sampleRate;

    // A trigger right at the beginning of the input buffer.
    float* source = star_AudioBlock_newWithValue(&allocator,
        audioSettings, 0.0f);
    source[0] = 1.0;

    struct star_sig_TimedTriggerCounter_Inputs inputs = {
        .source = source,
        .duration = star_AudioBlock_newWithValue(&allocator,
            audioSettings, halfBlockSecs),
        .count = star_AudioBlock_newWithValue(&allocator,
            audioSettings, 1.0f)
    };

    struct star_sig_TimedTriggerCounter* counter = star_sig_TimedTriggerCounter_new(&allocator, audioSettings,
        &inputs);

    // The output should contain a single trigger half block size.
    // (i.e. 24 samples after we received the
    // rising edge of the input trigger)
    counter->signal.generate(counter);
    float* expected = star_AudioBlock_newWithValue(&allocator,
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
    star_fillWithSilence(expected, audioSettings->blockSize);
    expected[47] = 1.0f;
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, counter->signal.output,
        audioSettings->blockSize);

    // When the input contains two triggers in the
    // specified duration, but we're looking for one,
    // no trigger should be output.
    star_fillWithSilence(source, audioSettings->blockSize);
    source[1] = 1.0f;
    source[20] = 1.0f;
    counter->signal.generate(counter);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(silentBlock, counter->signal.output,
        audioSettings->blockSize);

    // When we're looking for two triggers and we get two triggers,
    // one trigger should be fired.
    star_fillWithValue(counter->inputs->count, audioSettings->blockSize,
        2.0f);
    counter->signal.generate(counter);
    star_fillWithSilence(expected, audioSettings->blockSize);
    expected[24] = 1.0f;
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(expected, counter->signal.output,
        audioSettings->blockSize);

    // When we're looking for two triggers and
    // the second one comes too late,
    // No triggers should be fired.
    star_fillWithSilence(source, audioSettings->blockSize);
    source[0] = 1.0f;
    source[25] = 1.0f;
    counter->signal.generate(counter);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(silentBlock, counter->signal.output,
        audioSettings->blockSize);
}

void test_star_sig_Mul(void) {
    struct star_sig_BinaryOp_Inputs inputs = {
        .left = star_AudioBlock_newWithValue(&allocator,
            audioSettings, 0.5f),
        .right = star_AudioBlock_newWithValue(&allocator,
            audioSettings, 440.0f)
    };

    struct star_sig_BinaryOp* gain = star_sig_Mul_new(&allocator,
        audioSettings, &inputs);

    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(
        220.0f, gain->signal.output, audioSettings->blockSize);

    star_fillWithValue(inputs.left, audioSettings->blockSize, 0.0f);
    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(
        0.0f, gain->signal.output, audioSettings->blockSize);

    star_fillWithValue(inputs.left, audioSettings->blockSize, 2.0f);
    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(
        880.0f, gain->signal.output, audioSettings->blockSize);

    star_fillWithValue(inputs.left, audioSettings->blockSize, -1.0f);
    gain->signal.generate(gain);
    testAssertBufferContainsValueOnly(
        -440.0f, gain->signal.output, audioSettings->blockSize);

    star_sig_Mul_destroy(&allocator, gain);
    star_Allocator_free(&allocator, inputs.left);
    star_Allocator_free(&allocator, inputs.right);
}

// TODO: Move into libstar itself
// as a (semi?) generic input instantiator.
struct star_sig_Sine_Inputs* createSineInputs(struct star_Allocator* allocator,
    struct star_AudioSettings* audioSettings,
    float freq, float phaseOffset, float mul, float add) {

    struct star_sig_Sine_Inputs* inputs = (struct star_sig_Sine_Inputs*) star_Allocator_malloc(allocator,
        sizeof(struct star_sig_Sine_Inputs));

    inputs->freq = star_AudioBlock_newWithValue(allocator,
        audioSettings, freq);
    inputs->phaseOffset = star_AudioBlock_newWithValue(allocator,
        audioSettings, phaseOffset);
    inputs->mul = star_AudioBlock_newWithValue(allocator,
        audioSettings, mul);
    inputs->add = star_AudioBlock_newWithValue(allocator,
        audioSettings, add);

    return inputs;
}

// TODO: Move into libstar itself
// as a (semi?) generic input instantiator.
void destroySineInputs(struct star_Allocator* allocator,
    struct star_sig_Sine_Inputs* sineInputs) {
    star_Allocator_free(allocator, sineInputs->freq);
    star_Allocator_free(allocator, sineInputs->phaseOffset);
    star_Allocator_free(allocator, sineInputs->mul);
    star_Allocator_free(allocator, sineInputs->add);
    star_Allocator_free(allocator, sineInputs);
}

void test_star_sig_Sine(void) {
    float expected[BLOCK_SIZE] = {
        0.0,0.06264832615852355957031250,0.12505052983760833740234375,0.18696144223213195800781250,0.24813786149024963378906250,0.30833938717842102050781250,0.36732956767082214355468750,0.42487663030624389648437500,0.48075449466705322265625000,0.53474360704421997070312500,0.58663195371627807617187500,0.63621556758880615234375000,0.68329972028732299804687500,0.72769939899444580078125000,0.76924020051956176757812500,0.80775886774063110351562500,0.84310418367385864257812500,0.87513720989227294921875000,0.90373212099075317382812500,0.92877656221389770507812500,0.95017212629318237304687500,0.96783477067947387695312500,0.98169511556625366210937500,0.99169868230819702148437500,0.99780619144439697265625000,0.99999368190765380859375000,0.99825245141983032226562500,0.99258947372436523437500000,0.98302686214447021484375000,0.96960228681564331054687500,0.95236849784851074218750000,0.93139308691024780273437500,0.90675866603851318359375000,0.87856185436248779296875000,0.84691345691680908203125000,0.81193780899047851562500000,0.77377235889434814453125000,0.73256701231002807617187500,0.68848365545272827148437500,0.64169549942016601562500000,0.59238636493682861328125000,0.54074990749359130859375000,0.48698899149894714355468750,0.43131488561630249023437500,0.37394630908966064453125000,0.31510862708091735839843750,0.25503295660018920898437500,0.19395537674427032470703125,0.13211579620838165283203125,0.06975717842578887939453125,0.00712451478466391563415527,-0.05553614348173141479492188,-0.11797861754894256591796875,-0.17995759844779968261718750,-0.24122957885265350341796875,-0.30155384540557861328125000,-0.36069342494010925292968750,-0.41841593384742736816406250,-0.47449466586112976074218750,-0.52870923280715942382812500,-0.58084672689437866210937500,-0.63070219755172729492187500,-0.67807990312576293945312500,-0.72279369831085205078125000
    };

    struct star_AudioSettings audioSettings = {
        .blockSize = BLOCK_SIZE,
        .numChannels = 1,
        .sampleRate = 44100.0
    };

    struct star_sig_Sine_Inputs* inputs = createSineInputs(
        &allocator, &audioSettings, 440.0f, 0.0f, 1.0f, 0.0f);
    struct star_sig_Sine* sine = star_sig_Sine_new(&allocator,
        &audioSettings, inputs);

    star_sig_Sine_generate(sine);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(
        expected,
        sine->signal.output,
        sine->signal.audioSettings->blockSize);

    destroySineInputs(&allocator, inputs);
    star_sig_Sine_destroy(&allocator, sine);
}

void test_test_star_sig_Sine_isOffset(void) {
    float expected[BLOCK_SIZE] = {
        1.0,1.06264829635620117187500000,1.12505054473876953125000000,1.18696141242980957031250000,1.24813783168792724609375000,1.30833935737609863281250000,1.36732959747314453125000000,1.42487668991088867187500000,1.48075449466705322265625000,1.53474354743957519531250000,1.58663201332092285156250000,1.63621556758880615234375000,1.68329977989196777343750000,1.72769939899444580078125000,1.76924014091491699218750000,1.80775880813598632812500000,1.84310412406921386718750000,1.87513720989227294921875000,1.90373206138610839843750000,1.92877650260925292968750000,1.95017218589782714843750000,1.96783471107482910156250000,1.98169517517089843750000000,1.99169874191284179687500000,1.99780619144439697265625000,1.99999368190765380859375000,1.99825239181518554687500000,1.99258947372436523437500000,1.98302686214447021484375000,1.96960234642028808593750000,1.95236849784851074218750000,1.93139314651489257812500000,1.90675866603851318359375000,1.87856185436248779296875000,1.84691345691680908203125000,1.81193780899047851562500000,1.77377235889434814453125000,1.73256707191467285156250000,1.68848371505737304687500000,1.64169549942016601562500000,1.59238636493682861328125000,1.54074990749359130859375000,1.48698902130126953125000000,1.43131494522094726562500000,1.37394630908966064453125000,1.31510865688323974609375000,1.25503301620483398437500000,1.19395542144775390625000000,1.13211584091186523437500000,1.06975722312927246093750000,1.00712454319000244140625000,0.94446384906768798828125000,0.88202136754989624023437500,0.82004237174987792968750000,0.75877040624618530273437500,0.69844615459442138671875000,0.63930654525756835937500000,0.58158409595489501953125000,0.52550530433654785156250000,0.47129076719284057617187500,0.41915327310562133789062500,0.36929780244827270507812500,0.32192009687423706054687500,0.27720630168914794921875000
    };

    struct star_AudioSettings audioSettings = {
        .blockSize = BLOCK_SIZE,
        .numChannels = 1,
        .sampleRate = 44100.0f
    };

    struct star_sig_Sine_Inputs* inputs = createSineInputs(
        &allocator, &audioSettings, 440.0f, 0.0f, 1.0f, 1.0f);
    struct star_sig_Sine* sine = star_sig_Sine_new(&allocator,
        &audioSettings, inputs);

    star_sig_Sine_generate(sine);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(
        expected,
        sine->signal.output,
        sine->signal.audioSettings->blockSize);

    destroySineInputs(&allocator, inputs);
    star_sig_Sine_destroy(&allocator, sine);
}

void test_star_sig_Sine_accumulatesPhase(void) {
    struct star_sig_Sine_Inputs* inputs = createSineInputs(
        &allocator, audioSettings, 440.0f, 0.0f, 1.0f, 0.0f);
    struct star_sig_Sine* sine = star_sig_Sine_new(&allocator,
        audioSettings, inputs);

    // 440 Hz frequency at 48 KHz sample rate.
    float phaseStep = 0.05759586393833160400390625f;

    star_sig_Sine_generate(sine);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
        0.000001,
        phaseStep * 48.0,
        sine->phaseAccumulator,
        "The phase accumulator should have been incremented for each sample in the block."
    );

    star_sig_Sine_generate(sine);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
        0.000001,
        phaseStep * 96.0,
        sine->phaseAccumulator,
        "The phase accumulator should have continued to be incremented when generating a second block."
    );

    destroySineInputs(&allocator, inputs);
    star_sig_Sine_destroy(&allocator, sine);
}

void test_star_sig_Sine_phaseWrapsAt2PI(void) {
    struct star_sig_Sine_Inputs* inputs = createSineInputs(
        &allocator, audioSettings, 440.0f, 0.0f, 1.0f, 0.0f);
    struct star_sig_Sine* sine = star_sig_Sine_new(&allocator,
        audioSettings, inputs);

    star_sig_Sine_generate(sine);
    star_sig_Sine_generate(sine);
    star_sig_Sine_generate(sine);

    TEST_ASSERT_TRUE_MESSAGE(
        sine->phaseAccumulator <= star_TWOPI &&
        sine->phaseAccumulator >= 0.0,
        "The phase accumulator should wrap around when it is greater than 2*PI."
    );

    destroySineInputs(&allocator, inputs);
    star_sig_Sine_destroy(&allocator, sine);
}

void testDust(struct star_sig_Dust* dust,
    float min, float max, int16_t expectedNumDustPerBlock) {
    dust->signal.generate(dust);

    testAssertBufferNotSilent(dust->signal.output,
        audioSettings->blockSize);
    testAssertBufferValuesInRange(dust->signal.output,
        audioSettings->blockSize, min, max);
    // Signal should generate a number of non-zero samples
    // within 5% of the expected number (since it's random).
    testAssertGeneratedSignalContainsApproxNumNonZeroSamples(
        &dust->signal, expectedNumDustPerBlock, 0.005, 1500);
}

void test_star_sig_Dust(void) {
    int32_t expectedNumDustPerBlock = 5;
    float density = (audioSettings->sampleRate /
        audioSettings->blockSize) * expectedNumDustPerBlock;

    struct star_sig_Dust_Inputs inputs = {
        .density = star_AudioBlock_newWithValue(&allocator,
            audioSettings, density)
    };

    struct star_sig_Dust* dust = star_sig_Dust_new(&allocator,
        audioSettings, &inputs);

    // Unipolar output.
    testDust(dust, 0.0f, 1.0f, expectedNumDustPerBlock);

    // Bipolar output.
    dust->parameters.bipolar = 1.0f;
    testDust(dust, -1.0f, 1.0f, expectedNumDustPerBlock);

    star_Allocator_free(&allocator, inputs.density);
    star_sig_Dust_destroy(&allocator, dust);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_star_randf);
    RUN_TEST(test_star_unipolarToUint12);
    RUN_TEST(test_star_bipolarToUint12);
    RUN_TEST(test_star_bipolarToInvUint12);
    RUN_TEST(test_star_midiToFreq);
    RUN_TEST(test_star_fillWithValue);
    RUN_TEST(test_star_fillWithSilence);
    RUN_TEST(test_star_AudioSettings_new);
    RUN_TEST(test_star_AudioBlock_newWithValue);
    RUN_TEST(test_star_Buffer);
    RUN_TEST(test_star_sig_Value);
    RUN_TEST(test_star_sig_TimedTriggerCounter);
    RUN_TEST(test_star_sig_Mul);
    RUN_TEST(test_star_sig_Sine);
    RUN_TEST(test_star_sig_Sine_accumulatesPhase);
    RUN_TEST(test_star_sig_Sine_phaseWrapsAt2PI);
    RUN_TEST(test_test_star_sig_Sine_isOffset);
    RUN_TEST(test_star_sig_Dust);

    return UNITY_END();
}
