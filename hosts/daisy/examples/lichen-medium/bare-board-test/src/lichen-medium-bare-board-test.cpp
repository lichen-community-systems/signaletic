#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../include/lichen-medium-module.h"

#define SAMPLERATE 48000
#define HEAP_SIZE 1024 * 384 // 384 KB
#define MAX_NUM_SIGNALS 32

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;

lichen::medium::MediumDevice medium;
struct sig_dsp_Value* freq;
struct sig_dsp_Value* gain;
struct sig_dsp_Oscillator* sine;
struct sig_dsp_Value* switchValue;
struct sig_dsp_ScaleOffset* switchValueScale;
struct sig_dsp_BinaryOp* harmonizerFreqScale;
struct sig_dsp_Oscillator* harmonizer;
struct sig_dsp_Value* buttonValue;
struct sig_dsp_BinaryOp* mixer;
struct sig_dsp_BinaryOp* attenuator;

void buildSignalGraph(struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_List* signals,
    struct sig_AudioSettings* audioSettings,
    struct sig_Status* status) {

    freq = sig_dsp_Value_new(allocator, context);
    freq->parameters.value = 220.0f;
    sig_List_append(signals, freq, status);

    buttonValue = sig_dsp_Value_new(allocator, context);
    sig_List_append(signals, buttonValue, status);
    buttonValue->parameters.value = 0.0f;

    switchValue = sig_dsp_Value_new(allocator, context);
    sig_List_append(signals, switchValue, status);
    switchValue->parameters.value = 0.0f;

    switchValueScale = sig_dsp_ScaleOffset_new(allocator, context);
    sig_List_append(signals, switchValueScale, status);
    switchValueScale->inputs.source = switchValue->outputs.main;
    switchValueScale->parameters.scale = 1.25f;
    switchValueScale->parameters.offset = 0.75f;

    harmonizerFreqScale = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, harmonizerFreqScale, status);
    harmonizerFreqScale->inputs.left = freq->outputs.main;
    harmonizerFreqScale->inputs.right = switchValueScale->outputs.main;

    harmonizer = sig_dsp_LFTriangle_new(allocator, context);
    sig_List_append(signals, harmonizer, status);
    harmonizer->inputs.freq = harmonizerFreqScale->outputs.main;
    harmonizer->inputs.mul = buttonValue->outputs.main;

    sine = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, sine, status);
    sine->inputs.freq = freq->outputs.main;

    mixer = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, mixer, status);
    mixer->inputs.left = sine->outputs.main;
    mixer->inputs.right = harmonizer->outputs.main;

    gain = sig_dsp_Value_new(allocator, context);
    gain->parameters.value = 0.5f;
    sig_List_append(signals, gain, status);

    attenuator = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, attenuator, status);
    attenuator->inputs.left = mixer->outputs.main;
    attenuator->inputs.right = gain->outputs.main;
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    medium.Read();

    freq->parameters.value = 1760.0f *
        (medium.adcController.channelBank.values[0] +
        medium.adcController.channelBank.values[6]);
    buttonValue->parameters.value = medium.buttonBank.values[0];
    switchValue->parameters.value = medium.switchBank.values[0];

    evaluator->evaluate((struct sig_dsp_SignalEvaluator*) evaluator);

    for (size_t i = 0; i < size; i++) {
        float sig = attenuator->outputs.main[i];
        out[0][i] = sig;
        out[1][i] = sig;

        medium.dacOutputBank.values[0] = medium.gateBank.values[0] * 0.5f;
        medium.Write();
    }
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 1
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    medium.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(&allocator, context, &signals, &audioSettings, &status);

    medium.Start(AudioCallback);

    while (1) {

    }
}
