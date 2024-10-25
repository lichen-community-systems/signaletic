#pragma once

#include "patchsm-device.hpp"

using namespace sig::libdaisy;

enum {
    sig_host_KNOB_1 = 0,
    sig_host_KNOB_2,
    sig_host_KNOB_3,
    sig_host_KNOB_4
};

enum {
    sig_host_CV_IN_1 = 4,
    sig_host_CV_IN_2,
    sig_host_CV_IN_3,
    sig_host_CV_IN_4
};

enum {
    sig_host_CV_OUT_1 = 0,
    sig_host_CV_OUT_2
};

enum {
    sig_host_TOGGLE_1 = 0,
    sig_host_TOGGLE_2
};

namespace electrosmith {
namespace patchinit {
    static const size_t NUM_ADC_CHANNELS = 8;

    static ADCChannelSpec ADC_CHANNEL_SPECS[NUM_ADC_CHANNELS] = {
        // Knobs
        {patchsm::PIN_CV_1, INVERT, NO_MUX}, // CV1/Pin C5
        {patchsm::PIN_CV_2, INVERT, NO_MUX}, // CV2/Pin C4
        {patchsm::PIN_CV_3, INVERT, NO_MUX}, // CV3/Pin C3
        {patchsm::PIN_CV_4, INVERT, NO_MUX}, // CV4/Pin C2

        // CV Jacks
        {patchsm::PIN_CV_8, INVERT, NO_MUX}, // CV5/Pin C6
        {patchsm::PIN_CV_7, INVERT, NO_MUX}, // CV6/Pin C7
        {patchsm::PIN_CV_5, INVERT, NO_MUX}, // CV7/Pin C8
        {patchsm::PIN_CV_6, INVERT, NO_MUX}, // CV8/Pin C9

    };

    static const size_t NUM_GATES = 2;

    static dsy_gpio_pin GATE_IN_PINS[NUM_GATES] = {
        patchsm::PIN_B10,
        patchsm::PIN_B9
    };

    static dsy_gpio_pin GATE_OUT_PINS[NUM_GATES] = {
        patchsm::PIN_B5,
        patchsm::PIN_B6
    };

    static const size_t NUM_TOGGLES = 2;
    static dsy_gpio_pin TOGGLE_PINS[NUM_GATES] = {
        patchsm::PIN_B7,
        patchsm::PIN_B8
    };

    static const size_t NUM_DAC_CHANNELS = 2;

    class PatchInitDevice : public electrosmith::BasePatchSMDevice {
        public:
            ADCController<AnalogInput, NUM_ADC_CHANNELS> adcController;
            GateInput gateInputs[NUM_GATES];
            InputBank<GateInput, NUM_GATES> gateInputBank;
            GPIOOutput gateOutputs[NUM_GATES];
            OutputBank<GPIOOutput, NUM_GATES> gateOutputBank;
            Toggle toggles[NUM_TOGGLES];
            InputBank<Toggle, NUM_TOGGLES> toggleBank;
            BufferedAnalogOutput dacChannels[NUM_DAC_CHANNELS];
            OutputBank<BufferedAnalogOutput, NUM_DAC_CHANNELS> dacOutputBank;

            static void onEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                PatchInitDevice* self =
                    static_cast<PatchInitDevice*> (hardware->userData);
                self->Read();
            }

            static void afterEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                PatchInitDevice* self =
                    static_cast<PatchInitDevice*> (hardware->userData);
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
                    .gateInputs = gateInputBank.values,
                    .numGPIOOutputs = NUM_GATES,
                    .gpioOutputs = gateOutputBank.values,
                    .numToggles = NUM_TOGGLES,
                    .toggles = toggleBank.values,
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
                for (size_t i = 0; i < NUM_GATES; i++) {
                    gateInputs[i].Init(GATE_IN_PINS[i]);
                    gateOutputs[i].Init(GATE_OUT_PINS[i],
                        DSY_GPIO_MODE_OUTPUT_PP, DSY_GPIO_NOPULL);
                }
                gateInputBank.inputs = gateInputs;
                gateOutputBank.outputs = gateOutputs;

                for (size_t i = 0; i < NUM_TOGGLES; i++) {
                    toggles[i].Init(TOGGLE_PINS[i]);
                }
                toggleBank.inputs = toggles;
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
                gateInputBank.Read();
                toggleBank.Read();
            }

            inline void Write() {
                dacOutputBank.Write();
                gateOutputBank.Write();
            }
    };
};
};
