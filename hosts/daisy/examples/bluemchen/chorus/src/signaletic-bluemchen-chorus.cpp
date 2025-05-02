#include <libsignaletic.h>
#include "../../../../include/kxmx-bluemchen-device.hpp"

#define SAMPLERATE 48000
#define DELAY_LINE_LENGTH 48000 // 1 second
#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 32

FixedCapStr<20> displayStr;

float DSY_SDRAM_BSS leftDelayLineSamples[DELAY_LINE_LENGTH];
struct sig_Buffer leftDelayLineBuffer = {
    .length = DELAY_LINE_LENGTH,
    .samples = leftDelayLineSamples
};

float DSY_SDRAM_BSS rightDelayLineSamples[DELAY_LINE_LENGTH];
struct sig_Buffer rightDelayLineBuffer = {
    .length = DELAY_LINE_LENGTH,
    .samples = rightDelayLineSamples
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

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;
sig::libdaisy::DaisyHost<kxmx::bluemchen::BluemchenDevice> host;

struct sig_host_FilteredCVIn* mixKnob;
struct sig_host_FilteredCVIn* lfoSpeedKnob;
struct sig_dsp_ConstantValue* lfoDepth;
struct sig_dsp_ConstantValue* lfoOffset;
struct sig_host_AudioIn* leftIn;
struct sig_dsp_Oscillator* leftLFO;
struct sig_dsp_Delay* leftDelay;
struct sig_dsp_LinearXFade* leftWetDryMixer;
struct sig_host_AudioOut* leftOut;
struct sig_dsp_ConstantValue* lfoPhaseOffset;
struct sig_dsp_Oscillator* rightLFO;
struct sig_dsp_Delay* rightDelay;
struct sig_dsp_LinearXFade* rightWetDryMixer;
struct sig_host_AudioOut* rightOut;

void UpdateOled() {
    host.device.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Chorus");
    host.device.display.SetCursor(0, 0);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Mix ");
    displayStr.AppendFloat(mixKnob->outputs.main[0], 2);
    host.device.display.SetCursor(0, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("LFO ");
    displayStr.AppendFloat(lfoSpeedKnob->outputs.main[0], 2);
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

    // Juno-60 Chorus
    // https://github.com/pendragon-andyh/Juno60/blob/master/Chorus/README.md
    lfoSpeedKnob = sig_host_FilteredCVIn_new(&allocator, context);
    lfoSpeedKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, lfoSpeedKnob, status);
    lfoSpeedKnob->parameters.control = sig_host_KNOB_2;
    // These are in the range of the Juno chorus 1 & and 2
    // lfoSpeedKnob->parameters.scale = 0.35f;
    // lfoSpeedKnob->parameters.offset = 0.513f;
    // Whereas these can go a little wilder.
    // 4.0 is about the threshold of consonance at full wet.
    lfoSpeedKnob->parameters.scale = 9.75f;
    lfoSpeedKnob->parameters.time = 0.1f;

    lfoDepth = sig_dsp_ConstantValue_new(&allocator, context, 0.001845f);
    lfoOffset = sig_dsp_ConstantValue_new(&allocator, context, 0.003505f);

    leftIn = sig_host_AudioIn_new(&allocator, context);
    leftIn->hardware = &host.device.hardware;
    sig_List_append(&signals, leftIn, status);
    leftIn->parameters.channel = sig_host_AUDIO_IN_1;

    leftLFO = sig_dsp_LFTriangle_new(&allocator, context);
    sig_List_append(&signals, leftLFO, status);
    leftLFO->inputs.freq = lfoSpeedKnob->outputs.main;
    leftLFO->inputs.mul = lfoDepth->outputs.main;
    leftLFO->inputs.add = lfoOffset->outputs.main;

    leftDelay = sig_dsp_Delay_new(&allocator, context);
    sig_List_append(&signals, leftDelay, status);
    leftDelay->delayLine = sig_DelayLine_newWithTransferredBuffer(&allocator,
        &leftDelayLineBuffer);
    leftDelay->inputs.source = leftIn->outputs.main;
    leftDelay->inputs.delayTime = leftLFO->outputs.main;

    leftWetDryMixer = sig_dsp_LinearXFade_new(&allocator, context);
    sig_List_append(&signals, leftWetDryMixer, status);
    leftWetDryMixer->inputs.left = leftIn->outputs.main;
    leftWetDryMixer->inputs.right = leftDelay->outputs.main;
    leftWetDryMixer->inputs.mix = mixKnob->outputs.main;

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = leftWetDryMixer->outputs.main;

    // The phase of the right LFO is 180 degrees from the left.
    lfoPhaseOffset = sig_dsp_ConstantValue_new(&allocator, context, 0.5f);
    rightLFO = sig_dsp_LFTriangle_new(&allocator, context);
    sig_List_append(&signals, rightLFO, status);
    rightLFO->inputs.freq = lfoSpeedKnob->outputs.main;
    rightLFO->inputs.phaseOffset = lfoPhaseOffset->outputs.main;
    rightLFO->inputs.mul = lfoDepth->outputs.main;
    rightLFO->inputs.add = lfoOffset->outputs.main;

    rightDelay = sig_dsp_Delay_new(&allocator, context);
    sig_List_append(&signals, rightDelay, status);
    rightDelay->delayLine = sig_DelayLine_newWithTransferredBuffer(&allocator,
        &rightDelayLineBuffer);
    rightDelay->inputs.source = leftIn->outputs.main;
    rightDelay->inputs.delayTime = rightLFO->outputs.main;

    rightWetDryMixer = sig_dsp_LinearXFade_new(&allocator, context);
    sig_List_append(&signals, rightWetDryMixer, status);
    rightWetDryMixer->inputs.left = leftIn->outputs.main;
    rightWetDryMixer->inputs.right = rightDelay->outputs.main;
    rightWetDryMixer->inputs.mix = mixKnob->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->inputs.source = rightWetDryMixer->outputs.main;
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

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    buildSignalGraph(context, &status);

    host.Start();

    while (1) {
        UpdateOled();
    }
}
