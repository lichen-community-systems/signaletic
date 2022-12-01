#include <stdio.h>
#include <stdlib.h>
#include "libsignaletic.h"

#define HEAP_SIZE 1024 * 128

char memory[HEAP_SIZE];

struct sig_AllocatorHeap heap = {
    .length = HEAP_SIZE,
    .memory = memory
};

struct sig_Allocator allocator = {
    .impl = &sig_TLSFAllocatorImpl,
    .heap = &heap
};

void printSample(float sample) {
    printf("%.8f", sample);
}

void printBuffer(float* buffer, size_t blockSize) {
    for (size_t i = 0; i < blockSize - 1; i++) {
        printSample(buffer[i]);
        printf(", ");
    }

    printSample(buffer[blockSize - 1]);
    printf("\n\n");
}

int main(int argc, char *argv[]) {
    struct sig_AudioSettings audioSettings = sig_DEFAULT_AUDIOSETTINGS;

    allocator.impl->init(&allocator);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);
    struct sig_dsp_Oscillator* sine = sig_dsp_Sine_new(&allocator, context);
    struct sig_dsp_ConstantValue* freq = sig_dsp_ConstantValue_new(&allocator,
        context, 440.0f);
    struct sig_dsp_ConstantValue* amp = sig_dsp_ConstantValue_new(&allocator,
        context, 1.0f);
    sine->inputs.freq = freq->outputs.main;
    sine->inputs.mul = amp->outputs.main;

    puts("Sine wave (three blocks): ");
    for (int i = 0; i < 3; i++) {
        sine->signal.generate(sine);
        printBuffer(sine->outputs.main, audioSettings.blockSize);
    }

    sig_dsp_ConstantValue_destroy(&allocator, freq);
    sig_dsp_ConstantValue_destroy(&allocator, amp);
    sig_dsp_Sine_destroy(&allocator, sine);
    sig_SignalContext_destroy(&allocator, context);

    return EXIT_SUCCESS;
}
