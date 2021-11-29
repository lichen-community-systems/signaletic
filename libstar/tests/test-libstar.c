#include <math.h>
#include <stdlib.h> // TODO: Remove when we have a memory allocator.
#include <unity.h>
#include <libstar.h>

#define FLOAT_EPSILON powf(2, -23)

// TODO: Factor into a test utilities file.
void TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
    float expected, float* buffer, size_t bufferLen) {
    for (size_t i = 0; i < bufferLen; i++) {
        float actual = buffer[i];
        // TODO: Print the current index in the message.
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(FLOAT_EPSILON, expected, actual,
            "Buffer should be filled with expected value.");
    }
}

void TEST_ASSERT_BUFFER_CONTAINS_SILENCE(
    float* buffer, size_t bufferLen) {
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
        0.0f, buffer, bufferLen);
}

void TEST_ASSERT_BUFFER_EQUALS(
    float* expected, float* actual, size_t bufferLen) {
    for (size_t i = 0; i < bufferLen; i++) {
        float expectedSamp = expected[i];
        float actualSamp = actual[i];
        // TODO: Print the current index in the message.
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
            FLOAT_EPSILON, expectedSamp, actualSamp,
            "Buffer sample should contain the expected sample.");
    }
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
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
        440.4f, buffer, blockSize);
}

void test_star_Buffer_fillSilence(void) {
    size_t blockSize = 16;
    float buffer[blockSize];
    star_Buffer_fillSilence(buffer, blockSize);
    TEST_ASSERT_BUFFER_CONTAINS_SILENCE(buffer, blockSize);
}

void test_star_sig_Value(void) {
    struct star_AudioSettings audioSettings = star_DEFAULT_AUDIOSETTINGS;
    float output[audioSettings.blockSize];
    struct star_sig_Value value;
    star_sig_Value_init(&value, &audioSettings, output);
    value.parameters.value = 123.45f;

    // Output should contain the value parameter.
    value.signal.generate(&value);
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
        123.45f, output, audioSettings.blockSize);

    // Output should contain the updated value parameter.
    value.parameters.value = 1.111f;
    value.signal.generate(&value);
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
        1.111f, output, audioSettings.blockSize);

    // The lastSample member should have been updated.
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(FLOAT_EPSILON,
        1.111f, value.lastSample,
        "lastSample should have been updated.");

    // After multiple calls to generate(),
    // the output should continue to contain the value parameter.
    value.signal.generate(&value);
    value.signal.generate(&value);
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
        1.111f, output, audioSettings.blockSize);
}

void test_star_sig_Gain(void) {
    struct star_AudioSettings audioSettings = star_DEFAULT_AUDIOSETTINGS;
    float output[audioSettings.blockSize];

    float gainBuffer[audioSettings.blockSize];
    star_Buffer_fill(0.5f, gainBuffer, audioSettings.blockSize);
    float sourceBuffer[audioSettings.blockSize];
    star_Buffer_fill(440.0f, sourceBuffer, audioSettings.blockSize);

    struct star_sig_Gain_Inputs inputs = {
        .gain = gainBuffer,
        .source = sourceBuffer
    };

    struct star_sig_Gain gain;
    star_sig_Gain_init(&gain, &audioSettings, &inputs, output);
    star_sig_Gain_generate(&gain);
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
        220.0f, output, audioSettings.blockSize);

    star_Buffer_fill(0.0f, gainBuffer, audioSettings.blockSize);
    star_sig_Gain_generate(&gain);
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
        0.0f, output, audioSettings.blockSize);

    star_Buffer_fill(2.0f, gainBuffer, audioSettings.blockSize);
    star_sig_Gain_generate(&gain);
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
        880.0f, output, audioSettings.blockSize);

    star_Buffer_fill(-1.0f, gainBuffer, audioSettings.blockSize);
    star_sig_Gain_generate(&gain);
    TEST_ASSERT_BUFFER_CONTAINS_FLOAT_WITHIN(
        -440.0f, output, audioSettings.blockSize);
}

float* createBuffer(struct star_AudioSettings* audioSettings) {
    return malloc(sizeof(float) * audioSettings->blockSize);
}

