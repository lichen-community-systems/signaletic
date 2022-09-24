#include <libsignaletic.h>
#include "../examples/dpt/vendor/dpt/lib/daisy_dpt.h"
#include "../examples/bluemchen/vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"

enum {
    sig_daisy_GATEIN_1 = 0,
    sig_daisy_GATEIN_2,
    sig_daisy_GATEIN_LAST
};

enum {
    sig_daisy_DPT_CVOUT_1 = 0,
    sig_daisy_DPT_CVOUT_2,
    sig_daisy_DPT_CVOUT_3,
    sig_daisy_DPT_CVOUT_4,
    sig_daisy_DPT_CVOUT_5,
    sig_daisy_DPT_CVOUT_6,
    sig_daisy_DPT_CVOUT_LAST
};

struct sig_daisy_Host {
    struct sig_daisy_Host_Impl* impl;
    void* state;
};

struct sig_daisy_DPTState {
    daisy::dpt::DPT* dpt;
    uint16_t dacCVOuts[4];
};

struct sig_daisy_BluemchenState {
    kxmx::Bluemchen* bluemchen;
};


typedef float (*sig_daisy_Host_getControlValue)(
    struct sig_daisy_Host* host, int control);

typedef void (*sig_daisy_Host_setControlValue)(
    struct sig_daisy_Host* host, int control, float value);

typedef float (*sig_daisy_Host_getGateValue)(
    struct sig_daisy_Host* host, int control);

struct sig_daisy_Host_Impl {
    sig_daisy_Host_getControlValue getControlValue;
    sig_daisy_Host_setControlValue setControlValue;
    sig_daisy_Host_getGateValue getGateValue;
};

extern struct sig_daisy_Host_Impl sig_daisy_DPTHostImpl;
extern struct sig_daisy_Host_Impl sig_daisy_SeedHostImpl;

void sig_daisy_DPT_dacWriterCallback(void* dptState);

float sig_daisy_DPTHostImpl_getControlValue(
    struct sig_daisy_Host* host, int control);

void sig_daisy_DPTHostImpl_setControlValue(
    struct sig_daisy_Host* host, int control, float value);

float sig_daisy_DPTHostImpl_getGateValue(
    struct sig_daisy_Host* host, int control);


struct sig_daisy_GateIn {
    struct sig_dsp_Signal signal;
    struct sig_daisy_Host* host;
    int control;
};

struct sig_daisy_GateIn* sig_daisy_GateIn_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_daisy_Host* host,
    int control);
void sig_daisy_GateIn_init(struct sig_daisy_GateIn* self,
    struct sig_AudioSettings* settings,
    float_array_ptr output,
    struct sig_daisy_Host* host,
    int control);
void sig_daisy_GateIn_generate(void* signal);
void sig_daisy_GateIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_GateIn* self);


struct sig_daisy_CV_Parameters {
    float scale;
    float offset;
};

// FIXME: Control should at least be a parameter, if not an input.
struct sig_daisy_CVIn {
    struct sig_dsp_Signal signal;
    struct sig_daisy_CV_Parameters parameters;
    struct sig_daisy_Host* host;
    int control;
};

struct sig_daisy_CVIn* sig_daisy_CVIn_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_daisy_Host* host,
    int control);
void sig_daisy_CVIn_init(struct sig_daisy_CVIn* self,
    struct sig_AudioSettings* settings,
    float_array_ptr output,
    struct sig_daisy_Host* host,
    int control);
void sig_daisy_CVIn_generate(void* signal);
void sig_daisy_CVIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_CVIn* self);


struct sig_daisy_CVOut_Inputs {
    float_array_ptr source;
};

struct sig_daisy_CVOut {
    struct sig_dsp_Signal signal;
    struct sig_daisy_CV_Parameters parameters;
    struct sig_daisy_CVOut_Inputs* inputs;
    struct sig_daisy_Host* host;
    int control;
};

struct sig_daisy_CVOut* sig_daisy_CVOut_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* settings,
    struct sig_daisy_CVOut_Inputs* inputs,
    struct sig_daisy_Host* host,
    int control);
void sig_daisy_CVOut_init(struct sig_daisy_CVOut* self,
    struct sig_AudioSettings* settings,
    struct sig_daisy_CVOut_Inputs* inputs,
    float_array_ptr output,
    struct sig_daisy_Host* host,
    int control);
void sig_daisy_CVOut_generate(void* signal);
void sig_daisy_CVOut_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_CVOut* self);
