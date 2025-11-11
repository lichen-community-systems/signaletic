#include "../include/looper.h"

void sig_dsp_Looper_init(struct sig_dsp_Looper* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_dsp_Looper_generate);

    struct sig_dsp_Looper_Loop loop = {
        .buffer = NULL,
        .startIdx = 0,
        .length = 0,
        .isEmpty = true
    };

    self->loop = loop;
    self->playbackPos = 0.0f;
    self->previousRecord = 0.0f;
    self->previousClear = 0.0f;

    sig_dsp_Looper_setBuffer(self, context->emptyBuffer);

    sig_CONNECT_TO_SILENCE(self, source, context);
    sig_CONNECT_TO_SILENCE(self, start, context);
    sig_CONNECT_TO_UNITY(self, end, context);
    sig_CONNECT_TO_UNITY(self, speed, context);
    sig_CONNECT_TO_SILENCE(self, record, context);
    sig_CONNECT_TO_SILENCE(self, clear, context);
}

struct sig_dsp_Looper* sig_dsp_Looper_new(struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    struct sig_dsp_Looper* self = sig_MALLOC(allocator,
        struct sig_dsp_Looper);
    sig_dsp_Looper_init(self, context);
    sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
        context->audioSettings, &self->outputs);

    return self;
}

// TODO: Should buffers just be required at initialization? (Probably, yes.)
void sig_dsp_Looper_setBuffer(struct sig_dsp_Looper* self,
    struct sig_Buffer* buffer) {
    self->loop.buffer = buffer;
    self->loopLastIdx = self->loop.buffer->length - 1;
}

static inline float sig_dsp_Looper_record(struct sig_dsp_Looper* self,
    float startPos, float endPos, size_t i) {
    struct sig_dsp_Looper_Loop* loop = &self->loop;
    float* recordBuffer = FLOAT_ARRAY(loop->buffer->samples);
    size_t playbackIdx = (size_t) self->playbackPos;

    // Start with the current input sample.
    float sample = FLOAT_ARRAY(self->inputs.source)[i];

    if (loop->isEmpty) {
        if (self->previousRecord <= 0.0f) {
            // We're going forward; reset to the beginning of the buffer.
            self->playbackPos = 0.0f;
            loop->startIdx = 0;
        }

        if (loop->length < loop->buffer->length) {
            // TODO: Add support for recording in reverse
            // on the first overdub.
            loop->length++;
        }
    } else {
        // We're overdubbing.
        // Reduce the volume of the previously recorded audio by 10%
        float previousRecordedSample = recordBuffer[playbackIdx] * 0.9;

        // Mix it in with the input.
        sample += previousRecordedSample;
    }

    // Add a little distortion/limiting.
    sample = tanhf(sample);

    // Replace the audio in the buffer with the new mix.
    recordBuffer[playbackIdx] = sample;

    return sample;
}

static inline void sig_dsp_Looper_clearBuffer(struct sig_dsp_Looper* self) {
    // For performance reasons, the buffer is never actually zeroed,
    // it's just marked as empty.
    // TODO: Fade out before clearing the buffer (gh-28).
    self->loop.length = 0;
    self->loop.isEmpty = true;
    self->loopLastIdx = self->loop.buffer->length - 1;
}

static inline float sig_dsp_Looper_play(struct sig_dsp_Looper* self, size_t i) {
    struct sig_dsp_Looper_Loop* loop = &self->loop;
    if (self->previousRecord > 0.0f && loop->isEmpty) {
        loop->startIdx = 0;
        self->loopLastIdx = loop->length - 1;
        loop->isEmpty = false;
    }

    if (FLOAT_ARRAY(self->inputs.clear)[i] > 0.0f &&
        self->previousClear == 0.0f) {
        sig_dsp_Looper_clearBuffer(self);
    }

    float outputSample = FLOAT_ARRAY(self->inputs.source)[i];

    if (!loop->isEmpty) {
        // Only read from the record buffer if it has something in it
        // (because we don't actually erase its contents when clearing).

        // TODO: The sig_interpolate_linear implementation
        // may wrap around inappropriately to the beginning of
        // the buffer (not to the startPos) if we're right at
        // the end of the buffer.
        outputSample += sig_Buffer_readLinear(
            loop->buffer, self->playbackPos);
    }

    return outputSample;
}

static inline float sig_dsp_Looper_nextPosition(float currentPos,
    float startPos, float endPos, float speed) {
    float nextPlaybackPos = currentPos + speed;
    if (nextPlaybackPos > endPos) {
        nextPlaybackPos = startPos + (nextPlaybackPos - endPos);
    } else if (nextPlaybackPos < startPos) {
        nextPlaybackPos = endPos - (startPos - nextPlaybackPos);
    }

    return nextPlaybackPos;
}

// TODO:
// * Reduce clicks by crossfading the end and start of the window.
//      - should it be a true cross fade, requiring a few samples
//        on each end of the clip, or a very quick fade in/out
//        (e.g. 1-10ms/48-480 samples)?
// * Fade out before clearing. A whole loop's duration, or shorter?
// * Address glitches when the length is very short
// * Should we check if the buffer is null and output silence,
//   or should this be considered a user error?
//   (Or should we introduce some kind of validation function for signals?)
// * Unit tests
void sig_dsp_Looper_generate(void* signal) {
    struct sig_dsp_Looper* self = (struct sig_dsp_Looper*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float speed = FLOAT_ARRAY(self->inputs.speed)[i];

        // Clamp the start and end inputs to within 0..1.
        float start = sig_clamp(FLOAT_ARRAY(self->inputs.start)[i],
            0.0, 1.0);
        float end = sig_clamp(FLOAT_ARRAY(self->inputs.end)[i],
            0.0, 1.0);

        // Flip the start and end points if they're reversed.
        if (start > end) {
            float temp = start;
            start = end;
            end = temp;
        }

        float startPos = roundf(self->loopLastIdx * start);
        float endPos = roundf(self->loopLastIdx * end);

        float outputSample = 0.0f;
        if ((endPos - startPos) <= fabsf(speed)) {
            // The loop size is too small to play at this speed.
            outputSample = FLOAT_ARRAY(self->inputs.source)[i];
        } else if (FLOAT_ARRAY(self->inputs.record)[i] > 0.0f) {
            // Playback has to be at regular speed
            // while recording, so ignore any modulation
            // and only use its direction.
            // Also, the first recording has to be in forward.
            // TODO: Add support for cool tape-style effects
            // when overdubbing at different speeds.
            // Note: Naively omitting this will work,
            // but introduces lots pitched resampling artifacts.
            speed = self->loop.isEmpty ? 1.0 : speed >=0 ? 1.0 : -1.0;
            outputSample = sig_dsp_Looper_record(self, startPos, endPos, i);
        } else {
            outputSample = sig_dsp_Looper_play(self, i);
        }

        FLOAT_ARRAY(self->outputs.main)[i] = outputSample;

        self->playbackPos = sig_dsp_Looper_nextPosition(self->playbackPos,
            startPos, endPos, speed);

        self->previousRecord = FLOAT_ARRAY(self->inputs.record)[i];
        self->previousClear = FLOAT_ARRAY(self->inputs.clear)[i];
    }
}

void sig_dsp_Looper_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_Looper* self) {
    sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
        &self->outputs);
    sig_dsp_Signal_destroy(allocator, self);
}
