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

struct sig_host_FilteredCVIn* mixKnob;
struct sig_host_FilteredCVIn* timeScaleKnob;
struct sig_host_AudioIn* audioIn;
struct sig_dsp_ConstantValue* ohSeven;
struct sig_dsp_ConstantValue* minusOhSeven;
struct sig_DelayLine* dl1;
struct sig_dsp_ConstantValue* delayTime1;
struct sig_dsp_BinaryOp* delayTimeScale1;
struct sig_dsp_Allpass* ap1;
struct sig_DelayLine* dl2;
struct sig_dsp_ConstantValue* delayTime2;
struct sig_dsp_BinaryOp* delayTimeScale2;
struct sig_dsp_Allpass* ap2;
struct sig_DelayLine* dl3;
struct sig_dsp_ConstantValue* delayTime3;
struct sig_dsp_BinaryOp* delayTimeScale3;
struct sig_dsp_Allpass* ap3;
struct sig_DelayLine* dl4;
struct sig_dsp_ConstantValue* delayTime4;
struct sig_dsp_BinaryOp* delayTimeScale4;
struct sig_dsp_Allpass* ap4;
struct sig_DelayLine* dl5;
struct sig_dsp_ConstantValue* delayTime5;
struct sig_dsp_BinaryOp* delayTimeScale5;
struct sig_dsp_Allpass* ap5;
struct sig_dsp_LinearXFade* wetDryMixer;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void UpdateOled() {
    host.device.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("1961a");
    host.device.display.SetCursor(0, 0);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Mix ");
    displayStr.AppendFloat(mixKnob->outputs.main[0], 2);
    host.device.display.SetCursor(0, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Time ");
    displayStr.AppendFloat(delayTimeScale1->outputs.main[0], 2);
    host.device.display.SetCursor(0, 16);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    host.device.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    mixKnob = sig_host_FilteredCVIn_new(&allocator, context);
    mixKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, mixKnob, status);
    mixKnob->parameters.control = sig_host_KNOB_1;
    mixKnob->parameters.scale = 2.0f;
    mixKnob->parameters.offset = -1.0f;
    mixKnob->parameters.time = 0.1f;

    timeScaleKnob = sig_host_FilteredCVIn_new(&allocator, context);
    timeScaleKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, timeScaleKnob, status);
    timeScaleKnob->parameters.control = sig_host_KNOB_2;
    timeScaleKnob->parameters.scale = 9.9f;
    timeScaleKnob->parameters.offset = 0.1f;
    // Lots of smoothing to help with the pitch shift that occurs
    // when modulating a delay line.
    timeScaleKnob->parameters.time = 0.5f;

    audioIn = sig_host_AudioIn_new(&allocator, context);
    audioIn->hardware = &host.device.hardware;
    sig_List_append(&signals, audioIn, status);
    audioIn->parameters.channel = sig_host_AUDIO_IN_1;

    // Allpass parameters are from Shroeder and Logan,
    // 'Colorless' Artificial Reverb, 1961.
    // http://languagelog.ldc.upenn.edu/myl/Logan1961.pdf
    ohSeven = sig_dsp_ConstantValue_new(&allocator, context, 0.7f);
    minusOhSeven = sig_dsp_ConstantValue_new(&allocator, context, -0.7f);

    dl1 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    delayTime1 = sig_dsp_ConstantValue_new(&allocator, context, 0.1f);
    delayTimeScale1 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, delayTimeScale1, status);
    delayTimeScale1->inputs.left = delayTime1->outputs.main;
    delayTimeScale1->inputs.right = timeScaleKnob->outputs.main;
    ap1 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap1, status);
    ap1->delayLine = dl1;
    ap1->inputs.source = audioIn->outputs.main;
    ap1->inputs.delayTime = delayTimeScale1->outputs.main;
    ap1->inputs.g = ohSeven->outputs.main;

    dl2 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    delayTime2 = sig_dsp_ConstantValue_new(&allocator, context, 0.068f);
    delayTimeScale2 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, delayTimeScale2, status);
    delayTimeScale2->inputs.left = delayTime2->outputs.main;
    delayTimeScale2->inputs.right = timeScaleKnob->outputs.main;
    ap2 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap2, status);
    ap2->delayLine = dl2;
    ap2->inputs.source = ap1->outputs.main;
    ap2->inputs.delayTime = delayTimeScale2->outputs.main;
    ap2->inputs.g = minusOhSeven->outputs.main;

    dl3 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    delayTime3 = sig_dsp_ConstantValue_new(&allocator, context, 0.06f);
    delayTimeScale3 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, delayTimeScale3, status);
    delayTimeScale3->inputs.left = delayTime3->outputs.main;
    delayTimeScale3->inputs.right = timeScaleKnob->outputs.main;
    ap3 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap3, status);
    ap3->delayLine = dl3;
    ap3->inputs.source = ap2->outputs.main;
    ap3->inputs.delayTime = delayTimeScale3->outputs.main;
    ap3->inputs.g = ohSeven->outputs.main;

    dl4 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    delayTime4 = sig_dsp_ConstantValue_new(&allocator, context, 0.0197f);
    delayTimeScale4 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, delayTimeScale4, status);
    delayTimeScale4->inputs.left = delayTime4->outputs.main;
    delayTimeScale4->inputs.right = timeScaleKnob->outputs.main;
    ap4 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap4, status);
    ap4->delayLine = dl4;
    ap4->inputs.source = ap3->outputs.main;
    ap4->inputs.delayTime = delayTimeScale4->outputs.main;
    ap4->inputs.g = ohSeven->outputs.main;

    dl5 = sig_DelayLine_new(&delayLineAllocator, MAX_DELAY_LINE_LENGTH);
    delayTime5 = sig_dsp_ConstantValue_new(&allocator, context, 0.00585f);
    delayTimeScale5 = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, delayTimeScale5, status);
    delayTimeScale5->inputs.left = delayTime5->outputs.main;
    delayTimeScale5->inputs.right = timeScaleKnob->outputs.main;
    ap5 = sig_dsp_Allpass_new(&allocator, context);
    sig_List_append(&signals, ap5, status);
    ap5->delayLine = dl5;
    ap5->inputs.source = ap4->outputs.main;
    ap5->inputs.delayTime = delayTimeScale5->outputs.main;
    ap5->inputs.g = ohSeven->outputs.main;

    wetDryMixer = sig_dsp_LinearXFade_new(&allocator, context);
    sig_List_append(&signals, wetDryMixer, status);
    wetDryMixer->inputs.left = audioIn->outputs.main;
    wetDryMixer->inputs.right = ap5->outputs.main;
    wetDryMixer->inputs.mix = mixKnob->outputs.main;

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = wetDryMixer->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->inputs.source = wetDryMixer->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 48
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
