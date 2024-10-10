#pragma once

#include "signaletic-host.h"
#include "signaletic-daisy-host.hpp"
#include "sig-daisy-patch-sm.hpp"

using namespace sig::libdaisy;

enum {
    sig_host_KNOB_1 = 0,
    sig_host_KNOB_2,
    sig_host_KNOB_3,
    sig_host_KNOB_4,
    sig_host_KNOB_5,
    sig_host_KNOB_6
};

enum {
    sig_host_CV_IN_1 = 6,
    sig_host_CV_IN_2,
    sig_host_CV_IN_3,
    sig_host_CV_IN_4,
    sig_host_CV_IN_5,
    sig_host_CV_IN_6
};

enum {
    sig_host_TOGGLE_1 = 0
};

enum {
    sig_host_TRISWITCH_1 = 0
};

enum {
    sig_host_CV_OUT_1 = 0
};

enum {
    sig_host_AUDIO_IN_1 = 0,
    sig_host_AUDIO_IN_2
};

enum {
    sig_host_AUDIO_OUT_1 = 0,
    sig_host_AUDIO_OUT_2
};

namespace lichen {
namespace medium {
    static const size_t NUM_ADC_CHANNELS = 12;

    // These pins are ordered based on the panel:
    // knobs first in labelled order, then CV jacks in labelled order.
    static ADCChannelSpec ADC_CHANNEL_SPECS[NUM_ADC_CHANNELS] = {
        {patchsm::PIN_CV_5, INVERT}, // Knob one/POT_CV_1/Pin C8
        {patchsm::PIN_CV_6, INVERT}, // Knob two/POT_CV_2/Pin C9
        {patchsm::PIN_ADC_9, BI_TO_UNIPOLAR}, // Knob three/POT_CV_3/Pin A2
        {patchsm::PIN_ADC_11, BI_TO_UNIPOLAR}, // Knob four/POT_CV_4/Pin A3
        {patchsm::PIN_ADC_10, BI_TO_UNIPOLAR}, // Knob five/POT_CV_5/Pin D9
        {patchsm::PIN_ADC_12, BI_TO_UNIPOLAR}, // Knob six/POT_CV_6/Pin D8

        {patchsm::PIN_CV_1, INVERT}, // CV1 ("seven")/CV_IN_1/Pin C5
        {patchsm::PIN_CV_2, INVERT}, // CV2 ("eight")/CV_IN_2Pin C4
        {patchsm::PIN_CV_3, INVERT}, // CV3 ("nine")/CV_IN_3/Pin C3
        {patchsm::PIN_CV_7, INVERT}, // CV6 ("ten")/CV_IN_6/Pin C7
        {patchsm::PIN_CV_8, INVERT}, // CV5 ("eleven")/CV_IN_5/Pin C6
        {patchsm::PIN_CV_4, INVERT} // CV4 ("twelve")/CV_IN_4/Pin C2
    };

    static const size_t NUM_GATES = 1;

    static dsy_gpio_pin GATE_PINS[NUM_GATES] = {
        patchsm::PIN_B10
    };

    static const size_t NUM_TRISWITCHES = 1;
    static dsy_gpio_pin TRISWITCH_PINS[NUM_GATES][2] = {
        {
            patchsm::PIN_B7,
            patchsm::PIN_B8
        }
    };

    static const size_t NUM_BUTTONS = 1;
    static dsy_gpio_pin BUTTON_PINS[NUM_BUTTONS] = {
        patchsm::PIN_D1
    };

    static const size_t NUM_DAC_CHANNELS = 1;

    class MediumDevice {
        public:
            patchsm::PatchSMBoard board;
            ADCController<AnalogInput, NUM_ADC_CHANNELS> adcController;
            GateInput gates[NUM_GATES];
            InputBank<GateInput, NUM_GATES> gateBank;
            TriSwitch triSwitches[NUM_TRISWITCHES];
            InputBank<TriSwitch, NUM_TRISWITCHES> switchBank;
            Toggle buttons[NUM_BUTTONS];
            InputBank<Toggle, NUM_BUTTONS> buttonBank;
            BufferedAnalogOutput dacChannels[NUM_DAC_CHANNELS];
            OutputBank<BufferedAnalogOutput, NUM_DAC_CHANNELS> dacOutputBank;
            struct sig_host_HardwareInterface hardware;

            static void onEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                MediumDevice* self = static_cast<MediumDevice*> (hardware->userData);
                self->Read();
            }

            static void afterEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                MediumDevice* self = static_cast<MediumDevice*> (hardware->userData);
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
                    .numAudioInputChannels = 2,
                    .audioInputChannels = NULL, // Supplied by audio callback
                    .numAudioOutputChannels = 2,
                    .audioOutputChannels = NULL, // Supplied by audio callback
                    .numADCChannels = NUM_ADC_CHANNELS,
                    .adcChannels = adcController.channelBank.values,
                    .numDACChannels = NUM_DAC_CHANNELS,
                    .dacChannels = dacOutputBank.values,
                    .numGateInputs = NUM_GATES,
                    .gateInputs = gateBank.values,
                    .numGPIOOutputs = 0,
                    .gpioOutputs = NULL,
                    .numToggles = NUM_BUTTONS,
                    .toggles = buttonBank.values,
                    .numTriSwitches = NUM_TRISWITCHES,
                    .triSwitches = switchBank.values
                };
            }

            void InitADCController() {
                adcController.Init(&board.adc, ADC_CHANNEL_SPECS);
            }

            void InitDAC() {
                for (size_t i = 0; i < NUM_DAC_CHANNELS; i++) {
                    dacChannels[i].Init(board.dacOutputValues, i);
                }

                dacOutputBank.outputs = dacChannels;
            }

            void InitControls() {
                gates[0].Init(GATE_PINS[0]);
                gateBank.inputs = gates;
                triSwitches[0].Init(TRISWITCH_PINS[0]);
                switchBank.inputs = triSwitches;
                buttons[0].Init(BUTTON_PINS[0]);
                buttonBank.inputs = buttons;
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
                gateBank.Read();
                switchBank.Read();
                buttonBank.Read();
            }

            inline void Write() {
                dacOutputBank.Write();
            }
    };
};
};
