#include "./signaletic-daisy-host.h"
#include "../vendor/libDaisy/src/daisy_patch_sm.h"

enum {
    sig_daisy_PatchSM_CV_IN_1 = daisy::patch_sm::CV_1,
    sig_daisy_PatchSM_CV_IN_2 = daisy::patch_sm::CV_2,
    sig_daisy_PatchSM_CV_IN_3 = daisy::patch_sm::CV_3,
    sig_daisy_PatchSM_CV_IN_4 = daisy::patch_sm::CV_4,
    sig_daisy_PatchSM_CV_IN_5 = daisy::patch_sm::CV_5,
    sig_daisy_PatchSM_CV_IN_6 = daisy::patch_sm::CV_6,
    sig_daisy_PatchSM_CV_IN_7 = daisy::patch_sm::CV_7,
    sig_daisy_PatchSM_CV_IN_8 = daisy::patch_sm::CV_8,
    sig_daisy_PatchSM_ADC_IN_9 = daisy::patch_sm::ADC_9,
    sig_daisy_PatchSM_ADC_IN_10 = daisy::patch_sm::ADC_10,
    sig_daisy_PatchSM_ADC_IN_11 = daisy::patch_sm::ADC_11,
    sig_daisy_PatchSM_ADC_IN_12 = daisy::patch_sm::ADC_12,
    sig_daisy_PatchSM_CV_IN_LAST
};

const int sig_daisy_PatchSM_NUM_ANALOG_INPUTS = daisy::patch_sm::ADC_LAST;

enum {
    sig_daisy_PatchSM_CV_OUT_1 = daisy::patch_sm::CV_OUT_1,
    sig_daisy_PatchSM_CV_OUT_2 = daisy::patch_sm::CV_OUT_2,
    sig_daisy_PatchSM_CV_OUT_LAST
};

const int sig_daisy_PatchSM_NUM_ANALOG_OUTPUTS = 2;

enum {
    sig_daisy_PatchSM_GATE_IN_1 = sig_daisy_GATE_IN_1,
    sig_daisy_PatchSM_GATE_IN_2 = sig_daisy_GATE_IN_2,
    sig_daisy_PatchSM_GATE_IN_LAST = sig_daisy_GATE_IN_LAST
};

const int sig_daisy_PatchSM_NUM_GATE_INPUTS = sig_daisy_GATE_IN_LAST;

enum {
    sig_daisy_PatchSM_GATE_OUT_1 = sig_daisy_GATE_OUT_1,
    sig_daisy_PatchSM_GATE_OUT_2 = sig_daisy_GATE_OUT_2,
    sig_daisy_PatchSM_GATE_OUT_LAST = sig_daisy_GATE_OUT_LAST
};

const int sig_daisy_PatchSM_NUM_GATE_OUTPUTS = sig_daisy_GATE_OUT_LAST;

// TODO: What are the additional 4x ADC pins on the Patch SM,
// and where are they mapped (if at all) on the Patch init?
enum {
    sig_daisy_PatchInit_KNOB_1 = daisy::patch_sm::CV_1,
    sig_daisy_PatchInit_KNOB_2 = daisy::patch_sm::CV_2,
    sig_daisy_PatchInit_KNOB_3 = daisy::patch_sm::CV_3,
    sig_daisy_PatchInit_KNOB_4 = daisy::patch_sm::CV_4,
    sig_daisy_PatchInit_CV_IN_1 = daisy::patch_sm::CV_5,
    sig_daisy_PatchInit_CV_IN_2 = daisy::patch_sm::CV_6,
    sig_daisy_PatchInit_CV_IN_3 = daisy::patch_sm::CV_7,
    sig_daisy_PatchInit_CV_IN_4 = daisy::patch_sm::CV_8,
    sig_daisy_PatchInit_CV_IN_LAST
};

const int sig_daisy_PatchInit_NUM_ANALOG_INPUTS = 8;

enum {
    sig_daisy_PatchInit_CV_OUT = daisy::patch_sm::CV_OUT_1,
    sig_daisy_PatchInit_LED = daisy::patch_sm::CV_OUT_2,
    sig_daisy_PatchInit_CV_OUT_LAST
};

const int sig_daisy_PatchInit_NUM_ANALOG_OUTPUTS = 2;

enum {
    sig_daisy_PatchInit_GATE_IN_1 = sig_daisy_GATE_IN_1,
    sig_daisy_PatchInit_GATE_IN_2 = sig_daisy_GATE_IN_2,
    sig_daisy_PatchInit_GATE_IN_LAST = sig_daisy_GATE_IN_LAST
};

const int sig_daisy_PatchInit_NUM_GATE_INPUTS = sig_daisy_GATE_IN_LAST;

enum {
    sig_daisy_PatchInit_GATE_OUT_1 = sig_daisy_GATE_OUT_1,
    sig_daisy_PatchInit_GATE_OUT_2 = sig_daisy_GATE_OUT_2,
    sig_daisy_PatchInit_GATE_OUT_LAST = sig_daisy_GATE_OUT_LAST
};

const int sig_daisy_PatchInit_NUM_GATE_OUTPUTS = sig_daisy_GATE_OUT_LAST;

// TODO: Add support for the Patch Init's switch and button.


extern struct sig_daisy_Host_Impl sig_daisy_PatchSMHostImpl;

void sig_daisy_PatchSMHostImpl_start(
    struct sig_daisy_Host* host);

void sig_daisy_PatchSMHostImpl_stop(
    struct sig_daisy_Host* host);

void sig_daisy_PatchSMHostImpl_setControlValue(
    struct sig_daisy_Host* host, int control, float value);

float sig_daisy_PatchSMHostImpl_getGateValue(
    struct sig_daisy_Host* host, int control);

void sig_daisy_PatchSMHostImpl_setGateValue(
    struct sig_daisy_Host* host, int control, float value);

struct sig_daisy_Host* sig_daisy_PatchSMHost_new(
    struct sig_Allocator* allocator,
    daisy::patch_sm::DaisyPatchSM* board,
    struct sig_dsp_SignalEvaluator* evaluator);

void sig_daisy_PatchSMHost_init(struct sig_daisy_Host* self,
    daisy::patch_sm::DaisyPatchSM* patchSM,
    struct sig_dsp_SignalEvaluator* evaluator);

void sig_daisy_PatchSMHost_Board_init(struct sig_daisy_Host_Board* self,
        daisy::patch_sm::DaisyPatchSM* patchSM);

void sig_daisy_PatchSMHost_destroy(
    struct sig_Allocator* allocator,
    struct sig_daisy_Host* self);
