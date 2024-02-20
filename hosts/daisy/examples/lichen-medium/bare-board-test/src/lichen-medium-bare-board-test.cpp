#include "daisy.h"
#include <libsignaletic.h>
#include "../../../../include/sig-daisy-patch-sm.hpp"

class AnalogSource {
    public:
        uint16_t* adcPtr;

        void Init(daisy::AdcHandle* adc, int adcChannel) {
           adcPtr = adc->GetPtr(adcChannel);
        }

        inline float Value() {
            return (float) *adcPtr / 65536.0f;
        }
};

class GPIOSource {
    public:
        dsy_gpio gpio;

        void Init(dsy_gpio_pin pin, dsy_gpio_mode mode, dsy_gpio_pull pull) {
            gpio.pin = pin;
            gpio.mode = mode;
            gpio.pull = pull;

            dsy_gpio_init(&gpio);
        }

        inline float Value() {
            return (float) dsy_gpio_read(&gpio);
        }
};

class GateSource {
    public:
        GPIOSource gpioSource;

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
            previousDebounce = System::GetNow();
            state = 0x00;
            gpio.pin = pin;
            gpio.mode = DSY_GPIO_MODE_INPUT;
            gpio.pull = DSY_GPIO_PULLUP;
            dsy_gpio_init(&gpio);
        }

        inline float Value() {
            uint32_t now = System::GetNow();
            if (now - previousDebounce >= 1) {
                state = (state << 1) | !dsy_gpio_read(&gpio);
            }
            return (float) state == 0xff;
        }
};

class TriSwitch {
    public:
        Toggle switchA;
        Toggle switchB;

        void Init(dsy_gpio_pin pinA, dsy_gpio_pin pinB) {
            switchA.Init(pinA);
            switchB.Init(pinB);
        }

        inline float Value () {
            return (float) switchA.Value() + -switchB.Value();
        }
};

#define SAMPLERATE 48000
#define HEAP_SIZE 1024 * 384 // 384 KB
#define MAX_NUM_SIGNALS 32

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;

sig::libdaisy::PatchSM patchSM;
struct sig_daisy_Host* host;
AnalogSource knob1;
struct sig_dsp_Value* freq;
GateSource gateIn;
struct sig_dsp_Value* gain;
struct sig_dsp_Oscillator* sine;
struct sig_dsp_Value* switchValue;
struct sig_dsp_ScaleOffset* switchValueScale;
struct sig_dsp_BinaryOp* harmonizerFreqScale;
struct sig_dsp_Oscillator* harmonizer;
TriSwitch threeway;
Toggle button;
struct sig_dsp_Value* buttonValue;
struct sig_dsp_BinaryOp* mixer;
struct sig_dsp_BinaryOp* attenuator;

void buildSignalGraph(struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_List* signals,
    struct sig_AudioSettings* audioSettings,
    struct sig_Status* status) {

    freq = sig_dsp_Value_new(allocator, context);
    freq->parameters.value = 220.0f;
    sig_List_append(signals, freq, status);

    buttonValue = sig_dsp_Value_new(allocator, context);
    sig_List_append(signals, buttonValue, status);
    buttonValue->parameters.value = 0.0f;

    switchValue = sig_dsp_Value_new(allocator, context);
    sig_List_append(signals, switchValue, status);
    switchValue->parameters.value = 0.0f;

    switchValueScale = sig_dsp_ScaleOffset_new(allocator, context);
    sig_List_append(signals, switchValueScale, status);
    switchValueScale->inputs.source = switchValue->outputs.main;
    switchValueScale->parameters.scale = 1.25f;
    switchValueScale->parameters.offset = 0.75f;

    harmonizerFreqScale = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, harmonizerFreqScale, status);
    harmonizerFreqScale->inputs.left = freq->outputs.main;
    harmonizerFreqScale->inputs.right = switchValueScale->outputs.main;

    harmonizer = sig_dsp_LFTriangle_new(allocator, context);
    sig_List_append(signals, harmonizer, status);
    harmonizer->inputs.freq = harmonizerFreqScale->outputs.main;
    harmonizer->inputs.mul = buttonValue->outputs.main;

    sine = sig_dsp_Sine_new(allocator, context);
    sig_List_append(signals, sine, status);
    sine->inputs.freq = freq->outputs.main;

    mixer = sig_dsp_Add_new(allocator, context);
    sig_List_append(signals, mixer, status);
    mixer->inputs.left = sine->outputs.main;
    mixer->inputs.right = harmonizer->outputs.main;

    gain = sig_dsp_Value_new(allocator, context);
    gain->parameters.value = 0.5f;
    sig_List_append(signals, gain, status);

    attenuator = sig_dsp_Mul_new(allocator, context);
    sig_List_append(signals, attenuator, status);
    attenuator->inputs.left = mixer->outputs.main;
    attenuator->inputs.right = gain->outputs.main;
}

void AudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    freq->parameters.value = 1760.0f * knob1.Value();

    buttonValue->parameters.value = button.Value();
    switchValue->parameters.value = threeway.Value();

    evaluator->evaluate((struct sig_dsp_SignalEvaluator*) evaluator);

    for (size_t i = 0; i < size; i++) {
        float sig = attenuator->outputs.main[i];
        out[0][i] = sig;
        out[1][i] = sig;

        patchSM.dacBuffer[0][i] = sig_bipolarToUint12(gateIn.Value() - 0.75f);
    }
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 1
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    patchSM.Init(audioSettings.blockSize, audioSettings.sampleRate);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(&allocator, context, &signals, &audioSettings, &status);

    knob1.Init(&patchSM.adc, sig::libdaisy::PATCH_SM_ADC_7);
    gateIn.Init(sig::libdaisy::PATCH_SM_PIN_B10);
    threeway.Init(sig::libdaisy::PATCH_SM_PIN_B7, sig::libdaisy::PATCH_SM_PIN_B8);
    button.Init(sig::libdaisy::PATCH_SM_PIN_D1);

    patchSM.audio.Start(AudioCallback);

    while (1) {

    }
}
