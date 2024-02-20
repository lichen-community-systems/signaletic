#include "../include/sig-daisy-patch-sm.hpp"

using namespace daisy;

/** outside of class static buffer(s) for DMA and DAC access */
uint16_t DMA_BUFFER_MEM_SECTION sig_daisy_patch_sm_dac_buffer[2][48];
uint16_t sig_daisy_patch_sm_dac_output[2];

void sig::libdaisy::PatchSM::Init(size_t blockSize, float sampleRate) {
    dac_running_ = false;
    dac_buffer_size_ = 48;
    sig_daisy_patch_sm_dac_output[0] = 0;
    sig_daisy_patch_sm_dac_output[1] = 0;
    dacBuffer[0] = sig_daisy_patch_sm_dac_buffer[0];
    dacBuffer[1] = sig_daisy_patch_sm_dac_buffer[1];

    System::Config syscfg;
    syscfg.Boost();

    auto memory = System::GetProgramMemoryRegion();
    if (memory != System::MemoryRegion::INTERNAL_FLASH) {
        syscfg.skip_clocks = true;
    }

    system.Init(syscfg);
    /** Memories */
    if (memory == System::MemoryRegion::INTERNAL_FLASH) {
        /** FMC SDRAM */
        sdram.Init();
    }

    if (memory != System::MemoryRegion::QSPI) {
        /** QUADSPI FLASH */
        QSPIHandle::Config qspi_config;
        qspi_config.device = QSPIHandle::Config::Device::IS25LP064A;
        qspi_config.mode = QSPIHandle::Config::Mode::MEMORY_MAPPED;
        qspi_config.pin_config.io0 = {DSY_GPIOF, 8};
        qspi_config.pin_config.io1 = {DSY_GPIOF, 9};
        qspi_config.pin_config.io2 = {DSY_GPIOF, 7};
        qspi_config.pin_config.io3 = {DSY_GPIOF, 6};
        qspi_config.pin_config.clk = {DSY_GPIOF, 10};
        qspi_config.pin_config.ncs = {DSY_GPIOG, 6};
        qspi.Init(qspi_config);
    }

    /** Audio */
    // Audio Init
    SaiHandle::Config sai_config;
    sai_config.periph = SaiHandle::Config::Peripheral::SAI_1;
    sai_config.sr = SaiHandle::Config::SampleRate::SAI_48KHZ;
    sai_config.bit_depth = SaiHandle::Config::BitDepth::SAI_24BIT;
    sai_config.a_sync = SaiHandle::Config::Sync::MASTER; // Srsly, Electrosmith?
    sai_config.b_sync = SaiHandle::Config::Sync::SLAVE;  // Ooof!
    sai_config.a_dir = SaiHandle::Config::Direction::RECEIVE;
    sai_config.b_dir = SaiHandle::Config::Direction::TRANSMIT;
    sai_config.pin_config.fs = {DSY_GPIOE, 4};
    sai_config.pin_config.mclk = {DSY_GPIOE, 2};
    sai_config.pin_config.sck = {DSY_GPIOE, 5};
    sai_config.pin_config.sa = {DSY_GPIOE, 6};
    sai_config.pin_config.sb = {DSY_GPIOE, 3};
    SaiHandle sai_1_handle;
    sai_1_handle.Init(sai_config);
    I2CHandle::Config i2c_cfg;
    i2c_cfg.periph = I2CHandle::Config::Peripheral::I2C_2;
    i2c_cfg.mode = I2CHandle::Config::Mode::I2C_MASTER;
    i2c_cfg.speed = I2CHandle::Config::Speed::I2C_400KHZ;
    i2c_cfg.pin_config.scl = {DSY_GPIOB, 10};
    i2c_cfg.pin_config.sda = {DSY_GPIOB, 11};
    I2CHandle i2c2;
    i2c2.Init(i2c_cfg);
    codec.Init(i2c2);

    AudioHandle::Config audio_config;
    audio_config.blocksize  = blockSize;
    audio_config.samplerate = sampleRateFromFloat(sampleRate);
    audio_config.postgain   = 1.f;
    audio.Init(audio_config, sai_1_handle);
    callbackRate = audio.GetSampleRate() / audio.GetConfig().blocksize;

    /** ADC Init */
    AdcChannelConfig adc_config[PATCH_SM_ADC_LAST];
    /** Order of pins to match enum expectations */
    dsy_gpio_pin adc_pins[] = {
        PATCH_SM_PIN_ADC_CTRL_1,
        PATCH_SM_PIN_ADC_CTRL_2,
        PATCH_SM_PIN_ADC_CTRL_3,
        PATCH_SM_PIN_ADC_CTRL_4,
        PATCH_SM_PIN_ADC_CTRL_8,
        PATCH_SM_PIN_ADC_CTRL_7,
        PATCH_SM_PIN_ADC_CTRL_5,
        PATCH_SM_PIN_ADC_CTRL_6,
        PATCH_SM_PIN_ADC_CTRL_9,
        PATCH_SM_PIN_ADC_CTRL_10,
        PATCH_SM_PIN_ADC_CTRL_11,
        PATCH_SM_PIN_ADC_CTRL_12
    };

    for (int i = 0; i < PATCH_SM_ADC_LAST; i++) {
        adc_config[i].InitSingle(adc_pins[i]);
    }
    adc.Init(adc_config, PATCH_SM_ADC_LAST);

    /** Fixed-function Digital I/O */
    userLED.mode = DSY_GPIO_MODE_OUTPUT_PP;
    userLED.pull = DSY_GPIO_NOPULL;
    // userLED.pin = PIN_USER_LED;
    userLED.pin = {DSY_GPIOC, 7}; // FIXME: Why does the linker fail to
                                  // resolve PIN_USER_LED?
    dsy_gpio_init(&userLED);

    InitDac();
    adc.Start();
    StartDac();
}

void sig::libdaisy::PatchSM::InitDac() {
    DacHandle::Config dac_config;
    dac_config.mode = DacHandle::Mode::DMA;
    dac_config.bitdepth = DacHandle::BitDepth::
        BITS_12; /**< Sets the output value to 0-4095 */
    dac_config.chn = DacHandle::Channel::BOTH;
    dac_config.buff_state = DacHandle::BufferState::ENABLED;
    dac_config.target_samplerate = 48000;
    dac.Init(dac_config);
}

void sig::libdaisy::PatchSM::StartDac(DacHandle::DacCallback callback) {
    if (dac_running_) {
        dac.Stop();
    }

    dac.Start(dacBuffer[0],
        dacBuffer[1],
        dac_buffer_size_,
        callback == nullptr ? DefaultDacCallback : callback);
    dac_running_ = true;
}

void sig::libdaisy::PatchSM::DefaultDacCallback(uint16_t **output,
    size_t size) {
    for (size_t i = 0; i < size; i++) {
        output[0][i] = sig_daisy_patch_sm_dac_output[0];
        output[1][i] = sig_daisy_patch_sm_dac_output[1];
    }
}
