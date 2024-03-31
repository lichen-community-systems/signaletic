#ifndef SIGNALETIC_HOST_H
#define SIGNALETIC_HOST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libsignaletic.h>
#include <stddef.h> // For size_t

struct sig_host_HardwareInterface;

typedef void (*sig_host_onEvaluateSignals)(
    size_t size,
    struct sig_host_HardwareInterface* hardware);

typedef void (*sig_host_afterEvaluateSignals)(
    size_t size,
    struct sig_host_HardwareInterface* hardware);

void sig_host_noOpAudioEventCallback(size_t size,
    struct sig_host_HardwareInterface* hardware);


struct sig_host_HardwareInterface {
    struct sig_dsp_SignalEvaluator* evaluator;

    sig_host_onEvaluateSignals onEvaluateSignals;
    sig_host_afterEvaluateSignals afterEvaluateSignals;

    void* userData;

    size_t numAudioInputChannels;
    float** audioInputChannels;

    size_t numAudioOutputChannels;
    float** audioOutputChannels;

    size_t numADCChannels;
    float* adcChannels;

    size_t numDACChannels;
    float* dacChannels;

    size_t numGateInputs;
    float* gateInputs;

    size_t numGPIOOutputs;
    float* gpioOutputs;

    size_t numToggles;
    float* toggles;

    size_t numTriSwitches;
    float* triSwitches;
};

void sig_host_registerGlobalHardwareInterface(
    struct sig_host_HardwareInterface* hardware);

struct sig_host_HardwareInterface* sig_host_getGlobalHardwareInterface();

struct sig_host_CV_Parameters {
    float scale;
    float offset;
    int control;
};


struct sig_host_GateIn {
    struct sig_dsp_Signal signal;
    struct sig_host_CV_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
};

struct sig_host_GateIn* sig_host_GateIn_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_host_GateIn_init(struct sig_host_GateIn* self,
    struct sig_SignalContext* context);
void sig_host_GateIn_generate(void* signal);
void sig_host_GateIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_GateIn* self);

struct sig_host_GateOut_Inputs {
    float_array_ptr source;
};


struct sig_host_GateOut {
    struct sig_dsp_Signal signal;
    struct sig_host_CV_Parameters parameters;
    struct sig_host_GateOut_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
};

struct sig_host_GateOut* sig_host_GateOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_host_GateOut_init(struct sig_host_GateOut* self,
    struct sig_SignalContext* context);
void sig_host_GateOut_generate(void* signal);
void sig_host_GateOut_destroy(struct sig_Allocator* allocator,
    struct sig_host_GateOut* self);


struct sig_host_CVIn {
    struct sig_dsp_Signal signal;
    struct sig_host_CV_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
};

struct sig_host_CVIn* sig_host_CVIn_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_host_CVIn_init(struct sig_host_CVIn* self,
    struct sig_SignalContext* context);
void sig_host_CVIn_generate(void* signal);
void sig_host_CVIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_CVIn* self);


struct sig_host_FilteredCVIn_Parameters {
    float scale;
    float offset;
    int control;
    float time;
};

struct sig_host_FilteredCVIn {
    struct sig_dsp_Signal signal;
    struct sig_host_FilteredCVIn_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
    struct sig_host_CVIn* cvIn;
    struct sig_dsp_Smooth* filter;
};

struct sig_host_FilteredCVIn* sig_host_FilteredCVIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_host_FilteredCVIn_init(struct sig_host_FilteredCVIn* self,
    struct sig_SignalContext* context);
void sig_host_FilteredCVIn_generate(void* signal);
void sig_host_FilteredCVIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_FilteredCVIn* self);


struct sig_host_VOctCVIn_Parameters {
    float scale;
    float offset;
    int control;
    float middleFreq;
};

struct sig_host_VOctCVIn {
    struct sig_dsp_Signal signal;
    struct sig_host_VOctCVIn_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
    struct sig_host_CVIn* cvIn;
    struct sig_dsp_LinearToFreq* cvConverter;
};

struct sig_host_VOctCVIn* sig_host_VOctCVIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_host_VOctCVIn_init(struct sig_host_VOctCVIn* self,
    struct sig_SignalContext* context);
void sig_host_VOctCVIn_generate(void* signal);
void sig_host_VOctCVIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_VOctCVIn* self);


struct sig_host_CVOut_Inputs {
    float_array_ptr source;
};

// TODO: Should we have a "no output" outputs type,
// or continue with the idea of passing through the input
// for chaining?
struct sig_host_CVOut {
    struct sig_dsp_Signal signal;
    struct sig_host_CV_Parameters parameters;
    struct sig_host_CVOut_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
};

struct sig_host_CVOut* sig_host_CVOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_host_CVOut_init(struct sig_host_CVOut* self,
    struct sig_SignalContext* context);
void sig_host_CVOut_generate(void* signal);
void sig_host_CVOut_destroy(struct sig_Allocator* allocator,
    struct sig_host_CVOut* self);


struct sig_host_AudioParameters {
    int channel;
    float scale;
};

struct sig_host_AudioOut_Inputs {
    float_array_ptr source;
};

struct sig_host_AudioOut {
    struct sig_dsp_Signal signal;
    struct sig_host_AudioParameters parameters;
    struct sig_host_AudioOut_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
};

struct sig_host_AudioOut* sig_host_AudioOut_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_host_AudioOut_init(struct sig_host_AudioOut* self,
    struct sig_SignalContext* context);
void sig_host_AudioOut_generate(void* signal);
void sig_host_AudioOut_destroy(struct sig_Allocator* allocator,
    struct sig_host_AudioOut* self);


struct sig_host_AudioIn {
    struct sig_dsp_Signal signal;
    struct sig_host_AudioParameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
};

struct sig_host_AudioIn* sig_host_AudioIn_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context);
void sig_host_AudioIn_init(struct sig_host_AudioIn* self,
    struct sig_SignalContext* context);
void sig_host_AudioIn_generate(void* signal);
void sig_host_AudioIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_AudioIn* self);



// TODO: Replace SwitchIn and TriSwitchIn with a more generic
// implementation that operates on a configurable list of GPIO pins
// (since a three way switch just consists of two separate pins).

struct sig_host_SwitchIn {
    struct sig_dsp_Signal signal;
    struct sig_host_CV_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
};

struct sig_host_SwitchIn* sig_host_SwitchIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_host_SwitchIn_init(struct sig_host_SwitchIn* self,
    struct sig_SignalContext* context);
void sig_host_SwitchIn_generate(void* signal);
void sig_host_SwitchIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_SwitchIn* self);

struct sig_host_TriSwitchIn {
    struct sig_dsp_Signal signal;
    struct sig_host_CV_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
};

struct sig_host_TriSwitchIn* sig_host_TriSwitchIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_host_TriSwitchIn_init(struct sig_host_TriSwitchIn* self,
    struct sig_SignalContext* context);
void sig_host_TriSwitchIn_generate(void* signal);
void sig_host_TriSwitchIn_destroy(struct sig_Allocator* allocator,
    struct sig_host_TriSwitchIn* self);

#ifdef __cplusplus
}
#endif

#endif /* SIGNALETIC_HOST_H */
