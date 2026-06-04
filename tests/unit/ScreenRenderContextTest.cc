#include "IndexedFrameTestFixtures.h"
#include "ScreenRenderContext.h"

#include <assert.h>

static void testRowAccessorsHonorSourceAndDestinationPitch() {
    IndexedFrameFixture source(5, 3, 8);
    IndexedDisplayFrame destination;
    preparePaddedDestination(destination, 4, 2, 7, 0x5a);

    ScreenRenderContext context(source.frame(), destination, 12.5, 0.125, 59.5);

    assert(&context.source() == &source.frame());
    assert(&context.destination() == &destination);

    assert(context.sourcePixels() == source.frame().pixels);
    assert(context.sourceLine(2) == source.line(2));
    assert(context.sourceWidth() == 5);
    assert(context.sourceHeight() == 3);
    assert(context.sourcePitch() == 8);

    assert(context.destinationPixels() == destination.pixels());
    assert(context.destinationLine(1) == destination.line(1));
    assert(context.destinationWidth() == 4);
    assert(context.destinationHeight() == 2);
    assert(context.destinationPitch() == 7);
}

static void testFrameTimingValuesAreCopied() {
    IndexedFrameFixture source(5, 3, 8);
    IndexedDisplayFrame destination;
    preparePaddedDestination(destination, 4, 2, 7, 0x5a);

    ScreenRenderContext context(source.frame(), destination, 12.5, 0.125, 59.5);

    assert(context.frameTimeSeconds() == 12.5);
    assert(context.deltaTimeSeconds() == 0.125);
    assert(context.framesPerSecond() == 59.5);
}

int main() {
    testRowAccessorsHonorSourceAndDestinationPitch();
    testFrameTimingValuesAreCopied();
    return 0;
}
