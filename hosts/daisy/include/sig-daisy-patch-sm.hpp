#pragma once
#include "sig-daisy-board.hpp"

namespace sig {
namespace libdaisy {
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_1 = {DSY_GPIOA, 3};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_2 = {DSY_GPIOA, 6};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_3 = {DSY_GPIOA, 2};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_4 = {DSY_GPIOA, 7};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_5 = {DSY_GPIOB, 1};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_6 = {DSY_GPIOC, 4};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_7 = {DSY_GPIOC, 0};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_8 = {DSY_GPIOC, 1};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_9 = {DSY_GPIOA, 1};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_10 = {DSY_GPIOA, 0};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_11 = {DSY_GPIOC, 3};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_ADC_CTRL_12 = {DSY_GPIOC, 2};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_USER_LED = {DSY_GPIOC, 7};

    static constexpr dsy_gpio_pin PATCH_SM_PIN_A2 = {DSY_GPIOA, 1};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_A3 = {DSY_GPIOA, 0};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_A8 = {DSY_GPIOA, 14};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_A9 = {DSY_GPIOA, 15};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_B5 = {DSY_GPIOC, 13};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_B6 = {DSY_GPIOC, 14};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_B7 = {DSY_GPIOB, 8};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_B8 = {DSY_GPIOB, 9};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_B9 = {DSY_GPIOG, 14};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_B10 = {DSY_GPIOG, 13};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C1 = {DSY_GPIOA, 5};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C2 = PATCH_SM_PIN_ADC_CTRL_4;
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C3 = PATCH_SM_PIN_ADC_CTRL_3;
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C4 = PATCH_SM_PIN_ADC_CTRL_2;
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C5 = PATCH_SM_PIN_ADC_CTRL_1;
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C6 = PATCH_SM_PIN_ADC_CTRL_5;
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C7 = PATCH_SM_PIN_ADC_CTRL_6;
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C8 = PATCH_SM_PIN_ADC_CTRL_7;
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C9 = PATCH_SM_PIN_ADC_CTRL_8;
    static constexpr dsy_gpio_pin PATCH_SM_PIN_C10 = {DSY_GPIOA, 4};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D1 = {DSY_GPIOB, 4};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D2 = {DSY_GPIOC, 11};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D3 = {DSY_GPIOC, 10};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D4 = {DSY_GPIOC, 9};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D5 = {DSY_GPIOC, 8};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D6 = {DSY_GPIOC, 12};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D7 = {DSY_GPIOD, 2};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D8 = {DSY_GPIOC, 2};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D9 = {DSY_GPIOC, 3};
    static constexpr dsy_gpio_pin PATCH_SM_PIN_D10 = {DSY_GPIOD, 3};

    enum {
        PATCH_SM_ADC_1 = 0,
        PATCH_SM_ADC_2,
        PATCH_SM_ADC_3,
        PATCH_SM_ADC_4,
        PATCH_SM_ADC_5,
        PATCH_SM_ADC_6,
        PATCH_SM_ADC_7,
        PATCH_SM_ADC_8,
        PATCH_SM_ADC_9,
        PATCH_SM_ADC_10,
        PATCH_SM_ADC_11,
        PATCH_SM_ADC_12,
        PATCH_SM_ADC_LAST
    };

    class PatchSM : public Board {
        public:
            daisy::Pcm3060 codec;
            size_t dac_buffer_size_;
            uint16_t* dacBuffer[2];
            bool dac_running_;

            void Init(size_t blockSize, float sampleRate) override;
            void InitDac();
            void StartDac(daisy::DacHandle::DacCallback callback = nullptr);
            static void DefaultDacCallback(uint16_t **output, size_t size);
    };
};
};
