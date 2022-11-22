#include <stdio.h>
#include <stdlib.h>
#include "libsignaletic.h"

#define HEAP_SIZE 1024 * 128

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

    char memory[HEAP_SIZE];

    struct sig_AllocatorHeap heap = {
        .length = HEAP_SIZE,
        .memory = memory
    };

    struct sig_Allocator allocator = {
        .impl = &sig_TLSFAllocatorImpl,
        .heap = &heap
    };

    allocator.impl->init(&allocator);

    struct sig_SignalContext* context = sig_SignalContext_new(&allocator,
        &audioSettings);

    struct sig_dsp_Oscillator* sine = sig_dsp_Sine_new(&allocator, context);
    sine->inputs.freq = sig_AudioBlock_newWithValue(&allocator,
        &audioSettings, 440.0f);
    sine->inputs.mul = sig_AudioBlock_newWithValue(&allocator,
        &audioSettings, 1.0f);

    puts("Sine wave (three blocks): ");
    for (int i = 0; i < 3; i++) {
        sine->signal.generate(sine);
        printBuffer(sine->outputs.main, audioSettings.blockSize);
    }

    allocator.impl->free(&allocator, sine->inputs.freq);
    allocator.impl->free(&allocator, sine->inputs.mul);
    sig_dsp_Sine_destroy(&allocator, sine);
    sig_SignalContext_destroy(&allocator, context);

    return EXIT_SUCCESS;
}
