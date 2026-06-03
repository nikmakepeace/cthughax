#include "ViewportPresentation.h"

#include <assert.h>

static DisplayViewport viewportWithDestination() {
    DisplayViewport viewport;
    viewport.frameSize = PixelSize(4, 3);
    viewport.windowSize = PixelSize(12, 10);
    viewport.drawSize = PixelSize(4, 3);
    viewport.destination = PixelRect(4, 3, 4, 3);
    viewport.scaleMode = SCALE_MODE_FIXED_ZOOM;
    viewport.requestedZoom = 1;
    viewport.effectiveZoom = 1;
    return viewport;
}

static void testFullCopyRectCoversWindow() {
    DisplayViewport viewport = viewportWithDestination();

    assert(ViewportPresentation::fullCopyRect(viewport) == PixelRect(0, 0, 12, 10));
}

static void testDrawCopyRectUsesViewportDestination() {
    DisplayViewport viewport = viewportWithDestination();

    assert(ViewportPresentation::drawCopyRect(viewport) == PixelRect(4, 3, 4, 3));
}

static void testDrawOffsetUsesViewportDestination() {
    DisplayViewport viewport = viewportWithDestination();

    assert(ViewportPresentation::drawOffsetBytes(viewport, 40, 2) == 128);
}

int main() {
    testFullCopyRectCoversWindow();
    testDrawCopyRectUsesViewportDestination();
    testDrawOffsetUsesViewportDestination();
    return 0;
}
