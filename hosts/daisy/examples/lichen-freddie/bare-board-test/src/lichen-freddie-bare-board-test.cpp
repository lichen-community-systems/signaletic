#include "daisy.h"
#include <libsignaletic.h>
#include <array>
#include "../../../../include/signaletic-daisy-host.h"
#include "../../../../include/lichen-freddie-device.hpp"
#include "../include/ring-buffer.hpp"

#define SAMPLERATE 48000
#define HEAP_SIZE 1024 * 384 // 384 KB
#define MAX_NUM_SIGNALS 32
#define NUM_CONTROLS 8
#define MIDI_QUEUE_SIZE 128
#define MIDI_MESSAGE_SIZE 3
#define SMOOTH_COEFFICIENT 0.0001f

const uint8_t sliderToCC[NUM_CONTROLS] = {0, 1, 2, 3, 4, 5, 6, 7};
const uint8_t buttonToCC[NUM_CONTROLS] = {48, 49, 50, 51, 52, 53, 54, 55};

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

lichen::freddie::FreddieDevice device;
MidiUsbHandler usbMIDI;
MidiUartHandler trsMIDI;

sig::libdaisy::Toggle buttons[NUM_CONTROLS];
sig::libdaisy::GPIOOutput leds[NUM_CONTROLS];
float previousButtonValues[NUM_CONTROLS] = {0.0f};
uint8_t previousSliderMIDIValues[NUM_CONTROLS] = {0};
bool buttonState[NUM_CONTROLS] = {false};
sig::RingBuffer<std::array<uint8_t, MIDI_MESSAGE_SIZE>, MIDI_QUEUE_SIZE>
     midiEvents;
struct sig_filter_Smooth sliderFilters[NUM_CONTROLS];

void enqueueMIDIMessage(std::array<uint8_t, 3> msg) {
    midiEvents.write(msg);
}

void sendMIDIMessage(std::array<uint8_t, 3> msg) {
    usbMIDI.SendMessage(msg.data(), MIDI_MESSAGE_SIZE);
    trsMIDI.SendMessage(msg.data(), MIDI_MESSAGE_SIZE);
}

void midiCCEvent(uint8_t ccNum, uint8_t value, uint8_t channel = 0) {
    std::array<uint8_t, MIDI_MESSAGE_SIZE> msg = {0};
    msg[0] = (channel & 0x0F) + 0xB0;
    msg[1] = ccNum & 0x7F;
    msg[2] = value & 0x7F;
    enqueueMIDIMessage(msg);
}

void midiNoteEvent(bool isNoteOn, uint8_t note, uint8_t velocity,
    uint8_t channel = 0) {
    std::array<uint8_t, MIDI_MESSAGE_SIZE> msg = {0};
    msg[0] = (channel & 0x0F) + (isNoteOn ? 0x90 : 0x80);
    msg[1] = note & 0x7F;
    msg[2] = velocity & 0x7F;
    enqueueMIDIMessage(msg);
}

inline void processSliders(size_t control) {
    float sliderValue = device.hardware.adcChannels[control];
    float previousSliderMIDIValue = previousSliderMIDIValues[control];
    float smoothedSliderValue = sig_filter_Smooth_generate(
        &sliderFilters[control], sliderValue);
    uint8_t sliderMIDIValue = static_cast<uint8_t>(
        roundf(smoothedSliderValue * 127));

    if (sliderMIDIValue != previousSliderMIDIValue) {
        midiCCEvent(sliderToCC[control], sliderMIDIValue);
    }
    previousSliderMIDIValues[control] = sliderMIDIValue;
}

inline void processLEDs(size_t control) {
    device.hardware.gpioOutputs[control] = static_cast<float>(
            buttonState[control]);
}

inline void processButtons(size_t control) {
    // Read button presses.
    float buttonValue = device.hardware.toggles[control];
    float previousButtonValue = previousButtonValues[control];
    if (buttonValue != previousButtonValue) {
        // The button's state has changed (either pressed or released).
        if (buttonValue > 0.0f && previousButtonValue <= 0.0f) {
            // The button has been pressed. Toggle the LED.
            buttonState[control] = !buttonState[control];
        }

        midiCCEvent(buttonToCC[control], buttonValue <= 0 ? 0 : 127);
    }
    previousButtonValues[control] = buttonValue;
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {

    evaluator->evaluate((struct sig_dsp_SignalEvaluator*) evaluator);

    device.Read();

    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        processButtons(i);
        processLEDs(i);
        processSliders(i);
    }

    device.Write();
}

void processMIDIEvents() {
    size_t eventsToRead = midiEvents.readable();
    for (size_t i = 0; i < eventsToRead; i++) {
        sendMIDIMessage(midiEvents.read());
    }
}

void initMIDI() {
    MidiUsbHandler::Config usbConfig;
    usbConfig.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    usbMIDI.Init(usbConfig);

    MidiUartHandler::Config trsConfig;
    trsConfig.transport_config.tx = sig::libdaisy::seed::PIN_D13;
    trsConfig.transport_config.rx = sig::libdaisy::seed::PIN_NONE;
    trsConfig.transport_config.periph =
        UartHandler::Config::Peripheral::USART_1;
    trsMIDI.Init(trsConfig);

    midiEvents.Init();
}

void initSliderFilters() {
    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        sig_filter_Smooth_init(&sliderFilters[i], SMOOTH_COEFFICIENT);
    }
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
    device.Init(&audioSettings);
    initMIDI();
    initSliderFilters();
    device.Start();
    device.board.audio.Start(AudioCallback);

    while (1) {
        processMIDIEvents();
    }
}
