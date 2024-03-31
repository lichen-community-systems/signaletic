#include "../include/signaletic-daisy-host.h"

void sig::libdaisy::DaisyHostAudioCallback(daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out, size_t size) {
    struct sig_host_HardwareInterface* hardware =
        sig_host_getGlobalHardwareInterface();
    hardware->audioInputChannels = (float**) in;
    hardware->audioOutputChannels = (float**) out;

    // Invoke callback before evaluating the signal graph.
    hardware->onEvaluateSignals(size, hardware);

    // Evaluate the signal graph.
    struct sig_dsp_SignalEvaluator* evaluator = hardware->evaluator;
    evaluator->evaluate(evaluator);

    // Invoke the after callback.
    hardware->afterEvaluateSignals(size, hardware);
}
