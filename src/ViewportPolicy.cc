#include "ViewportPolicy.h"

static int centeredOffset(int windowPixels, int drawPixels) {
    int offset = (windowPixels - drawPixels) / 2;
    return offset > 0 ? offset : 0;
}

static PixelSize fixedDrawSize(PixelSize frameSize, int zoom) {
    return PixelSize(frameSize.width * zoom, frameSize.height * zoom);
}

DisplayViewport ViewportPolicy::viewportFor(PixelSize frameSize,
    PixelSize windowSize, int zoomMode) const {
    DisplayViewport viewport;
    viewport.frameSize = frameSize;
    viewport.windowSize = windowSize;
    viewport.requestedZoom = zoomMode;

    if (!frameSize.valid() || !windowSize.valid())
        return viewport;

    if (zoomMode == 0) {
        viewport.scaleMode = SCALE_MODE_FIT_WINDOW;
        viewport.effectiveZoom = 0;
        viewport.drawSize = windowSize;
    } else {
        viewport.scaleMode = SCALE_MODE_FIXED_ZOOM;
        viewport.effectiveZoom = zoomMode;
        viewport.drawSize = fixedDrawSize(frameSize, viewport.effectiveZoom);

        while ((viewport.drawSize.width > windowSize.width
                   || viewport.drawSize.height > windowSize.height)
            && viewport.effectiveZoom > 0) {
            viewport.effectiveZoom /= 2;
            viewport.reductionCount++;
            viewport.drawSize = fixedDrawSize(frameSize, viewport.effectiveZoom);
        }
    }

    viewport.destination = PixelRect(
        centeredOffset(windowSize.width, viewport.drawSize.width),
        centeredOffset(windowSize.height, viewport.drawSize.height),
        viewport.drawSize.width, viewport.drawSize.height);
    return viewport;
}
