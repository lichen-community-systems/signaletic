#pragma once

#include "signaletic-host.h"
#include "signaletic-daisy-host.h"
#include "sig-daisy-seed.hpp"
#include "dev/oled_ssd130x.h"

using namespace sig::libdaisy;

enum {
    sig_host_KNOB_1 = 0,
    sig_host_KNOB_2
};

enum {
    sig_host_CV_IN_1 = 0,
    sig_host_CV_IN_2
};

enum {
    sig_host_AUDIO_IN_1 = 0,
    sig_host_AUDIO_IN_2
};

enum {
    sig_host_AUDIO_OUT_1 = 0,
    sig_host_AUDIO_OUT_2
};

namespace kxmx {
namespace bluemchen {
    static const size_t NUM_ADC_CHANNELS = 4;

    static dsy_gpio_pin ADC_PINS[NUM_ADC_CHANNELS] = {
        seed::PIN_D16,
        seed::PIN_D15,
        seed::PIN_D21,
        seed::PIN_D18
    };

    class BluemchenDevice {
        public:
            seed::SeedBoard board;
            ADCController<AnalogInput, NUM_ADC_CHANNELS> adcController;
            daisy::OledDisplay<daisy::SSD130xI2c64x32Driver>
                display;
            daisy::MidiUartHandler midi;
            struct sig_host_HardwareInterface hardware;

        void Init(struct sig_AudioSettings* audioSettings) {
            board.Init(audioSettings->blockSize, audioSettings->sampleRate);
            InitADCController();

            hardware = {
                .numAudioInputChannels = 2,
                .audioInputChannels = NULL, // Supplied by audio callback
                .numAudioOutputChannels = 2,
                .audioOutputChannels = NULL, // Supplied by audio callback
                .numADCChannels = NUM_ADC_CHANNELS,
                .adcChannels = adcController.channelBank.values,
                .numDACChannels = 0,
                .dacChannels = NULL,
                .numGateInputs = 0,
                .gateInputs = NULL,
                .numGPIOOutputs = 0,
                .gpioOutputs = NULL,
                .numToggles = 0,
                .toggles = NULL,
                .numTriSwitches = 0,
                .triSwitches = NULL
            };
        }

        void InitADCController() {
            adcController.Init(&board.adc, ADC_PINS);
        }

        void InitDisplay() {
            daisy::OledDisplay<daisy::SSD130xI2c64x32Driver>::Config config;
            config.driver_config.transport_config.Defaults();
            display.Init(config);
        }

        void InitMidi() {
            daisy::MidiUartHandler::Config config;
            midi.Init(config);
        }

        void Start() {
            adcController.Start();
        }

        void Stop () {
            adcController.Stop();
        }

        inline void Read() {
            adcController.Read();
        }

        inline void Write() {}
    };
};
};
