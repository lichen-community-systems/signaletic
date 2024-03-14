#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../include/signaletic-daisy-host.h"
#include "../../../../include/sig-daisy-seed.hpp"
#include "dev/oled_ssd130x.h"

#define SAMPLERATE 48000
#define HEAP_SIZE 1024 * 384 // 384 KB
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

sig::libdaisy::Seed board;
daisy::OledDisplay<daisy::SSD130xI2c64x32Driver> display;
FixedCapStr<20> displayStr;
struct sig_daisy_Host* host;
sig::libdaisy::AnalogInput knob1;
struct sig_dsp_Value* freq;
sig::libdaisy::GateInput gateIn;
struct sig_dsp_Value* gain;
struct sig_dsp_Oscillator* sine;
struct sig_dsp_Value* switchValue;
struct sig_dsp_ScaleOffset* switchValueScale;
struct sig_dsp_BinaryOp* harmonizerFreqScale;
struct sig_dsp_Oscillator* harmonizer;
sig::libdaisy::Toggle button;
struct sig_dsp_Value* buttonValue;
struct sig_dsp_BinaryOp* mixer;
struct sig_dsp_BinaryOp* attenuator;

void initDisplay() {
    daisy::OledDisplay<daisy::SSD130xI2c64x32Driver>::Config display_config;
    display_config.driver_config.transport_config.Defaults();
    display.Init(display_config);
}

void buildSignalGraph(struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_List* signals,
    struct sig_AudioSettings* audioSettings,
    struct sig_Status* status) {

    freq = sig_dsp_Value_new(allocator, context);
    freq->parameters.value = 220.0f;
    sig_List_append(signals, freq, status);

    buttonValue = sig_dsp_Value_new(allocator, context);
    sig_List_append(signals, buttonValue, status);
    buttonValue->parameters.value = 0.0f;

    switchValue = sig_dsp_Value_new(allocator, context);
    sig_List_append(signals, switchValue, status);
    switchValue->parameters.value = -1.0f;

    switchValueScale = sig_dsp_ScaleOffset_new(allocator, context);
    sig_List_append(signals, switchValueScale, status);
    switchValueScale->inputs.source = switchValue->outputs.main;
    switchValueScale->parameters.scale = 1.25f;
    switchValueScale->parameters.offset = 0.75f;

    harmonizerFreqScale = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, harmonizerFreqScale, status);
    harmonizerFreqScale->inputs.left = freq->outputs.main;
    harmonizerFreqScale->inputs.right = switchValueScale->outputs.main;

    harmonizer = sig_dsp_LFTriangle_new(allocator, context);
    sig_List_append(signals, harmonizer, status);
    harmonizer->inputs.freq = harmonizerFreqScale->outputs.main;
    harmonizer->inputs.mul = buttonValue->outputs.main;

    sine = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, sine, status);
    sine->inputs.freq = freq->outputs.main;

    mixer = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, mixer, status);
    mixer->inputs.left = sine->outputs.main;
    mixer->inputs.right = harmonizer->outputs.main;

    gain = sig_dsp_Value_new(allocator, context);
    gain->parameters.value = 0.5f;
    sig_List_append(signals, gain, status);

    attenuator = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, attenuator, status);
    attenuator->inputs.left = mixer->outputs.main;
    attenuator->inputs.right = gain->outputs.main;
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    freq->parameters.value = 1760.0f * knob1.Value();
    buttonValue->parameters.value = button.Value();

    evaluator->evaluate((struct sig_dsp_SignalEvaluator*) evaluator);

    for (size_t i = 0; i < size; i++) {
        float sig = attenuator->outputs.main[i];
        out[0][i] = sig;
        out[1][i] = sig;
    }
}

void updateOLED() {
    display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Button ");
    displayStr.AppendFloat(button.Value(), 1);
    display.SetCursor(0, 0);
    display.WriteString(displayStr.Cstr(), Font_6x8, true);

    display.Update();
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 1
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    board.Init(audioSettings.blockSize, audioSettings.sampleRate);

    // TODO: Move ADC initialization out of the Init function
    dsy_gpio_pin adcPins[] = {
        sig::libdaisy::SEED_PIN_D16,
        sig::libdaisy::SEED_PIN_D15,
        sig::libdaisy::SEED_PIN_D21,
        sig::libdaisy::SEED_PIN_D18
    };

    board.InitADC(adcPins, 4);
    board.adc.Start();

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(&allocator, context, &signals, &audioSettings, &status);

    knob1.Init(&board.adc, 1);
    // gateIn.Init(sig::libdaisy::PATCH_SM_PIN_B10);
    button.Init(sig::libdaisy::SEED_PIN_D28); // Encoder button.

    board.audio.Start(AudioCallback);

    initDisplay();

    while (1) {
        updateOLED();
    }
}
