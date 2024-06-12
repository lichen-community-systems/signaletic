#include <libsignaletic.h>
#include "../../../../include/kxmx-bluemchen-device.hpp"

#define SAMPLERATE 48000
#define HEAP_SIZE 1024 * 256 // 256KB
#define MAX_NUM_SIGNALS 32

uint8_t memory[HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = (void*) memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

daisy::FixedCapStr<20> displayStr;
struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;
sig::libdaisy::DaisyHost<kxmx::bluemchen::BluemchenDevice> host;

struct sig_dsp_ConstantValue* ampScale;
struct sig_host_EncoderIn* encoderIn;
struct sig_dsp_Accumulate* encoderAccumulator;
struct sig_dsp_Branch* leftCalibrationSelector;
struct sig_dsp_Branch* rightCalibrationSelector;
struct sig_host_CVIn* cv1In;
struct sig_host_CVIn* cv2In;
struct sig_dsp_Calibrator* cv1Calibrator;
struct sig_dsp_Calibrator* cv2Calibrator;

struct sig_dsp_Oscillator* sine1;
struct sig_dsp_LinearToFreq* sine1Voct;
struct sig_dsp_Oscillator* sine2;
struct sig_dsp_LinearToFreq* sine2Voct;

struct sig_host_AudioOut* audio1Out;
struct sig_host_AudioOut* audio2Out;

void UpdateOled() {
    host.device.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Calibrator");
    host.device.display.SetCursor(0, 0);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    float sine1Hz = sine1Voct->outputs.main[0];
    float sine2Hz = sine2Voct->outputs.main[0];
    float calibrationSide = (int) encoderAccumulator->outputs.main[0];
    int stage = (int) (calibrationSide == 0.0f ?
        cv1Calibrator->stage : cv2Calibrator->stage);
    displayStr.Clear();
    displayStr.AppendFloat(sine1Hz, 3);
    displayStr.Append("Hz");
    host.device.display.SetCursor(0, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.AppendFloat(sine2Hz, 3);
    displayStr.Append("Hz");
    host.device.display.SetCursor(0, 16);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append("CV ");
    displayStr.AppendInt((int) calibrationSide + 1);
    displayStr.Append(" - ");
    displayStr.AppendInt(stage);
    displayStr.Append("V");
    host.device.display.SetCursor(0, 24);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    host.device.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context, struct sig_Status* status) {
    encoderIn = sig_host_EncoderIn_new(&allocator, context);
    encoderIn->hardware = &host.device.hardware;
    sig_List_append(&signals, encoderIn, status);
    encoderIn->parameters.buttonControl = sig_host_BUTTON_1;
    encoderIn->parameters.turnControl = sig_host_ENCODER_1;

    encoderAccumulator = sig_dsp_Accumulate_new(&allocator, context);
    sig_List_append(&signals, encoderAccumulator, status);
    encoderAccumulator->parameters.accumulatorStart = 0.0f;
    // Due to a bug in sig_dsp_Accumulate, we have to manually set
    // the accumulator state. Signaletic needs dataflow support for
    // parameters and connections!
    encoderAccumulator->accumulator =
        encoderAccumulator->parameters.accumulatorStart;
    encoderAccumulator->parameters.maxValue = 1.0f;
    encoderAccumulator->parameters.wrap = 1.0f;
    encoderAccumulator->inputs.source = encoderIn->outputs.increment;

    leftCalibrationSelector = sig_dsp_Branch_new(&allocator, context);
    sig_List_append(&signals, leftCalibrationSelector, status);
    leftCalibrationSelector->inputs.condition =
        encoderAccumulator->outputs.main;
    leftCalibrationSelector->inputs.on = context->silence->outputs.main;
    leftCalibrationSelector->inputs.off = encoderIn->outputs.button;

    rightCalibrationSelector = sig_dsp_Branch_new(&allocator, context);
    sig_List_append(&signals, rightCalibrationSelector, status);
    rightCalibrationSelector->inputs.condition =
        encoderAccumulator->outputs.main;
    rightCalibrationSelector->inputs.on = encoderIn->outputs.button;
    rightCalibrationSelector->inputs.off = context->silence->outputs.main;

    cv1In = sig_host_CVIn_new(&allocator, context);
    cv1In->hardware = &host.device.hardware;
    sig_List_append(&signals, cv1In, status);
    cv1In->parameters.control = sig_host_CV_IN_1;
    cv1In->parameters.scale = 5.0f;

    cv1Calibrator = sig_dsp_Calibrator_new(&allocator, context);
    sig_List_append(&signals, cv1Calibrator, status);
    cv1Calibrator->inputs.source = cv1In->outputs.main;
    cv1Calibrator->inputs.gate = leftCalibrationSelector->outputs.main;

    cv2In = sig_host_CVIn_new(&allocator, context);
    cv2In->hardware = &host.device.hardware;
    sig_List_append(&signals, cv2In, status);
    cv2In->parameters.control = sig_host_CV_IN_2;
    cv2In->parameters.scale = 5.0f;

    cv2Calibrator = sig_dsp_Calibrator_new(&allocator, context);
    sig_List_append(&signals, cv2Calibrator, status);
    cv2Calibrator->inputs.source = cv2In->outputs.main;
    cv2Calibrator->inputs.gate = rightCalibrationSelector->outputs.main;

    ampScale = sig_dsp_ConstantValue_new(&allocator, context, 0.5f);

    sine1Voct = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, sine1Voct, status);
    sine1Voct->inputs.source = cv1Calibrator->outputs.main;

    sine1 = sig_dsp_Sine_new(&allocator, context);
    sig_List_append(&signals, sine1, status);
    sine1->inputs.freq = sine1Voct->outputs.main;
    sine1->inputs.mul = ampScale->outputs.main;

    audio1Out = sig_host_AudioOut_new(&allocator, context);
    audio1Out->hardware = &host.device.hardware;
    sig_List_append(&signals, audio1Out, status);
    audio1Out->parameters.channel = sig_host_AUDIO_OUT_1;
    audio1Out->inputs.source = sine1->outputs.main;

    sine2Voct = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, sine2Voct, status);
    sine2Voct->inputs.source = cv2Calibrator->outputs.main;

    sine2 = sig_dsp_Sine_new(&allocator, context);
    sig_List_append(&signals, sine2, status);
    sine2->inputs.freq = sine2Voct->outputs.main;
    sine2->inputs.mul = ampScale->outputs.main;

    audio2Out = sig_host_AudioOut_new(&allocator, context);
    audio2Out->hardware = &host.device.hardware;
    sig_List_append(&signals, audio2Out, status);
    audio2Out->parameters.channel = sig_host_AUDIO_OUT_2;
    audio2Out->inputs.source = sine2->outputs.main;
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = SAMPLERATE,
        .numChannels = 2,
        .blockSize = 48
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    buildSignalGraph(context, &status);

    host.Start();

    while (1) {
        UpdateOled();
    }
}
