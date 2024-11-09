#include <libsignaletic.h>
#include "../../../../include/kxmx-bluemchen-device.hpp"
#include "../../../../shared/include/multi-tap-delay.h"

FixedCapStr<20> displayStr;

#define SAMPLERATE 48000
#define MAX_DELAY_LINE_LENGTH SAMPLERATE // One second
#define DELAY_LINE_HEAP_SIZE 63*1024*1024 // Grab nearly the whole SDRAM.
#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 64

#define NUM_EARLY_ECHO_TAPS 19
float earlyEchoDelayTimeValues[NUM_EARLY_ECHO_TAPS] = {
    0.0f, 0.0043f, 0.0215f, 0.0225f,
    0.0268f, 0.0270f, 0.0298f, 0.0458f,
    0.0485f, 0.0572f, 0.0587f, 0.0595f,
    0.0612f, 0.0707f, 0.0708f, 0.0726f,
    0.0741f, 0.0753f, 0.0797f
};

struct sig_Buffer earlyEchoDelayTimes = {
    .length = NUM_EARLY_ECHO_TAPS,
    .samples = (float_array_ptr) &earlyEchoDelayTimeValues
};

float earlyEchoGainValues[NUM_EARLY_ECHO_TAPS] = {
    1.0f, 0.841f, 0.504f, 0.491f,
    0.379f, 0.380f, 0.346f, 0.289f,
    0.272f, 0.192f, 0.193f, 0.217f,
    0.181f, 0.180f, 0.181f, 0.176f,
    0.142f, 0.167f, 0.134f
};

struct sig_Buffer earlyEchoGains = {
    .length = NUM_EARLY_ECHO_TAPS,
    .samples = (float_array_ptr) &earlyEchoGainValues
};

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

struct sig_host_AudioIn* audioIn;
struct sig_host_FilteredCVIn* delayTimeScaleKnob;
struct sig_host_FilteredCVIn* combGainKnob;
struct sig_dsp_ConstantValue* one;

struct sig_DelayLine* earlyEchoDL;
struct sig_dsp_MultiTapDelay* earlyEchoes;
struct sig_dsp_ConstantValue* reverberatorDelayTime;
struct sig_dsp_BinaryOp* reverberatorDelayTimeScale;
struct sig_DelayLine* reverberatorDelayDL;
struct sig_dsp_Delay* reverberatorDelay;

struct sig_DelayLine* dl1;
struct sig_dsp_ConstantValue* c1DelayTime;
struct sig_dsp_BinaryOp* c1ScaledDelayTime;
struct sig_dsp_ConstantValue* c1LPFGain;
struct sig_dsp_BinaryOp* oneMinusC1G1;
struct sig_dsp_BinaryOp* c1G2;
struct sig_dsp_Comb* c1;

struct sig_DelayLine* dl2;
struct sig_dsp_ConstantValue* c2DelayTime;
struct sig_dsp_BinaryOp* c2ScaledDelayTime;
struct sig_dsp_ConstantValue* c2LPFGain;
struct sig_dsp_BinaryOp* oneMinusC2G1;
struct sig_dsp_BinaryOp* c2G2;
struct sig_dsp_Comb* c2;

struct sig_DelayLine* dl3;
struct sig_dsp_ConstantValue* c3DelayTime;
struct sig_dsp_BinaryOp* c3ScaledDelayTime;
struct sig_dsp_ConstantValue* c3LPFGain;
struct sig_dsp_BinaryOp* oneMinusC3G1;
struct sig_dsp_BinaryOp* c3G2;
struct sig_dsp_Comb* c3;

struct sig_DelayLine* dl4;
struct sig_dsp_ConstantValue* c4DelayTime;
struct sig_dsp_BinaryOp* c4ScaledDelayTime;
struct sig_dsp_ConstantValue* c4LPFGain;
struct sig_dsp_BinaryOp* oneMinusC4G1;
struct sig_dsp_BinaryOp* c4G2;
struct sig_dsp_Comb* c4;

struct sig_DelayLine* dl5;
struct sig_dsp_ConstantValue* c5DelayTime;
struct sig_dsp_BinaryOp* c5ScaledDelayTime;
struct sig_dsp_ConstantValue* c5LPFGain;
struct sig_dsp_BinaryOp* oneMinusC5G1;
struct sig_dsp_BinaryOp* c5G2;
struct sig_dsp_Comb* c5;

struct sig_DelayLine* dl6;
struct sig_dsp_ConstantValue* c6DelayTime;
struct sig_dsp_BinaryOp* c6ScaledDelayTime;
struct sig_dsp_ConstantValue* c6LPFGain;
struct sig_dsp_BinaryOp* oneMinusC6G1;
struct sig_dsp_BinaryOp* c6G2;
struct sig_dsp_Comb* c6;

