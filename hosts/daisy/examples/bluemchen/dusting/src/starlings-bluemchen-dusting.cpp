#include "../../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include <tlsf.h>
#include <libstar.h>
#include "../../../../include/cc-signals.h"
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter densityKnob;
Parameter durationKnob;

FixedCapStr<20> displayStr;

#define SIGNAL_HEAP_SIZE 1024 * 384
char signalHeap[SIGNAL_HEAP_SIZE];
struct star_Allocator allocator = {
    .heapSize = SIGNAL_HEAP_SIZE,
    .heap = (void*) signalHeap
};

struct star_sig_Value* density;
struct star_sig_Value* duration;
struct star_sig_TempoClockDetector* clockFreq;
struct star_sig_BinaryOp* densityClockSum;
struct cc_sig_DustGate* cvDustGate;
struct star_sig_BinaryOp* audioDensityMultiplier;
struct cc_sig_DustGate* audioDustGate;

void UpdateOled() {
    bluemchen.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Dust Dust");
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Den: ");
    displayStr.AppendFloat(density->signal.output[0], 3);
    bluemchen.display.SetCursor(0, 8);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Dur: ");
    displayStr.AppendFloat(cvDustGate->densityDurationMultiplier->signal.output[0], 3);
    bluemchen.display.SetCursor(0, 16);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("Clk: ");
    displayStr.AppendFloat(clockFreq->signal.output[0], 3);
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
    clockFreq->inputs->source = (float_array_ptr) in[0];

    EvaluateSignalGraph();

    bluemchen.seed.dac.WriteValue(daisy::DacHandle::Channel::BOTH,
        star_unipolarToUint12(cvDustGate->gate->signal.output[0]));

    // Copy mono buffer to stereo output.
    for (size_t i = 0; i < size; i++) {
        out[0][i] = audioDustGate->gate->signal.output[i];
        out[1][i] = audioDustGate->dust->signal.output[i];
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
    star_Allocator_init(&allocator);

    struct star_AudioSettings audioSettings = {
        .sampleRate = bluemchen.AudioSampleRate(),
        .numChannels = 2,
        .blockSize = 2
    };

    bluemchen.SetAudioBlockSize(audioSettings.blockSize);
    bluemchen.StartAdc();
    initControls();

    density = star_sig_Value_new(&allocator, &audioSettings);
    density->parameters.value = 1.0f;
    duration = star_sig_Value_new(&allocator, &audioSettings);
    duration->parameters.value = 1.0f;

    struct star_sig_TempoClockDetector_Inputs clockFreqInputs = {
        .source = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 0.0f)
    };
    clockFreq = star_sig_TempoClockDetector_new(&allocator, &audioSettings,
        &clockFreqInputs);

    struct star_sig_BinaryOp_Inputs densityClockSumInputs = {
        .left = clockFreq->signal.output,
        .right = density->signal.output
    };
    densityClockSum = star_sig_Add_new(&allocator, &audioSettings,
        &densityClockSumInputs);

    struct cc_sig_DustGate_Inputs cvInputs = {
        density: densityClockSum->signal.output,
        durationPercentage: duration->signal.output
    };

    cvDustGate = cc_sig_DustGate_new(&allocator,
        &audioSettings, &cvInputs);

    struct star_sig_BinaryOp_Inputs audioDensityMuliplierInputs = {
        .left = densityClockSum->signal.output,
        .right = star_AudioBlock_newWithValue(&allocator,
            &audioSettings, 200.0f),
    };
    audioDensityMultiplier = star_sig_Mul_new(&allocator, &audioSettings,
        &audioDensityMuliplierInputs);

    struct cc_sig_DustGate_Inputs audioInputs = {
        density: audioDensityMultiplier->signal.output,
        durationPercentage: duration->signal.output
    };

    audioDustGate = cc_sig_DustGate_new(&allocator,
        &audioSettings, &audioInputs);
    audioDustGate->dust->parameters.bipolar = 1.0f;
    audioDustGate->gate->parameters.bipolar = 1.0f;

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
