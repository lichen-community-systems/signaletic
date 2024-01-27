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
struct sig_dsp_BinaryOp* apScaledGain;

struct sig_DelayLine* ap1DL;
struct sig_dsp_ConstantValue* ap1DelayTime;
struct sig_dsp_BinaryOp* ap1ScaledDelayTime;
struct sig_dsp_Allpass* ap1;
struct sig_DelayLine* ap2DL;
struct sig_dsp_ConstantValue* ap2DelayTime;
struct sig_dsp_BinaryOp* ap2ScaledDelayTime;
struct sig_dsp_Allpass* ap2;
struct sig_DelayLine* ap3DL;
struct sig_dsp_ConstantValue* ap3DelayTime;
struct sig_dsp_BinaryOp* ap3ScaledDelayTime;
struct sig_dsp_Allpass* ap3;

struct sig_dsp_ConstantValue* combLPFCoefficient;
struct sig_DelayLine* c1DL;
struct sig_dsp_ConstantValue* c1DelayTime;
struct sig_dsp_BinaryOp* c1ScaledDelayTime;
struct sig_dsp_ConstantValue* c1Gain;
struct sig_dsp_BinaryOp* c1ScaledGain;
struct sig_dsp_Comb* c1;
struct sig_DelayLine* c2DL;
struct sig_dsp_ConstantValue* c2DelayTime;
struct sig_dsp_BinaryOp* c2ScaledDelayTime;
struct sig_dsp_ConstantValue* c2Gain;
struct sig_dsp_BinaryOp* c2ScaledGain;
struct sig_dsp_Comb* c2;
struct sig_DelayLine* c3DL;
struct sig_dsp_ConstantValue* c3DelayTime;
struct sig_dsp_BinaryOp* c3ScaledDelayTime;
struct sig_dsp_ConstantValue* c3Gain;
struct sig_dsp_BinaryOp* c3ScaledGain;
struct sig_dsp_Comb* c3;
struct sig_DelayLine* c4DL;
struct sig_dsp_ConstantValue* c4DelayTime;
struct sig_dsp_BinaryOp* c4ScaledDelayTime;
struct sig_dsp_ConstantValue* c4Gain;
struct sig_dsp_BinaryOp* c4ScaledGain;
struct sig_dsp_Comb* c4;

struct sig_dsp_BinaryOp* sum1;
struct sig_dsp_BinaryOp* sum2;
struct sig_dsp_BinaryOp* sum3;
struct sig_dsp_ConstantValue* combGain;
struct sig_dsp_BinaryOp* scaledCombMix;

struct sig_DelayLine* d1DL;
struct sig_dsp_ConstantValue* d1DelayTime;
struct sig_dsp_Delay* d1;
struct sig_DelayLine* d2DL;
struct sig_dsp_ConstantValue* d2DelayTime;
struct sig_dsp_Delay* d2;

