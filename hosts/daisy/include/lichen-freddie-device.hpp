#pragma once

#include "signaletic-host.h"
#include "signaletic-daisy-host.hpp"
#include "sig-daisy-seed.hpp"

using namespace sig::libdaisy;

enum {
    sig_host_KNOB_1 = 0,
    sig_host_KNOB_2,
    sig_host_KNOB_3,
    sig_host_KNOB_4,
    sig_host_KNOB_5,
    sig_host_KNOB_6,
    sig_host_KNOB_7,
    sig_host_KNOB_8
};

namespace lichen {
namespace freddie {
    static const size_t NUM_CONTROLS = 8;

    static ADCChannelSpec ADC_CHANNEL_SPECS[NUM_CONTROLS] = {
        {seed::PIN_ADC_0, INVERT},
        {seed::PIN_ADC_1, INVERT},
        {seed::PIN_ADC_2, INVERT},
        {seed::PIN_ADC_3, INVERT},
        {seed::PIN_ADC_4, INVERT},
        {seed::PIN_ADC_5, INVERT},
        {seed::PIN_ADC_6, INVERT},
        {seed::PIN_ADC_7, INVERT}
    };

    static dsy_gpio_pin BUTTON_PINS[NUM_CONTROLS] = {
        seed::PIN_D0,
        seed::PIN_D1,
        seed::PIN_D2,
        seed::PIN_D3,
        seed::PIN_D4,
        seed::PIN_D5,
        seed::PIN_D6,
        seed::PIN_D7
    };

    static dsy_gpio_pin LED_PINS[] = {
        seed::PIN_D8,
        seed::PIN_D9,
        seed::PIN_D10,
        seed::PIN_D29,
        seed::PIN_D30,
        seed::PIN_D25,
        seed::PIN_D26,
        seed::PIN_D27
    };

    class FreddieDevice {
        public:
            seed::SeedBoard board;
            ADCController<UnipolarAnalogInput, NUM_CONTROLS> adcController;
            Toggle buttons[NUM_CONTROLS];
            InputBank<Toggle, NUM_CONTROLS> buttonBank;
            GPIOOutput leds[NUM_CONTROLS];
            OutputBank<GPIOOutput, NUM_CONTROLS> ledBank;
            struct sig_host_HardwareInterface hardware;

        void Init(struct sig_AudioSettings* audioSettings) {
            board.Init(audioSettings->blockSize, audioSettings->sampleRate);
            InitADCController();
            InitButtons();
            InitLEDs();

            hardware = {
                .numAudioInputChannels = 0,
                .audioInputChannels = NULL,
                .numAudioOutputChannels = 0,
                .audioOutputChannels = NULL,
                .numADCChannels = NUM_CONTROLS,
                .adcChannels = adcController.channelBank.values,
                .numDACChannels = 0,
                .dacChannels = NULL,
                .numGateInputs = 0,
                .gateInputs = NULL,
                .numGPIOOutputs = NUM_CONTROLS,
                .gpioOutputs = ledBank.values,
                .numToggles = NUM_CONTROLS,
                .toggles = buttonBank.values,
                .numTriSwitches = 0,
                .triSwitches = NULL
            };
        }

        void InitADCController() {
            adcController.Init(&board.adc, ADC_CHANNEL_SPECS);
        }

        void InitButtons() {
            for (size_t i = 0; i < NUM_CONTROLS; i++) {
                buttons[i].Init(BUTTON_PINS[i]);
            }

            buttonBank.inputs = buttons;
        }

        void InitLEDs() {
            for (size_t i = 0; i < NUM_CONTROLS; i++) {
                leds[i].Init(LED_PINS[i], DSY_GPIO_MODE_OUTPUT_PP,
                    DSY_GPIO_NOPULL);
            }

            ledBank.outputs = leds;
        }

        void Start() {
            adcController.Start();
        }

        void Stop () {
            adcController.Stop();
        }

        inline void Read() {
            adcController.Read();
            buttonBank.Read();
        }

        inline void Write() {
            ledBank.Write();
        }
    };
};
};
