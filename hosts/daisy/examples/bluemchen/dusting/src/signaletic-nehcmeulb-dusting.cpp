#include "../../../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libsignaletic.h>
#include "../../../../include/dust-gate.h"
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter densityKnob;
Parameter durationKnob;

FixedCapStr<20> displayStr;

#define SIGNAL_HEAP_SIZE 1024 * 384

char heapMemory[SIGNAL_HEAP_SIZE];

struct sig_AllocatorHeap heap = {
    .length = SIGNAL_HEAP_SIZE,
    .memory = heapMemory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

struct sig_dsp_Value* density;
struct sig_dsp_Value* duration;
struct sig_dsp_ClockFreqDetector* clockFreq;
struct sig_dsp_BinaryOp* densityClockSum;
struct cc_sig_DustGate* cvDustGate;
struct sig_dsp_BinaryOp* audioDensityMultiplier;
struct cc_sig_DustGate* audioDustGate;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Dust Dust");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Den: ");
    displayStr.AppendFloat(density->outputs.main[0], 3);
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Dur: ");
    displayStr.AppendFloat(
        cvDustGate->densityDurationMultiplier->outputs.main[0], 3);
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Clk: ");
    displayStr.AppendFloat(clockFreq->outputs.main[0], 3);
    bluemchen.display.SetCursor(0, 24);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    bluemchen.display.Update();
}

void UpdateControls() {
    bluemchen.encoder.Debounce();
    densityKnob.Process();
    durationKnob.Process();
}

void EvaluateSignalGraph() {
    density->parameters.value = densityKnob.Value();
    density->signal.generate(density);
    duration->parameters.value = durationKnob.Value();
    duration->signal.generate(duration);
    clockFreq->signal.generate(clockFreq);
    densityClockSum->signal.generate(densityClockSum);
    cvDustGate->signal.generate(cvDustGate);
    audioDensityMultiplier->signal.generate(audioDensityMultiplier);
    audioDustGate->signal.generate(audioDustGate);
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    UpdateControls();

    // TODO: Need to implement a Host-provided Input signal.
    clockFreq->inputs.source = (float_array_ptr) in[0];

    // TODO: Add a CV input for controlling density,
    // which is centred on 0.0 and allows for negative values
    // so that an incoming clock can be slowed down.

    EvaluateSignalGraph();

    bluemchen.seed.dac.WriteValue(daisy::DacHandle::Channel::BOTH,
        sig_unipolarToUint12(cvDustGate->outputs.main[0]));

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = audioDustGate->outputs.main[i];
        out[1][i] = audioDustGate->outputs.main[i];
    }
}

void initControls() {
    densityKnob.Init(bluemchen.controls[bluemchen.CTRL_1],
        1.0f/60.0f, 20.0f, Parameter::LINEAR);
    durationKnob.Init(bluemchen.controls[bluemchen.CTRL_2],
        0, 1, Parameter::LINEAR);
}

int main(void) {
    bluemchen.Init();
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = bluemchen.AudioSampleRate(),
        .numChannels = 2,
        .blockSize = 2
    };

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);

    bluemchen.SetAudioBlockSize(audioSettings.blockSize);
    bluemchen.StartAdc();
    initControls();

    density = sig_dsp_Value_new(&allocator, context);
    density->parameters.value = 1.0f;
    duration = sig_dsp_Value_new(&allocator, context);
    duration->parameters.value = 1.0f;

    clockFreq = sig_dsp_ClockFreqDetector_new(&allocator, context);
    // TODO: Creat an AudioInput signal for Daisy. In the meantime,
    // this creates an empty buffer that will be written into directly
    // in the audio callback.
    clockFreq->inputs.source = sig_AudioBlock_newWithValue(&allocator,
        &audioSettings, 0.0f);

    densityClockSum = sig_dsp_Add_new(&allocator, context);
    densityClockSum->inputs.left = clockFreq->outputs.main;
    densityClockSum->inputs.right = density->outputs.main;

    struct cc_sig_DustGate_Inputs cvInputs = {
        density: densityClockSum->outputs.main,
        durationPercentage: duration->outputs.main
    };

    cvDustGate = cc_sig_DustGate_new(&allocator, context, cvInputs);

    audioDensityMultiplier = sig_dsp_Mul_new(&allocator, context);
    audioDensityMultiplier->inputs.left = densityClockSum->outputs.main;
    audioDensityMultiplier->inputs.right = sig_AudioBlock_newWithValue(
        &allocator, &audioSettings, 200.0f);

    struct cc_sig_DustGate_Inputs audioInputs = {
        density: audioDensityMultiplier->outputs.main,
        durationPercentage: duration->outputs.main
    };

    audioDustGate = cc_sig_DustGate_new(&allocator,
        context, audioInputs);
    audioDustGate->dust->parameters.bipolar = 1.0f;
    audioDustGate->gate->parameters.bipolar = 1.0f;

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
