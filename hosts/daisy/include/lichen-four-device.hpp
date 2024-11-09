#pragma once

#include "signaletic-host.h"
#include "signaletic-daisy-host.hpp"
#include "sig-daisy-patch-sm.hpp"

using namespace sig::libdaisy;

enum {
    sig_host_CV_IN_1 = 0,
    sig_host_CV_IN_2,
    sig_host_CV_IN_3,
    sig_host_CV_IN_4,
    sig_host_CV_IN_5,
    sig_host_CV_IN_6,
    sig_host_CV_IN_7,
    sig_host_CV_IN_8
};

enum {
    sig_host_KNOB_1 = 8,
    sig_host_KNOB_2,
    sig_host_KNOB_3,
    sig_host_KNOB_4,
    sig_host_KNOB_5,
    sig_host_KNOB_6,
    sig_host_KNOB_7,
    sig_host_KNOB_8
};

enum {
    sig_host_TOGGLE_1 = 0
};

enum {
    sig_host_GATE_IN_1 = 0
};

enum {
    sig_host_GATE_OUT_1 = 0,
    sig_host_GATE_OUT_2
};

enum {
    sig_host_CV_OUT_1 = 0,
    sig_host_CV_OUT_2
};

enum {
    sig_host_AUDIO_OUT_1 = 0,
    sig_host_AUDIO_OUT_2
};

namespace lichen {
namespace four {
    static const size_t NUM_ADC_CHANNELS = 9;
    static const size_t NUM_MUX_CHANNELS = 8;
    static const size_t NUM_ADC_INPUTS = NUM_ADC_CHANNELS + NUM_MUX_CHANNELS;

    static ADCChannelSpec ADC_CHANNEL_SPECS[NUM_ADC_CHANNELS] = {
        {patchsm::PIN_CV_1, INVERT, NO_MUX},
        {patchsm::PIN_CV_2, INVERT, NO_MUX},
        {patchsm::PIN_CV_3, INVERT, NO_MUX},
        {patchsm::PIN_CV_4, INVERT, NO_MUX},
        {patchsm::PIN_CV_5, INVERT, NO_MUX},
        {patchsm::PIN_CV_6, INVERT, NO_MUX},
        {patchsm::PIN_CV_7, INVERT, NO_MUX},
        {patchsm::PIN_CV_8, INVERT, NO_MUX},
        {patchsm::PIN_ADC_9, BI_TO_UNIPOLAR, {
            .numMuxChannels = 8,
            .selA = patchsm::PIN_D2,
            .selB = patchsm::PIN_D3,
            .selC = patchsm::PIN_D4
        }}
    };

    static const size_t NUM_BUTTONS = 1;
    static dsy_gpio_pin BUTTON_PINS[NUM_BUTTONS] = {
        patchsm::PIN_D1
    };

    static const size_t NUM_DAC_CHANNELS = 2;
    static const size_t NUM_GATES = 1;
    static dsy_gpio_pin GATE_INPUT_PINS[NUM_GATES] = {
        patchsm::PIN_B10
    };

    static const size_t NUM_GPIO_OUTPUTS = 2;
    static dsy_gpio_pin GPIO_OUTPUT_PINS[NUM_GPIO_OUTPUTS] = {
        patchsm::PIN_B5, // Gate output
        patchsm::PIN_D5  // LED
    };

    class FourDevice {
        public:
            patchsm::PatchSMBoard board;
            ADCController<AnalogInput, NUM_ADC_INPUTS> adcController;
            Toggle buttons[NUM_BUTTONS];
            InputBank<Toggle, NUM_BUTTONS> buttonBank;
            BufferedAnalogOutput dacChannels[NUM_DAC_CHANNELS];
            OutputBank<BufferedAnalogOutput, NUM_DAC_CHANNELS> dacOutputBank;
            GateInput gateInputs[NUM_GATES];
            InputBank<GateInput, NUM_GATES> gateInputBank;
            GPIOOutput gpioOutputs[NUM_GPIO_OUTPUTS];
            OutputBank<GPIOOutput, NUM_GPIO_OUTPUTS> gpioOutputBank;

            struct sig_host_HardwareInterface hardware;

            static void onEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                FourDevice* self = static_cast<FourDevice*> (hardware->userData);
                self->Read();
            }

            static void afterEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                FourDevice* self = static_cast<FourDevice*> (hardware->userData);
                self->Write();
            }

            void Init(struct sig_AudioSettings* audioSettings,
                struct sig_dsp_SignalEvaluator* evaluator) {
                board.Init(audioSettings->blockSize, audioSettings->sampleRate);
                // The DAC and ADC have to be initialized after the board.
                InitADCController();
                InitDAC();
                InitControls();

                hardware = {
                    .evaluator = evaluator,
                    .onEvaluateSignals = onEvaluateSignals,
                    .afterEvaluateSignals = afterEvaluateSignals,
                    .userData = this,
                    .numAudioInputChannels = 0,
                    .audioInputChannels = NULL, // Supplied by audio callback
                    .numAudioOutputChannels = 2,
                    .audioOutputChannels = NULL, // Supplied by audio callback
                    .numADCChannels = NUM_ADC_CHANNELS,
                    .adcChannels = adcController.channelBank.values,
                    .numDACChannels = NUM_DAC_CHANNELS,
                    .dacChannels = dacOutputBank.values,
                    .numGateInputs = NUM_GATES,
                    .gateInputs = gateInputBank.values,
                    .numGPIOOutputs = NUM_GPIO_OUTPUTS,
                    .gpioOutputs = gpioOutputBank.values,
                    .numToggles = NUM_BUTTONS,
                    .toggles = buttonBank.values,
                    .numTriSwitches = 0,
                    .triSwitches = NULL
                };
            }

            void InitADCController() {
                adcController.Init(&board.adc, ADC_CHANNEL_SPECS,
                    NUM_ADC_CHANNELS);
            }

            void InitDAC() {
                for (size_t i = 0; i < NUM_DAC_CHANNELS; i++) {
                    dacChannels[i].Init(board.dacOutputValues, i,
                        BI_TO_UNIPOLAR);
                }

                dacOutputBank.outputs = dacChannels;
            }

            void InitControls() {
                buttons[0].Init(BUTTON_PINS[0]);
                buttonBank.inputs = buttons;
                gateInputs[0].Init(GATE_INPUT_PINS[0]);
                gateInputBank.inputs = gateInputs;

                for (size_t i = 0; i < NUM_GPIO_OUTPUTS; i++) {
                    gpioOutputs[i].Init(GPIO_OUTPUT_PINS[i],
                        DSY_GPIO_MODE_OUTPUT_PP, DSY_GPIO_NOPULL);
                }
                gpioOutputBank.outputs = gpioOutputs;
            }

            void Start(daisy::AudioHandle::AudioCallback callback) {
                adcController.Start();
                board.StartDac();
                board.audio.Start(callback);
            }

            void Stop () {
                adcController.Stop();
                board.dac.Stop();
                board.audio.Stop();
            }

            inline void Read() {
                adcController.Read();
                buttonBank.Read();
                gateInputBank.Read();
            }

            inline void Write() {
                gpioOutputBank.Write();
                dacOutputBank.Write();
            }
    };
};
};
