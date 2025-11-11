#include "../include/looper.h"
#include "../../../../include/kxmx-bluemchen-device.hpp"
#include "../../../../include/looper-view.h"
#include <string>

#define SAMPLERATE 48000
#define LOOP_TIME_SECS 60
#define LOOP_LENGTH SAMPLERATE * LOOP_TIME_SECS
#define SIGNAL_HEAP_SIZE 1024 * 384
#define MAX_NUM_SIGNALS 32
#define LONG_ENCODER_PRESS 2.0f

float DSY_SDRAM_BSS leftSamples[LOOP_LENGTH];
struct sig_Buffer leftBuffer = {
    .length = LOOP_LENGTH,
    .samples = leftSamples
};
float DSY_SDRAM_BSS rightSamples[LOOP_LENGTH];
struct sig_Buffer rightBuffer = {
    .length = LOOP_LENGTH,
    .samples = rightSamples
};

char signalMemory[SIGNAL_HEAP_SIZE];
struct sig_AllocatorHeap heap = {
    .length = SIGNAL_HEAP_SIZE,
    .memory = (void*) signalMemory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;
sig::libdaisy::DaisyHost<kxmx::bluemchen::BluemchenDevice> host;
struct sig_ui_daisy_LooperView looperView;

struct sig_host_EncoderIn* encoder;
struct sig_dsp_ConstantValue* encoderSpeedScale;
struct sig_dsp_BinaryOp* encoderSpeedIncrement;
struct sig_dsp_Accumulate* speedAccumulator;
struct sig_host_FilteredCVIn* startKnob;
struct sig_host_FilteredCVIn* endKnob;
struct sig_host_FilteredCVIn* speedCV;
struct sig_host_FilteredCVIn* skewCV;
struct sig_dsp_BinaryOp* speedAdder;
struct sig_dsp_Invert* leftSpeedSkewInverter;
struct sig_dsp_BinaryOp* leftSpeed;
struct sig_dsp_BinaryOp* rightSpeed;
struct sig_dsp_ConstantValue* tapDuration;
struct sig_dsp_ConstantValue* longPressDuration;
struct sig_dsp_TimedTriggerCounter* encoderTap;
struct sig_dsp_GatedTimer* encoderLongPress;
struct sig_dsp_ToggleGate* recordGate;
struct sig_host_AudioIn* leftIn;
struct sig_host_AudioIn* rightIn;
struct sig_dsp_Looper* leftLooper;
struct sig_dsp_Looper* rightLooper;
struct sig_dsp_BinaryOp* leftGain;
struct sig_dsp_BinaryOp* rightGain;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void UpdateOled() {
    // Note: this is required until we have something that
    // offers some way to get the current value from a Signal.
    // See https://github.com/continuing-creativity/signaletic/issues/19
    size_t lastSampIdx = leftSpeed->signal.audioSettings->blockSize - 1;
    looperView.leftSpeed = leftSpeed->outputs.main[lastSampIdx];
    looperView.rightSpeed = rightSpeed->outputs.main[lastSampIdx];

    bool foregroundOn = !recordGate->isGateOpen;
    host.device.display.Fill(!foregroundOn);

    sig_ui_daisy_LooperView_render(&looperView,
        startKnob->filter->previousSample,
        endKnob->filter->previousSample,
        leftLooper->playbackPos,
        foregroundOn);

    host.device.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
    struct sig_Status* status) {
    startKnob = sig_host_FilteredCVIn_new(&allocator, context);
    startKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, startKnob, status);
    startKnob->parameters.control = sig_host_KNOB_1;

