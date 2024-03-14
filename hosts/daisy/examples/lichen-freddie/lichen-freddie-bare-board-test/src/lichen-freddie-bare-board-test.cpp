#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../include/signaletic-daisy-host.h"
#include "../../../../include/sig-daisy-seed.hpp"
#include "dev/oled_ssd130x.h"

#define SAMPLERATE 48000
#define HEAP_SIZE 1024 * 384 // 384 KB
#define MAX_NUM_SIGNALS 32
#define NUM_CONTROLS 8

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
MidiUsbHandler usbMIDI;
MidiUartHandler trsMIDI;
struct sig_daisy_Host* host;

sig::libdaisy::AnalogInput sliders[NUM_CONTROLS];
sig::libdaisy::Toggle buttons[NUM_CONTROLS];
sig::libdaisy::GPIOOutput leds[NUM_CONTROLS];
uint8_t midiNotes[NUM_CONTROLS] = {0};
bool midiState[NUM_CONTROLS] = {false};
bool previousMidiState[NUM_CONTROLS] = {false};

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {

    evaluator->evaluate((struct sig_dsp_SignalEvaluator*) evaluator);

    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        float buttonValue = buttons[i].Value();
        previousMidiState[i] = midiState[i];
        midiState[i] = (bool) buttonValue;
        leds[i].Write(buttonValue);
        // TODO: Need smoothing to avoid stuck notes.
        midiNotes[i] = sliders[i].Value() * 127;
    }

    for (size_t i = 0; i < size; i++) {
        out[0][i] = 0;
        out[1][i] = 0;
    }
}

void fireMidiNoteEvent(bool isNoteOn, uint8_t note, uint8_t velocity) {
    uint8_t channel = 0;
    uint8_t data[3] = {0};
    data[0] = (channel & 0x0F) + (isNoteOn ? 0x90 : 0x80);
    data[1] = note & 0x7F;
    data[2] = velocity & 0x7F;
    usbMIDI.SendMessage(data, 3);
    trsMIDI.SendMessage(data, 3);
}

void initMIDI() {
    MidiUsbHandler::Config usbConfig;
    usbConfig.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    usbMIDI.Init(usbConfig);

    MidiUartHandler::Config trsConfig;
    trsConfig.transport_config.tx = sig::libdaisy::SEED_PIN_D13;
    trsConfig.transport_config.rx = sig::libdaisy::SEED_PIN_NONE;
    trsConfig.transport_config.periph =
        UartHandler::Config::Peripheral::USART_1;
    trsMIDI.Init(trsConfig);
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

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    board.Init(audioSettings.blockSize, audioSettings.sampleRate);

    dsy_gpio_pin adcPins[] = {
        sig::libdaisy::SEED_PIN_ADC_0,
        sig::libdaisy::SEED_PIN_ADC_1,
        sig::libdaisy::SEED_PIN_ADC_2,
        sig::libdaisy::SEED_PIN_ADC_3,
        sig::libdaisy::SEED_PIN_ADC_4,
        sig::libdaisy::SEED_PIN_ADC_5,
        sig::libdaisy::SEED_PIN_ADC_6,
        sig::libdaisy::SEED_PIN_ADC_7,
        sig::libdaisy::SEED_PIN_ADC_8
    };

    board.InitADC(adcPins, NUM_CONTROLS);
    board.adc.Start();

    dsy_gpio_pin buttonPins[] = {
        sig::libdaisy::SEED_PIN_D0,
        sig::libdaisy::SEED_PIN_D1,
        sig::libdaisy::SEED_PIN_D2,
        sig::libdaisy::SEED_PIN_D3,
        sig::libdaisy::SEED_PIN_D4,
        sig::libdaisy::SEED_PIN_D5,
        sig::libdaisy::SEED_PIN_D6,
        sig::libdaisy::SEED_PIN_D7
    };

    dsy_gpio_pin ledPins[] = {
        sig::libdaisy::SEED_PIN_D8,
        sig::libdaisy::SEED_PIN_D9,
        sig::libdaisy::SEED_PIN_D10,
        sig::libdaisy::SEED_PIN_D11,
        sig::libdaisy::SEED_PIN_D12,
        sig::libdaisy::SEED_PIN_D25,
        sig::libdaisy::SEED_PIN_D26,
        sig::libdaisy::SEED_PIN_D27
    };

    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        leds[i].Init(ledPins[i], DSY_GPIO_MODE_OUTPUT_PP, DSY_GPIO_NOPULL);
        sliders[i].Init(&board.adc, i);
        buttons[i].Init(buttonPins[i]);
    }

    initMIDI();

    board.audio.Start(AudioCallback);

    uint32_t lastMidiTick = System::GetUs();

    while (1) {
        uint32_t now = System::GetUs();
        if (now - lastMidiTick >= 1000) {
            for (size_t i = 0; i < NUM_CONTROLS; i++) {
                bool noteState = midiState[i];
                if (noteState != previousMidiState[i]) {
                    fireMidiNoteEvent(noteState, midiNotes[i], 127);
                }
            }
            lastMidiTick = now;
        }
    }
}
