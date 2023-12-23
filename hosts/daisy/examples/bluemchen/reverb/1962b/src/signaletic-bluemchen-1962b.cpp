#include <string>
#include <tlsf.h>
#include <libsignaletic.h>
#include "../../../../include/daisy-bluemchen-host.h"

using namespace kxmx;
using namespace daisy;

FixedCapStr<20> displayStr;

#define MAX_DELAY_LINE_LENGTH 48000 // 1 second.
#define DELAY_LINE_HEAP_SIZE 63*1024*1024 // Grab nearly the whole SDRAM.
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

uint8_t DSY_SDRAM_BSS delayLineMemory[DELAY_LINE_HEAP_SIZE];
struct sig_AllocatorHeap delayLineHeap = {
    .length = DELAY_LINE_HEAP_SIZE,
    .memory = (void *) delayLineMemory
};
struct sig_Allocator delayLineAllocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &delayLineHeap
};

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;

Bluemchen bluemchen;
struct sig_daisy_Host* host;

struct sig_daisy_FilteredCVIn* gKnob;
struct sig_daisy_FilteredCVIn* delayTimeKnob;
struct sig_daisy_AudioIn* audioIn;
struct sig_DelayLine* dl1;
struct sig_DelayLine* dl2;
struct sig_DelayLine* dl3;
struct sig_DelayLine* dl4;
struct sig_DelayLine* dl5;
struct sig_DelayLine* outerDL;
struct sig_dsp_Schroeder* reverb;
struct sig_daisy_AudioOut* leftOut;
struct sig_daisy_AudioOut* rightOut;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("1962b");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Delay ");
    displayStr.AppendFloat(delayTimeKnob->outputs.main[0], 2);
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("g ");
    displayStr.AppendFloat(gKnob->outputs.main[0], 2);
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    bluemchen.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    delayTimeKnob = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, delayTimeKnob, status);
    delayTimeKnob->parameters.control = sig_daisy_Bluemchen_CV_IN_KNOB_1;
    // Lots of smoothing to help with the pitch shift that occurs
    // when modulating a delay line.
    delayTimeKnob->parameters.time = 0.5f;

    gKnob = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, gKnob, status);
    gKnob->parameters.control = sig_daisy_Bluemchen_CV_IN_KNOB_2;        gKnob->parameters.scale = 0.9f;
    gKnob->parameters.time = 0.1f;

    audioIn = sig_daisy_AudioIn_new(&allocator, context, host);
    sig_List_append(&signals, audioIn, status);
    audioIn->parameters.channel = sig_daisy_AUDIO_IN_1;

    dl1 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    dl2 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    dl3 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    dl4 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    dl5 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    outerDL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);

    // Allpass parameters are from Shroeder,
    // Natural Sounding Artificial Reverberation, 1961.
    // delayTime = 30 ms
    // g = 0.893
    reverb = sig_dsp_Schroeder_new(&allocator, context);
    sig_List_append(&signals, reverb, status);
    reverb->reverberatorDelayLines[0] = dl1;
    reverb->reverberatorDelayLines[1] = dl2;
    reverb->reverberatorDelayLines[2] = dl3;
    reverb->reverberatorDelayLines[3] = dl4;
    reverb->reverberatorDelayLines[4] = dl5;
    reverb->outerDelayLine = outerDL;
    reverb->inputs.source = audioIn->outputs.main;
    reverb->inputs.delayTime = delayTimeKnob->outputs.main;
    reverb->inputs.g = gKnob->outputs.main;

    leftOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_daisy_AUDIO_OUT_1;
    leftOut->inputs.source = reverb->outputs.main;

    rightOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_daisy_AUDIO_OUT_2;
    rightOut->inputs.source = reverb->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 48000,
        .numChannels = 2,
        .blockSize = 48
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host = sig_daisy_BluemchenHost_new(&allocator,
        &audioSettings,
        &bluemchen,
        (struct sig_dsp_SignalEvaluator*) evaluator);
    sig_daisy_Host_registerGlobalHost(host);

    // Can't write to memory in SDRAM until after the board has
    // been initialized.
    delayLineAllocator.impl->init(&delayLineAllocator);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(context, &status);
    host->impl->start(host);

    while (1) {
        UpdateOled();
    }
}
