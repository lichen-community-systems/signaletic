// Note: This is a C++ file because I haven't gotten around to
// considering how to wrap Daisy objects in C,
// and doing so doesn't seem hugely important since we're certain
// to be compiling Daisy apps with a C++ compiler anyway.
// Nonetheless the API is C-style in case part of it ends up being
// generalized for different environments
// (e.g. a version that draws to an HTML canvas instead.)

#include "daisy_seed.h"

struct sig_ui_CanvasGeometry {
    int xStart;
    int width;
    int xEnd;
    int yStart;
    int height;
    int yEnd;
    int yCentre;
};

// 0,0 is top left.
struct sig_ui_Rect {
    int x;
    int y;
    int width;
    int height;
};

struct sig_ui_daisy_Canvas {
    struct sig_ui_CanvasGeometry geometry;
    // TODO: How do we handle this in a driver-agnostic way?
    daisy::OledDisplay<daisy::SSD130xI2c64x32Driver>* display;
};



struct sig_ui_CanvasGeometry sig_ui_daisy_Canvas_geometryFromRect(
    struct sig_ui_Rect* rect) {
    struct sig_ui_CanvasGeometry geom;

    geom.xStart = rect->x;
    geom.width = rect->width;
    geom.yStart = rect->y;
    geom.height = rect->height;
    geom.xEnd = geom.xStart + geom.width;
    geom.yEnd = geom.yStart + geom.height;
    geom.yCentre = geom.height / 2;

    return geom;
}

void sig_ui_daisy_Canvas_drawLine(struct sig_ui_daisy_Canvas* canvas,
    int thickness, int x1, int y1, int x2, int y2, bool on) {
    for (int i = 0; i < thickness; i++) {
        canvas->display->DrawLine(x1 + i, y1, x2 + i, y2, on);
    }
}


struct sig_ui_daisy_LoopRenderer {
    struct sig_ui_daisy_Canvas* canvas;
    struct sig_dsp_Looper_Loop* loop;
};

void sig_ui_daisy_LoopRenderer_drawWaveform(
    struct sig_ui_daisy_LoopRenderer* self, bool foregroundOn) {
    struct sig_ui_CanvasGeometry geom = self->canvas->geometry;
    size_t yCentre = geom.yCentre;
    int thickness = 1;
    int xStart = geom.xStart;
    int xEnd = geom.width - xStart;
    int yStart = geom.yStart;
    int prevX = xStart;
    int prevY = yStart + geom.yCentre;

    if (self->loop->length == 0) {
        sig_ui_daisy_Canvas_drawLine(self->canvas,
            thickness, xStart, prevY, xEnd, prevY, foregroundOn);
        return;
    }

    float* samples = self->loop->buffer->samples;
    size_t loopStartIdx = self->loop->startIdx;
    size_t step = self->loop->length / geom.width;

    for (int x = xStart; x < xEnd; x += 2) {
        size_t idx = (size_t) (x + loopStartIdx) * step;
        float samp = self->loop->length == 0 ? 0.0f : samples[idx];
        int scaled = (int) roundf((-samp) * yCentre + yCentre);
        int y = scaled + yStart;

        sig_ui_daisy_Canvas_drawLine(self->canvas,
            thickness, prevX, prevY, x, y, foregroundOn);

        prevX = x;
        prevY = y;
    }
}

// TODO: Add customizable height so this function can be used
// by drawLoopPoints.
void sig_ui_daisy_LoopRenderer_drawLine(
    struct sig_ui_daisy_LoopRenderer* self,
    float relativePosition, int thickness, int foregroundOn) {
    struct sig_ui_CanvasGeometry geom = self->canvas->geometry;
    int x = (int) roundf(relativePosition * (geom.xEnd - thickness));
    sig_ui_daisy_Canvas_drawLine(self->canvas,
        thickness, x, geom.yStart, x, geom.yEnd, foregroundOn);
}

void sig_ui_daisy_LoopRenderer_drawPositionLine(
    sig_ui_daisy_LoopRenderer* self, float pos, int thickness,
    bool foregroundOn) {
    struct sig_dsp_Looper_Loop* loop = self->loop;
    // Although it's a float, "pos" is expressed in samples,
    // so needs to be scaled to a relative value between 0 and 1.
    size_t loopLength = loop->isEmpty ? loop->buffer->length : loop->length;
    float scaledPlaybackPos = pos / loopLength;
    sig_ui_daisy_LoopRenderer_drawLine(self,
        scaledPlaybackPos, thickness, foregroundOn);
}

struct sig_ui_daisy_LooperView {
    struct sig_ui_daisy_Canvas* canvas;
    struct sig_ui_daisy_LoopRenderer* loopRenderer;
    daisy::FixedCapStr<10> text;
    struct sig_dsp_Looper* looper;
    float leftSpeed;
    float rightSpeed;
    int positionLineThickness;
    int loopPointLineThickness;
};

// TODO: Render the loop region with reverse contrast
void sig_ui_daisy_LooperView_drawLoopPoints(
    struct sig_ui_daisy_LooperView* self,
    float startPos, float endPos, bool foregroundOn) {
    int lineThickness = self->loopPointLineThickness;
    struct sig_ui_CanvasGeometry geom = self->canvas->geometry;

    int lineHeight = geom.yCentre;
    int yStart = geom.yStart + ((geom.height - lineHeight) / 2);
    int yEnd = yStart + lineHeight;

    int startPointX = roundf(startPos * (geom.xEnd - lineThickness));
    int endPointX = roundf(endPos * (geom.xEnd - lineThickness));

    sig_ui_daisy_Canvas_drawLine(self->canvas,
        lineThickness, startPointX, yStart, startPointX, yEnd, foregroundOn);
    sig_ui_daisy_Canvas_drawLine(self->canvas,
        lineThickness, endPointX, yStart, endPointX, yEnd,
        foregroundOn);
}

void sig_ui_daisy_LooperView_writeSpeeds(
    struct sig_ui_daisy_LooperView* self, int foregroundOn) {
    self->text.Clear();
    self->text.AppendFloat(self->leftSpeed, 2);
    self->text.Append("  ");
    self->text.AppendFloat(self->rightSpeed, 2);
    self->canvas->display->SetCursor(0, 0);
    self->canvas->display->WriteString(self->text.Cstr(),
        Font_6x8, foregroundOn);
}
void sig_ui_daisy_LooperView_render(struct sig_ui_daisy_LooperView* self,
    float start, float length, float playbackPos, int foregroundOn) {
    sig_ui_daisy_LooperView_writeSpeeds(self, foregroundOn);
    sig_ui_daisy_LooperView_drawLoopPoints(self, start, length,
        foregroundOn);
    sig_ui_daisy_LoopRenderer_drawWaveform(self->loopRenderer,
        foregroundOn);
    sig_ui_daisy_LoopRenderer_drawPositionLine(self->loopRenderer,
        playbackPos, self->positionLineThickness, foregroundOn);
}
