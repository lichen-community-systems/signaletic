#ifndef BOB_FILTER_H
#define BOB_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libsignaletic.h>

/**
 * @brief Miller Puckette's 24dB Moog-style ladder low pass filter.
 * Imitates a Moog resonant filter by Runge-Kutte numerical integration
 * of a differential equation approximately describing the dynamics of
 * the circuit.
 *
 * The differential equations are:
 *	y1' = k * (S(x - r * y4) - S(y1))
 *	y2' = k * (S(y1) - S(y2))
 *	y3' = k * (S(y2) - S(y3))
 *	y4' = k * (S(y3) - S(y4))
 * where k controls the cutoff frequency,
 * r is feedback (<= 4 for stability),
 * and S(x) is a saturation function.
 *
 * Adapted from Dimitri Diakopoulos' version of
 * Pure Data's BSD-licensed ~bob implementation.
 *
 * https://github.com/ddiakopoulos/MoogLadders/blob/9cef8fac86a35c40f4a99bcc9b52b8874ee5ff2b/src/RKSimulationModel.h
 * https://github.com/pure-data/pure-data/blob/3b4be8c6e228397b27615b279451578e71cabfcf/extra/bob~/bob~.c
 *
 * Inputs:
 *  - source: the input to filter
 *  - frequency: the cutoff frequency in Hz
 *  - resonance: the resonance; values > 4 lead to instability
 */
struct sig_dsp_Bob {
    struct sig_dsp_Signal signal;
    struct sig_dsp_FourPoleFilter_Inputs inputs;
    // TODO: Add outputs for the other filter poles.
    struct sig_dsp_FourPoleFilter_Outputs outputs;

    float state[4];
    float deriv1[4];
    float deriv2[4];
    float deriv3[4];
    float deriv4[4];
    float tempState[4];

    float saturation;
    float saturationInv;
    uint8_t oversample;
    float stepSize;
};

struct sig_dsp_Bob* sig_dsp_Bob_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Bob_init(struct sig_dsp_Bob* self,
    struct sig_SignalContext* context);
float sig_dsp_Bob_clip(float value, float saturation,
    float saturationInv);
void sig_dsp_Bob_generate(void* signal);
void sig_dsp_Bob_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Bob* self);

#ifdef __cplusplus
}
#endif

#endif /* BOB_FILTER_H */
