#pragma once

#include <libsignaletic.h>
#include "daisy.h"
#include "signaletic-host.h"

namespace sig {
namespace libdaisy {

struct Normalization {
    float scale;
    float offset;
};

static constexpr struct Normalization NO_NORMALIZATION = {
    .scale = 1.0f,
    .offset = 0.0f
};

static constexpr struct Normalization UNI_TO_BIPOLAR = {
    .scale = 2.0f,
    .offset = -1.0f
};

static constexpr struct Normalization BI_TO_UNIPOLAR = {
    .scale = 0.5f,
    .offset = 0.5f
};

static constexpr struct Normalization INVERT = {
    .scale = -1.0f
};

static constexpr struct Normalization INV_UNI_TO_BIPOLAR = {
    .scale = -2.0f,
    .offset = 1.0f
};

static constexpr struct Normalization INV_BI_TO_UNIPOLAR = {
    .scale = -0.5f,
    .offset = 0.5f
};

struct ADCChannelSpec {
    dsy_gpio_pin pin;
    struct Normalization normalization;
};

template<typename T, size_t size> class InputBank {
    public:
        T* inputs;
        float values[size] = {0.0};

        inline void Read() {
            for (size_t i = 0; i < size; i++) {
                values[i] = inputs[i].Value();
            }
        }
};

template<typename T, size_t size> class OutputBank {
    public:
        T* outputs;
        float values[size];
        inline void Write() {
            for (size_t i = 0; i < size; i++) {
                outputs[i].Write(values[i]);
            }
        }
};

class BaseAnalogInput {
    public:
        uint16_t* adcPtr;
        float scale = 1.0f;
        float offset = 0.0f;

        void Init(daisy::AdcHandle* adc, ADCChannelSpec spec, int adcChannel) {
            adcPtr = adc->GetPtr(adcChannel);
            scale = spec.normalization.scale;
            offset = spec.normalization.offset;
        }
};

class AnalogInput : public BaseAnalogInput {
    public:
        /**
         * @brief Returns the value of the ADC channel as a float
         * scaled to a range of -1.0 to 1.0f.
         *
         * @return float the ADC channel's current value
         */
        inline float Value() {
            float converted = sig_uint16ToBipolar(*adcPtr);
            float normalized =  converted * scale + offset;
            return normalized;
        }
};

class UnipolarAnalogInput : public BaseAnalogInput {
    public:
        /**
         * @brief Returns the inverted value of the ADC channel as
         * a float scaled to the range of 0.0 to 1.0f.
         *
         * @return float the ADC channel's current value
         */
        inline float Value() {
            return sig_uint16ToUnipolar(*adcPtr);
        }
};

class InvertedAnalogInput : public AnalogInput {
    public:
        /**
         * @brief Returns the inverted value of the ADC channel as
         * a float scaled to the range of -1.0 to 1.0f.
         *
         * @return float the ADC channel's current value
         */
        inline float Value() {
            float converted = sig_invUint16ToBipolar(*adcPtr);
            return converted;
        }
};

template<typename T, size_t numChannels> class ADCController {
    public:
        daisy::AdcHandle* adcHandle;
        T inputs[numChannels];
        InputBank<T, numChannels> channelBank;

        void Init(daisy::AdcHandle* inAdcHandle, ADCChannelSpec* channelSpecs) {
            adcHandle = inAdcHandle;

            daisy::AdcChannelConfig adcConfigs[numChannels];
            for (size_t i = 0; i < numChannels; i++) {
                adcConfigs[i].InitSingle(channelSpecs[i].pin);
            }

            adcHandle->Init(adcConfigs, numChannels);

            for (size_t i = 0; i < numChannels; i++) {
                inputs[i].Init(adcHandle, channelSpecs[i], i);
            }

            channelBank.inputs = inputs;
        }

        void Start() {
            adcHandle->Start();
        }

        void Stop() {
            adcHandle->Stop();
        }

        inline void Read() {
            channelBank.Read();
        }
};

class AnalogOutput {
    public:
        uint16_t* dacOutputs;
        size_t channel;

        void Init(uint16_t* inDACOutputs, size_t inChannel) {
            dacOutputs = inDACOutputs;
            channel = inChannel;
        }