// TODO: Oof, need multichannel inputs.
struct sig_dsp_BinaryOp* sum1;
struct sig_dsp_BinaryOp* sum2;
struct sig_dsp_BinaryOp* sum3;
struct sig_dsp_BinaryOp* sum4;
struct sig_dsp_BinaryOp* sum5;

struct sig_dsp_ConstantValue* combMixGain;
struct sig_dsp_BinaryOp* scaledCombMix;

struct sig_DelayLine* dl7;
struct sig_dsp_ConstantValue* apDelayTime;
struct sig_dsp_ConstantValue* apGain;
struct sig_dsp_Allpass* ap;

struct sig_dsp_BinaryOp* reverberatorMixScale;
struct sig_dsp_BinaryOp* earlyEchoesReverberatorMix;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void UpdateOled() {
    host.device.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Moorer");
    host.device.display.SetCursor(0, 0);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Del ");
    displayStr.AppendFloat(delayTimeScaleKnob->outputs.main[0], 2);
    host.device.display.SetCursor(0, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("g ");
    displayStr.AppendFloat(combGainKnob->outputs.main[0], 2);
    host.device.display.SetCursor(0, 16);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);
    host.device.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    // Parameters are from Moorer (1979) "About This Reverberation Business."
    // Unit     Delay Time      g1      g2
    // Six parallel comb filters:
    // The gain (g1) values listed here are interpolated to 48KHz from
    // Moorer's tables, which listed values at 25 and 50 KHz.
    // c1       0.05            0.44    "all the g2 terms set to a constant
    //                                  number g times (1 - g1)...
    //                                  each comb will have a different value
    //                                  of g1, but should have all the same
    //                                  value for g. This number g determines
    //                                  the overall reverberation time.
    //                                  For example, values around 0.83 seem to
    //                                  give a reverberation time of about
    //                                  2.0 seconds with these delays."
    // c2       0.056           0.46
    // c3       0.061           0.48
    // c4       0.068           0.50
    // c5       0.072           0.51
    // c6       0.078           0.53
    // One all pass:
    // ap       0.006           0.7

