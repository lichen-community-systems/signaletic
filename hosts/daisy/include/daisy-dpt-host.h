#include "./signaletic-daisy-host.h"
#include "../vendor/dpt/lib/daisy_dpt.h"

enum {
    sig_daisy_DPT_CVOUT_1 = 0,
    sig_daisy_DPT_CVOUT_2,
    sig_daisy_DPT_CVOUT_3,
    sig_daisy_DPT_CVOUT_4,
    sig_daisy_DPT_CVOUT_5,
    sig_daisy_DPT_CVOUT_6,
    sig_daisy_DPT_CVOUT_LAST
};

struct sig_daisy_DPTState {
    daisy::dpt::DPT* dpt;
    uint16_t dacCVOuts[4];
};

extern struct sig_daisy_Host_Impl sig_daisy_DPTHostImpl;

void sig_daisy_DPT_dacWriterCallback(void* dptState);

float sig_daisy_DPTHostImpl_getControlValue(
    struct sig_daisy_Host* host, int control);

void sig_daisy_DPTHostImpl_setControlValue(
    struct sig_daisy_Host* host, int control, float value);

float sig_daisy_DPTHostImpl_getGateValue(
    struct sig_daisy_Host* host, int control);
