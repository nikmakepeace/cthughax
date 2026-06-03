#include "ViewportPolicy.h"

#include <assert.h>

static void assertViewport(const DisplayViewport& viewport, ScaleMode scaleMode,
    PixelSize frameSize, PixelSize windowSize, PixelSize drawSize,
    PixelRect destination, int requestedZoom, int effectiveZoom,
    int reductionCount) {
    assert(viewport.valid());
    assert(viewport.scaleMode == scaleMode);
    assert(viewport.frameSize == frameSize);
    assert(viewport.windowSize == windowSize);
    assert(viewport.drawSize == drawSize);
    assert(viewport.destination == destination);
    assert(viewport.requestedZoom == requestedZoom);
    assert(viewport.effectiveZoom == effectiveZoom);
    assert(viewport.reductionCount == reductionCount);
    assert(viewport.zoomWasReduced() == (reductionCount > 0));
}

static void testFitToWindowUsesWholeDisplayArea() {
    ViewportPolicy policy;

    DisplayViewport viewport = policy.viewportFor(PixelSize(4, 3),
        PixelSize(10, 9), 0);

    assertViewport(viewport, SCALE_MODE_FIT_WINDOW, PixelSize(4, 3),
        PixelSize(10, 9), PixelSize(10, 9), PixelRect(0, 0, 10, 9),
        0, 0, 0);
    assert(viewport.screenOffsetBytes(40, 2) == 0);
}

static void testFixed1xCentersSourceSizedOutput() {
    ViewportPolicy policy;

    DisplayViewport viewport = policy.viewportFor(PixelSize(4, 3),
        PixelSize(10, 9), 1);

    assertViewport(viewport, SCALE_MODE_FIXED_ZOOM, PixelSize(4, 3),
        PixelSize(10, 9), PixelSize(4, 3), PixelRect(3, 3, 4, 3),
        1, 1, 0);
}

static void testFixed2xScalesAndCentersOutput() {
    ViewportPolicy policy;

    DisplayViewport viewport = policy.viewportFor(PixelSize(4, 3),
        PixelSize(20, 20), 2);

    assertViewport(viewport, SCALE_MODE_FIXED_ZOOM, PixelSize(4, 3),
        PixelSize(20, 20), PixelSize(8, 6), PixelRect(6, 7, 8, 6),
        2, 2, 0);
}

static void testOversizedFixed2xReducesToFixed1x() {
    ViewportPolicy policy;

    DisplayViewport viewport = policy.viewportFor(PixelSize(8, 4),
        PixelSize(10, 5), 2);

    assertViewport(viewport, SCALE_MODE_FIXED_ZOOM, PixelSize(8, 4),
        PixelSize(10, 5), PixelSize(8, 4), PixelRect(1, 0, 8, 4),
        2, 1, 1);
}

static void testOversizedFixed1xReducesToCurrentZeroDrawSizeBehavior() {
    ViewportPolicy policy;

    DisplayViewport viewport = policy.viewportFor(PixelSize(12, 8),
        PixelSize(10, 9), 1);

    assertViewport(viewport, SCALE_MODE_FIXED_ZOOM, PixelSize(12, 8),
        PixelSize(10, 9), PixelSize(0, 0), PixelRect(5, 4, 0, 0),
        1, 0, 1);
}

static void testOddSizedWindowUsesLegacyIntegerOffsets() {
    ViewportPolicy policy;

    DisplayViewport viewport = policy.viewportFor(PixelSize(5, 3),
        PixelSize(12, 10), 1);

    assertViewport(viewport, SCALE_MODE_FIXED_ZOOM, PixelSize(5, 3),
        PixelSize(12, 10), PixelSize(5, 3), PixelRect(3, 3, 5, 3),
        1, 1, 0);
}

static void testViewportChangesDescribeBorderClearCases() {
    ViewportPolicy policy;

    DisplayViewport previous = policy.viewportFor(PixelSize(4, 3),
        PixelSize(10, 9), 1);
    DisplayViewport same = policy.viewportFor(PixelSize(4, 3),
        PixelSize(10, 9), 1);
    DisplayViewport resized = policy.viewportFor(PixelSize(4, 3),
        PixelSize(12, 9), 1);
    DisplayViewport fit = policy.viewportFor(PixelSize(4, 3),
        PixelSize(10, 9), 0);

    assert(!same.requiresBorderClearFrom(previous));
    assert(resized.requiresBorderClearFrom(previous));
    assert(fit.requiresBorderClearFrom(previous));
}

int main() {
    testFitToWindowUsesWholeDisplayArea();
    testFixed1xCentersSourceSizedOutput();
    testFixed2xScalesAndCentersOutput();
    testOversizedFixed2xReducesToFixed1x();
    testOversizedFixed1xReducesToCurrentZeroDrawSizeBehavior();
    testOddSizedWindowUsesLegacyIntegerOffsets();
    testViewportChangesDescribeBorderClearCases();
    return 0;
}
