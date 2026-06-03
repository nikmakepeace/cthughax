#include "DisplayGeometry.h"

#include <assert.h>

static void testPixelSizeDefaultsToInvalidEmptySize() {
    PixelSize size;

    assert(size.width == 0);
    assert(size.height == 0);
    assert(!size.valid());
}

static void testPixelSizeConvertsToAndFromLegacyXy() {
    PixelSize size = PixelSize::fromXy(xy(7, 5));
    xy legacy = size.toXy();

    assert(size == PixelSize(7, 5));
    assert(size != PixelSize(5, 7));
    assert(size.valid());
    assert(legacy.x == 7);
    assert(legacy.y == 5);
}

static void testPixelRectReportsEdgesAndSize() {
    PixelRect rect(3, 4, 10, 6);

    assert(rect.valid());
    assert(rect.size() == PixelSize(10, 6));
    assert(rect.right() == 13);
    assert(rect.bottom() == 10);
    assert(rect == PixelRect(3, 4, 10, 6));
    assert(rect != PixelRect(3, 5, 10, 6));
}

static void testDisplayViewportCalculatesLegacyScreenOffsetBytes() {
    DisplayViewport viewport;
    viewport.frameSize = PixelSize(4, 3);
    viewport.windowSize = PixelSize(12, 10);
    viewport.drawSize = PixelSize(4, 3);
    viewport.destination = PixelRect(4, 3, 4, 3);

    assert(viewport.valid());
    assert(viewport.screenOffsetX() == 4);
    assert(viewport.screenOffsetY() == 3);
    assert(viewport.screenOffsetBytes(40, 2) == 128);
}

static void testDisplayViewportBorderClearComparison() {
    DisplayViewport previous;
    previous.frameSize = PixelSize(4, 3);
    previous.windowSize = PixelSize(10, 9);
    previous.drawSize = PixelSize(4, 3);
    previous.destination = PixelRect(3, 3, 4, 3);
    previous.scaleMode = SCALE_MODE_FIXED_ZOOM;
    previous.requestedZoom = 1;
    previous.effectiveZoom = 1;

    DisplayViewport same = previous;
    DisplayViewport moved = previous;
    moved.windowSize = PixelSize(12, 9);
    moved.destination = PixelRect(4, 3, 4, 3);

    assert(!same.requiresBorderClearFrom(previous));
    assert(moved.requiresBorderClearFrom(previous));
}

int main() {
    testPixelSizeDefaultsToInvalidEmptySize();
    testPixelSizeConvertsToAndFromLegacyXy();
    testPixelRectReportsEdgesAndSize();
    testDisplayViewportCalculatesLegacyScreenOffsetBytes();
    testDisplayViewportBorderClearComparison();
    return 0;
}
