#include "../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libsignaletic.h>
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter knob1;
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
    freqMod->parameters.value = sig_midiToFreq(cv2.Value());

    // Evaluate the signal graph.
    ampMod->signal.generate(ampMod);
    freqMod->signal.generate(freqMod);
    carrier->signal.generate(carrier);
    gainValue->signal.generate(gainValue);
    gain->signal.generate(gain);

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = gain->signal.output[i];
        out[1][i] = gain->signal.output[i];
    }
}

void initControls() {
    knob1.Init(bluemchen.controls[bluemchen.CTRL_1],
        0.0f, 0.85f, Parameter::LINEAR);
    cv1.Init(bluemchen.controls[bluemchen.CTRL_3],
        -1.0f, 1.0f, Parameter::LINEAR);
    // Scale CV frequency input to MIDI note numbers.
    cv2.Init(bluemchen.controls[bluemchen.CTRL_4],
        0, 120.0f, Parameter::LINEAR);
}

int main(void) {
    bluemchen.Init();
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = bluemchen.AudioSampleRate(),
        .numChannels = 1,
        .blockSize = 1
    };

    bluemchen.SetAudioBlockSize(audioSettings.blockSize);
    bluemchen.StartAdc();
    initControls();

    /** Modulators **/
    freqMod = sig_dsp_Value_new(&allocator, &audioSettings);
    freqMod->parameters.value = 440.0f;
    ampMod = sig_dsp_Value_new(&allocator, &audioSettings);
    ampMod->parameters.value = 1.0f;

    /** Carrier **/
    struct sig_dsp_Oscillator_Inputs carrierInputs = {
        .freq = freqMod->signal.output,
        .phaseOffset = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f),
        .mul = ampMod->signal.output,
        .add = sig_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f),
    };

    carrier = sig_dsp_Sine_new(&allocator, &audioSettings,
        &carrierInputs);

    /** Gain **/
    // Bluemchen's output circuit clips as it approaches full gain,
    // so 0.85 seems to be around the practical maximum value.
    gainValue = sig_dsp_Value_new(&allocator, &audioSettings);
    gainValue->parameters.value = 0.85f;

    struct sig_dsp_BinaryOp_Inputs gainInputs = {
        .left = carrier->signal.output,
        .right = gainValue->signal.output
    };
    gain = sig_dsp_Mul_new(&allocator, &audioSettings,
        &gainInputs);

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
