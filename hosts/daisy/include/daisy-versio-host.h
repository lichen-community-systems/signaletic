#ifndef SIGNALETIC_DAISY_VERSIO_H
#define SIGNALETIC_DAISY_VERSIO_H

#include "./signaletic-daisy-host.h"
#include "../vendor/libDaisy/src/daisy_versio.h"

enum {
    sig_daisy_Versio_CV_IN_1 = daisy::DaisyVersio::AV_KNOBS::KNOB_0,
    sig_daisy_Versio_CV_IN_2 = daisy::DaisyVersio::AV_KNOBS::KNOB_1,
    sig_daisy_Versio_CV_IN_3 = daisy::DaisyVersio::AV_KNOBS::KNOB_2,
    sig_daisy_Versio_CV_IN_4 = daisy::DaisyVersio::AV_KNOBS::KNOB_3,
    sig_daisy_Versio_CV_IN_5 = daisy::DaisyVersio::AV_KNOBS::KNOB_4,
    sig_daisy_Versio_CV_IN_6 = daisy::DaisyVersio::AV_KNOBS::KNOB_5,
    sig_daisy_Versio_CV_IN_7 = daisy::DaisyVersio::AV_KNOBS::KNOB_6,
    sig_daisy_Versio_CV_IN_LAST
};

const int sig_daisy_Versio_NUM_ANALOG_INPUTS = sig_daisy_Versio_CV_IN_LAST;
const int sig_daisy_Versio_NUM_ANALOG_OUTPUTS = 0;
const int sig_daisy_Versio_NUM_GATE_INPUTS = 0;
const int sig_daisy_Versio_NUM_GATE_OUTPUTS = 0;
const int sig_daisy_Versio_NUM_ENCODERS = 0;

// Versio has one button
enum {
    sig_daisy_Versio_SWITCH_1,
    sig_daisy_Versio_SWITCH_LAST
};

const int sig_daisy_Versio_NUM_SWITCHES = sig_daisy_Versio_SWITCH_LAST;

enum {
    sig_daisy_Versio_TRI_SWITCH_1 = daisy::DaisyVersio::AV_TOGGLE3::SW_0,
    sig_daisy_Versio_TRI_SWITCH_2 = daisy::DaisyVersio::AV_TOGGLE3::SW_1,
    sig_daisy_Versio_TRI_SWITCH_LAST
};

const int sig_daisy_Versio_NUM_TRI_SWITCHES = sig_daisy_Versio_TRI_SWITCH_LAST;


// TODO: Add support for RGB LEDs.

extern struct sig_daisy_Host_Impl sig_daisy_VersioHostImpl;

void sig_daisy_VersioHostImpl_start(
    struct sig_daisy_Host* host);

void sig_daisy_VersioHostImpl_stop(
    struct sig_daisy_Host* host);

struct sig_daisy_Host* sig_daisy_VersioHost_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    daisy::DaisyVersio* versio,
    struct sig_dsp_SignalEvaluator* evaluator);

void sig_daisy_VersioHost_Board_init(struct sig_daisy_Host_Board* self,
    daisy::DaisyVersio* versio);

void sig_daisy_VersioHost_init(struct sig_daisy_Host* self,
    struct sig_AudioSettings* audioSettings,
    daisy::DaisyVersio* versio,
    struct sig_dsp_SignalEvaluator* evaluator);

#endif // SIGNALETIC_DAISY_VERSIO_H
