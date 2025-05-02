#include "../include/sig-daisy-seed.hpp"

extern "C" {
    #include "dev/codec_ak4556.h"
}

using namespace daisy;

void sig::libdaisy::seed::SeedBoard::Init(size_t blockSize, float sampleRate) {
    System::Config syscfg;
    syscfg.Boost();

    QSPIHandle::Config qspi_config;
    qspi_config.device = QSPIHandle::Config::Device::IS25LP064A;
    qspi_config.mode   = QSPIHandle::Config::Mode::MEMORY_MAPPED;

    qspi_config.pin_config.io0 = dsy_pin(DSY_GPIOF, 8);
    qspi_config.pin_config.io1 = dsy_pin(DSY_GPIOF, 9);
    qspi_config.pin_config.io2 = dsy_pin(DSY_GPIOF, 7);
    qspi_config.pin_config.io3 = dsy_pin(DSY_GPIOF, 6);
    qspi_config.pin_config.clk = dsy_pin(DSY_GPIOF, 10);
    qspi_config.pin_config.ncs = dsy_pin(DSY_GPIOG, 6);

    // Configure the built-in GPIOs.
    userLED.pin = PIN_USER_LED;
    userLED.mode = DSY_GPIO_MODE_OUTPUT_PP;

    auto memory = System::GetProgramMemoryRegion();

    if (memory != System::MemoryRegion::INTERNAL_FLASH) {
        syscfg.skip_clocks = true;
    }

    system.Init(syscfg);

    if (memory != System::MemoryRegion::QSPI) {
        qspi.Init(qspi_config);
    }

    if (memory == System::MemoryRegion::INTERNAL_FLASH) {
        dsy_gpio_init(&userLED);
        sdram.Init();
    }

    InitAudio(blockSize, sampleRate);
}

void sig::libdaisy::seed::SeedBoard::InitAudio(size_t blockSize,
    float sampleRate) {
    // SAI1 -- Peripheral
    // Configure
    SaiHandle::Config sai_config;
    sai_config.periph          = SaiHandle::Config::Peripheral::SAI_1;
    sai_config.sr              = SaiHandle::Config::SampleRate::SAI_48KHZ;
    sai_config.bit_depth       = SaiHandle::Config::BitDepth::SAI_24BIT;
    sai_config.a_sync          = SaiHandle::Config::Sync::MASTER;
    sai_config.b_sync          = SaiHandle::Config::Sync::SLAVE;
    sai_config.pin_config.fs   = {DSY_GPIOE, 4};
    sai_config.pin_config.mclk = {DSY_GPIOE, 2};
    sai_config.pin_config.sck  = {DSY_GPIOE, 5};

    // Device-based Init
    switch(CheckBoardVersion()) {
        case BoardVersion::DAISY_SEED_1_1:
        {
            // Data Line Directions
            sai_config.a_dir         = SaiHandle::Config::Direction::RECEIVE;
            sai_config.pin_config.sa = {DSY_GPIOE, 6};
            sai_config.b_dir         = SaiHandle::Config::Direction::TRANSMIT;
            sai_config.pin_config.sb = {DSY_GPIOE, 3};
            I2CHandle::Config i2c_config;
            i2c_config.mode           = I2CHandle::Config::Mode::I2C_MASTER;
            i2c_config.periph         = I2CHandle::Config::Peripheral::I2C_2;
            i2c_config.speed          = I2CHandle::Config::Speed::I2C_400KHZ;
            i2c_config.pin_config.scl = {DSY_GPIOH, 4};
            i2c_config.pin_config.sda = {DSY_GPIOB, 11};
            I2CHandle i2c_handle;
            i2c_handle.Init(i2c_config);
            Wm8731::Config codec_cfg;
            codec_cfg.Defaults();
            Wm8731 codec;
            codec.Init(codec_cfg, i2c_handle);
        }
        break;
        case BoardVersion::DAISY_SEED:
        default:
        {
            // Data Line Directions
            sai_config.a_dir = SaiHandle::Config::Direction::TRANSMIT;
            sai_config.pin_config.sa = {DSY_GPIOE, 6};
            sai_config.b_dir = SaiHandle::Config::Direction::RECEIVE;
            sai_config.pin_config.sb = {DSY_GPIOE, 3};
            dsy_gpio_pin codec_reset_pin;
            codec_reset_pin = {DSY_GPIOB, 11};
            Ak4556::Init(codec_reset_pin);
        }
        break;
    }

    // Then Initialize
    SaiHandle sai_1_handle;
    sai_1_handle.Init(sai_config);

    // Audio
    AudioHandle::Config audio_config;
    audio_config.blocksize = blockSize;
    audio_config.samplerate = sampleRateFromFloat(sampleRate);
    audio_config.postgain = 1.f;
    audio.Init(audio_config, sai_1_handle);
    callbackRate = audio.GetSampleRate() / audio.GetConfig().blocksize;
}

void sig::libdaisy::seed::SeedBoard::InitDAC(
    daisy::DacHandle::Channel channel) {
    // TODO: This is sourced from the kxmx_Bluemchen.
    // Is DMA-based DAC access an option instead of polling?
    daisy::DacHandle::Config cfg;
    cfg.bitdepth = daisy::DacHandle::BitDepth::BITS_12;
    cfg.buff_state = daisy::DacHandle::BufferState::ENABLED;
    cfg.mode = daisy::DacHandle::Mode::POLLING; // TODO: Use DMA
    cfg.chn = channel;
    dac.Init(cfg);
    dac.WriteValue(daisy::DacHandle::Channel::BOTH, 0);
}

sig::libdaisy::seed::SeedBoard::BoardVersion
    sig::libdaisy::seed::SeedBoard::CheckBoardVersion() {
    /** Version Checks:
     *  * Fall through is Daisy Seed v1 (aka Daisy Seed rev4)
     *  * PD3 tied to gnd is Daisy Seed v1.1 (aka Daisy Seed rev5)
     *  * PD4 tied to gnd reserved for future hardware
     */
    dsy_gpio pincheck {
        .pin = {DSY_GPIOD, 3},
        .mode = DSY_GPIO_MODE_INPUT,
        .pull = DSY_GPIO_PULLUP
    };
    dsy_gpio_init(&pincheck);

    if(!dsy_gpio_read(&pincheck)) {
        return BoardVersion::DAISY_SEED_1_1;
    } else {
        return BoardVersion::DAISY_SEED;
    }
}
