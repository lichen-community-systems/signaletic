#include "./signaletic-daisy-host.h"
#include "../vendor/dpt/lib/daisy_dpt.h"

enum {
    sig_daisy_DPT_CV_IN_1 = daisy::dpt::CV_1,
    sig_daisy_DPT_CV_IN_2 = daisy::dpt::CV_2,
    sig_daisy_DPT_CV_IN_3 = daisy::dpt::CV_3,
    sig_daisy_DPT_CV_IN_4 = daisy::dpt::CV_4,
    sig_daisy_DPT_CV_IN_5 = daisy::dpt::CV_5,
    sig_daisy_DPT_CV_IN_6 = daisy::dpt::CV_6,
    sig_daisy_DPT_CV_IN_7 = daisy::dpt::CV_7,
    sig_daisy_DPT_CV_IN_8 = daisy::dpt::CV_8,
    sig_daisy_DPT_CV_IN_LAST
};

const int sig_daisy_DPT_NUM_ANALOG_INPUTS = daisy::dpt::ADC_LAST;

enum {
    sig_daisy_DPT_CV_OUT_1 = 0,
    sig_daisy_DPT_CV_OUT_2,
    sig_daisy_DPT_CV_OUT_3,
    sig_daisy_DPT_CV_OUT_4,
    sig_daisy_DPT_CV_OUT_5,
    sig_daisy_DPT_CV_OUT_6,
    sig_daisy_DPT_CV_OUT_LAST
};

const int sig_daisy_DPT_NUM_ANALOG_OUTPUTS = sig_daisy_DPT_CV_OUT_LAST;

enum {
    sig_daisy_DPT_GATE_IN_1 = sig_daisy_GATE_IN_1,
    sig_daisy_DPT_GATE_IN_2 = sig_daisy_GATE_IN_2,
    sig_daisy_DPT_GATE_IN_LAST = sig_daisy_GATE_IN_LAST
};

const int sig_daisy_DPT_NUM_GATE_INPUTS = sig_daisy_DPT_GATE_IN_LAST;

enum {
    sig_daisy_DPT_GATE_OUT_1 = sig_daisy_GATE_OUT_1,
    sig_daisy_DPT_GATE_OUT_2 = sig_daisy_GATE_OUT_2,
    sig_daisy_DPT_GATE_OUT_LAST = sig_daisy_GATE_OUT_LAST
};

const int sig_daisy_DPT_NUM_GATE_OUTPUTS = sig_daisy_GATE_OUT_LAST;

const int sig_daisy_DPT_NUM_SWITCHES = 0;
const int sig_daisy_DPT_NUM_ENCODERS = 0;

void sig_daisy_DPT_dacWriterCallback(void* dptHost);

struct sig_daisy_DPTHost {
    struct sig_daisy_Host host;
    daisy::Dac7554* expansionDAC;
    uint16_t expansionDACBuffer[4];
};

extern struct sig_daisy_Host_Impl sig_daisy_DPTHostImpl;

void sig_daisy_DPTHostImpl_start(struct sig_daisy_Host* host);

void sig_daisy_DPTHostImpl_stop(struct sig_daisy_Host* host);

void sig_daisy_DPTHostImpl_setControlValue(
    struct sig_daisy_Host* host, int control, float value);

float sig_daisy_DPTHostImpl_getGateValue(
    struct sig_daisy_Host* host, int control);

void sig_daisy_DPTHostImpl_setGateValue(
    struct sig_daisy_Host* host, int control, float value);

struct sig_daisy_Host* sig_daisy_DPTHost_new(struct sig_Allocator* allocator,
    sig_AudioSettings* audioSettings,
    daisy::dpt::DPT* dpt,
    struct sig_dsp_SignalEvaluator* evaluator);

void sig_daisy_DPTHost_Board_init(struct sig_daisy_Host_Board* self,
    daisy::dpt::DPT* dpt);

void sig_daisy_DPTHost_init(struct sig_daisy_DPTHost* self,
    sig_AudioSettings* audioSettings,
    daisy::dpt::DPT* dpt,
    struct sig_dsp_SignalEvaluator* evaluator);

void sig_daisy_DPTHost_destroy(
    struct sig_Allocator* allocator,
    struct sig_daisy_DPTHost* self);
