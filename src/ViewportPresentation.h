#ifndef __VIEWPORT_PRESENTATION_H
#define __VIEWPORT_PRESENTATION_H

#include "DisplayGeometry.h"

class ViewportPresentation {
public:
    static PixelRect fullCopyRect(const DisplayViewport& viewport) {
        return PixelRect(0, 0, viewport.windowSize.width, viewport.windowSize.height);
    }

    static PixelRect drawCopyRect(const DisplayViewport& viewport) {
        return viewport.destination;
    }

    static int drawOffsetBytes(
        const DisplayViewport& viewport, int bytesPerLine, int bytesPerPixel) {
        return viewport.screenOffsetBytes(bytesPerLine, bytesPerPixel);
    }
};

#endif
