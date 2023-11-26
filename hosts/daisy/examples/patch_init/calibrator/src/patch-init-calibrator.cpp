#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../include/daisy-patch-sm-host.h"

#define HEAP_SIZE 1024 * 256 // 256KB
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

daisy::patch_sm::DaisyPatchSM patchInit;

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;
struct sig_daisy_Host* host;

struct sig_dsp_ConstantValue* ampScale;
struct sig_daisy_SwitchIn* button;
struct sig_daisy_CVIn* cv1In;
struct sig_dsp_Calibrator* cv1Calibrator;
struct sig_dsp_Oscillator* sine1;
struct sig_dsp_LinearToFreq* sine1Voct;
struct sig_daisy_AudioOut* audio1Out;
struct sig_daisy_AudioOut* audio2Out;


void buildGraph(struct sig_SignalContext* context, struct sig_Status* status) {
    button = sig_daisy_SwitchIn_new(&allocator, context, host);
    sig_List_append(&signals, button, status);
    button->parameters.control = sig_daisy_PatchSM_SWITCH_1;

    cv1In = sig_daisy_CVIn_new(&allocator, context, host);
    sig_List_append(&signals, cv1In, status);
    cv1In->parameters.control = sig_daisy_PatchInit_CV_IN_1;
    cv1In->parameters.scale = 5.0f;

    cv1Calibrator = sig_dsp_Calibrator_new(&allocator, context);
    sig_List_append(&signals, cv1Calibrator, status);
    cv1Calibrator->inputs.source = cv1In->outputs.main;
    cv1Calibrator->inputs.gate = button->outputs.main;

    ampScale = sig_dsp_ConstantValue_new(&allocator, context, 0.5f);

    sine1Voct = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, sine1Voct, status);
    sine1Voct->inputs.source = cv1Calibrator->outputs.main;

    sine1 = sig_dsp_Sine_new(&allocator, context);
    sig_List_append(&signals, sine1, status);
    sine1->inputs.freq = sine1Voct->outputs.main;
    sine1->inputs.mul = ampScale->outputs.main;

    audio1Out = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, audio1Out, status);
    audio1Out->parameters.channel = sig_daisy_AUDIO_OUT_1;
    audio1Out->inputs.source = sine1->outputs.main;

    audio2Out = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, audio2Out, status);
    audio2Out->parameters.channel = sig_daisy_AUDIO_OUT_2;
    audio2Out->inputs.source = sine1->outputs.main;

}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 48000,
        .numChannels = 2,
        .blockSize = 48
    };

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host = sig_daisy_PatchSMHost_new(&allocator,
        &audioSettings, &patchInit,
        (struct sig_dsp_SignalEvaluator*) evaluator);
    sig_daisy_Host_registerGlobalHost(host);
    buildGraph(context, &status);

    host->impl->start(host);

    while (1) {}
}