        inline void Write(float value) {
            dacOutputs[channel] = sig_unipolarToUint12(value);
        }
};

class GPIO {
    public:
        dsy_gpio gpio;

        void Init(dsy_gpio_pin pin, dsy_gpio_mode mode, dsy_gpio_pull pull) {
            gpio.pin = pin;
            gpio.mode = mode;
            gpio.pull = pull;

            dsy_gpio_init(&gpio);
        }
};

class GPIOInput : public GPIO {
    public:
        inline float Value() {
            return (float) dsy_gpio_read(&gpio);
        }
};

class GPIOOutput : public GPIO {
    public:
        inline void Write(float value) {
            dsy_gpio_write(&gpio, (uint8_t) value);
        }
};


class GateInput {
    public:
        GPIOInput gpioSource;

        void Init(dsy_gpio_pin pin) {
            gpioSource.Init(pin, DSY_GPIO_MODE_INPUT, DSY_GPIO_PULLUP);
        }

        inline float Value() {
            return !gpioSource.Value();
        }
};

class Toggle {
    public:
        uint32_t previousDebounce;
        uint8_t state;
        dsy_gpio gpio;

        void Init(dsy_gpio_pin pin) {
            previousDebounce = daisy::System::GetNow();
            state = 0x00;
            gpio.pin = pin;
            gpio.mode = DSY_GPIO_MODE_INPUT;
            gpio.pull = DSY_GPIO_PULLUP;
            dsy_gpio_init(&gpio);
        }

        inline float Value() {
            // See https://www.ganssle.com/debouncing-pt2.htm
            // for a good overview of debouncing algorithms.
            uint32_t now = daisy::System::GetNow();
            if (now - previousDebounce >= 1) {
                state = (state << 1) | !dsy_gpio_read(&gpio);
                previousDebounce = now;
            }
            return (float) state == 0xff;
        }
};

class TriSwitch {
    public:
        Toggle switchA;
        Toggle switchB;

        void Init(dsy_gpio_pin pins[2]) {
            switchA.Init(pins[0]);
            switchB.Init(pins[1]);
        }

        void Init(dsy_gpio_pin pinA, dsy_gpio_pin pinB) {
            switchA.Init(pinA);
            switchB.Init(pinB);
        }

        inline float Value() {
            return switchA.Value() + -switchB.Value();
        }
};

class Encoder {
    public:
        uint32_t previousDebounce;
        uint8_t a;
        uint8_t b;
        dsy_gpio gpioA;
        dsy_gpio gpioB;

        void Init(dsy_gpio_pin pinA, dsy_gpio_pin pinB) {
            gpioA.pin = pinA;
            gpioA.mode = DSY_GPIO_MODE_INPUT;
            gpioA.pull = DSY_GPIO_PULLUP;

            gpioB.pin = pinB;
            gpioB.mode = DSY_GPIO_MODE_INPUT;
            gpioB.pull = DSY_GPIO_PULLUP;

            dsy_gpio_init(&gpioA);
            dsy_gpio_init(&gpioB);

            previousDebounce = daisy::System::GetNow();
            a = 255;
            b = 255;
        }

        inline float Value() {
            uint32_t now = daisy::System::GetNow();
            float increment = 0.0f;

            if (now - previousDebounce >= 1) {
                a = (a << 1) | dsy_gpio_read(&gpioA);
                b = (b << 1) | dsy_gpio_read(&gpioB);
                uint8_t aMasked = a & 3;
                uint8_t bMasked = b & 3;

                if (aMasked == 2 && bMasked == 0) {
                    increment = 1.0f;
                } else if (bMasked == 2 && aMasked == 0) {
                    increment = -1.0f;
                }

                previousDebounce = now;
            }

            return increment;
        }
};


void DaisyHostAudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size);

template<typename T> class DaisyHost {
    public:
        T device;
        struct sig_AudioSettings* audioSettings;

        void Init(struct sig_AudioSettings* inAudioSettings,
            struct sig_dsp_SignalEvaluator* evaluator) {
            audioSettings = inAudioSettings;
            device.Init(audioSettings, evaluator);
            sig_host_registerGlobalHardwareInterface(&device.hardware);
        }

        void Start() {
            device.Start(DaisyHostAudioCallback);
        }

        void Stop() {
            device.Stop();
            device.board.audio.Stop();
        }
};
};
};
