#include "./signaletic-daisy-host.h"
#include "../vendor/dpt/lib/daisy_dpt.h"

enum {
    sig_daisy_DPT_CVIN_1 = daisy::dpt::CV_1,
    sig_daisy_DPT_CVIN_2 = daisy::dpt::CV_2,
    sig_daisy_DPT_CVIN_3 = daisy::dpt::CV_3,
    sig_daisy_DPT_CVIN_4 = daisy::dpt::CV_4,
    sig_daisy_DPT_CVIN_5 = daisy::dpt::CV_5,
    sig_daisy_DPT_CVIN_6 = daisy::dpt::CV_6,
    sig_daisy_DPT_CVIN_7 = daisy::dpt::CV_7,
    sig_daisy_DPT_CVIN_8 = daisy::dpt::CV_8,
    sig_daisy_DPT_CVIN_LAST
};

const int sig_daisy_DPT_NUM_ANALOG_CONTROLS = daisy::dpt::ADC_LAST;

enum {
    sig_daisy_DPT_CVOUT_1 = 0,
    sig_daisy_DPT_CVOUT_2,
    sig_daisy_DPT_CVOUT_3,
    sig_daisy_DPT_CVOUT_4,
    sig_daisy_DPT_CVOUT_5,
    sig_daisy_DPT_CVOUT_6,
    sig_daisy_DPT_CVOUT_LAST
};

extern struct sig_daisy_Host_Impl sig_daisy_DPTHostImpl;

void sig_daisy_DPT_dacWriterCallback(void* dptState);

void sig_daisy_DPTHostImpl_setControlValue(
    struct sig_daisy_Host* host, int control, float value);

float sig_daisy_DPTHostImpl_getGateValue(
    struct sig_daisy_Host* host, int control);

struct sig_daisy_DPTState {
    daisy::dpt::DPT* dpt;
    uint16_t dacCVOuts[4];
};

void sig_daisy_DPTState_init(struct sig_daisy_DPTState* self,
    daisy::dpt::DPT* board);

struct sig_daisy_Host* sig_daisy_DPTHost_new(struct sig_Allocator* allocator,
    daisy::dpt::DPT* board);

void sig_daisy_DPTHost_init(struct sig_daisy_Host* self,
    struct sig_daisy_DPTState* boardState);

void sig_daisy_DPTHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self);
