#include <libsignaletic.h>

struct sig_dsp_MultiTapDelay_Inputs {
    float_array_ptr source;
    float_array_ptr timeScale;
};


// FIXME: Need multichannel inputs so these delay taps can be modulatable.
struct sig_dsp_MultiTapDelay {
    struct sig_dsp_Signal signal;
    struct sig_dsp_MultiTapDelay_Inputs inputs;
    struct sig_dsp_ScaleOffset_Parameters parameters;
    struct sig_dsp_Signal_SingleMonoOutput outputs;

    struct sig_DelayLine* delayLine;
    struct sig_Buffer* tapTimes;
    struct sig_Buffer* tapGains;
};

struct sig_dsp_MultiTapDelay* sig_dsp_MultiTapDelay_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context);
void sig_dsp_MultiTapDelay_init(struct sig_dsp_MultiTapDelay* self,
    struct sig_SignalContext* context);
void sig_dsp_MultiTapDelay_generate(void* signal);
void sig_dsp_MultiTapDelay_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_MultiTapDelay* self);

struct sig_dsp_MultiTapDelay* sig_dsp_MultiTapDelay_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_MultiTapDelay* self = sig_MALLOC(allocator,
        struct sig_dsp_MultiTapDelay);
    // TODO: Improve buffer management throughout Signaletic.
    self->delayLine = context->oneSampleDelayLine;
    self->tapTimes = context->emptyBuffer;
    self->tapGains = context->emptyBuffer;

    sig_dsp_MultiTapDelay_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

void sig_dsp_MultiTapDelay_init(struct sig_dsp_MultiTapDelay* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_MultiTapDelay_generate);

    struct sig_dsp_ScaleOffset_Parameters parameters = {
        .scale = 1.0,
        .offset= 0.0f
    };
    self->parameters = parameters;
    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_UNITY(self, timeScale, context);
}

void sig_dsp_MultiTapDelay_generate(void* signal) {
    struct sig_dsp_MultiTapDelay* self = (struct sig_dsp_MultiTapDelay*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float source = FLOAT_ARRAY(self->inputs.source)[i];
        float timeScale = FLOAT_ARRAY(self->inputs.timeScale)[i];
        float tapSum = sig_DelayLine_linearReadAtTimes(
            self->delayLine,
            source,
            self->tapTimes->samples,
            self->tapGains->samples,
            self->tapTimes->length,
            self->signal.audioSettings->sampleRate,
            timeScale);

        FLOAT_ARRAY(self->outputs.main)[i] = tapSum * self->parameters.scale +
            self->parameters.offset;
        sig_DelayLine_write(self->delayLine, source);
    }
}

void sig_dsp_MultiTapDelay_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_MultiTapDelay* self) {
    // Don't destroy the delay line or the buffers; they're not owned.
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}
