#pragma once

#include "signaletic-host.h"
#include "signaletic-daisy-host.hpp"
#include "sig-daisy-seed.hpp"

using namespace sig::libdaisy;

enum {
    sig_host_CV_IN_1 = 0,
    sig_host_CV_IN_2,
    sig_host_CV_IN_3,
    sig_host_CV_IN_4,
    sig_host_CV_IN_5,
    sig_host_CV_IN_6,
    sig_host_CV_IN_7
};

enum {
    sig_host_GATE_IN_1 = 0
};

enum {
    sig_host_BUTTON_1 = 0
};

enum {
    sig_host_TRISWITCH_1 = 0,
    sig_host_TRISWITCH_2
};

enum {
    sig_host_AUDIO_IN_1 = 0,
    sig_host_AUDIO_IN_2
};

enum {
    sig_host_AUDIO_OUT_1 = 0,
    sig_host_AUDIO_OUT_2
};

// TODO: Add support for Versio's LED banks.
namespace ne {
namespace versio {
    static const size_t NUM_ADC_CHANNELS = 7;
    static ADCChannelSpec ADC_CHANNEL_SPECS[NUM_ADC_CHANNELS] = {
        {seed::PIN_D21, INVERT, NO_MUX},
        {seed::PIN_D22, INVERT, NO_MUX},
        {seed::PIN_D28, INVERT, NO_MUX},
        {seed::PIN_D23, INVERT, NO_MUX},
        {seed::PIN_D16, INVERT, NO_MUX},
        {seed::PIN_D17, INVERT, NO_MUX},
        {seed::PIN_D19, INVERT, NO_MUX}
    };

    static const size_t NUM_GATES = 1;
    static dsy_gpio_pin GATE_INPUT_PINS[NUM_GATES] = {
        seed::PIN_D24
    };

    static const size_t NUM_BUTTONS = 1;
    static dsy_gpio_pin BUTTON_PINS[NUM_BUTTONS] = {
        seed::PIN_D30
    };

    static const size_t NUM_TRISWITCHES = 2;
    static dsy_gpio_pin TRISWITCH_PINS[NUM_TRISWITCHES][2] = {
        {seed::PIN_D6, seed::PIN_D5},
        {seed::PIN_D1, seed::PIN_D0}
    };

    class VersioDevice {
        public:
            seed::SeedBoard board;
            ADCController<AnalogInput, NUM_ADC_CHANNELS> adcController;
            GateInput gateInputs[NUM_GATES];
            InputBank<GateInput, NUM_GATES> gateInputBank;
            Toggle buttons[NUM_BUTTONS];
            InputBank<Toggle, NUM_BUTTONS> buttonBank;
            TriSwitch triSwitches[NUM_TRISWITCHES];
            InputBank<TriSwitch, NUM_TRISWITCHES> switchBank;
            struct sig_host_HardwareInterface hardware;

        void Init(struct sig_AudioSettings* audioSettings,
            struct sig_dsp_SignalEvaluator* evaluator) {
            board.Init(audioSettings->blockSize, audioSettings->sampleRate);
            InitADCController();
            InitControls();

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
                .numGateInputs = 1,
                .gateInputs = gateInputBank.values,
                .numGPIOOutputs = 0,
                .gpioOutputs = NULL,
                .numToggles = 1,
                .toggles = buttonBank.values,
                .numTriSwitches = NUM_TRISWITCHES,
                .triSwitches = switchBank.values,
                .numEncoders = 0,
                .encoders = NULL
            };

            InitControls();
        }

        void InitADCController() {
            adcController.Init(&board.adc, ADC_CHANNEL_SPECS);
        }

        void InitControls() {
            gateInputs[0].Init(GATE_INPUT_PINS[0]);
            gateInputBank.inputs = gateInputs;

            for (size_t i = 0; i < NUM_TRISWITCHES; i++) {
                triSwitches[i].Init(TRISWITCH_PINS[i]);
            }
            switchBank.inputs = triSwitches;

            buttons[0].Init(BUTTON_PINS[0]);
            buttonBank.inputs = buttons;
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
            gateInputBank.Read();
            buttonBank.Read();
            switchBank.Read();
        }

        static void onEvaluateSignals(size_t size,
            struct sig_host_HardwareInterface* hardware) {
            VersioDevice* self = static_cast<VersioDevice*>(
                hardware->userData);
            self->Read();
        }

        static void afterEvaluateSignals(size_t size,
            struct sig_host_HardwareInterface* hardware) {}
    };
};
};
