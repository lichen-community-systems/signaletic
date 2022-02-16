#include <stdio.h>
#include <stdlib.h>
#include "libstar.h"

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
    struct star_AudioSettings settings = star_DEFAULT_AUDIOSETTINGS;

    char heap[HEAP_SIZE];

    struct star_Allocator allocator = {
        .heapSize = HEAP_SIZE,
        .heap = heap
    };
    star_Allocator_init(&allocator);

    struct star_sig_Sine_Inputs inputs = {
        .freq = star_AudioBlock_newWithValue(&allocator,
            &settings, 440.0f),
        .phaseOffset = star_AudioBlock_newWithValue(&allocator,
            &settings, 0.0f),
        .mul = star_AudioBlock_newWithValue(&allocator,
            &settings, 1.0f),
        .add = star_AudioBlock_newWithValue(&allocator,
            &settings, 0.0f)
    };

    struct star_sig_Sine* sine = star_sig_Sine_new(&allocator,
        &settings, &inputs);

    puts("Sine wave (three blocks): ");
    for (int i = 0; i < 3; i++) {
        sine->signal.generate(sine);
        printBuffer(sine->signal.output, settings.blockSize);
    }

    return EXIT_SUCCESS;
}
