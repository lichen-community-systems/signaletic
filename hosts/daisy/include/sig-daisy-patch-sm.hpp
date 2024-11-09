#pragma once
#include "sig-daisy-board.hpp"

namespace sig {
namespace libdaisy {
namespace patchsm {
    static constexpr dsy_gpio_pin PIN_NONE = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_A1 = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_A2 = {DSY_GPIOA, 1};
    static constexpr dsy_gpio_pin PIN_A3 = {DSY_GPIOA, 0};
    static constexpr dsy_gpio_pin PIN_A4 = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_A5 = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_A6 = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_A7 = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_A8 = {DSY_GPIOA, 14};
    static constexpr dsy_gpio_pin PIN_A9 = {DSY_GPIOA, 15};
    static constexpr dsy_gpio_pin PIN_A10 = {DSY_GPIOX, 0};

    static constexpr dsy_gpio_pin PIN_B1 = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_B2 = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_B3 = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_B4 = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_B5 = {DSY_GPIOC, 14};
    static constexpr dsy_gpio_pin PIN_B6 = {DSY_GPIOC, 13};
    static constexpr dsy_gpio_pin PIN_B7 = {DSY_GPIOB, 8};
    static constexpr dsy_gpio_pin PIN_B8 = {DSY_GPIOB, 9};
    static constexpr dsy_gpio_pin PIN_B9 = {DSY_GPIOG, 14};
    static constexpr dsy_gpio_pin PIN_B10 = {DSY_GPIOG, 13};

    static constexpr dsy_gpio_pin PIN_C1 = {DSY_GPIOA, 5};
    static constexpr dsy_gpio_pin PIN_C2 = {DSY_GPIOA, 7};
    static constexpr dsy_gpio_pin PIN_C3 = {DSY_GPIOA, 2};
    static constexpr dsy_gpio_pin PIN_C4 = {DSY_GPIOA, 6};
    static constexpr dsy_gpio_pin PIN_C5 = {DSY_GPIOA, 3};
    static constexpr dsy_gpio_pin PIN_C6 = {DSY_GPIOB, 1};
    static constexpr dsy_gpio_pin PIN_C7 = {DSY_GPIOC, 4};
    static constexpr dsy_gpio_pin PIN_C8 = {DSY_GPIOC, 0};
    static constexpr dsy_gpio_pin PIN_C9 = {DSY_GPIOC, 1};
    static constexpr dsy_gpio_pin PIN_C10 = {DSY_GPIOA, 4};

    static constexpr dsy_gpio_pin PIN_D1 = {DSY_GPIOB, 4};
    static constexpr dsy_gpio_pin PIN_D2 = {DSY_GPIOC, 11};
    static constexpr dsy_gpio_pin PIN_D3 = {DSY_GPIOC, 10};
    static constexpr dsy_gpio_pin PIN_D4 = {DSY_GPIOC, 9};
    static constexpr dsy_gpio_pin PIN_D5 = {DSY_GPIOC, 8};
    static constexpr dsy_gpio_pin PIN_D6 = {DSY_GPIOC, 12};
    static constexpr dsy_gpio_pin PIN_D7 = {DSY_GPIOD, 2};
    static constexpr dsy_gpio_pin PIN_D8 = {DSY_GPIOC, 2};
    static constexpr dsy_gpio_pin PIN_D9 = {DSY_GPIOC, 3};
    static constexpr dsy_gpio_pin PIN_D10 = {DSY_GPIOD, 3};

    static constexpr dsy_gpio_pin PIN_CV_1 = PIN_C5;
    static constexpr dsy_gpio_pin PIN_CV_2 = PIN_C4;
    static constexpr dsy_gpio_pin PIN_CV_3 = PIN_C3;
    static constexpr dsy_gpio_pin PIN_CV_4 = PIN_C2;
    static constexpr dsy_gpio_pin PIN_CV_5 = PIN_C6;
    static constexpr dsy_gpio_pin PIN_CV_6 = PIN_C7;
    static constexpr dsy_gpio_pin PIN_CV_7 = PIN_C8;
    static constexpr dsy_gpio_pin PIN_CV_8 = PIN_C9;
    static constexpr dsy_gpio_pin PIN_ADC_9 = PIN_A2;
    static constexpr dsy_gpio_pin PIN_ADC_10 = PIN_A3;
    static constexpr dsy_gpio_pin PIN_ADC_11 = PIN_D9;
    static constexpr dsy_gpio_pin PIN_ADC_12 = PIN_D8;
    static constexpr dsy_gpio_pin PIN_USER_LED = {DSY_GPIOC, 7};

    class PatchSMBoard : public Board {
        public:
            daisy::Pcm3060 codec;
            size_t dac_buffer_size_;
            uint16_t* dacBuffer[2];
            uint16_t* dacOutputValues;
            bool dac_running_;

            void Init(size_t blockSize, float sampleRate);
            void InitDac();
            void StartDac(daisy::DacHandle::DacCallback callback = nullptr);
            static void DefaultDacCallback(uint16_t **output, size_t size);
    };
};
};
};
