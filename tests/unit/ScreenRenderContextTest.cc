#include "IndexedFrameTestFixtures.h"
#include "ScreenRenderContext.h"

#include <assert.h>

class CountingSelectionController : public ScreenSelectionController {
public:
    int calls;
    int by;
    int doSave;

    CountingSelectionController()
        : calls(0)
        , by(0)
        , doSave(0) {
    }

    virtual void change(int by_, int doSave_) {
        calls++;
        by = by_;
        doSave = doSave_;
    }
};

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

static void testSelectionControllerReceivesRetryRequest() {
    IndexedFrameFixture source(5, 3, 8);
    IndexedDisplayFrame destination;
    preparePaddedDestination(destination, 4, 2, 7, 0x5a);
    CountingSelectionController controller;

    ScreenRenderContext context(source.frame(), destination, 12.5, 0.125, 59.5,
        &controller);

    assert(context.requestScreenChange(+1, 0));
    assert(controller.calls == 1);
    assert(controller.by == +1);
    assert(controller.doSave == 0);
}

static void testMissingSelectionControllerRejectsRetryRequest() {
    IndexedFrameFixture source(5, 3, 8);
    IndexedDisplayFrame destination;
    preparePaddedDestination(destination, 4, 2, 7, 0x5a);

    ScreenRenderContext context(source.frame(), destination, 12.5, 0.125, 59.5);

    assert(!context.requestScreenChange(+1, 0));
}

int main() {
    testRowAccessorsHonorSourceAndDestinationPitch();
    testFrameTimingValuesAreCopied();
    testSelectionControllerReceivesRetryRequest();
    testMissingSelectionControllerRejectsRetryRequest();
    return 0;
}
