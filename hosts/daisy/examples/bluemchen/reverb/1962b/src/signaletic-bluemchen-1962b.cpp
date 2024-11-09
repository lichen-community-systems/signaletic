/*
 Manfred Schroeder's second reverb topology in
 "Natural Sounding Artificial Reverberation" (1962),
 which nests five all pass filters into an additional
 allpass-style delay line and mixes the dry and wet signals together.

 Sean Costello has a good blog post about the design:
 https://valhalladsp.com/2009/05/30/schroeder-reverbs-the-forgotten-algorithm/
*/
#include <libsignaletic.h>
#include "../../../../include/kxmx-bluemchen-device.hpp"

FixedCapStr<20> displayStr;

#define SAMPLERATE 48000
#define MAX_DELAY_LINE_LENGTH SAMPLERATE * 1 // 1 second.
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

DaisyHost<kxmx::bluemchen::BluemchenDevice> host;

struct sig_dsp_ConstantValue* one;
struct sig_dsp_ConstantValue* ohSeven;
struct sig_dsp_ConstantValue* minusOhSeven;
struct sig_host_FilteredCVIn* gKnob;
struct sig_host_FilteredCVIn* delayTimeKnob;
struct sig_host_AudioIn* audioIn;
struct sig_DelayLine* outerDL;
struct sig_dsp_Delay* delayTap;
struct sig_DelayLine* dl1;
struct sig_dsp_ConstantValue* ap1DelayTime;
struct sig_dsp_Allpass* ap1;
struct sig_DelayLine* dl2;
struct sig_dsp_ConstantValue* ap2DelayTime;
struct sig_dsp_Allpass* ap2;
struct sig_DelayLine* dl3;
struct sig_dsp_ConstantValue* ap3DelayTime;
struct sig_dsp_Allpass* ap3;
struct sig_DelayLine* dl4;
struct sig_dsp_ConstantValue* ap4DelayTime;
struct sig_dsp_Allpass* ap4;
struct sig_DelayLine* dl5;
struct sig_dsp_ConstantValue* ap5DelayTime;
struct sig_dsp_Allpass* ap5;
struct sig_dsp_BinaryOp* feedback;
struct sig_dsp_BinaryOp* feedbackInputSum;
struct sig_dsp_DelayWrite* delayWrite;
struct sig_dsp_BinaryOp* gSquared;
struct sig_dsp_BinaryOp* wetGain;
struct sig_dsp_BinaryOp* wet;
struct sig_dsp_Invert* minusG;
struct sig_dsp_BinaryOp* dry;
struct sig_dsp_BinaryOp* wetDryMix;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void UpdateOled() {
    host.device.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("1962b");
    host.device.display.SetCursor(0, 0);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Delay ");
    displayStr.AppendFloat(delayTimeKnob->outputs.main[0], 2);
    host.device.display.SetCursor(0, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("g ");
    displayStr.AppendFloat(gKnob->outputs.main[0], 2);
    host.device.display.SetCursor(0, 16);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    host.device.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    one = sig_dsp_ConstantValue_new(&allocator, context, 1.0f);
    ohSeven = sig_dsp_ConstantValue_new(&allocator, context, 0.7f);
    minusOhSeven = sig_dsp_ConstantValue_new(&allocator, context, -0.7f);

    delayTimeKnob = sig_host_FilteredCVIn_new(&allocator, context);
    delayTimeKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, delayTimeKnob, status);
    delayTimeKnob->parameters.scale = 0.999f;
    delayTimeKnob->parameters.offset = 0.001f;
    delayTimeKnob->parameters.control = sig_host_KNOB_1;
    // Lots of smoothing to help with the pitch shift that occurs
    // when modulating a delay line.
    delayTimeKnob->parameters.time = 0.25f;

