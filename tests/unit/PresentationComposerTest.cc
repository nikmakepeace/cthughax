#include "FramePalette.h"
#include "IndexedFrameTestFixtures.h"
#include "PresentationComposer.h"
#include "Screen.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int copySourceRenderer(ScreenRenderContext& context) {
    for (int y = 0; y < context.sourceHeight(); ++y)
        memcpy(context.destinationLine(y), context.sourceLine(y),
            context.sourceWidth());

    return 0;
}

static int rejectRenderer(ScreenRenderContext&) {
    return 1;
}

static int conditionalRejectRendererShouldReject = 0;
static int conditionalRejectRenderer(ScreenRenderContext& context) {
    if (conditionalRejectRendererShouldReject)
        return 1;

    return copySourceRenderer(context);
}

int screen_up(ScreenRenderContext&) { return 0; }
int screen_down(ScreenRenderContext&) { return 0; }
int screen_2hor(ScreenRenderContext&) { return 0; }
int screen_r2hor(ScreenRenderContext&) { return 0; }
int screen_4hor(ScreenRenderContext&) { return 0; }
int screen_2verd(ScreenRenderContext&) { return 0; }
int screen_r2verd(ScreenRenderContext&) { return 0; }
int screen_4kal(ScreenRenderContext&) { return 0; }
int screen_hfield(ScreenRenderContext&) { return 0; }
int screen_roll(ScreenRenderContext&) { return 0; }
int screen_zick(ScreenRenderContext&) { return 0; }
int screen_bent(ScreenRenderContext&) { return 0; }
int screen_plate(ScreenRenderContext&) { return 0; }
int screen_vscale_hmirror(ScreenRenderContext&) { return 0; }
int screen_hscale_vmirror(ScreenRenderContext&) { return 0; }
int screen_source(ScreenRenderContext&) { return 0; }

int cth_log_enabled(int /*lvl*/) {
    return 0;
}

int cth_log(int /*lvl*/, const char* /*fmt*/, ...) {
    return 0;
}

int cth_log_context(int /*lvl*/, const char* /*context*/, const char* /*fmt*/,
    ...) {
    return 0;
}

int cth_log_error(const char* /*fmt*/, ...) {
    return 0;
}

int cth_log_errno(int /*errnum*/, const char* /*fmt*/, ...) {
    return 0;
}

class StaticSelection : public PresentationScreenSelection {
    ScreenEntry* currentValue;

public:
    explicit StaticSelection(ScreenEntry& entry)
        : currentValue(&entry) {
    }

    virtual ScreenEntry* current() {
        return currentValue;
    }

    void set(ScreenEntry& entry) {
        currentValue = &entry;
    }
};

static void assertRowEquals(const IndexedDisplayFrame& frame, int y,
    const unsigned char* expected, int width) {
    int mismatch = firstIndexedRowMismatch(frame.line(y), expected, width);
    if (mismatch != -1) {
        fprintf(stderr, "row %d mismatch at x=%d: got %u expected %u\n", y,
            mismatch, unsigned(frame.line(y)[mismatch]),
            unsigned(expected[mismatch]));
    }
    assert(mismatch == -1);
}

static void assertSourceRows(const IndexedDisplayFrame& frame,
    const IndexedFrameFixture& source) {
    for (int y = 0; y < source.height(); ++y)
        assertRowEquals(frame, y, source.line(y), source.width());
}

static void assertHorizontallyMirroredSourceRow(const IndexedDisplayFrame& frame,
    const IndexedFrameFixture& source, int frameY, int sourceY) {
    unsigned char expected[16];
    assert(2 * source.width() <= int(sizeof(expected)));

    for (int x = 0; x < source.width(); ++x) {
        expected[x] = source.line(sourceY)[x];
        expected[2 * source.width() - x - 1] = source.line(sourceY)[x];
    }

    assertRowEquals(frame, frameY, expected, 2 * source.width());
}