    /** Controls **/
    delayTimeScaleKnob = sig_host_FilteredCVIn_new(&allocator, context);
    delayTimeScaleKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, delayTimeScaleKnob, status);
    delayTimeScaleKnob->parameters.control = sig_host_KNOB_1;
    delayTimeScaleKnob->parameters.scale = 9.999f;
    delayTimeScaleKnob->parameters.offset = 0.001f;
    delayTimeScaleKnob->parameters.time = 0.25f; // Smoother mod pitch shift

    combGainKnob = sig_host_FilteredCVIn_new(&allocator, context);
    combGainKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, combGainKnob, status);
    combGainKnob->parameters.control = sig_host_KNOB_2;
    combGainKnob->parameters.scale = 1.6999f;
    combGainKnob->parameters.offset = 0.001f;

    audioIn = sig_host_AudioIn_new(&allocator, context);
    audioIn->hardware = &host.device.hardware;
    sig_List_append(&signals, audioIn, status);
    audioIn->parameters.channel = sig_host_AUDIO_IN_1;

    /** Early Echoes **/
    // TODO: Add support for scaling the delay taps,
    // which will allow the user to change the room size.
    earlyEchoDL = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    earlyEchoes = sig_dsp_MultiTapDelay_new(&allocator, context);
    sig_List_append(&signals, earlyEchoes, status);
    earlyEchoes->delayLine = earlyEchoDL;
    earlyEchoes->tapTimes = &earlyEchoDelayTimes;
    earlyEchoes->tapGains = &earlyEchoGains;
    earlyEchoes->inputs.source = audioIn->outputs.main;
    earlyEchoes->inputs.timeScale = delayTimeScaleKnob->outputs.main;
    earlyEchoes->parameters.scale = 0.125f; // TODO: Is this scaling needed?

    /** Comb Filters **/
    one = sig_dsp_ConstantValue_new(&allocator, context, 1.0f);

    dl1 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c1DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.05f);
    c1ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c1ScaledDelayTime, status);
    c1ScaledDelayTime->inputs.left = c1DelayTime->outputs.main;
    c1ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c1LPFGain = sig_dsp_ConstantValue_new(&allocator, context, 0.44f);
    oneMinusC1G1 = sig_dsp_Sub_new(&allocator, context);
    sig_List_append(&signals, oneMinusC1G1, status);
    oneMinusC1G1->inputs.left = one->outputs.main;
    oneMinusC1G1->inputs.right = c1LPFGain->outputs.main;
    c1G2 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c1G2, status);
    c1G2->inputs.left = combGainKnob->outputs.main;
    c1G2->inputs.right = oneMinusC1G1->outputs.main;
    c1 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c1, status);
    c1->delayLine = dl1;
    c1->inputs.source = earlyEchoes->outputs.main;
    c1->inputs.feedbackGain = c1G2->outputs.main;
    c1->inputs.delayTime = c1ScaledDelayTime->outputs.main;
    c1->inputs.lpfCoefficient = c1LPFGain->outputs.main;

    dl2 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c2DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.056f);
    c2ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c2ScaledDelayTime, status);
    c2ScaledDelayTime->inputs.left = c2DelayTime->outputs.main;
    c2ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c2LPFGain = sig_dsp_ConstantValue_new(&allocator, context, 0.46f);
    oneMinusC2G1 = sig_dsp_Sub_new(&allocator, context);
    sig_List_append(&signals, oneMinusC2G1, status);
    oneMinusC2G1->inputs.left = one->outputs.main;
    oneMinusC2G1->inputs.right = c2LPFGain->outputs.main;
    c2G2 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c2G2, status);
    c2G2->inputs.left = combGainKnob->outputs.main;
    c2G2->inputs.right = oneMinusC2G1->outputs.main;
    c2 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c2, status);
    c2->delayLine = dl2;
    c2->inputs.source = earlyEchoes->outputs.main;
    c2->inputs.feedbackGain = c2G2->outputs.main;
    c2->inputs.delayTime = c2ScaledDelayTime->outputs.main;
    c2->inputs.lpfCoefficient = c2LPFGain->outputs.main;

    dl3 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c3DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.061f);
    c3ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c3ScaledDelayTime, status);
    c3ScaledDelayTime->inputs.left = c3DelayTime->outputs.main;
    c3ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c3LPFGain = sig_dsp_ConstantValue_new(&allocator, context, 0.48f);
    oneMinusC3G1 = sig_dsp_Sub_new(&allocator, context);
    sig_List_append(&signals, oneMinusC3G1, status);
    oneMinusC3G1->inputs.left = one->outputs.main;
    oneMinusC3G1->inputs.right = c3LPFGain->outputs.main;
    c3G2 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c3G2, status);
    c3G2->inputs.left = combGainKnob->outputs.main;
    c3G2->inputs.right = oneMinusC3G1->outputs.main;
    c3 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c3, status);
    c3->delayLine = dl3;
    c3->inputs.source = earlyEchoes->outputs.main;
    c3->inputs.feedbackGain = c3G2->outputs.main;
    c3->inputs.delayTime = c3ScaledDelayTime->outputs.main;
    c3->inputs.lpfCoefficient = c3LPFGain->outputs.main;

    dl4 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c4DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.068f);
    c4ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c4ScaledDelayTime, status);
    c4ScaledDelayTime->inputs.left = c4DelayTime->outputs.main;
    c4ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c4LPFGain = sig_dsp_ConstantValue_new(&allocator, context, 0.50f);
    oneMinusC4G1 = sig_dsp_Sub_new(&allocator, context);
    sig_List_append(&signals, oneMinusC4G1, status);
    oneMinusC4G1->inputs.left = one->outputs.main;
    oneMinusC4G1->inputs.right = c4LPFGain->outputs.main;
    c4G2 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c4G2, status);
    c4G2->inputs.left = combGainKnob->outputs.main;
    c4G2->inputs.right = oneMinusC4G1->outputs.main;
    c4 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c4, status);
    c4->delayLine = dl4;
    c4->inputs.source = earlyEchoes->outputs.main;
    c4->inputs.feedbackGain = c4G2->outputs.main;
    c4->inputs.delayTime = c4ScaledDelayTime->outputs.main;
    c4->inputs.lpfCoefficient = c4LPFGain->outputs.main;

    dl5 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c5DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.072f);
    c5ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c5ScaledDelayTime, status);
    c5ScaledDelayTime->inputs.left = c5DelayTime->outputs.main;
    c5ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c5LPFGain = sig_dsp_ConstantValue_new(&allocator, context, 0.51f);
    oneMinusC5G1 = sig_dsp_Sub_new(&allocator, context);
    sig_List_append(&signals, oneMinusC5G1, status);
    oneMinusC5G1->inputs.left = one->outputs.main;
    oneMinusC5G1->inputs.right = c5LPFGain->outputs.main;
    c5G2 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c5G2, status);
    c5G2->inputs.left = combGainKnob->outputs.main;
    c5G2->inputs.right = oneMinusC5G1->outputs.main;
    c5 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c5, status);
    c5->delayLine = dl5;
    c5->inputs.source = earlyEchoes->outputs.main;
    c5->inputs.feedbackGain = c5G2->outputs.main;
    c5->inputs.delayTime = c5ScaledDelayTime->outputs.main;
    c5->inputs.lpfCoefficient = c5LPFGain->outputs.main;

    dl6 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    c6DelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.078f);
    c6ScaledDelayTime = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c6ScaledDelayTime, status);
    c6ScaledDelayTime->inputs.left = c6DelayTime->outputs.main;
    c6ScaledDelayTime->inputs.right = delayTimeScaleKnob->outputs.main;
    c6LPFGain = sig_dsp_ConstantValue_new(&allocator, context, 0.53f);
    oneMinusC6G1 = sig_dsp_Sub_new(&allocator, context);
    sig_List_append(&signals, oneMinusC6G1, status);
    oneMinusC6G1->inputs.left = one->outputs.main;
    oneMinusC6G1->inputs.right = c6LPFGain->outputs.main;
    c6G2 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, c6G2, status);
    c6G2->inputs.left = combGainKnob->outputs.main;
    c6G2->inputs.right = oneMinusC6G1->outputs.main;
    c6 = sig_dsp_Comb_new(&allocator, context);
    sig_List_append(&signals, c6, status);
    c6->delayLine = dl6;
    c6->inputs.source = earlyEchoes->outputs.main;
    c6->inputs.feedbackGain = c6G2->outputs.main;
    c6->inputs.delayTime = c6ScaledDelayTime->outputs.main;
    c6->inputs.lpfCoefficient = c6LPFGain->outputs.main;

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

    sum4 = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, sum4, status);
    sum4->inputs.left = sum3->outputs.main;
    sum4->inputs.right = c5->outputs.main;

    sum5 = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, sum5, status);
    sum5->inputs.left = sum4->outputs.main;
    sum5->inputs.right = c6->outputs.main;

    combMixGain = sig_dsp_ConstantValue_new(&allocator, context, 0.2f);
    scaledCombMix = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, scaledCombMix, status);
    scaledCombMix->inputs.left = sum5->outputs.main;
    scaledCombMix->inputs.right = combMixGain->outputs.main;

    /** All Pass **/
    // Note: I don't scale the all pass delay time with the delay time knob,
    // because I think it sounds better tuned as is (Moorer has a comment about
    // needing to keep the delay time of the all pass short but
    // not too short on p19).
    apDelayTime = sig_dsp_ConstantValue_new(&allocator, context, 0.005f);
    apGain = sig_dsp_ConstantValue_new(&allocator, context, 0.7f);
    dl7 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    ap = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap, status);
    ap->delayLine = dl7;
    ap->inputs.source = scaledCombMix->outputs.main;
    ap->inputs.delayTime = apDelayTime->outputs.main;
    ap->inputs.g = apGain->outputs.main;

    reverberatorDelayTime = sig_dsp_ConstantValue_new(&allocator, context,
        0.0247); // FIXME: This has to be responsive to delay time scale.
                 // Moorer says "the delays D1 and D2 are set such that the
                 // first echo from the reverberator [0.055] coincides
                 // with the end of the last echo from the early
                 // response [0.0797]. This means either
                 // D1 [the early echo delay] or D2 [the reverberator delay]
                 // will be zero, depending on whether the total delay of
                 // the early echo is longer or
                 // shorter than the shortest delay in the reverberator."
    reverberatorDelayTimeScale = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, reverberatorDelayTimeScale, status);
    reverberatorDelayTimeScale->inputs.left =
        reverberatorDelayTime->outputs.main;
    reverberatorDelayTimeScale->inputs.right = delayTimeScaleKnob->outputs.main;
    reverberatorDelayDL = sig_DelayLine_new(&delayLineAllocator,
        MAX_DELAY_LINE_LENGTH);
    reverberatorDelay = sig_dsp_Delay_new(&allocator, context);
    sig_List_append(&signals, reverberatorDelay, status);
    reverberatorDelay->delayLine = reverberatorDelayDL;
    reverberatorDelay->inputs.source = ap->outputs.main;
    reverberatorDelay->inputs.delayTime =
        reverberatorDelayTimeScale->outputs.main;

    reverberatorMixScale = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, reverberatorMixScale, status);
    reverberatorMixScale->inputs.left = reverberatorDelay->outputs.main;
    reverberatorMixScale->inputs.right = combGainKnob->outputs.main;

    earlyEchoesReverberatorMix = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, earlyEchoesReverberatorMix, status);
    earlyEchoesReverberatorMix->inputs.left = earlyEchoes->outputs.main;
    earlyEchoesReverberatorMix->inputs.right =
        reverberatorMixScale->outputs.main;

    // TODO: Add wet/dry mix

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = earlyEchoesReverberatorMix->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->inputs.source = earlyEchoesReverberatorMix->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 2
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
