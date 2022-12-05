#include <libsignaletic.h>
#include "daisy.h"

enum {
    sig_daisy_GATEIN_1 = 0,
    sig_daisy_GATEIN_2,
    sig_daisy_GATEIN_LAST
};

struct sig_daisy_Host {
    struct sig_daisy_Host_Impl* impl;
    daisy::AnalogControl* analogControls;
    void* boardState;
};

typedef float (*sig_daisy_Host_getControlValue)(
    struct sig_daisy_Host* host, int control);

typedef void (*sig_daisy_Host_setControlValue)(
    struct sig_daisy_Host* host, int control, float value);

typedef float (*sig_daisy_Host_getGateValue)(
    struct sig_daisy_Host* host, int control);

struct sig_daisy_Host_Impl {
    int numAnalogControls;
    sig_daisy_Host_getControlValue getControlValue;
    sig_daisy_Host_setControlValue setControlValue;
    sig_daisy_Host_getGateValue getGateValue;
};

/**
 * @brief Processes and returns the value for the specified Analog Control.
 * This implementation should work with most Daisy boards, assuming they
 * provide public access to an array of daisy::AnalogControls (which most do).
 *
 * @param host the host instance
 * @param control the control number to process
 * @return float the value of the control
 */
float sig_daisy_processControlValue(struct sig_daisy_Host* host,
    int control);

struct sig_daisy_CV_Parameters {
    float scale;
    float offset;
    int control;
};

struct sig_daisy_GateIn {
    struct sig_dsp_Signal signal;
    struct sig_daisy_CV_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_daisy_Host* host;
};

struct sig_daisy_GateIn* sig_daisy_GateIn_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_daisy_Host* host);
void sig_daisy_GateIn_init(struct sig_daisy_GateIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host);
void sig_daisy_GateIn_generate(void* signal);
void sig_daisy_GateIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_GateIn* self);


struct sig_daisy_CVIn {
    struct sig_dsp_Signal signal;
    struct sig_daisy_CV_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_daisy_Host* host;
};

struct sig_daisy_CVIn* sig_daisy_CVIn_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context, struct sig_daisy_Host* host);
void sig_daisy_CVIn_init(struct sig_daisy_CVIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host);
void sig_daisy_CVIn_generate(void* signal);
void sig_daisy_CVIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_CVIn* self);


struct sig_daisy_CVOut_Inputs {
    float_array_ptr source;
};

// TODO: Should we have a "no output" outputs type,
// or continue with the idea of passing through the input
// for chaining?
struct sig_daisy_CVOut {
    struct sig_dsp_Signal signal;
    struct sig_daisy_CV_Parameters parameters;
    struct sig_daisy_CVOut_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_daisy_Host* host;
};

struct sig_daisy_CVOut* sig_daisy_CVOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_daisy_Host* host);
void sig_daisy_CVOut_init(struct sig_daisy_CVOut* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host);
void sig_daisy_CVOut_generate(void* signal);
void sig_daisy_CVOut_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_CVOut* self);