static void assertClassicMirroredSource(const IndexedDisplayFrame& frame,
    const IndexedFrameFixture& source) {
    assert(frame.width() == 2 * source.width());
    assert(frame.height() == 2 * source.height());

    for (int y = 0; y < source.height(); ++y) {
        assertHorizontallyMirroredSourceRow(frame, source, y, y);
        assertHorizontallyMirroredSourceRow(frame, source,
            2 * source.height() - y - 1, y);
    }
}

static void testSourceSizedOutputUsesSelectedEntryGeometry() {
    ScreenEntry sourceEntry(copySourceRenderer, "Source", "Source size",
        xy(1, 1), xy(1, 1));
    StaticSelection selection(sourceEntry);
    IndexedFrameFixture source(4, 3, 6);
    IndexedDisplayFrame destination;
    PresentationComposer composer;

    const IndexedDisplayFrame& result = composer.compose(source.frame(),
        destination, selection, 12.0, 0.25, 60.0);

    assert(&result == &destination);
    assert(composer.renderedScreen() == &sourceEntry);
    assert(destination.width() == 4);
    assert(destination.height() == 3);
    assert(destination.pitch() == 4);
    assertSourceRows(destination, source);
}

static void testClassicOutputIsCompletedInIndexedFrame() {
    ScreenEntry classicEntry(copySourceRenderer, "Up", "Classic 2x",
        xy(1, 1));
    StaticSelection selection(classicEntry);
    IndexedFrameFixture source(4, 3, 6);
    IndexedDisplayFrame destination;
    PresentationComposer composer;

    composer.compose(source.frame(), destination, selection, 12.0, 0.25, 60.0);

    assertClassicMirroredSource(destination, source);
}

static void testScaleYCompletesHorizontallyOnly() {
    ScreenEntry scaleYEntry(copySourceRenderer, "scaley", "Scale Y",
        xy(1, 1), xy(2, 1));
    StaticSelection selection(scaleYEntry);
    IndexedFrameFixture source(4, 3, 6);
    IndexedDisplayFrame destination;
    PresentationComposer composer;

    composer.compose(source.frame(), destination, selection, 12.0, 0.25, 60.0);

    assert(destination.width() == 8);
    assert(destination.height() == 3);
    for (int y = 0; y < source.height(); ++y)
        assertHorizontallyMirroredSourceRow(destination, source, y, y);
}

static void testScaleXCompletesVerticallyOnly() {
    ScreenEntry scaleXEntry(copySourceRenderer, "scalex", "Scale X",
        xy(1, 1), xy(1, 2));
    StaticSelection selection(scaleXEntry);
    IndexedFrameFixture source(4, 3, 6);
    IndexedDisplayFrame destination;
    PresentationComposer composer;

    composer.compose(source.frame(), destination, selection, 12.0, 0.25, 60.0);

    assert(destination.width() == 4);
    assert(destination.height() == 6);
    for (int y = 0; y < source.height(); ++y) {
        assertRowEquals(destination, y, source.line(y), source.width());
        assertRowEquals(destination, 2 * source.height() - y - 1,
            source.line(y), source.width());
    }
}

static void testPalettePointerIsPropagated() {
    ScreenEntry sourceEntry(copySourceRenderer, "Source", "Source size",
        xy(1, 1), xy(1, 1));
    StaticSelection selection(sourceEntry);
    IndexedFrameFixture fixture(4, 3, 6);
    FramePalette framePalette;
    IndexedFrame source(fixture.frame().pixels, fixture.width(), fixture.height(),
        fixture.pitch(), &framePalette);
    IndexedDisplayFrame destination;
    PresentationComposer composer;

    composer.compose(source, destination, selection, 12.0, 0.25, 60.0);

    assert(destination.framePalette() == &framePalette);
}

