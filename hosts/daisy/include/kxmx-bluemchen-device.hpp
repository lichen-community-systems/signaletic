#pragma once

#include "signaletic-host.h"
#include "signaletic-daisy-host.hpp"
#include "sig-daisy-seed.hpp"
#include "dev/oled_ssd130x.h"

using namespace sig::libdaisy;

enum {
    sig_host_KNOB_1 = 0,
    sig_host_KNOB_2
};

enum {
    sig_host_CV_IN_1 = 2,
    sig_host_CV_IN_2
};

enum {
    sig_host_BUTTON_1 = 0
};

enum {
    sig_host_ENCODER_1 = 0
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
    static const size_t NUM_BUTTONS = 1;
    static dsy_gpio_pin BUTTON_PINS[NUM_BUTTONS] = {
        seed::PIN_D28
    };

    static const size_t NUM_ENCODERS = 1;
    static dsy_gpio_pin ENCODER_PINS[NUM_BUTTONS][2] = {
        {seed::PIN_D27, seed::PIN_D26}
    };

    static ADCChannelSpec ADC_CHANNEL_SPECS[NUM_ADC_CHANNELS] = {
        {seed::PIN_D16, BI_TO_UNIPOLAR},
        {seed::PIN_D15, BI_TO_UNIPOLAR},
        {seed::PIN_D21, INVERT},
        {seed::PIN_D18, INVERT}
    };

    class BluemchenDevice {
        public:
            seed::SeedBoard board;
            ADCController<AnalogInput, NUM_ADC_CHANNELS> adcController;
            Toggle buttons[NUM_BUTTONS];
            InputBank<Toggle, NUM_BUTTONS> buttonBank;
            sig::libdaisy::Encoder encoders[NUM_ENCODERS];
            InputBank<sig::libdaisy::Encoder, NUM_ENCODERS> encoderBank;
            daisy::OledDisplay<daisy::SSD130xI2c64x32Driver>
                display;
            daisy::MidiUartHandler midi;
            struct sig_host_HardwareInterface hardware;

        void Init(struct sig_AudioSettings* audioSettings,
            struct sig_dsp_SignalEvaluator* evaluator) {
            board.Init(audioSettings->blockSize, audioSettings->sampleRate);
            InitADCController();

            hardware = {
                .evaluator = evaluator,
                .onEvaluateSignals = onEvaluateSignals,
                .afterEvaluateSignals = afterEvaluateSignals,
                .userData = this,
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
                .numToggles = 1,
                .toggles = buttonBank.values,
                .numTriSwitches = 0,
                .triSwitches = NULL,
                .numEncoders = 1,
                .encoders = encoderBank.values
            };

            InitDisplay();
            InitMidi();
            InitControls();
        }

        void InitADCController() {
            adcController.Init(&board.adc, ADC_CHANNEL_SPECS);
        }

        void InitDisplay() {
            daisy::OledDisplay<daisy::SSD130xI2c64x32Driver>::Config config;
            config.driver_config.transport_config.Defaults();
            display.Init(config);
        }

        void InitControls() {
            buttons[0].Init(BUTTON_PINS[0]);
            buttonBank.inputs = buttons;

            encoders[0].Init(ENCODER_PINS[0][0], ENCODER_PINS[0][1]);
            encoderBank.inputs = encoders;
        }

        void InitMidi() {
            daisy::MidiUartHandler::Config config;
            midi.Init(config);
        }

        void Start(daisy::AudioHandle::AudioCallback callback) {
            adcController.Start();
            board.audio.Start(callback);
        }

        void Stop () {
            adcController.Stop();
            board.audio.Stop();
        }

        inline void Read() {
            adcController.Read();
            buttonBank.Read();
            encoderBank.Read();
        }

        inline void Write() {}

        static void onEvaluateSignals(size_t size,
            struct sig_host_HardwareInterface* hardware) {
            BluemchenDevice* self = static_cast<BluemchenDevice*>(
                hardware->userData);
            self->Read();
        }

        static void afterEvaluateSignals(size_t size,
            struct sig_host_HardwareInterface* hardware) {
            BluemchenDevice* self = static_cast<BluemchenDevice*>(
                hardware->userData);
            self->Write();
        }
    };
};
};
