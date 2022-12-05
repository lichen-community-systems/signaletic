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

Bluemchen bluemchen;
struct sig_daisy_Host* host;

struct sig_dsp_ConstantValue* smoothCoefficient;
struct sig_daisy_CVIn* coarseFreqKnob;
struct sig_dsp_OnePole* coarseLPF;
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

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals = {
    .items = (void**) &listStorage,
    .capacity = MAX_NUM_SIGNALS,
    .length = 0
};

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

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    // Evaluate the signal graph.
    sig_dsp_generateSignals(&signals);

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = out[1][i] = gain->outputs.main[i];
    }
}


int main(void) {
    allocator.impl->init(&allocator);
    struct sig_Status status;
    sig_Status_init(&status);

    bluemchen.Init();
    host = sig_daisy_BluemchenHost_new(&allocator, &bluemchen);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = bluemchen.AudioSampleRate(),
        .numChannels = 1,
        .blockSize = 1
    };

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);

    bluemchen.SetAudioBlockSize(audioSettings.blockSize);
    bluemchen.StartAdc();

    /** Frequency controls **/
    // Bluemchen AnalogControls are all unipolar,
    // so they need to be scaled to bipolar values.

    smoothCoefficient = sig_dsp_ConstantValue_new(&allocator, context, 0.01);

    coarseFreqKnob = sig_daisy_CVIn_new(&allocator, context, host);
    sig_List_append(&signals, coarseFreqKnob, &status);
    coarseFreqKnob->parameters.control = bluemchen.CTRL_1;
    coarseFreqKnob->parameters.scale = 10.0f;
    coarseFreqKnob->parameters.offset = -5.0f;

    coarseLPF = sig_dsp_OnePole_new(&allocator, context);
    sig_List_append(&signals, coarseLPF, &status);
    coarseLPF->inputs.coefficient = smoothCoefficient->outputs.main;
    coarseLPF->inputs.source = coarseFreqKnob->outputs.main;

    fineFreqKnob = sig_daisy_CVIn_new(&allocator, context, host);
    sig_List_append(&signals, fineFreqKnob, &status);
    fineFreqKnob->parameters.control = bluemchen.CTRL_2;
    fineFreqKnob->parameters.offset = -0.5f;

    fineLPF = sig_dsp_OnePole_new(&allocator, context);
    sig_List_append(&signals, fineLPF, &status);
    fineLPF->inputs.coefficient = smoothCoefficient->outputs.main;
    fineLPF->inputs.source = fineFreqKnob->outputs.main;

    vOctCVIn = sig_daisy_CVIn_new(&allocator, context, host);
    sig_List_append(&signals, vOctCVIn, &status);
    vOctCVIn->parameters.control = bluemchen.CTRL_4;
    vOctCVIn->parameters.scale = 10.0f;
    vOctCVIn->parameters.offset = -5.0f;

    coarsePlusVOct = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, coarsePlusVOct, &status);
    coarsePlusVOct->inputs.left = coarseLPF->outputs.main;
    coarsePlusVOct->inputs.right = vOctCVIn->outputs.main;

    coarseVOctPlusFine = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, coarseVOctPlusFine, &status);
    coarseVOctPlusFine->inputs.left = coarsePlusVOct->outputs.main;
    coarseVOctPlusFine->inputs.right = fineLPF->outputs.main;

    frequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, frequency, &status);
    frequency->inputs.source = coarseVOctPlusFine->outputs.main;

    ampMod = sig_dsp_ConstantValue_new(&allocator, context, 1.0f);
    osc = sig_dsp_Sine_new(&allocator, context);
    sig_List_append(&signals, osc, &status);
    osc->inputs.freq = frequency->outputs.main;
    osc->inputs.mul = ampMod->outputs.main;

    /** Gain **/
    // Bluemchen's output circuit clips as it approaches full gain,
    // so 0.85 seems to be around the practical maximum value.
    gainLevel = sig_dsp_ConstantValue_new(&allocator, context, 0.85f);
    gain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, gain, &status);
    gain->inputs.left = osc->outputs.main;
    gain->inputs.right = gainLevel->outputs.main;
    sig_List_append(&signals, gain, &status);

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