static void testCapacityIsReusedWhenFrameShrinks() {
    ScreenEntry classicEntry(copySourceRenderer, "Up", "Classic 2x",
        xy(1, 1));
    ScreenEntry sourceEntry(copySourceRenderer, "Source", "Source size",
        xy(1, 1), xy(1, 1));
    StaticSelection largeSelection(classicEntry);
    StaticSelection smallSelection(sourceEntry);
    IndexedFrameFixture largeSource(4, 4, 6);
    IndexedFrameFixture smallSource(2, 2, 3);
    IndexedDisplayFrame destination;
    PresentationComposer composer;

    composer.compose(largeSource.frame(), destination, largeSelection, 12.0,
        0.25, 60.0);
    unsigned char* largePixels = destination.pixels();
    int largeCapacity = destination.capacityByteCount();

    composer.compose(smallSource.frame(), destination, smallSelection, 12.5,
        0.25, 60.0);

    assert(destination.width() == 2);
    assert(destination.height() == 2);
    assert(destination.pixels() == largePixels);
    assert(destination.capacityByteCount() == largeCapacity);
    assertSourceRows(destination, smallSource);
}

static void testRejectedSelectionFallsBackToLastRenderedScreen() {
    ScreenEntry previousEntry(copySourceRenderer, "previous", "Previous",
        xy(1, 1));
    ScreenEntry rejectEntry(rejectRenderer, "reject", "Reject", xy(1, 1),
        xy(1, 1));
    StaticSelection selection(previousEntry);
    IndexedFrameFixture source(4, 3, 6);
    IndexedDisplayFrame destination;
    PresentationComposer composer;

    composer.compose(source.frame(), destination, selection, 12.0, 0.25, 60.0);

    assert(composer.renderedScreen() == &previousEntry);
    assertClassicMirroredSource(destination, source);

    selection.set(rejectEntry);
    composer.compose(source.frame(), destination, selection, 12.5, 0.25, 60.0);

    assert(selection.current() == &rejectEntry);
    assert(composer.renderedScreen() == &previousEntry);
    assertClassicMirroredSource(destination, source);
}

static void testFirstFrameRejectionFallsBackToSafeScreen() {
    ScreenEntry rejectEntry(rejectRenderer, "reject", "Reject", xy(1, 1),
        xy(1, 1));
    StaticSelection selection(rejectEntry);
    IndexedFrameFixture source(4, 3, 6);
    IndexedDisplayFrame destination;
    PresentationComposer composer;

    composer.compose(source.frame(), destination, selection, 12.0, 0.25, 60.0);

    assert(selection.current() == &rejectEntry);
    assert(composer.renderedScreen() == screenByIndex(0));
    assert(destination.width() == 8);
    assert(destination.height() == 6);
}

static void testRejectedLastRenderedScreenFallsBackToSafeScreen() {
    ScreenEntry previousEntry(conditionalRejectRenderer, "conditional",
        "Conditional", xy(1, 1));
    ScreenEntry rejectEntry(rejectRenderer, "reject", "Reject", xy(1, 1),
        xy(1, 1));
    StaticSelection selection(previousEntry);
    IndexedFrameFixture source(4, 3, 6);
    IndexedDisplayFrame destination;
    PresentationComposer composer;

    conditionalRejectRendererShouldReject = 0;
    composer.compose(source.frame(), destination, selection, 12.0, 0.25, 60.0);
    assert(composer.renderedScreen() == &previousEntry);

    conditionalRejectRendererShouldReject = 1;
    selection.set(rejectEntry);
    composer.compose(source.frame(), destination, selection, 12.5, 0.25, 60.0);
    conditionalRejectRendererShouldReject = 0;

    assert(selection.current() == &rejectEntry);
    assert(composer.renderedScreen() == screenByIndex(0));
    assert(destination.width() == 8);
    assert(destination.height() == 6);
}

int main() {
    testSourceSizedOutputUsesSelectedEntryGeometry();
    testClassicOutputIsCompletedInIndexedFrame();
    testScaleYCompletesHorizontallyOnly();
    testScaleXCompletesVerticallyOnly();
    testPalettePointerIsPropagated();
    testCapacityIsReusedWhenFrameShrinks();
    testRejectedSelectionFallsBackToLastRenderedScreen();
    testFirstFrameRejectionFallsBackToSafeScreen();
    testRejectedLastRenderedScreenFallsBackToSafeScreen();
    return 0;
}