float* createFilledBuffer(
    float value, struct star_AudioSettings* audioSettings) {
    float* buffer = createBuffer(audioSettings);
    star_Buffer_fill(value, buffer, audioSettings->blockSize);

    return buffer;
}

// TODO: Move into libstar itself (when we have a memory allocator).
struct star_sig_Sine* createSine(
    struct star_AudioSettings* audioSettings,
    float freq, float phaseOffset, float mul, float add) {

    struct star_sig_Sine_Inputs* inputs = (struct star_sig_Sine_Inputs*) malloc(sizeof(struct star_sig_Sine_Inputs));

    inputs->freq = createFilledBuffer(freq, audioSettings);
    inputs->phaseOffset = createFilledBuffer(phaseOffset, audioSettings);
    inputs->mul = createFilledBuffer(mul, audioSettings);
    inputs->add = createFilledBuffer(add, audioSettings);

    float* output = createFilledBuffer(0.0f, audioSettings);

    struct star_sig_Sine* sine = (struct star_sig_Sine*) malloc(sizeof(struct star_sig_Sine));
    star_sig_Sine_init(sine, audioSettings, inputs, output);

    return sine;
}

// TODO: Move into libstar itself (when we have a memory allocator).
void destroySine(struct star_sig_Sine* sine) {
    free(sine->inputs->freq);
    free(sine->inputs->phaseOffset);
    free(sine->inputs->mul);
    free(sine->inputs->add);
    free(sine->signal.output);
    free(sine);
}

void test_star_sig_Sine(void) {
    float expected[64] = {
        0.0,0.06264832615852355957031250,0.12505052983760833740234375,0.18696144223213195800781250,0.24813786149024963378906250,0.30833938717842102050781250,0.36732956767082214355468750,0.42487663030624389648437500,0.48075449466705322265625000,0.53474360704421997070312500,0.58663195371627807617187500,0.63621556758880615234375000,0.68329972028732299804687500,0.72769939899444580078125000,0.76924020051956176757812500,0.80775886774063110351562500,0.84310418367385864257812500,0.87513720989227294921875000,0.90373212099075317382812500,0.92877656221389770507812500,0.95017212629318237304687500,0.96783477067947387695312500,0.98169511556625366210937500,0.99169868230819702148437500,0.99780619144439697265625000,0.99999368190765380859375000,0.99825245141983032226562500,0.99258947372436523437500000,0.98302686214447021484375000,0.96960228681564331054687500,0.95236849784851074218750000,0.93139308691024780273437500,0.90675866603851318359375000,0.87856185436248779296875000,0.84691345691680908203125000,0.81193780899047851562500000,0.77377235889434814453125000,0.73256701231002807617187500,0.68848365545272827148437500,0.64169549942016601562500000,0.59238636493682861328125000,0.54074990749359130859375000,0.48698899149894714355468750,0.43131488561630249023437500,0.37394630908966064453125000,0.31510862708091735839843750,0.25503295660018920898437500,0.19395537674427032470703125,0.13211579620838165283203125,0.06975717842578887939453125,0.00712451478466391563415527,-0.05553614348173141479492188,-0.11797861754894256591796875,-0.17995759844779968261718750,-0.24122957885265350341796875,-0.30155384540557861328125000,-0.36069342494010925292968750,-0.41841593384742736816406250,-0.47449466586112976074218750,-0.52870923280715942382812500,-0.58084672689437866210937500,-0.63070219755172729492187500,-0.67807990312576293945312500,-0.72279369831085205078125000
    };

    struct star_AudioSettings audioSettings = {
        .blockSize = 64,
        .numChannels = 1,
        .sampleRate = 44100.0
    };

    struct star_sig_Sine* sine = createSine(
        &audioSettings, 440.0f, 0.0f, 1.0f, 0.0f);

    star_sig_Sine_generate(sine);
    TEST_ASSERT_BUFFER_EQUALS(
        expected,
        sine->signal.output,
        sine->signal.audioSettings->blockSize);
    destroySine(sine);
}

