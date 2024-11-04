#pragma once

#include "signaletic-host.h"
#include "signaletic-daisy-host.hpp"
#include "sig-daisy-patch-sm.hpp"
#include "../vendor/dpt/lib/dev/DAC7554.h"
#include "../vendor/libDaisy/src/util/hal_map.h"
#include "../vendor/libDaisy/src/per/tim.h"

using namespace sig::libdaisy;

struct Normalization DPT_INTERNAL_DAC_NORMALIZATION = {
    .scale = 0.651f,
    .offset = -0.348f
};

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
    sig_host_CV_OUT_1 = 0,
    sig_host_CV_OUT_2,
    sig_host_CV_OUT_3,
    sig_host_CV_OUT_4,
    sig_host_CV_OUT_5,
    sig_host_CV_OUT_6
};

enum {
    sig_host_GATE_IN_1 = 0,
    sig_host_GATE_IN_2
};

enum {
    sig_host_GATE_OUT_1 = 0,
    sig_host_GATE_OUT_2
};

enum {
    sig_host_AUDIO_IN_1 = 0,
    sig_host_AUDIO_IN_2
};

enum {
    sig_host_AUDIO_OUT_1 = 0,
    sig_host_AUDIO_OUT_2
};

namespace dspcoffee {
namespace dpt {
    static const size_t NUM_ADC_CHANNELS = 8;

    // DPT exposes eight CV inputs from the PatchSM;
    // its knobs configured as hardware attenuators of the incoming
    // CV jacks, normaled to ~5V when unplugged.
    static ADCChannelSpec ADC_CHANNEL_SPECS[NUM_ADC_CHANNELS] = {
        {patchsm::PIN_CV_1, INVERT, NO_MUX}, // C5
        {patchsm::PIN_CV_2, INVERT, NO_MUX}, // C4
        {patchsm::PIN_CV_3, INVERT, NO_MUX}, // C3
        {patchsm::PIN_CV_4, INVERT, NO_MUX}, // C2
        {patchsm::PIN_CV_8, INVERT, NO_MUX}, // C9
        {patchsm::PIN_CV_7, INVERT, NO_MUX}, // C8
        {patchsm::PIN_CV_5, INVERT, NO_MUX}, // C6
        {patchsm::PIN_CV_6, INVERT, NO_MUX}  // C7
    };

    static const size_t NUM_GATES = 2;

    static dsy_gpio_pin GATE_IN_PINS[NUM_GATES] = {
        patchsm::PIN_B10,
        patchsm::PIN_B9
    };

    static dsy_gpio_pin GATE_OUT_PINS[NUM_GATES] = {
        patchsm::PIN_B6,
        patchsm::PIN_B5
    };

    static const size_t NUM_INTERNAL_DAC_CHANNELS = 2;
    static const size_t NUM_EXTERNAL_DAC_CHANNELS = 4;
    static const size_t NUM_DAC_CHANNELS = NUM_INTERNAL_DAC_CHANNELS +
        NUM_EXTERNAL_DAC_CHANNELS;

    class DPTDevice {
        public:
            patchsm::PatchSMBoard board;
            uint16_t externalDACBuffer[NUM_EXTERNAL_DAC_CHANNELS];
            daisy::Dac7554 externalDAC;
            daisy::TimerHandle tim5;
            ADCController<AnalogInput, NUM_ADC_CHANNELS> adcController;
            GateInput gateInputs[NUM_GATES];
            InputBank<GateInput, NUM_GATES> gateInputBank;
            GPIOOutput gateOutputs[NUM_GATES];
            OutputBank<GPIOOutput, NUM_GATES> gateOutputBank;
            BipolarInvertedBufferedAnalogOutput dacChannels[NUM_DAC_CHANNELS];
            OutputBank<BipolarInvertedBufferedAnalogOutput, NUM_DAC_CHANNELS>
                dacOutputBank;
            struct sig_host_HardwareInterface hardware;

            static void onEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                DPTDevice* self = static_cast<DPTDevice*>(hardware->userData);
                self->Read();
            }

            static void afterEvaluateSignals(size_t size,
                struct sig_host_HardwareInterface* hardware) {
                DPTDevice* self = static_cast<DPTDevice*>(hardware->userData);
                self->Write();
            }

            static void externalDACWriteCallback(void* data) {
                DPTDevice* self = static_cast<DPTDevice*>(data);
                self->externalDAC.Write(self->externalDACBuffer);
                self->externalDAC.WriteDac7554();
            }

            void InitExternalDACWriteTimer(
                struct sig_AudioSettings* audioSettings,
                daisy::TimerHandle::PeriodElapsedCallback cb, void *data) {
                uint32_t target_freq =
                    static_cast<uint32_t>(audioSettings->sampleRate);
                uint32_t tim_base_freq = daisy::System::GetPClk2Freq();
                uint32_t tim_period = tim_base_freq / target_freq;

                daisy::TimerHandle::Config timerConfig;
                timerConfig.periph =
                    daisy::TimerHandle::Config::Peripheral::TIM_5;
                timerConfig.dir = daisy::TimerHandle::Config::CounterDir::UP;
                timerConfig.period = tim_period;
                timerConfig.enable_irq = true;
                tim5.Init(timerConfig);
                HAL_NVIC_SetPriority(TIM5_IRQn, 0x0, 0x0);
                tim5.SetCallback(cb, data);
            }

            void InitExternalDAC(struct sig_AudioSettings* audioSettings,
                daisy::TimerHandle::PeriodElapsedCallback cb, void *data) {
                externalDAC.Init();
                InitExternalDACWriteTimer(audioSettings, cb, data);
            }

            void Init(struct sig_AudioSettings* audioSettings,
                struct sig_dsp_SignalEvaluator* evaluator) {
                board.Init(audioSettings->blockSize, audioSettings->sampleRate);
                // The DAC and ADC have to be initialized after the board.
                InitADCController();
                InitDAC();
                InitExternalDAC(audioSettings, externalDACWriteCallback,
                    this);
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
                    .numToggles = 0,
                    .toggles = NULL,
                    .numTriSwitches = 0,
                    .triSwitches = NULL
                };
            }

            void InitADCController() {
                adcController.Init(&board.adc, ADC_CHANNEL_SPECS,
                    NUM_ADC_CHANNELS);
            }

            void InitDAC() {
                // TODO: We're actually managing two different buffers
                // here. A better architecture would allow for
                // multiple DAC (and ADC) instances.
                for (size_t i = 0; i < NUM_INTERNAL_DAC_CHANNELS; i++) {
                    dacChannels[i].Init(board.dacOutputValues, i,
                        DPT_INTERNAL_DAC_NORMALIZATION);
                }

                for (size_t i = 0; i < NUM_EXTERNAL_DAC_CHANNELS; i++) {
                    size_t j = NUM_INTERNAL_DAC_CHANNELS + i;
                    dacChannels[j].Init(externalDACBuffer, i, NO_NORMALIZATION);
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
            }

            void Start(daisy::AudioHandle::AudioCallback callback) {
                adcController.Start();
                board.StartDac();
                tim5.Start();
                board.audio.Start(callback);
            }

            void Stop () {
                adcController.Stop();
                board.dac.Stop();
                tim5.Stop();
                board.audio.Stop();
            }

            inline void Read() {
                adcController.Read();
                gateInputBank.Read();
            }

            inline void Write() {
                gateOutputBank.Write();
                dacOutputBank.Write();
            }
    };
};
};
