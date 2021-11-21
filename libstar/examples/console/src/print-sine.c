#include <stdio.h>
#include <stdlib.h>
#include "libstar.h"

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
    struct star_AudioSettings audioSettings = star_DEFAULT_AUDIOSETTINGS;

    float output[audioSettings.blockSize];
    star_Buffer_fillSilence(output, audioSettings.blockSize);

    float freq[audioSettings.blockSize];
    star_Buffer_fill(440.0f, freq, audioSettings.blockSize);

    float phaseOffset[audioSettings.blockSize];
    star_Buffer_fill(0.0f, phaseOffset, audioSettings.blockSize);

    float mul[audioSettings.blockSize];
    star_Buffer_fill(1.0f, mul, audioSettings.blockSize);

    float add[audioSettings.blockSize];
    star_Buffer_fill(0.0f, add, audioSettings.blockSize);

    struct star_sig_Sine_Inputs sineInputs = {
        .freq = freq,
        .phaseOffset = phaseOffset,
        .mul = mul,
        .add = add
    };

    struct star_sig_Sine sine;
    star_sig_Sine_init(&sine, &audioSettings, &sineInputs, output);


    puts("Sine wave (three blocks): ");
    for (int i = 0; i < 3; i++) {
        sine.signal.generate(&sine);
        printBuffer(sine.signal.output, audioSettings.blockSize);
    }

    return EXIT_SUCCESS;
}
