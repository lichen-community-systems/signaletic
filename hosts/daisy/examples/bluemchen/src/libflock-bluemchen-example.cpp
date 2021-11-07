#include "../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"
#include "../../../../libflock/include/libflock.h"
#include <string>

using namespace kxmx;
using namespace daisy;

Bluemchen bluemchen;
Parameter knob1;
Parameter cv1;
Parameter cv2;

flock_sig_Value freqMod;
flock_sig_Value ampMod;
flock_sig_Sine carrier;
flock_sig_Value gainValue;
flock_sig_Gain gain;

void UpdateOled() {
    bluemchen.display.Fill(false);
    FixedCapStr<20> displayStr;

    displayStr.Append("A: ");
    displayStr.AppendFloat(cv1.Value(), 3);
    bluemchen.display.SetCursor(0, 0);
    bluemchen.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("F: ");
    displayStr.AppendFloat(cv2.Value(), 3);
    bluemchen.display.SetCursor(0, 8);
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

    gainValue.parameters.value = knob1.Value();
    ampMod.parameters.value = cv1.Value();
    ampMod.signal.generate(&ampMod);
    freqMod.parameters.value = flock_midiToFreq(cv2.Value());
    freqMod.signal.generate(&freqMod);
    carrier.signal.generate(&carrier);

    gainValue.signal.generate(&gainValue);
    gain.signal.generate(&gain);

    for (size_t i = 0; i < size; i++) {
        out[0][i] = gain.signal.output[i];
        out[1][i] = gain.signal.output[i];
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

    struct flock_AudioSettings audioSettings = {
        .sampleRate = bluemchen.AudioSampleRate(),
        .numChannels = 1,
        .blockSize = 1
    };

    bluemchen.SetAudioBlockSize((size_t) audioSettings.blockSize);
    bluemchen.StartAdc();
    initControls();

    /**************
     * Modulators *
     **************/

    float freqModOutput[audioSettings.blockSize];
    flock_Buffer_fillSilence(freqModOutput, audioSettings.blockSize);
    flock_sig_Value_init(&freqMod, &audioSettings, freqModOutput);
    freqMod.parameters.value = 440.0f;

    float ampModOutput[audioSettings.blockSize];
    flock_Buffer_fillSilence(ampModOutput, audioSettings.blockSize);
    // float ampModFreq[audioSettings.blockSize];
    // flock_Buffer_fill(1.0f, ampModFreq, audioSettings.blockSize);
    // float ampModPhaseOffset[audioSettings.blockSize];
    // flock_Buffer_fill(0.0f, ampModPhaseOffset, audioSettings.blockSize);
    // float ampModMul[audioSettings.blockSize];
    // flock_Buffer_fill(1.0f, ampModMul, audioSettings.blockSize);
    // float ampModAdd[audioSettings.blockSize];
    // flock_Buffer_fill(0.0f, ampModAdd, audioSettings.blockSize);
    // struct flock_sig_Sine_Inputs ampModInputs = {
    //     .freq = ampModFreq,
    //     .phaseOffset = ampModPhaseOffset,
    //     .mul = ampModMul,
    //     .add = ampModAdd
    // };
    // flock_sig_Sine_init(&ampMod, &audioSettings, &ampModInputs, ampModOutput);
    flock_sig_Value_init(&ampMod, &audioSettings, ampModOutput);
    ampMod.parameters.value = 1.0f;

    /****************
     * Sine Carrier *
     ****************/
    float phaseOffset[audioSettings.blockSize];
    flock_Buffer_fill(0.0f, phaseOffset, audioSettings.blockSize);
    float add[audioSettings.blockSize];
    flock_Buffer_fill(0.0f, add, audioSettings.blockSize);

    struct flock_sig_Sine_Inputs carrierInputs = {
        .freq = freqMod.signal.output,
        .phaseOffset = phaseOffset,
        .mul = ampMod.signal.output,
        .add = add
    };

    float sineOutput[audioSettings.blockSize];
    flock_Buffer_fillSilence(sineOutput, audioSettings.blockSize);

    flock_sig_Sine_init(&carrier,
        &audioSettings, &carrierInputs, sineOutput);

    /********
     * Gain *
     * ******/

    // Bluemchen's output circuit clips as it approaches full gain,
    // so 0.85 seems to be around the practical maximum value.
    float gainValueOutput[audioSettings.blockSize];
    flock_Buffer_fillSilence(gainValueOutput, audioSettings.blockSize);
    flock_sig_Value_init(&gainValue, &audioSettings, gainValueOutput);
    gainValue.parameters.value = 0.85f;

    struct flock_sig_Gain_Inputs gainInputs = {
        .gain = gainValue.signal.output,
        .source = carrier.signal.output
    };

    float gainOutput[audioSettings.blockSize];
    flock_Buffer_fillSilence(gainOutput, audioSettings.blockSize);
    flock_sig_Gain_init(&gain, &audioSettings, &gainInputs, gainOutput);

    bluemchen.StartAudio(AudioCallback);

    while (1) {
        UpdateOled();
    }
}
