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
    sig_host_KNOB_5
};

enum {
    sig_host_CV_IN_1 = 5,
    sig_host_CV_IN_2,
    sig_host_CV_IN_3,
    sig_host_CV_IN_4,
    sig_host_CV_IN_5
};

enum {
    sig_host_TOGGLE_1 = 0
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
namespace bifocals {
    static const size_t NUM_ADC_CHANNELS = 10;

    // These pins are ordered based on the panel:
    // knobs first in labelled order, then CV jacks in labelled order.
    static ADCChannelSpec ADC_CHANNEL_SPECS[NUM_ADC_CHANNELS] = {
        // Freq Knob/POT_2_CV_7/Pin C8 - Unipolar
        {patchsm::PIN_CV_7, INVERT},
        // Gain Knob/CV_IN_5/Pin C6
        {patchsm::PIN_CV_5, INVERT},
        // Skew Knob/POT_1_CV_6/Pin C7
        {patchsm::PIN_CV_6, INVERT},
        // Shape/POT_4_CV_9/Pin A2
        {patchsm::PIN_ADC_9, BI_TO_UNIPOLAR},
        // Reso/POT_5_CV_10/Pin A3
        {patchsm::PIN_ADC_10, BI_TO_UNIPOLAR},

        // Shape CV/CV_IN_1/Pin C5
        {patchsm::PIN_CV_1, INVERT},
        // Frequency CV/CV_IN_2/Pin C4
        {patchsm::PIN_CV_2, INVERT},
        // Reso CV/CV_IN_3/Pin C3
        {patchsm::PIN_CV_3, INVERT},
        // Gain CV/CV_IN_4/Pin C2
        {patchsm::PIN_CV_4, INVERT},
        // Skew CV/POT_3_CV_8/Pin C9
        {patchsm::PIN_CV_8, INVERT}
    };

    static const size_t NUM_BUTTONS = 1;
    static dsy_gpio_pin BUTTON_PINS[NUM_BUTTONS] = {
        patchsm::PIN_B7
    };

    static const size_t NUM_DAC_CHANNELS = 1;

    class BifocalsDevice {
        public:
            patchsm::PatchSMBoard board;
            ADCController<AnalogInput, NUM_ADC_CHANNELS> adcController;
            Toggle buttons[NUM_BUTTONS];
            InputBank<Toggle, NUM_BUTTONS> buttonBank;
            BufferedAnalogOutput dacChannels[NUM_DAC_CHANNELS];
            OutputBank<BufferedAnalogOutput, NUM_DAC_CHANNELS> dacOutputBank;
            struct sig_host_HardwareInterface hardware;

            static void onEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                BifocalsDevice* self = static_cast<BifocalsDevice*> (hardware->userData);
                self->Read();
            }

            static void afterEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                BifocalsDevice* self = static_cast<BifocalsDevice*> (hardware->userData);
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
                    .numGateInputs = 0,
                    .gateInputs = NULL,
                    .numGPIOOutputs = 0,
                    .gpioOutputs = NULL,
                    .numToggles = NUM_BUTTONS,
                    .toggles = buttonBank.values,
                    .numTriSwitches = 0,
                    .triSwitches = NULL
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
                buttonBank.Read();
            }

            inline void Write() {
                dacOutputBank.Write();
            }
    };
};
};
