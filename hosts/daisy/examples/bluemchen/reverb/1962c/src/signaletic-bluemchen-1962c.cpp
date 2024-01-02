#include <string>
#include <tlsf.h>
#include <libsignaletic.h>
#include "../../../../include/daisy-bluemchen-host.h"

using namespace kxmx;
using namespace daisy;

FixedCapStr<20> displayStr;

#define MAX_DELAY_LINE_LENGTH 96000 // 1 second.
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
struct sig_daisy_AudioIn* audioIn;
struct sig_daisy_FilteredCVIn* delayTimeScaleKnob;
struct sig_daisy_FilteredCVIn* feedbackGainScaleKnob;
struct sig_dsp_ConstantValue* apGain;
struct sig_dsp_ConstantValue* combLPFCoefficient;
struct sig_DelayLine* dl1;
struct sig_dsp_ConstantValue* c1DelayTime;
struct sig_dsp_BinaryOp* c1ScaledDelayTime;
struct sig_dsp_Comb* c1;
struct sig_DelayLine* dl2;
struct sig_dsp_ConstantValue* c2DelayTime;
struct sig_dsp_BinaryOp* c2ScaledDelayTime;
struct sig_dsp_Comb* c2;
struct sig_DelayLine* dl3;
struct sig_dsp_ConstantValue* c3DelayTime;
struct sig_dsp_BinaryOp* c3ScaledDelayTime;
struct sig_dsp_Comb* c3;
struct sig_DelayLine* dl4;
struct sig_dsp_ConstantValue* c4DelayTime;
struct sig_dsp_BinaryOp* c4ScaledDelayTime;
struct sig_dsp_Comb* c4;
struct sig_dsp_BinaryOp* sum1;
struct sig_dsp_BinaryOp* sum2;
struct sig_dsp_BinaryOp* sum3;
struct sig_dsp_ConstantValue* combGain;
struct sig_dsp_BinaryOp* scaledCombMix;
struct sig_DelayLine* dl5;
struct sig_dsp_ConstantValue* ap1DelayTime;
struct sig_dsp_Allpass* ap1;
struct sig_DelayLine* dl6;
struct sig_dsp_ConstantValue* ap2DelayTime;
struct sig_dsp_Allpass* ap2;
struct sig_daisy_AudioOut* leftOut;
struct sig_dsp_Invert* ap2Inverted;
struct sig_daisy_AudioOut* rightOut;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("1962c");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Del ");
    displayStr.AppendFloat(delayTimeScaleKnob->outputs.main[0], 2);
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("g ");
    displayStr.AppendFloat(feedbackGainScaleKnob->outputs.main[0], 2);
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);
    bluemchen.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    audioIn = sig_daisy_AudioIn_new(&allocator, context, host);
    sig_List_append(&signals, audioIn, status);
    audioIn->parameters.channel = sig_daisy_AUDIO_IN_1;

    // Parameters are from Dodge and Jerse
    // rvt between 1.5 and 2 seconds for a concert hall
    // node  rvt    delay time
    // c1    rvt    29.7 ms
    // c2    rvt    37.1 ms
    // c3    rvt    41.1 ms
    // c4    rvt    43.7 ms
    // ap1   5 ms   96.83 ms
    // ap2   1.7 ms 32.92 ms
    // (The gains for both all passes calculate as 0.87479,
    // but Schroeder recommends a g of 0.7 for both allpasses).

    delayTimeScaleKnob = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, delayTimeScaleKnob, status);
    delayTimeScaleKnob->parameters.control = sig_daisy_Bluemchen_CV_IN_KNOB_1;
    delayTimeScaleKnob->parameters.scale = 99.999f;
    delayTimeScaleKnob->parameters.offset = 0.001f;
    // Lots of smoothing to help with the pitch shift that occurs
    // when modulating a delay line.
    delayTimeScaleKnob->parameters.time = 0.25f;

    feedbackGainScaleKnob = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, feedbackGainScaleKnob, status);
    feedbackGainScaleKnob->parameters.control = sig_daisy_Bluemchen_CV_IN_KNOB_2;
    feedbackGainScaleKnob->parameters.scale = 0.999f;
    feedbackGainScaleKnob->parameters.offset = 0.001f;

    apGain = sig_dsp_ConstantValue_new(&allocator, context, 0.7f);
    combLPFCoefficient = sig_dsp_ConstantValue_new(&allocator, context, 0.55f);

    dl1 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c1DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.0297f);
    c1ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c1ScaledDelayTime, status);
    c1ScaledDelayTime->inputs.left = c1DelayTime->outputs.main;
    c1ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c1 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c1, status);
    c1->delayLine = dl1;
    c1->inputs.source = audioIn->outputs.main;
    c1->inputs.feedbackGain = feedbackGainScaleKnob->outputs.main;
    c1->inputs.delayTime = c1ScaledDelayTime->outputs.main;
    c1->inputs.lpfCoefficient = combLPFCoefficient->outputs.main;

    dl2 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c2DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.0371f);
    c2ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c2ScaledDelayTime, status);
    c2ScaledDelayTime->inputs.left = c2DelayTime->outputs.main;
    c2ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c2 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c2, status);
    c2->delayLine = dl2;
    c2->inputs.source = audioIn->outputs.main;
    c2->inputs.feedbackGain = feedbackGainScaleKnob->outputs.main;
    c2->inputs.delayTime = c2ScaledDelayTime->outputs.main;
    c2->inputs.lpfCoefficient = combLPFCoefficient->outputs.main;

    dl3 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c3DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.0411f);
    c3ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c3ScaledDelayTime, status);
    c3ScaledDelayTime->inputs.left = c3DelayTime->outputs.main;
    c3ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c3 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c3, status);
    c3->delayLine = dl3;
    c3->inputs.source = audioIn->outputs.main;
    c3->inputs.feedbackGain = feedbackGainScaleKnob->outputs.main;
    c3->inputs.delayTime = c3ScaledDelayTime->outputs.main;
    c3->inputs.lpfCoefficient = combLPFCoefficient->outputs.main;

    dl4 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c4DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.0437f);
    c4ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c4ScaledDelayTime, status);
    c4ScaledDelayTime->inputs.left = c4DelayTime->outputs.main;
    c4ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c4 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c4, status);
    c4->delayLine = dl4;
    c4->inputs.source = audioIn->outputs.main;
    c4->inputs.feedbackGain = feedbackGainScaleKnob->outputs.main;
    c4->inputs.delayTime = c4ScaledDelayTime->outputs.main;
    c4->inputs.lpfCoefficient = combLPFCoefficient->outputs.main;

    sum1 = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, sum1, status);
    sum1->inputs.left = c1->outputs.main;
    sum1->inputs.right = c2->outputs.main;

    sum2 = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, sum2, status);
    sum2->inputs.left = sum1->outputs.main;
    sum2->inputs.right = c3->outputs.main;

    sum3 = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, sum3, status);
    sum3->inputs.left = sum2->outputs.main;
    sum3->inputs.right = c4->outputs.main;

    combGain = sig_dsp_ConstantValue_new(&allocator, context, 0.2f);
    scaledCombMix = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, scaledCombMix, status);
    scaledCombMix->inputs.left = sum3->outputs.main;
    scaledCombMix->inputs.right = combGain->outputs.main;

    ap1DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.09683f);
    dl5 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap1 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap1, status);
    ap1->delayLine = dl5;
    ap1->inputs.source = scaledCombMix->outputs.main;
    ap1->inputs.delayTime = ap1DelayTime->outputs.main;
    ap1->inputs.g = apGain->outputs.main;

    ap2DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.03292f);
    dl6 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap2 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap2, status);
    ap2->delayLine = dl6;
    ap2->inputs.source = ap1->outputs.main;
    ap2->inputs.delayTime = ap2DelayTime->outputs.main;
    ap2->inputs.g = apGain->outputs.main;

    leftOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_daisy_AUDIO_OUT_1;
    leftOut->inputs.source = ap2->outputs.main;

    ap2Inverted = sig_dsp_Invert_new(&allocator, context);
    sig_List_append(&signals, ap2Inverted, status);
    ap2Inverted->inputs.source = ap2->outputs.main;

    rightOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_daisy_AUDIO_OUT_2;
    rightOut->inputs.source = ap2Inverted->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 96000,
        .numChannels = 2,
        .blockSize = 96
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
