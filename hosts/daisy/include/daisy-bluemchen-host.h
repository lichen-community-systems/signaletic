#include "./signaletic-daisy-host.h"
#include "../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"

enum {
    sig_daisy_Bluemchen_CVIN_KNOB1 = kxmx::Bluemchen::Ctrl::CTRL_1,
    sig_daisy_Bluemchen_CVIN_KNOB2 = kxmx::Bluemchen::Ctrl::CTRL_2,
    sig_daisy_Bluemchen_CVIN_CV1 = kxmx::Bluemchen::Ctrl::CTRL_3,
    sig_daisy_Bluemchen_CVIN_CV2 = kxmx::Bluemchen::Ctrl::CTRL_4,
    sig_daisy_Bluemchen_CVIN_LAST = kxmx::Bluemchen::Ctrl::CTRL_LAST
};

const int sig_daisy_Bluemchen_NUM_ANALOG_CONTROLS = kxmx::Bluemchen::Ctrl::CTRL_LAST;

enum {
    sig_daisy_Nechmeulb_CVIN_KNOB1 = kxmx::Bluemchen::Ctrl::CTRL_1,
    sig_daisy_Nechmeulb_CVIN_KNOB2 = kxmx::Bluemchen::Ctrl::CTRL_2,
    sig_daisy_Nechmeulb_CVIN_LAST
};

enum {
    sig_daisy_Nechmeulb_CVOUT_1 = 0,
    sig_daisy_Nechmeulb_CVOUT_2,
    sig_daisy_Nechmeulb_CVOUT_BOTH,
    sig_daisy_Nechmeulb_CVOUT_LAST
};

extern struct sig_daisy_Host_Impl sig_daisy_BluemchenHostImpl;

void sig_daisy_BluemchenHostImpl_start(struct sig_daisy_Host* host);

void sig_daisy_BluemchenHostImpl_stop(struct sig_daisy_Host* host);

void sig_daisy_BluemchenHostImpl_setControlValue(
    struct sig_daisy_Host* host, int control, float value);

/**
 * @brief Always returns silence, since Bluemchen has no gate inputs.
 *
 * @param host the host instance
 * @param control the gate control (null is fine, since Bluemchen has no gates)
 * @return float always returns 0.0f
 */
float sig_daisy_BluemchenHostImpl_getGateValue(
    struct sig_daisy_Host* host, int control);

struct sig_daisy_BluemchenState {
    kxmx::Bluemchen* bluemchen;
};

void sig_daisy_BluemchenState_init(struct sig_daisy_BluemchenState* self,
    kxmx::Bluemchen* board);

struct sig_daisy_Host* sig_daisy_BluemchenHost_new(
    struct sig_Allocator* allocator, kxmx::Bluemchen* board,
    struct sig_dsp_SignalEvaluator* evaluator);

void sig_daisy_BluemchenHost_init(struct sig_daisy_Host* self,
    struct sig_daisy_BluemchenState* boardState,
    struct sig_dsp_SignalEvaluator* evaluator);

void sig_daisy_BluemchenHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self);