    endKnob = sig_host_FilteredCVIn_new(&allocator, context);
    endKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, endKnob, status);
    endKnob->parameters.control = sig_host_KNOB_2;

    encoder = sig_host_EncoderIn_new(&allocator, context);
    encoder->hardware = &host.device.hardware;
    sig_List_append(&signals, encoder, status);
    encoder->parameters.turnControl = sig_host_ENCODER_1;
    encoder->parameters.buttonControl = sig_host_BUTTON_1;

    encoderSpeedScale = sig_dsp_ConstantValue_new(&allocator, context, 0.01f);

    encoderSpeedIncrement = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, encoderSpeedIncrement, status);
    encoderSpeedIncrement->inputs.left = encoder->outputs.increment;
    encoderSpeedIncrement->inputs.right = encoderSpeedScale->outputs.main;

    speedAccumulator = sig_dsp_Accumulate_new(&allocator, context);
    sig_List_append(&signals, speedAccumulator, status);
    speedAccumulator->parameters.accumulatorStart = 1.0f;
    speedAccumulator->inputs.source = encoderSpeedIncrement->outputs.main;

    speedCV = sig_host_FilteredCVIn_new(&allocator, context);
    speedCV->hardware = &host.device.hardware;
    sig_List_append(&signals, speedCV, status);
    speedCV->parameters.control = sig_host_CV_IN_1;
    speedCV->parameters.scale = 2.0f;

    speedAdder = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, speedAdder, status);
    speedAdder->inputs.left = speedAccumulator->outputs.main;
    speedAdder->inputs.right = speedCV->outputs.main;

    skewCV = sig_host_FilteredCVIn_new(&allocator, context);
    skewCV->hardware = &host.device.hardware;
    sig_List_append(&signals, skewCV, status);
    skewCV->parameters.control = sig_host_CV_IN_2;
    skewCV->parameters.scale = 0.5f;

    leftSpeedSkewInverter = sig_dsp_Invert_new(&allocator, context);
    sig_List_append(&signals, leftSpeedSkewInverter, status);
    leftSpeedSkewInverter->inputs.source = skewCV->outputs.main;

    leftSpeed = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, leftSpeed, status);
    leftSpeed->inputs.left = speedAdder->outputs.main;
    leftSpeed->inputs.right = leftSpeedSkewInverter->outputs.main;

    rightSpeed = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, rightSpeed, status);
    rightSpeed->inputs.left = speedAdder->outputs.main;
    rightSpeed->inputs.right = skewCV->outputs.main;

    tapDuration = sig_dsp_ConstantValue_new(&allocator, context, 0.5f);

    encoderTap = sig_dsp_TimedTriggerCounter_new(&allocator, context);
    sig_List_append(&signals, encoderTap, status);
    encoderTap->inputs.source = encoder->outputs.button;
    encoderTap->inputs.duration = tapDuration->outputs.main;
    encoderTap->inputs.count = context->unity->outputs.main;

    recordGate = sig_dsp_ToggleGate_new(&allocator, context);
    sig_List_append(&signals, recordGate, status);
    recordGate->inputs.trigger = encoderTap->outputs.main;

    longPressDuration = sig_dsp_ConstantValue_new(&allocator, context,
        LONG_ENCODER_PRESS);

    encoderLongPress = sig_dsp_GatedTimer_new(&allocator, context);
    sig_List_append(&signals, encoderLongPress, status);
    encoderLongPress->inputs.gate = encoder->outputs.button;
    encoderLongPress->inputs.duration = longPressDuration->outputs.main;

    leftIn = sig_host_AudioIn_new(&allocator, context);
    leftIn->hardware = &host.device.hardware;
    sig_List_append(&signals, leftIn, status);
    leftIn->parameters.channel = sig_host_AUDIO_IN_1;

    leftLooper = sig_dsp_Looper_new(&allocator, context);
    sig_List_append(&signals, leftLooper, status);
    leftLooper->inputs.source = leftIn->outputs.main;
    leftLooper->inputs.start = startKnob->outputs.main;
    leftLooper->inputs.end = endKnob->outputs.main;
    leftLooper->inputs.speed = leftSpeed->outputs.main;
    leftLooper->inputs.record = recordGate->outputs.main;
    leftLooper->inputs.clear = encoderLongPress->outputs.main;

    // TODO: Need better buffer management.
    sig_fillWithSilence(leftSamples, LOOP_LENGTH);
    sig_dsp_Looper_setBuffer(leftLooper, &leftBuffer);

    rightIn = sig_host_AudioIn_new(&allocator, context);
    rightIn->hardware = &host.device.hardware;
    sig_List_append(&signals, rightIn, status);
    rightIn->parameters.channel = sig_host_AUDIO_IN_2;

    rightLooper = sig_dsp_Looper_new(&allocator, context);
    sig_List_append(&signals, rightLooper, status);
    rightLooper->inputs.source = rightIn->outputs.main;
    rightLooper->inputs.start = startKnob->outputs.main;
    rightLooper->inputs.end = endKnob->outputs.main;
    rightLooper->inputs.speed = rightSpeed->outputs.main;
    rightLooper->inputs.record = recordGate->outputs.main;
    rightLooper->inputs.clear = encoderLongPress->outputs.main;

    sig_fillWithSilence(rightSamples, LOOP_LENGTH);
    sig_dsp_Looper_setBuffer(rightLooper, &rightBuffer);

    // Bluemchen's output circuit clips as it approaches full gain,
    // so 0.85 seems to be around the practical maximum value.
    // TODO: This should be moved into the Device implemenation.
    struct sig_dsp_ConstantValue* gainAmount = sig_dsp_ConstantValue_new(
        &allocator, context, 0.70f);

    leftGain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, leftGain, status);
    leftGain->inputs.left = leftLooper->outputs.main;
    leftGain->inputs.right = gainAmount->outputs.main;

    rightGain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, rightGain, status);
    rightGain->inputs.left = rightLooper->outputs.main;
    rightGain->inputs.right = gainAmount->outputs.main;

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    sig_List_append(&signals, leftOut, status);
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = leftGain->outputs.main;

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    sig_List_append(&signals, rightOut, status);
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->inputs.source = rightGain->outputs.main;
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

    struct sig_ui_Rect looperViewRect = {
        .x = 0,
        .y = 8,
        .width = 64,
        .height = 24
    };

    struct sig_ui_daisy_Canvas canvas = {
        .geometry = sig_ui_daisy_Canvas_geometryFromRect(&looperViewRect),
        .display = &host.device.display
    };

    struct sig_ui_daisy_LoopRenderer loopRenderer = {
        .canvas = &canvas,
        .loop = &leftLooper->loop
    };

    looperView.canvas = &canvas;
    looperView.loopRenderer = &loopRenderer;
    looperView.looper = leftLooper;
    looperView.leftSpeed = 0.0f;
    looperView.rightSpeed = 0.0f;
    looperView.positionLineThickness = 1;
    looperView.loopPointLineThickness = 2;

    while (1) {
        UpdateOled();
    }
}