void test_test_star_sig_Sine_isOffset(void) {
    float expected[64] = {
        1.0,1.06264829635620117187500000,1.12505054473876953125000000,1.18696141242980957031250000,1.24813783168792724609375000,1.30833935737609863281250000,1.36732959747314453125000000,1.42487668991088867187500000,1.48075449466705322265625000,1.53474354743957519531250000,1.58663201332092285156250000,1.63621556758880615234375000,1.68329977989196777343750000,1.72769939899444580078125000,1.76924014091491699218750000,1.80775880813598632812500000,1.84310412406921386718750000,1.87513720989227294921875000,1.90373206138610839843750000,1.92877650260925292968750000,1.95017218589782714843750000,1.96783471107482910156250000,1.98169517517089843750000000,1.99169874191284179687500000,1.99780619144439697265625000,1.99999368190765380859375000,1.99825239181518554687500000,1.99258947372436523437500000,1.98302686214447021484375000,1.96960234642028808593750000,1.95236849784851074218750000,1.93139314651489257812500000,1.90675866603851318359375000,1.87856185436248779296875000,1.84691345691680908203125000,1.81193780899047851562500000,1.77377235889434814453125000,1.73256707191467285156250000,1.68848371505737304687500000,1.64169549942016601562500000,1.59238636493682861328125000,1.54074990749359130859375000,1.48698902130126953125000000,1.43131494522094726562500000,1.37394630908966064453125000,1.31510865688323974609375000,1.25503301620483398437500000,1.19395542144775390625000000,1.13211584091186523437500000,1.06975722312927246093750000,1.00712454319000244140625000,0.94446384906768798828125000,0.88202136754989624023437500,0.82004237174987792968750000,0.75877040624618530273437500,0.69844615459442138671875000,0.63930654525756835937500000,0.58158409595489501953125000,0.52550530433654785156250000,0.47129076719284057617187500,0.41915327310562133789062500,0.36929780244827270507812500,0.32192009687423706054687500,0.27720630168914794921875000
    };

    struct star_AudioSettings audioSettings = {
        .blockSize = 64,
        .numChannels = 1,
        .sampleRate = 44100.0f
    };

    struct star_sig_Sine* sine = createSine(
        &audioSettings, 440.0f, 0.0f, 1.0f, 1.0f);

    star_sig_Sine_generate(sine);
    TEST_ASSERT_BUFFER_EQUALS(
        expected,
        sine->signal.output,
        sine->signal.audioSettings->blockSize);
    destroySine(sine);
}

void test_star_sig_Sine_accumulatesPhase(void) {
    struct star_AudioSettings audioSettings = star_DEFAULT_AUDIOSETTINGS;

    struct star_sig_Sine* sine = createSine(
        &audioSettings, 440.0f, 0.0f, 1.0f, 0.0f
    );

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

    destroySine(sine);
}

void test_star_sig_Sine_phaseWrapsAt2PI(void) {
    struct star_AudioSettings audioSettings = star_DEFAULT_AUDIOSETTINGS;
    struct star_sig_Sine* sine = createSine(
        &audioSettings, 440.0f, 0.0f, 1.0f, 0.0f
    );

    star_sig_Sine_generate(sine);
    star_sig_Sine_generate(sine);
    star_sig_Sine_generate(sine);

    TEST_ASSERT_TRUE_MESSAGE(
        sine->phaseAccumulator <= star_TWOPI &&
        sine->phaseAccumulator >= 0.0,
        "The phase accumulator should wrap around when it is greater than 2*PI."
    );

    destroySine(sine);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_star_midiToFreq);
    RUN_TEST(test_star_Buffer_fill);
    RUN_TEST(test_star_Buffer_fillSilence);
    RUN_TEST(test_star_sig_Value);
    RUN_TEST(test_star_sig_Gain);
    RUN_TEST(test_star_sig_Sine);
    RUN_TEST(test_star_sig_Sine_accumulatesPhase);
    RUN_TEST(test_star_sig_Sine_phaseWrapsAt2PI);
    RUN_TEST(test_test_star_sig_Sine_isOffset);
    return UNITY_END();
}
