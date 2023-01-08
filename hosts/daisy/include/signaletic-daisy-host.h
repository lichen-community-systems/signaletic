#include <libsignaletic.h>
#include "daisy.h"

enum {
    sig_daisy_AUDIO_IN_1 = 0,
    sig_daisy_AUDIO_IN_2,
    sig_daisy_AUDIO_IN_LAST
};

enum {
    sig_daisy_AUDIO_OUT_1 = 0,
    sig_daisy_AUDIO_OUT_2,
    sig_daisy_AUDIO_OUT_LAST
};

enum {
    sig_daisy_GATE_IN_1 = 0,
    sig_daisy_GATE_IN_2,
    sig_daisy_GATE_IN_LAST
};

enum {
    sig_daisy_GATE_OUT_1 = 0,
    sig_daisy_GATE_OUT_2,
    sig_daisy_GATE_OUT_LAST
};

struct sig_daisy_Host_BoardConfiguration {
    int numAudioInputChannels;
    int numAudioOutputChannels;
    int numAnalogInputs;
    int numAnalogOutputs;
    int numGateInputs;
    int numGateOutputs;
    int numSwitches;
};

struct sig_daisy_Host_Board {
    struct sig_daisy_Host_BoardConfiguration* config;
    daisy::AudioHandle::InputBuffer audioInputs;
    daisy::AudioHandle::OutputBuffer audioOutputs;
    daisy::AnalogControl* analogControls;
    daisy::DacHandle* dac;
    daisy::GateIn* gateInputs[2];
    dsy_gpio* gateOutputs[2];
    daisy::Switch switches[2];
    void* boardInstance;
};

typedef void (*sig_daisy_Host_onEvaluateSignals)(
    daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out,
    size_t size,
    struct sig_daisy_Host* host,
    void* userData);

typedef void (*sig_daisy_Host_afterEvaluateSignals)(
    daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out,
    size_t size,
    struct sig_daisy_Host* host,
    void* userData);

struct sig_daisy_Host {
    struct sig_daisy_Host_Impl* impl;
    struct sig_daisy_Host_Board board;
    struct sig_AudioSettings* audioSettings;
    struct sig_dsp_SignalEvaluator* evaluator;
    sig_daisy_Host_onEvaluateSignals onEvaluateSignals;
    sig_daisy_Host_afterEvaluateSignals afterEvaluateSignals;
    void* userData;
};

daisy::SaiHandle::Config::SampleRate sig_daisy_Host_convertSampleRate(
    float sampleRate);

void sig_daisy_Host_addOnEvaluateSignalsListener(
    struct sig_daisy_Host* self,
    sig_daisy_Host_onEvaluateSignals listener,
    void* userData);

void sig_daisy_Host_addAfterEvaluateSignalsListener(
    struct sig_daisy_Host* self,
    sig_daisy_Host_onEvaluateSignals listener,
    void* userData);

void sig_daisy_Host_audioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size);

void sig_daisy_Host_init(struct sig_daisy_Host* self);

void sig_daisy_Host_registerGlobalHost(struct sig_daisy_Host* host);

void sig_daisy_Host_noOpAudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size,
    struct sig_daisy_Host* host, void* userData);

typedef float (*sig_daisy_Host_getControlValue)(
    struct sig_daisy_Host* host, int control);

typedef void (*sig_daisy_Host_setControlValue)(
    struct sig_daisy_Host* host, int control, float value);

typedef float (*sig_daisy_Host_getGateValue)(
    struct sig_daisy_Host* host, int control);

typedef void (*sig_daisy_Host_setGateValue)(
    struct sig_daisy_Host* host, int control, float value);

typedef float (*sig_daisy_Host_getSwitchValue)(
    struct sig_daisy_Host* host, int control);

typedef void (*sig_daisy_Host_start)(struct sig_daisy_Host* host);
typedef void (*sig_daisy_Host_stop)(struct sig_daisy_Host* host);

struct sig_daisy_Host_Impl {
    sig_daisy_Host_getControlValue getControlValue;
    sig_daisy_Host_setControlValue setControlValue;
    sig_daisy_Host_getGateValue getGateValue;
    sig_daisy_Host_setGateValue setGateValue;
    sig_daisy_Host_getSwitchValue getSwitchValue;
    sig_daisy_Host_start start;
    sig_daisy_Host_stop stop;
};

/**
 * @brief Writes a bipolar float in the range of -1.0 to 1.0 to the
 * on-board Daisy DAC that has been configured for polling (i.e. not DMA) mode.
 *
 * @param dac a handle to the DAC
 * @param control the control to write to
 * @param value the value to write
 */
void sig_daisy_Host_writeValueToDACPolling(daisy::DacHandle* dac,
    int control, float value);

float sig_daisy_HostImpl_noOpGetControl(struct sig_daisy_Host* host,
    int control);

void sig_daisy_HostImpl_noOpSetControl(struct sig_daisy_Host* host,
    int control, float value);

/**
 * @brief Processes and returns the value for the specified Analog Control.
 * This implementation should work with most Daisy boards, assuming they
 * provide public access to an array of daisy::AnalogControls (which most do).
 *
 * @param host the host instance
 * @param control the control number to process
 * @return float the value of the control
 */
float sig_daisy_HostImpl_processControlValue(struct sig_daisy_Host* host,
    int control);

void sig_daisy_HostImpl_setControlValue(
    struct sig_daisy_Host* host, int control, float value);

float sig_daisy_HostImpl_getGateValue(struct sig_daisy_Host* host,
    int control);

void sig_daisy_HostImpl_setGateValue(struct sig_daisy_Host* host,
    int control, float value);

float sig_daisy_HostImpl_getSwitchValue(struct sig_daisy_Host* host,
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


struct sig_daisy_GateOut_Inputs {
    float_array_ptr source;
};

struct sig_daisy_GateOut {
    struct sig_dsp_Signal signal;
    struct sig_daisy_CV_Parameters parameters;
    struct sig_daisy_GateOut_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_daisy_Host* host;
};

struct sig_daisy_GateOut* sig_daisy_GateOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_daisy_Host* host);
void sig_daisy_GateOut_init(struct sig_daisy_GateOut* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host);
void sig_daisy_GateOut_generate(void* signal);
void sig_daisy_GateOut_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_GateOut* self);


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


struct sig_daisy_SwitchIn {
    struct sig_dsp_Signal signal;
    struct sig_daisy_CV_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_daisy_Host* host;
    daisy::Switch switchInstance;
};

struct sig_daisy_SwitchIn* sig_daisy_SwitchIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context,
    struct sig_daisy_Host* host);
void sig_daisy_SwitchIn_init(struct sig_daisy_SwitchIn* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host);
void sig_daisy_SwitchIn_generate(void* signal);
void sig_daisy_SwitchIn_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_SwitchIn* self);


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

struct sig_daisy_AudioParameters {
    int channel;
};

struct sig_daisy_AudioOut_Inputs {
    float_array_ptr source;
};

struct sig_daisy_AudioOut {
    struct sig_dsp_Signal signal;
    struct sig_daisy_AudioParameters parameters;
    struct sig_daisy_AudioOut_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_daisy_Host* host;
};

struct sig_daisy_AudioOut* sig_daisy_AudioOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context,
    struct sig_daisy_Host* host);
void sig_daisy_AudioOut_init(struct sig_daisy_AudioOut* self,
    struct sig_SignalContext* context, struct sig_daisy_Host* host);
void sig_daisy_AudioOut_generate(void* signal);
void sig_daisy_AudioOut_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_AudioOut* self);
