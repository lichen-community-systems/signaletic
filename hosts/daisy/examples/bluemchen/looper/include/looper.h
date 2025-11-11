#ifndef LOOPER_H
#define LOOPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libsignaletic.h>

struct sig_dsp_Looper_Inputs {
    float_array_ptr source;
    float_array_ptr start;
    float_array_ptr end;
    float_array_ptr speed;
    float_array_ptr record;
    float_array_ptr clear;
};

struct sig_dsp_Looper_Loop {
    struct sig_Buffer* buffer;
    size_t startIdx;
    size_t length;
    bool isEmpty;
};

struct sig_dsp_Looper {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Looper_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_dsp_Looper_Loop loop;
    size_t loopLastIdx;
    float playbackPos;
    float previousRecord;
    float previousClear;
};

void sig_dsp_Looper_init(struct sig_dsp_Looper* self,
    struct sig_SignalContext* context);
struct sig_dsp_Looper* sig_dsp_Looper_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_Looper_setBuffer(struct sig_dsp_Looper* self,
    struct sig_Buffer* buffer);
void sig_dsp_Looper_generate(void* signal);
void sig_dsp_Looper_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Looper* self);

    #ifdef __cplusplus
}
#endif

#endif /* LOOPER_H */
