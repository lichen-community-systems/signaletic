#include "../../../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libsignaletic.h>
#include <string>

using namespace kxmx;
using namespace daisy;

FixedCapStr<20> displayStr;

#define HEAP_SIZE 1024 * 256 // 256KB
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
Parameter freqCoarseKnob;
Parameter freqFineKnob;
Parameter vOctCV;

struct sig_dsp_Value* freqMod;
struct sig_dsp_ConstantValue* ampMod;
struct sig_dsp_Oscillator* osc;
struct sig_dsp_ConstantValue* gainLevel;
struct sig_dsp_BinaryOp* gain;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Starscill");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.AppendFloat(freqMod->parameters.value, 1);
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

    // Bind control values to Signals.
    // TODO: This should be handled by a Host-provided Signal (gh-22).
    // TODO: Replace this with in-graph math.
    // TODO: Add LPF smoothing for the rather glitchy knobs.
    float vOct = freqCoarseKnob.Process() + vOctCV.Process() +
        freqFineKnob.Process();

    // TODO: Make a v/Oct Signal
    freqMod->parameters.value = sig_linearToFreq(vOct, sig_FREQ_C4);

    // Evaluate the signal graph.
    ampMod->signal.generate(ampMod);
    freqMod->signal.generate(freqMod);
    osc->signal.generate(osc);
    gain->signal.generate(gain);

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = gain->outputs.main[i];
        out[1][i] = gain->outputs.main[i];
    }
}

void initControls() {
    // FIXME: More than +5 octaves causes wild results when sweeping
    // the coarse frequency knob. Why?
    freqCoarseKnob.Init(bluemchen.controls[bluemchen.CTRL_1],
        -5.0f, 5.0f, Parameter::LINEAR);
    freqFineKnob.Init(bluemchen.controls[bluemchen.CTRL_2],
        -0.5f, 0.5f, Parameter::LINEAR);
    vOctCV.Init(bluemchen.controls[bluemchen.CTRL_4],
        -5.0f, 5.0f, Parameter::LINEAR);
}

int main(void) {
    bluemchen.Init();
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = bluemchen.AudioSampleRate(),
        .numChannels = 1,
        .blockSize = 1
    };

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);

    bluemchen.SetAudioBlockSize(audioSettings.blockSize);
    bluemchen.StartAdc();
    initControls();

    /** Modulation **/
    freqMod = sig_dsp_Value_new(&allocator, context);
    freqMod->parameters.value = sig_FREQ_C4;
    ampMod = sig_dsp_ConstantValue_new(&allocator, context, 1.0f);

    osc = sig_dsp_Sine_new(&allocator, context);
    osc->inputs.freq = freqMod->outputs.main;
    osc->inputs.mul = ampMod->outputs.main;

    /** Gain **/
    // Bluemchen's output circuit clips as it approaches full gain,
    // so 0.85 seems to be around the practical maximum value.
    gainLevel = sig_dsp_ConstantValue_new(&allocator, context, 0.85f);
    gain = sig_dsp_Mul_new(&allocator, context);
    gain->inputs.left = osc->outputs.main;
    gain->inputs.right = gainLevel->outputs.main;

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
