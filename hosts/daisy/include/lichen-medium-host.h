#ifndef SIGNALETIC_LICHEN_MEDIUM_HOST_H
#define SIGNALETIC_LICHEN_MEDIUM_HOST_H

#include "./signaletic-daisy-host.h"
#include "../vendor/libDaisy/src/daisy_patch_sm.h"

enum {
    sig_lichen_Medium_CV_IN_1 = daisy::patch_sm::CV_1,
    sig_lichen_Medium_CV_IN_2 = daisy::patch_sm::CV_2,
    sig_lichen_Medium_CV_IN_3 = daisy::patch_sm::CV_3,
    sig_lichen_Medium_CV_IN_4 = daisy::patch_sm::CV_4,
    sig_lichen_Medium_CV_IN_5 = daisy::patch_sm::CV_5,
    sig_lichen_Medium_CV_IN_6 = daisy::patch_sm::CV_6,
    sig_lichen_Medium_KNOB_1 = daisy::patch_sm::CV_7,
    sig_lichen_Medium_KNOB_2 = daisy::patch_sm::CV_8,
    sig_lichen_Medium_KNOB_3 = daisy::patch_sm::ADC_9,
    sig_lichen_Medium_KNOB_4 = daisy::patch_sm::ADC_12,
    sig_lichen_Medium_KNOB_5 = daisy::patch_sm::ADC_10,
    sig_lichen_Medium_KNOB_6 = daisy::patch_sm::ADC_11,
    sig_lichen_Medium_CV_IN_LAST
};

enum {
    sig_lichen_Medium_CV_OUT_LED = daisy::patch_sm::CV_OUT_1,
    sig_lichen_Medium_CV_OUT_LAST
};

#endif // SIGNALETIC_LICHEN_MEDIUM_HOST_H
