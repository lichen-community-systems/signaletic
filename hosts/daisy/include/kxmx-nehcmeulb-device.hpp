#pragma once

#include "kxmx-bluemchen-device.hpp"

enum {
    sig_host_CV_OUT_1 = 0,
    sig_host_CV_OUT_2
};

namespace kxmx {
namespace bluemchen {
    static const size_t NUM_DAC_CHANNELS = 2;

    class NehcmeulbDevice : public kxmx::bluemchen::BluemchenDevice {
        public:
            PollingAnalogOutput dacChannels[NUM_DAC_CHANNELS];
            OutputBank<PollingAnalogOutput, NUM_DAC_CHANNELS> dacOutputBank;

            void Init(struct sig_AudioSettings* audioSettings,
                struct sig_dsp_SignalEvaluator* evaluator) {
                board.Init(audioSettings->blockSize, audioSettings->sampleRate);
                InitADCController();
                InitDAC();

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

            void InitDAC() {
                board.InitDAC();
                for (size_t i = 0; i < NUM_DAC_CHANNELS; i++) {
                    dacChannels[i].Init(&board.dac, i);
                }

                dacOutputBank.outputs = dacChannels;
            }

            void Start(daisy::AudioHandle::AudioCallback callback) {
                adcController.Start();
                board.audio.Start(callback);
            }

            void Stop () {
                adcController.Stop();
                board.dac.Stop();
                board.audio.Stop();
            }

            void Write() {
                dacOutputBank.Write();
            }

            static void afterEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                NehcmeulbDevice* self = static_cast<NehcmeulbDevice*>(
                    hardware->userData);
                self->Write();
            }
    };
}
}
