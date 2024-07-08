#pragma once

#include "signaletic-host.h"
#include "signaletic-daisy-host.hpp"
#include "sig-daisy-patch-sm.hpp"

using namespace sig::libdaisy;

enum {
    sig_host_AUDIO_IN_1 = 0,
    sig_host_AUDIO_IN_2
};

enum {
    sig_host_AUDIO_OUT_1 = 0,
    sig_host_AUDIO_OUT_2
};

namespace electrosmith {
    class BasePatchSMDevice {
        public:
            patchsm::PatchSMBoard board;
            struct sig_host_HardwareInterface hardware;
    };
};
