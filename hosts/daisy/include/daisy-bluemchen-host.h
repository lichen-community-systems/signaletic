#include "./signaletic-daisy-host.h"
#include "../vendor/kxmx_bluemchen/src/kxmx_bluemchen.h"

enum {
    sig_daisy_Bluemchen_CV_IN_KNOB_1 = kxmx::Bluemchen::Ctrl::CTRL_1,
    sig_daisy_Bluemchen_CV_IN_KNOB_2 = kxmx::Bluemchen::Ctrl::CTRL_2,
    sig_daisy_Bluemchen_CV_IN_CV1 = kxmx::Bluemchen::Ctrl::CTRL_3,
    sig_daisy_Bluemchen_CV_IN_CV2 = kxmx::Bluemchen::Ctrl::CTRL_4,
    sig_daisy_Bluemchen_CV_IN_LAST = kxmx::Bluemchen::Ctrl::CTRL_LAST
};

const int sig_daisy_Bluemchen_NUM_ANALOG_INPUTS = kxmx::Bluemchen::Ctrl::CTRL_LAST;
const int sig_daisy_Bluemchen_NUM_ANALOG_OUTPUTS = 0;
const int sig_daisy_Bluemchen_NUM_GATE_INPUTS = 0;
const int sig_daisy_Bluemchen_NUM_GATE_OUTPUTS = 0;
const int sig_daisy_Bluemchen_NUM_SWITCHES = 0;
const int sig_daisy_Bluemchen_NUM_ENCODERS = 1;

enum {
    sig_daisy_Nehcmeulb_CV_IN_KNOB1 = kxmx::Bluemchen::Ctrl::CTRL_1,
    sig_daisy_Nehcmeulb_CV_IN_KNOB2 = kxmx::Bluemchen::Ctrl::CTRL_2,
    sig_daisy_Nehcmeulb_CV_IN_LAST
};

enum {
    sig_daisy_Nehcmeulb_CV_OUT_1 = 0,
    sig_daisy_Nehcmeulb_CV_OUT_2,
    sig_daisy_Nehcmeulb_CV_OUT_BOTH,
    sig_daisy_Nehcmeulb_CV_OUT_LAST
};

const int sig_daisy_Nehcmeulb_NUM_ANALOG_INPUTS = sig_daisy_Nehcmeulb_CV_IN_LAST;
const int sig_daisy_Nehcmeulb_NUM_ANALOG_OUTPUTS = 2;
const int sig_daisy_Nehcmeulb_NUM_GATE_INPUTS = 0;
const int sig_daisy_Nehcmeulb_NUM_GATE_OUTPUTS = 0;
const int sig_daisy_Nehcmeulb_NUM_SWITCHES = 0;
const int sig_daisy_Nehcmeulb_NUM_ENCODERS = 1;

extern struct sig_daisy_Host_Impl sig_daisy_BluemchenHostImpl;

void sig_daisy_BluemchenHostImpl_setControlValue(struct sig_daisy_Host* host,
    int control, float value);

void sig_daisy_BluemchenHostImpl_start(struct sig_daisy_Host* host);

void sig_daisy_BluemchenHostImpl_stop(struct sig_daisy_Host* host);

/**
 * @brief Creates a new BluemchenHost instance.
 *
 * @param allocator the allocator to use
 * @param bluemchen the Bluemchen board support object
 * @param evaluator a signal evaluator to use in the audio callback
 * @return struct sig_daisy_Host*
 */
struct sig_daisy_Host* sig_daisy_BluemchenHost_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    kxmx::Bluemchen* bluemchen,
    struct sig_dsp_SignalEvaluator* evaluator);

/**
 * @brief Initializes a Bluemchen board object with the specified configuration
 *
 * @param self the BluemchenHost_Board instance to initialize
 * @param bluemchen the Blumchen board support object
 * @param boardConfig the board configuration object to use (e.g. for the Bluemchen or Nehcmeulb)
 */
void sig_daisy_BluemchenHost_Board_init(struct sig_daisy_Host_Board* self,
    kxmx::Bluemchen* bluemchen,
    struct sig_daisy_Host_BoardConfiguration* boardConfig);

/**
 * @brief Initializes the state of a BluemchenHost instance.
 *
 * @param self the BluemchenHost to initialize
 * @param bluemchen the Bluemchen board support object
 * @param evaluator a signal evaluator to use in the audio callback
 */
void sig_daisy_BluemchenHost_init(struct sig_daisy_Host* self,
    struct sig_AudioSettings* audioSettings,
    struct sig_daisy_Host_BoardConfiguration* boardConfig,
    kxmx::Bluemchen* bluemchen,
    struct sig_dsp_SignalEvaluator* evaluator);

/**
 * @brief Destroys a BluemchenHost instance.
 *
 * @param allocator the allocator to use
 * @param self the BluemchenHost to destroy
 */
void sig_daisy_BluemchenHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self);


struct sig_daisy_Host* sig_daisy_NehcmeulbHost_new(
    struct sig_Allocator* allocator,
    struct sig_AudioSettings* audioSettings,
    kxmx::Bluemchen* bluemchen,
    struct sig_dsp_SignalEvaluator* evaluator);

void sig_daisy_NehcmeulbHost_destroy(struct sig_Allocator* allocator,
    struct sig_daisy_Host* self);
