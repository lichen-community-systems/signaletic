#pragma once

#include "signaletic-daisy-host.h"
#include "sig-daisy-patch-sm.hpp"

enum {
    sig_HOST_CV_IN_1 = 0,
    sig_HOST_CV_IN_2,
    sig_HOST_CV_IN_3,
    sig_HOST_CV_IN_4,
    sig_HOST_CV_IN_5,
    sig_HOST_CV_IN_6
};

enum {
    sig_HOST_KNOB_1 = 6,
    sig_HOST_KNOB_2,
    sig_HOST_KNOB_3,
    sig_HOST_KNOB_4,
    sig_HOST_KNOB_5,
    sig_HOST_KNOB_6
};

namespace lichen {
class MediumModule {
    public:
        sig::libdaisy::ADCController<12> adcController;
        sig::libdaisy::GateInput gates[1];
        sig::libdaisy::InputBank<sig::libdaisy::GateInput, 1> gateBank;

};
namespace medium {
    static const size_t NUM_ADC_PINS = 12;

    // These pins are ordered based on the panel:
    // knobs first in labelled order, then CV jacks in labelled order.
    static dsy_gpio_pin ADC_PINS[NUM_ADC_PINS] = {
        sig::libdaisy::PatchSM::PIN_CV_5, // Knob one/POT_CV_1/Pin C8
        sig::libdaisy::PatchSM::PIN_CV_6, // Knob two/POT_CV_2/Pin C9
        sig::libdaisy::PatchSM::PIN_ADC_9, // Knob three/POT_CV_3/Pin A2
        sig::libdaisy::PatchSM::PIN_ADC_11, // Knob four/POT_CV_4/Pin A3
        sig::libdaisy::PatchSM::PIN_ADC_10, // Knob five/POT_CV_5/Pin D9
        sig::libdaisy::PatchSM::PIN_ADC_12, // Knob six/POT_CV_6/Pin D8

        sig::libdaisy::PatchSM::PIN_CV_1, // CV1 ("seven")/CV_IN_1/Pin C5
        sig::libdaisy::PatchSM::PIN_CV_2, // CV2 ("eight")/CV_IN_2Pin C4
        sig::libdaisy::PatchSM::PIN_CV_3, // CV3 ("nine")/CV_IN_3/Pin C3
        sig::libdaisy::PatchSM::PIN_CV_7, // CV6 ("ten")/CV_IN_6/Pin C7
        sig::libdaisy::PatchSM::PIN_CV_8, // CV5 ("eleven")/CV_IN_5/Pin C6
        sig::libdaisy::PatchSM::PIN_CV_4 // CV4 ("twelve")/CV_IN_4/Pin C2
    };

    static const sig::libdaisy::ModuleDefinition MediumDefinition = {
        .adcPins = ADC_PINS
    };
};
};
