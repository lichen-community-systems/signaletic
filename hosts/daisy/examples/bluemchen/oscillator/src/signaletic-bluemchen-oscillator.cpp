#include <tlsf.h>
#include <string>
#include <libsignaletic.h>
#include "../../../../include/daisy-bluemchen-host.h"

using namespace kxmx;
using namespace daisy;

FixedCapStr<20> displayStr;

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

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;

Bluemchen bluemchen;
struct sig_daisy_Host* host;

struct sig_dsp_ConstantValue* smoothCoefficient;
struct sig_daisy_CVIn* coarseFreqKnob;
struct sig_dsp_OnePole* coarseFrequencyLPF;
struct sig_daisy_CVIn* fineFreqKnob;
struct sig_dsp_OnePole* fineLPF;
struct sig_daisy_CVIn* vOctCVIn;
struct sig_dsp_BinaryOp* coarsePlusVOct;
struct sig_dsp_BinaryOp* coarseVOctPlusFine;
struct sig_dsp_LinearToFreq* frequency;
struct sig_dsp_ConstantValue* ampMod;
struct sig_dsp_Oscillator* osc;
struct sig_dsp_ConstantValue* gainLevel;
struct sig_dsp_BinaryOp* gain;
struct sig_daisy_AudioOut* leftOut;
struct sig_daisy_AudioOut* rightOut;


void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Starscill");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.AppendFloat(frequency->outputs.main[0], 1);
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append(" Hz");
    bluemchen.display.SetCursor(46, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    bluemchen.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    smoothCoefficient = sig_dsp_ConstantValue_new(&allocator, context, 0.01);

    /** Frequency controls **/
    // Bluemchen AnalogControls are all unipolar,
    // so they need to be scaled to bipolar values.
    coarseFreqKnob = sig_daisy_CVIn_new(&allocator, context, host);
    sig_List_append(&signals, coarseFreqKnob, status);
    coarseFreqKnob->parameters.control = bluemchen.CTRL_1;
    coarseFreqKnob->parameters.scale = 10.0f;
    coarseFreqKnob->parameters.offset = -5.0f;

    coarseFrequencyLPF = sig_dsp_OnePole_new(&allocator, context);
    sig_List_append(&signals, coarseFrequencyLPF, status);
    coarseFrequencyLPF->inputs.coefficient = smoothCoefficient->outputs.main;
    coarseFrequencyLPF->inputs.source = coarseFreqKnob->outputs.main;

    fineFreqKnob = sig_daisy_CVIn_new(&allocator, context, host);
    sig_List_append(&signals, fineFreqKnob, status);
    fineFreqKnob->parameters.control = bluemchen.CTRL_2;
    fineFreqKnob->parameters.offset = -0.5f;

    fineLPF = sig_dsp_OnePole_new(&allocator, context);
    sig_List_append(&signals, fineLPF, status);
    fineLPF->inputs.coefficient = smoothCoefficient->outputs.main;
    fineLPF->inputs.source = fineFreqKnob->outputs.main;

    vOctCVIn = sig_daisy_CVIn_new(&allocator, context, host);
    sig_List_append(&signals, vOctCVIn, status);
    vOctCVIn->parameters.control = bluemchen.CTRL_4;
    vOctCVIn->parameters.scale = 10.0f;
    vOctCVIn->parameters.offset = -5.0f;

    coarsePlusVOct = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, coarsePlusVOct, status);
    coarsePlusVOct->inputs.left = coarseFrequencyLPF->outputs.main;
    coarsePlusVOct->inputs.right = vOctCVIn->outputs.main;

    coarseVOctPlusFine = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, coarseVOctPlusFine, status);
    coarseVOctPlusFine->inputs.left = coarsePlusVOct->outputs.main;
    coarseVOctPlusFine->inputs.right = fineLPF->outputs.main;

    frequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, frequency, status);
    frequency->inputs.source = coarseVOctPlusFine->outputs.main;

    ampMod = sig_dsp_ConstantValue_new(&allocator, context, 1.0f);
    osc = sig_dsp_Sine_new(&allocator, context);
    sig_List_append(&signals, osc, status);
    osc->inputs.freq = frequency->outputs.main;
    osc->inputs.mul = ampMod->outputs.main;

    /** Gain **/
    // The Daisy Seed's output circuit clips as it approaches full gain.
    // With the original AK556 codec, 0.85 seems to be around the practical
    // maximum value. On the newer model with the WM8731 codec, a lower
    // gain is required. 0.70f seems safe.
    // I haven't noticed an issue like this with modules based
    // on the Patch SM board.
    gainLevel = sig_dsp_ConstantValue_new(&allocator, context, 0.70f);

    gain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, gain, status);
    gain->inputs.left = osc->outputs.main;
    gain->inputs.right = gainLevel->outputs.main;
    sig_List_append(&signals, gain, status);

    leftOut = sig_daisy_AudioOut_new(&allocator, context, host);
    leftOut->parameters.channel = 0;
    leftOut->inputs.source = gain->outputs.main;
    sig_List_append(&signals, leftOut, status);

    rightOut = sig_daisy_AudioOut_new(&allocator, context, host);
    rightOut->parameters.channel = 1;
    rightOut->inputs.source = gain->outputs.main;
    sig_List_append(&signals, rightOut, status);
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 96000,
        .numChannels = 2,
        .blockSize = 1
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

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(context, &status);
    host->impl->start(host);

    while (1) {
        UpdateOled();
    }
}
