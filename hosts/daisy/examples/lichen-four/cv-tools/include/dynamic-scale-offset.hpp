#include <libsignaletic.h>

struct sig_dsp_DynamicScaleOffset_Inputs {
    float_array_ptr source;
    float_array_ptr scale;
    float_array_ptr offset;
};

struct sig_dsp_DynamicScaleOffset {
    struct sig_dsp_Signal signal;
    struct sig_dsp_DynamicScaleOffset_Inputs inputs;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
};

void sig_dsp_DynamicScaleOffset_generate(void* signal) {
    struct sig_dsp_DynamicScaleOffset* self = (struct sig_dsp_DynamicScaleOffset*) signal;
    float* source = FLOAT_ARRAY(self->inputs.source);
    float* scale = FLOAT_ARRAY(self->inputs.scale);
    float* offset = FLOAT_ARRAY(self->inputs.offset);
    float* mainOutput = FLOAT_ARRAY(self->outputs.main);

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        mainOutput[i] = source[i] * scale[i] + offset[i];
    }
}

void sig_dsp_DynamicScaleOffset_init(struct sig_dsp_DynamicScaleOffset* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_DynamicScaleOffset_generate);

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_UNITY(self, scale, context);
    sig_CONNECT_TO_SILENCE(self, offset, context);
}

struct sig_dsp_DynamicScaleOffset* sig_dsp_DynamicScaleOffset_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_dsp_DynamicScaleOffset* self = sig_MALLOC(allocator,
        struct sig_dsp_DynamicScaleOffset);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);
    sig_dsp_DynamicScaleOffset_init(self, context);

    return self;
}

void sig_dsp_DynamicScaleOffset_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_DynamicScaleOffset* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, (void*) self);
}
