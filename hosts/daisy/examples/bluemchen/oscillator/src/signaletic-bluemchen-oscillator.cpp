#include "../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libsignaletic.h>
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter knob1;
Parameter knob2;
Parameter cv1;
Parameter cv2;

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

struct sig_dsp_Value* freqMod;
struct sig_dsp_Value* ampMod;
struct sig_dsp_Oscillator* carrier;
struct sig_dsp_Value* gainValue;
struct sig_dsp_BinaryOp* gain;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Starscill");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("A: ");
    displayStr.AppendFloat(cv1.Value(), 3);
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("F: ");
    displayStr.AppendFloat(cv2.Value(), 3);
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    bluemchen.display.Update();
}

void UpdateControls() {
    knob1.Process();
    knob2.Process();
    cv1.Process();
    cv2.Process();
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    UpdateControls();

    // Bind control values to Signals.
    // TODO: This should be handled by a
    // Host-provided Signal (gh-22).
    gainValue->parameters.value = knob1.Value();
    ampMod->parameters.value = cv1.Value();

    // TODO: Make a MidiFrequency Signal
    // and then replace this with in-graph math.
    // TODO: Treating the knob as an attenuator doesn't really
    // have the effect we want. Instead, the CV and knob values
    // should be added.
    freqMod->parameters.value = sig_midiToFreq(cv2.Value()) * knob2.Value();

    // Evaluate the signal graph.
    ampMod->signal.generate(ampMod);
    freqMod->signal.generate(freqMod);
    carrier->signal.generate(carrier);
    gainValue->signal.generate(gainValue);
    gain->signal.generate(gain);

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = gain->outputs.main[i];
        out[1][i] = gain->outputs.main[i];
    }
}

void initControls() {
    knob1.Init(bluemchen.controls[bluemchen.CTRL_1],
        0.0f, 0.85f, Parameter::LINEAR);
    cv1.Init(bluemchen.controls[bluemchen.CTRL_3],
        -1.0f, 1.0f, Parameter::LINEAR);

    knob2.Init(bluemchen.controls[bluemchen.CTRL_2],
        0.0f, 1.0f, Parameter::LINEAR);
    // Scale CV input to MIDI note numbers.
    // TODO: But then is this tracking 1V/Oct?
    cv2.Init(bluemchen.controls[bluemchen.CTRL_4],
        0.0f, 120.0f, Parameter::LINEAR);
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

    /** Modulators **/
    freqMod = sig_dsp_Value_new(&allocator, context);
    freqMod->parameters.value = 440.0f;
    ampMod = sig_dsp_Value_new(&allocator, context);
    ampMod->parameters.value = 1.0f;

    carrier = sig_dsp_Sine_new(&allocator, context);
    carrier->inputs.freq = freqMod->outputs.main;
    carrier->inputs.mul = ampMod->outputs.main;

    /** Gain **/
    // Bluemchen's output circuit clips as it approaches full gain,
    // so 0.85 seems to be around the practical maximum value.
    gainValue = sig_dsp_Value_new(&allocator, context);
    gainValue->parameters.value = 0.85f;

    gain = sig_dsp_Mul_new(&allocator, context);
    gain->inputs.left = carrier->outputs.main;
    gain->inputs.right = gainValue->outputs.main;

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
