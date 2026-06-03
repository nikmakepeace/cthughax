#ifndef __DISPLAY_GEOMETRY_H
#define __DISPLAY_GEOMETRY_H

#include "cthugha.h"

enum ScaleMode {
    SCALE_MODE_FIT_WINDOW,
    SCALE_MODE_FIXED_ZOOM
};

class PixelSize {
public:
    int width;
    int height;

    PixelSize()
        : width(0)
        , height(0) { }

    PixelSize(int width_, int height_)
        : width(width_)
        , height(height_) { }

    static PixelSize fromXy(xy value) {
        return PixelSize(value.x, value.y);
    }

    xy toXy() const {
        return xy(width, height);
    }

    int valid() const {
        return width > 0 && height > 0;
    }

    bool operator==(const PixelSize& other) const {
        return width == other.width && height == other.height;
    }

    bool operator!=(const PixelSize& other) const {
        return !(*this == other);
    }
};

class PixelRect {
public:
    int x;
    int y;
    int width;
    int height;

    PixelRect()
        : x(0)
        , y(0)
        , width(0)
        , height(0) { }

    PixelRect(int x_, int y_, int width_, int height_)
        : x(x_)
        , y(y_)
        , width(width_)
        , height(height_) { }

    PixelSize size() const {
        return PixelSize(width, height);
    }

    int right() const {
        return x + width;
    }

    int bottom() const {
        return y + height;
    }

    int valid() const {
        return width > 0 && height > 0;
    }

    bool operator==(const PixelRect& other) const {
        return x == other.x && y == other.y && width == other.width
            && height == other.height;
    }

    bool operator!=(const PixelRect& other) const {
        return !(*this == other);
    }
};

class DisplayViewport {
public:
    PixelSize frameSize;
    PixelSize windowSize;
    PixelSize drawSize;
    PixelRect destination;
    ScaleMode scaleMode;
    int requestedZoom;
    int effectiveZoom;
    int reductionCount;

    DisplayViewport()
        : frameSize()
        , windowSize()
        , drawSize()
        , destination()
        , scaleMode(SCALE_MODE_FIT_WINDOW)
        , requestedZoom(0)
        , effectiveZoom(0)
        , reductionCount(0) { }

    int valid() const {
        return frameSize.valid() && windowSize.valid();
    }

    int zoomWasReduced() const {
        return reductionCount > 0;
    }

    int screenOffsetX() const {
        return destination.x;
    }

    int screenOffsetY() const {
        return destination.y;
    }

    int screenOffsetBytes(int bytesPerLine, int bytesPerPixel) const {
        return bytesPerLine * screenOffsetY() + bytesPerPixel * screenOffsetX();
    }

    int requiresBorderClearFrom(const DisplayViewport& previous) const {
        if (!valid() || !previous.valid())
            return 0;

        return windowSize != previous.windowSize
            || destination != previous.destination
            || requestedZoom != previous.requestedZoom
            || effectiveZoom != previous.effectiveZoom;
    }
};

#endif