struct sig_daisy_AudioOut* leftOut;
struct sig_daisy_AudioOut* rightOut;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("JCRev");
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
    // Parameters are from Julius O. Smith,
    // "A Schroeder Reverb called JCRev"
    // https://ccrma.stanford.edu/~jos/Reverb/A_Schroeder_Reverberator_called.html
    // Note that there are two different versions of JCRev topology
    // documented by JOS. One uses decorrelation delays at the end,
    // the other does not.
    // Both seem to come from the CLM source code:
    // https://github.com/radiganm/clm/blob/master/jcrev.ins
    // My best guess is that the delay times specified in the JOS link
    // above are at 44.1KHz, whereas those documented at the link below
    // are at 12.8KHz:
    // https://ccrma.stanford.edu/~jos/pasp/Schroeder_Reverberators.html
    // Node     Delay Time      G
    // ap1      0.02383 sec     0.7
    // ap2      0.00764 sec     0.7
    // ap3      0.00256 sec     0.7
    // c1       0.10882 sec     0.742
    // c2       0.11336 sec     0.733
    // c3       0.12243 sec     0.715
    // c4       0.13154 sec     0.697
    // d1       0.013   sec     -
    // d2       0.011   sec     -
    // d3       0.15    sec     - (not used)
    // d4       0.017   sec     - (not used)
    // (First JOS link has d1-d4 delay times as 0.046, 0.057, 0.041, 0.054)

    audioIn = sig_daisy_AudioIn_new(&allocator, context, host);
    sig_List_append(&signals, audioIn, status);
    audioIn->parameters.channel = sig_daisy_AUDIO_IN_1;

    delayTimeScaleKnob = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, delayTimeScaleKnob, status);
    delayTimeScaleKnob->parameters.control = sig_daisy_Bluemchen_CV_IN_KNOB_1;
    delayTimeScaleKnob->parameters.scale = 9.999f;
    delayTimeScaleKnob->parameters.offset = 0.001f;
    // Lots of smoothing to help with the pitch shift that occurs
    // when modulating a delay line.
    delayTimeScaleKnob->parameters.time = 0.25f;

    feedbackGainScaleKnob = sig_daisy_FilteredCVIn_new(&allocator, context, host);
    sig_List_append(&signals, feedbackGainScaleKnob, status);
    feedbackGainScaleKnob->parameters.control = sig_daisy_Bluemchen_CV_IN_KNOB_2;
    feedbackGainScaleKnob->parameters.scale = 1.349f;
    feedbackGainScaleKnob->parameters.offset = 0.001f;

    apGain = sig_dsp_ConstantValue_new(&allocator, context, 0.7f);
    apScaledGain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, apScaledGain, status);
    apScaledGain->inputs.left = apGain->outputs.main;
    apScaledGain->inputs.right = feedbackGainScaleKnob->outputs.main;
    combLPFCoefficient = sig_dsp_ConstantValue_new(&allocator, context, 0.0f);

    /** Series All Pass **/
    ap1DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.02383f);
    ap1ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, ap1ScaledDelayTime, status);
    ap1ScaledDelayTime->inputs.left = ap1DelayTime->outputs.main;
    ap1ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    ap1DL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap1 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap1, status);
    ap1->delayLine = ap1DL;
    ap1->inputs.source = audioIn->outputs.main;
    ap1->inputs.delayTime = ap1ScaledDelayTime->outputs.main;
    ap1->inputs.g = apScaledGain->outputs.main;

    ap2DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.00764f);
    ap2ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, ap2ScaledDelayTime, status);
    ap2ScaledDelayTime->inputs.left = ap2DelayTime->outputs.main;
    ap2ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    ap2DL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap2 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap2, status);
    ap2->delayLine = ap2DL;
    ap2->inputs.source = ap1->outputs.main;
    ap2->inputs.delayTime = ap2ScaledDelayTime->outputs.main;
    ap2->inputs.g = apScaledGain->outputs.main;

    ap3DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.00256f);
    ap3ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, ap3ScaledDelayTime, status);
    ap3ScaledDelayTime->inputs.left = ap3DelayTime->outputs.main;
    ap3ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    ap3DL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap3 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap3, status);
    ap3->delayLine = ap3DL;
    ap3->inputs.source = ap2->outputs.main;
    ap3->inputs.delayTime = ap3ScaledDelayTime->outputs.main;
    ap3->inputs.g = apScaledGain->outputs.main;

    /** Parallel Combs */
    c1DL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c1DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.10882f);
    c1ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c1ScaledDelayTime, status);
    c1ScaledDelayTime->inputs.left = c1DelayTime->outputs.main;
    c1ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c1Gain = sig_dsp_ConstantValue_new(&allocator, context, 0.742f);
    c1ScaledGain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c1ScaledGain, status);
    c1ScaledGain->inputs.left = c1Gain->outputs.main;
    c1ScaledGain->inputs.right = feedbackGainScaleKnob->outputs.main;
    c1 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c1, status);
    c1->delayLine = c1DL;
    c1->inputs.source = ap3->outputs.main;
    c1->inputs.feedbackGain = c1ScaledGain->outputs.main;
    c1->inputs.delayTime = c1ScaledDelayTime->outputs.main;
    c1->inputs.lpfCoefficient = combLPFCoefficient->outputs.main;

    c2DL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c2DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.11336f);
    c2ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c2ScaledDelayTime, status);
    c2ScaledDelayTime->inputs.left = c2DelayTime->outputs.main;
    c2ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c2Gain = sig_dsp_ConstantValue_new(&allocator, context, 0.733f);
    c2ScaledGain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c2ScaledGain, status);
    c2ScaledGain->inputs.left = c2Gain->outputs.main;
    c2ScaledGain->inputs.right = feedbackGainScaleKnob->outputs.main;
    c2 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c2, status);
    c2->delayLine = c2DL;
    c2->inputs.source = ap3->outputs.main;
    c2->inputs.feedbackGain = c2ScaledGain->outputs.main;
    c2->inputs.delayTime = c2ScaledDelayTime->outputs.main;
    c2->inputs.lpfCoefficient = combLPFCoefficient->outputs.main;

    c3DL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c3DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.12243f);
    c3ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c3ScaledDelayTime, status);
    c3ScaledDelayTime->inputs.left = c3DelayTime->outputs.main;
    c3ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c3Gain = sig_dsp_ConstantValue_new(&allocator, context, 0.715f);
    c3ScaledGain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c3ScaledGain, status);
    c3ScaledGain->inputs.left = c3Gain->outputs.main;
    c3ScaledGain->inputs.right = feedbackGainScaleKnob->outputs.main;
    c3 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c3, status);
    c3->delayLine = c3DL;
    c3->inputs.source = ap3->outputs.main;
    c3->inputs.feedbackGain = c3ScaledGain->outputs.main;
    c3->inputs.delayTime = c3ScaledDelayTime->outputs.main;
    c3->inputs.lpfCoefficient = combLPFCoefficient->outputs.main;

    c4DL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c4DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.13154f);
    c4ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c4ScaledDelayTime, status);
    c4ScaledDelayTime->inputs.left = c4DelayTime->outputs.main;
    c4ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c4Gain = sig_dsp_ConstantValue_new(&allocator, context, 0.697f);
    c4ScaledGain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c4ScaledGain, status);
    c4ScaledGain->inputs.left = c4Gain->outputs.main;
    c4ScaledGain->inputs.right = feedbackGainScaleKnob->outputs.main;
    c4 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c4, status);
    c4->delayLine = c4DL;
    c4->inputs.source = ap3->outputs.main;
    c4->inputs.feedbackGain = c4ScaledGain->outputs.main;
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

    /** Decorrelation Delays **/
    d1DL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    d1DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.046f);
    d1 = sig_dsp_Delay_new(&allocator, context);
    sig_List_append(&signals, d1, status);
    d1->delayLine = d1DL;
    d1->inputs.source = scaledCombMix->outputs.main;
    d1->inputs.delayTime = d1DelayTime->outputs.main;

    d2DL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    d2DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.057f);
    d2 = sig_dsp_Delay_new(&allocator, context);
    sig_List_append(&signals, d2, status);
    d2->delayLine = d2DL;
    d2->inputs.source = scaledCombMix->outputs.main;
    d2->inputs.delayTime = d2DelayTime->outputs.main;

    leftOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_daisy_AUDIO_OUT_1;
    leftOut->inputs.source = d1->outputs.main;

    rightOut = sig_daisy_AudioOut_new(&allocator, context, host);
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_daisy_AUDIO_OUT_2;
    rightOut->inputs.source = d2->outputs.main;
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