    gKnob = sig_host_FilteredCVIn_new(&allocator, context);
    gKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, gKnob, status);
    gKnob->parameters.control = sig_host_KNOB_2;
    gKnob->parameters.scale = 0.999f;
    gKnob->parameters.time = 0.001f;

    audioIn = sig_host_AudioIn_new(&allocator, context);
    audioIn->hardware = &host.device.hardware;
    sig_List_append(&signals, audioIn, status);
    audioIn->parameters.channel = sig_host_AUDIO_IN_1;

    // Allpass parameters are from Shroeder,
    // Natural Sounding Artificial Reverberation, 1962.
    // Outer delay line delayTime = 30 ms, g = 0.893
    // Inner all pass parameters match the 1962a algorithm.
    outerDL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    delayTap = sig_dsp_DelayTap_new(&allocator, context);
    sig_List_append(&signals, delayTap, status);
    delayTap->delayLine = outerDL;
    delayTap->inputs.source = audioIn->outputs.main;
    delayTap->inputs.delayTime = delayTimeKnob->outputs.main;

    dl1 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap1DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.1f);
    ap1 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap1, status);
    ap1->delayLine = dl1;
    ap1->inputs.source = delayTap->outputs.main;
    ap1->inputs.delayTime = ap1DelayTime->outputs.main;
    ap1->inputs.g = ohSeven->outputs.main;

    dl2 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap2DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.068f);
    ap2 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap2, status);
    ap2->delayLine = dl2;
    ap2->inputs.source = ap1->outputs.main;
    ap2->inputs.delayTime = ap2DelayTime->outputs.main;
    ap2->inputs.g = minusOhSeven->outputs.main;

    dl3 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap3DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.06f);
    ap3 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap3, status);
    ap3->delayLine = dl3;
    ap3->inputs.source = ap2->outputs.main;
    ap3->inputs.delayTime = ap3DelayTime->outputs.main;
    ap3->inputs.g = ohSeven->outputs.main;

    dl4 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap4DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.0197f);
    ap4 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap4, status);
    ap4->delayLine = dl4;
    ap4->inputs.source = ap3->outputs.main;
    ap4->inputs.delayTime = ap4DelayTime->outputs.main;
    ap4->inputs.g = ohSeven->outputs.main;

    dl5 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap5DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.00585f);
    ap5 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap5, status);
    ap5->delayLine = dl5;
    ap5->inputs.source = ap4->outputs.main;
    ap5->inputs.delayTime = ap5DelayTime->outputs.main;
    ap5->inputs.g = ohSeven->outputs.main;

    feedback = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, feedback, status);
    feedback->inputs.left = ap5->outputs.main;
    feedback->inputs.right = gKnob->outputs.main;

    feedbackInputSum = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, feedbackInputSum, status);
    feedbackInputSum->inputs.left = audioIn->outputs.main;
    feedbackInputSum->inputs.right = feedback->outputs.main;

    delayWrite = sig_dsp_DelayWrite_new(&allocator, context);
    sig_List_append(&signals, delayWrite, status);
    delayWrite->delayLine = outerDL;
    delayWrite->inputs.source = feedbackInputSum->outputs.main;

    gSquared = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, gSquared, status);
    gSquared->inputs.left = gKnob->outputs.main;
    gSquared->inputs.right = gKnob->outputs.main;

    wetGain = sig_dsp_Sub_new(&allocator, context);
    sig_List_append(&signals, wetGain, status);
    wetGain->inputs.left = one->outputs.main;
    wetGain->inputs.right = gSquared->outputs.main;

    wet = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, wet, status);
    wet->inputs.left = ap5->outputs.main;
    wet->inputs.right = wetGain->outputs.main;

    minusG = sig_dsp_Invert_new(&allocator, context);
    sig_List_append(&signals, minusG, status);
    minusG->inputs.source = gKnob->outputs.main;

    dry = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, dry, status);
    dry->inputs.left = audioIn->outputs.main;
    dry->inputs.right = minusG->outputs.main;

    wetDryMix = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, wetDryMix, status);
    wetDryMix->inputs.left = wet->outputs.main;
    wetDryMix->inputs.right = dry->outputs.main;

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = wetDryMix->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->inputs.source = wetDryMix->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 1  // This graph must run at a block size of one because it
                        // uses DelayWrite to create a nested delay line around
                        // a series all pass reverb.
                        // TODO: Add support for mixed rates.
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    // Can't write to memory in SDRAM until after the board has
    // been initialized.
    delayLineAllocator.impl->init(&delayLineAllocator);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(context, &status);
    host.Start();

    while (1) {
        UpdateOled();
    }
}
