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
    struct sig_AudioSettings settings = sig_DEFAULT_AUDIOSETTINGS;

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

    struct sig_dsp_Sine_Inputs inputs = {
        .freq = sig_AudioBlock_newWithValue(&allocator,
            &settings, 440.0f),
        .phaseOffset = sig_AudioBlock_newWithValue(&allocator,
            &settings, 0.0f),
        .mul = sig_AudioBlock_newWithValue(&allocator,
            &settings, 1.0f),
        .add = sig_AudioBlock_newWithValue(&allocator,
            &settings, 0.0f)
    };

    struct sig_dsp_Sine* sine = sig_dsp_Sine_new(&allocator,
        &settings, &inputs);

    puts("Sine wave (three blocks): ");
    for (int i = 0; i < 3; i++) {
        sine->signal.generate(sine);
        printBuffer(sine->signal.output, settings.blockSize);
    }

    return EXIT_SUCCESS;
}
