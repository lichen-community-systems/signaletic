#pragma once

#include "sig-daisy-board.hpp"

namespace sig {
namespace libdaisy {
namespace seed {
    static constexpr dsy_gpio_pin PIN_NONE = {DSY_GPIOX, 0};
    static constexpr dsy_gpio_pin PIN_D0 = {DSY_GPIOB, 12};
    static constexpr dsy_gpio_pin PIN_D1 = {DSY_GPIOC, 11};
    static constexpr dsy_gpio_pin PIN_D2 = {DSY_GPIOC, 10};
    static constexpr dsy_gpio_pin PIN_D3 = {DSY_GPIOC, 9};
    static constexpr dsy_gpio_pin PIN_D4 = {DSY_GPIOC, 8};
    static constexpr dsy_gpio_pin PIN_D5 = {DSY_GPIOD, 2};
    static constexpr dsy_gpio_pin PIN_D6 = {DSY_GPIOC, 12};
    static constexpr dsy_gpio_pin PIN_D7 = {DSY_GPIOG, 10};
    static constexpr dsy_gpio_pin PIN_D8 = {DSY_GPIOG, 11};
    static constexpr dsy_gpio_pin PIN_D9 = {DSY_GPIOB, 4};
    static constexpr dsy_gpio_pin PIN_D10 = {DSY_GPIOB, 5};
    static constexpr dsy_gpio_pin PIN_D11 = {DSY_GPIOB, 8};
    static constexpr dsy_gpio_pin PIN_D12 = {DSY_GPIOB, 9};
    static constexpr dsy_gpio_pin PIN_D13 = {DSY_GPIOB, 6};
    static constexpr dsy_gpio_pin PIN_D14 = {DSY_GPIOB, 7};
    static constexpr dsy_gpio_pin PIN_D15 = {DSY_GPIOC, 0};
    static constexpr dsy_gpio_pin PIN_D16 = {DSY_GPIOA, 3};
    static constexpr dsy_gpio_pin PIN_D17 = {DSY_GPIOB, 1};
    static constexpr dsy_gpio_pin PIN_D18 = {DSY_GPIOA, 7};
    static constexpr dsy_gpio_pin PIN_D19 = {DSY_GPIOA, 6};
    static constexpr dsy_gpio_pin PIN_D20 = {DSY_GPIOC, 1};
    static constexpr dsy_gpio_pin PIN_D21 = {DSY_GPIOC, 4};
    static constexpr dsy_gpio_pin PIN_D22 = {DSY_GPIOA, 5};
    static constexpr dsy_gpio_pin PIN_D23 = {DSY_GPIOA, 4};
    static constexpr dsy_gpio_pin PIN_D24 = {DSY_GPIOA, 1};
    static constexpr dsy_gpio_pin PIN_D25 = {DSY_GPIOA, 0};
    static constexpr dsy_gpio_pin PIN_D26 = {DSY_GPIOD, 11};
    static constexpr dsy_gpio_pin PIN_D27 = {DSY_GPIOG, 9};
    static constexpr dsy_gpio_pin PIN_D28 = {DSY_GPIOA, 2};
    static constexpr dsy_gpio_pin PIN_D29 = {DSY_GPIOB, 14};
    static constexpr dsy_gpio_pin PIN_D30 = {DSY_GPIOB, 15};
    static constexpr dsy_gpio_pin PIN_USER_LED = {DSY_GPIOC, 7};

    /** Analog pins share same pins as digital pins */
    static constexpr dsy_gpio_pin PIN_A0 = PIN_D15;
    static constexpr dsy_gpio_pin PIN_A1 = PIN_D16;
    static constexpr dsy_gpio_pin PIN_A2 = PIN_D17;
    static constexpr dsy_gpio_pin PIN_A3 = PIN_D18;
    static constexpr dsy_gpio_pin PIN_A4 = PIN_D19;
    static constexpr dsy_gpio_pin PIN_A5 = PIN_D20;
    static constexpr dsy_gpio_pin PIN_A6 = PIN_D21;
    static constexpr dsy_gpio_pin PIN_A7 = PIN_D22;
    static constexpr dsy_gpio_pin PIN_A8 = PIN_D23;
    static constexpr dsy_gpio_pin PIN_A9 = PIN_D24;
    static constexpr dsy_gpio_pin PIN_A10 = PIN_D25;
    static constexpr dsy_gpio_pin PIN_A11 = PIN_D28;

    static constexpr dsy_gpio_pin PIN_ADC_0 = PIN_A0;
    static constexpr dsy_gpio_pin PIN_ADC_1 = PIN_A1;
    static constexpr dsy_gpio_pin PIN_ADC_2 = PIN_A2;
    static constexpr dsy_gpio_pin PIN_ADC_3 = PIN_A3;
    static constexpr dsy_gpio_pin PIN_ADC_4 = PIN_A4;
    static constexpr dsy_gpio_pin PIN_ADC_5 = PIN_A5;
    static constexpr dsy_gpio_pin PIN_ADC_6 = PIN_A6;
    static constexpr dsy_gpio_pin PIN_ADC_7 = PIN_A7;
    static constexpr dsy_gpio_pin PIN_ADC_8 = PIN_A8;
    static constexpr dsy_gpio_pin PIN_ADC_9 = PIN_A9;
    static constexpr dsy_gpio_pin PIN_ADC_10 = PIN_A10;
    static constexpr dsy_gpio_pin PIN_ADC_11 = PIN_A11;

    static constexpr dsy_gpio_pin PIN_DAC_OUT_1 = PIN_ADC_8;
    static constexpr dsy_gpio_pin PIN_DAC_OUT_2= PIN_ADC_7;

    /** Pins unique to Daisy Seed 2 DFM */
    static constexpr dsy_gpio_pin PIN_D31 = {DSY_GPIOC, 2};
    static constexpr dsy_gpio_pin PIN_D32 = {DSY_GPIOC, 3};

    /** Analog Pin alias */
    static constexpr dsy_gpio_pin PIN_A12 = PIN_D31;
    static constexpr dsy_gpio_pin PIN_A13 = PIN_D32;

    class SeedBoard : public Board {
        public:
            enum class BoardVersion {
                DAISY_SEED,
                DAISY_SEED_1_1
            };

            void Init(size_t blockSize, float sampleRate);
            void InitAudio(size_t blockSize, float sampleRate);

            /**
             * @brief Initializes the onboard DAC.
             *
             * Not called by default, since the DAC pins are shared
             * with the ADC, so some configurations may choose not
             * to initialize it at all.
             */
            void InitDAC(daisy::DacHandle::Channel channel =
                daisy::DacHandle::Channel::BOTH);

            BoardVersion CheckBoardVersion();
    };
};
};
};
