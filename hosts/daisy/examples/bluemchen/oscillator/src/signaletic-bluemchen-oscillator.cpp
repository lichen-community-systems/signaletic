#include <string>
#include <tlsf.h>
#include <libsignaletic.h>
#include "../../../../include/signaletic-daisy-host.hpp"
#include "../../../../include/kxmx-bluemchen-device.hpp"

using namespace kxmx::bluemchen;
using namespace daisy;
using namespace sig::libdaisy;

FixedCapStr<20> displayStr;

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

struct sig_dsp_Signal* listStorage[MAX_NUM_SIGNALS];
struct sig_List signals;
struct sig_dsp_SignalListEvaluator* evaluator;

DaisyHost<BluemchenDevice> host;

struct sig_host_FilteredCVIn* coarseFreqKnob;
struct sig_host_FilteredCVIn* fineFreqKnob;
struct sig_host_CVIn* vOctCVIn;
struct sig_dsp_BinaryOp* coarsePlusVOct;
struct sig_dsp_BinaryOp* coarseVOctPlusFine;
struct sig_dsp_LinearToFreq* frequency;
struct sig_dsp_Oscillator* osc;
struct sig_dsp_ConstantValue* gainLevel;
struct sig_dsp_BinaryOp* gain;
struct sig_host_AudioOut* leftOut;
struct sig_host_AudioOut* rightOut;

void UpdateOled() {
    host.device.display.Fill(false);

    displayStr.Clear();
    displayStr.Append("Starscill");
    host.device.display.SetCursor(0, 0);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.AppendFloat(frequency->outputs.main[0], 1);
    host.device.display.SetCursor(0, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    displayStr.Clear();
    displayStr.Append(" Hz");
    host.device.display.SetCursor(46, 8);
    host.device.display.WriteString(displayStr.Cstr(), Font_6x8, true);

    host.device.display.Update();
}

void buildSignalGraph(struct sig_SignalContext* context,
     struct sig_Status* status) {
    /** Frequency controls **/
    coarseFreqKnob = sig_host_FilteredCVIn_new(&allocator, context);
    coarseFreqKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, coarseFreqKnob, status);
    coarseFreqKnob->parameters.control = sig_host_KNOB_1;
    coarseFreqKnob->parameters.scale = 17.27f; // Nearly 20 KHz
    coarseFreqKnob->parameters.offset = -11.0f; // 0.1 Hz
    coarseFreqKnob->parameters.time = 0.01f;

    fineFreqKnob = sig_host_FilteredCVIn_new(&allocator, context);
    fineFreqKnob->hardware = &host.device.hardware;
    sig_List_append(&signals, fineFreqKnob, status);
    fineFreqKnob->parameters.control = sig_host_KNOB_2;
    // Fine frequency knob range is scaled to +/- 1 semitone
    fineFreqKnob->parameters.scale = 2.0f/12.0f;
    fineFreqKnob->parameters.offset = -(1.0f/12.0f);
    fineFreqKnob->parameters.time = 0.01f;

    vOctCVIn = sig_host_CVIn_new(&allocator, context);
    vOctCVIn->hardware = &host.device.hardware;
    sig_List_append(&signals, vOctCVIn, status);
    vOctCVIn->parameters.control = sig_host_CV_IN_1;
    vOctCVIn->parameters.scale = 5.0f;

    coarsePlusVOct = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, coarsePlusVOct, status);
    coarsePlusVOct->inputs.left = coarseFreqKnob->outputs.main;
    coarsePlusVOct->inputs.right = vOctCVIn->outputs.main;

    coarseVOctPlusFine = sig_dsp_Add_new(&allocator, context);
    sig_List_append(&signals, coarseVOctPlusFine, status);
    coarseVOctPlusFine->inputs.left = coarsePlusVOct->outputs.main;
    coarseVOctPlusFine->inputs.right = fineFreqKnob->outputs.main;

    frequency = sig_dsp_LinearToFreq_new(&allocator, context);
    sig_List_append(&signals, frequency, status);
    frequency->inputs.source = coarseVOctPlusFine->outputs.main;

    osc = sig_dsp_Sine_new(&allocator, context);
    sig_List_append(&signals, osc, status);
    osc->inputs.freq = frequency->outputs.main;

    /** Gain **/
    // The Daisy Seed's output circuit clips as it approaches full gain.
    // With the original AK556 codec, 0.85 seems to be around the practical
    // maximum value. On the newer model with the WM8731 codec, a lower
    // gain is required. 0.70f seems safe.
    // I haven't noticed an issue like this with modules based
    // on the Patch SM board.
    gainLevel = sig_dsp_ConstantValue_new(&allocator, context, 0.70f);

    gain = sig_dsp_Mul_new(&allocator, context);
    sig_List_append(&signals, gain, status);
    gain->inputs.left = osc->outputs.main;
    gain->inputs.right = gainLevel->outputs.main;
    sig_List_append(&signals, gain, status);

    leftOut = sig_host_AudioOut_new(&allocator, context);
    leftOut->hardware = &host.device.hardware;
    leftOut->parameters.channel = sig_host_AUDIO_OUT_1;
    leftOut->inputs.source = gain->outputs.main;
    sig_List_append(&signals, leftOut, status);

    rightOut = sig_host_AudioOut_new(&allocator, context);
    rightOut->hardware = &host.device.hardware;
    rightOut->parameters.channel = sig_host_AUDIO_OUT_2;
    rightOut->inputs.source = gain->outputs.main;
    sig_List_append(&signals, rightOut, status);
}

int main(void) {
    allocator.impl->init(&allocator);

    struct sig_AudioSettings audioSettings = {
        .sampleRate = 96000,
        .numChannels = 2,
        .blockSize = 1
    };

    struct sig_Status status;
    sig_Status_init(&status);
    sig_List_init(&signals, (void**) &listStorage, MAX_NUM_SIGNALS);

    evaluator = sig_dsp_SignalListEvaluator_new(&allocator, &signals);
    host.Init(&audioSettings, (struct sig_dsp_SignalEvaluator*) evaluator);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    buildSignalGraph(context, &status);
    host.Start();

    while (1) {
        UpdateOled();
    }
}
