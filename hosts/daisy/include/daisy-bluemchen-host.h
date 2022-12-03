#include "./signaletic-daisy-host.h"
#include "../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"

struct sig_daisy_BluemchenState {
    kxmx::Bluemchen* bluemchen;
};
