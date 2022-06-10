// Note: This is a C++ file because I haven't gotten around to
// considering how to wrap Daisy objects in C,
// and doing so doesn't seem hugely important since we're certain
// to be compiling Daisy apps with a C++ compiler anyway.
// Nonetheless the API is C-style in case part of it ends up being
// generalized for different environments
// (e.g. a version that draws to an HTML canvas instead.)

#include "daisy_seed.h"

struct sig_CanvasGeometry {
    int xStart;
    int width;
    int xEnd;
    int yStart;
    int height;
    int yEnd;
    int yCentre;
};

struct sig_Canvas {
    struct sig_CanvasGeometry geometry;
    // TODO: How do we handle this in a driver-agnostic way?
    daisy::OledDisplay<daisy::SSD130xI2c64x32Driver>* display;
};

// 0,0 is top left.
struct sig_Rect {
    int x;
    int y;
    int width;
    int height;
};

struct sig_CanvasGeometry sig_Canvas_geometryFromRect(
    struct sig_Rect* rect) {
    struct sig_CanvasGeometry geom;

    geom.xStart = rect->x;
    geom.width = rect->width;
    geom.yStart = rect->y;
    geom.height = rect->height;
    geom.xEnd = geom.xStart + geom.width;
    geom.yEnd = geom.yStart + geom.height;
    geom.yCentre = geom.height / 2;

    return geom;
}

void sig_Canvas_drawLine(struct sig_Canvas* canvas,
    int thickness, int x1, int y1, int x2, int y2, bool on) {
    for (int i = 0; i < thickness; i++) {
        canvas->display->DrawLine(x1 + i, y1, x2 + i, y2, on);
    }
}


struct sig_BufferView {
    struct sig_Canvas* canvas;
    struct sig_Buffer* buffer;
};

void sig_BufferView_drawWaveform(struct sig_BufferView* bufferView,
    bool foregroundOn) {
    struct sig_CanvasGeometry geom = bufferView->canvas->geometry;

    float* samples = bufferView->buffer->samples;
    size_t step = bufferView->buffer->length / geom.width;
    size_t yCentre = geom.yCentre;

    int xStart = geom.xStart;
    int yStart = geom.yStart;
    int xEnd = geom.width - xStart;
    int prevX = geom.xStart;
    int prevY = geom.yStart + geom.yCentre;

    for (int x = xStart; x < xEnd; x += 2) {
        float samp = samples[(size_t) x * step];
        int scaled = (int) roundf((-samp) * yCentre + yCentre);
        int y = scaled + yStart;

        sig_Canvas_drawLine(bufferView->canvas,
            1, prevX, prevY, x, y, foregroundOn);

        prevX = x;
        prevY = y;
    }
}

// TODO: Add customizable height so this function can be used
// by drawLoopPoints.
void sig_BufferView_drawLine(struct sig_BufferView* bufferView,
    float relativePosition, int thickness, int foregroundOn) {
    struct sig_CanvasGeometry geom = bufferView->canvas->geometry;
    int x = (int) roundf(relativePosition * (geom.xEnd - thickness));
    sig_Canvas_drawLine(bufferView->canvas,
        thickness, x, geom.yStart, x, geom.yEnd, foregroundOn);
}

void sig_BufferView_drawPositionLine(
    struct sig_BufferView* bufferView, float pos, int thickness,
    bool foregroundOn) {
    // Although it's a float "pos" is expressed in samples,
    // so needs to be scaled to a relative value between 0 and 1.
    float scaledPlaybackPos = pos / bufferView->buffer->length;
    sig_BufferView_drawLine(bufferView, scaledPlaybackPos, thickness,
        foregroundOn);
}

struct sig_LooperView {
    struct sig_Canvas* canvas;
    struct sig_BufferView* bufferView;
    daisy::FixedCapStr<10> text;
    struct sig_dsp_Looper* looper;
    float leftSpeed;
    float rightSpeed;
    int positionLineThickness;
    int loopPointLineThickness;
};

// TODO: Render the loop region with reverse contrast
void sig_LooperView_drawLoopPoints(struct sig_LooperView* looperView,
    float startPos, float endPos, bool foregroundOn) {
    int lineThickness = looperView->loopPointLineThickness;
    struct sig_CanvasGeometry geom = looperView->canvas->geometry;

    int lineHeight = geom.yCentre;
    int yStart = geom.yStart + ((geom.height - lineHeight) / 2);
    int yEnd = yStart + lineHeight;

    int startPointX = roundf(startPos * (geom.xEnd - lineThickness));
    int endPointX = roundf(endPos * (geom.xEnd - lineThickness));

    sig_Canvas_drawLine(looperView->canvas,
        lineThickness, startPointX, yStart, startPointX, yEnd, foregroundOn);
    sig_Canvas_drawLine(looperView->canvas,
        lineThickness, endPointX, yStart, endPointX, yEnd,
        foregroundOn);
}

void sig_LooperView_writeSpeeds(struct sig_LooperView* looperView,
    int foregroundOn) {
    looperView->text.Clear();
    looperView->text.AppendFloat(looperView->leftSpeed, 2);
    looperView->text.Append("  ");
    looperView->text.AppendFloat(looperView->rightSpeed, 2);
    looperView->canvas->display->SetCursor(0, 0);
    looperView->canvas->display->WriteString(looperView->text.Cstr(),
        Font_6x8, foregroundOn);
}
void sig_LooperView_render(struct sig_LooperView* looperView,
    float start, float length, float playbackPos, int foregroundOn) {
    sig_LooperView_writeSpeeds(looperView, foregroundOn);
    sig_LooperView_drawLoopPoints(looperView, start, length,
        foregroundOn);
    sig_BufferView_drawWaveform(looperView->bufferView, foregroundOn);
    sig_BufferView_drawPositionLine(looperView->bufferView,
        playbackPos, looperView->positionLineThickness, foregroundOn);
}
