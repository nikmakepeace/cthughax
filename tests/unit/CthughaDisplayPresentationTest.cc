#include "CthughaDisplay.h"
#include "DisplayDevice.h"
#include "DisplayRuntime.h"
#include "FramePalette.h"
#include "IndexedFrameTestFixtures.h"
#include "Screen.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

DisplayDevice::DisplayDevice()
    : textOnScreen(0)
    , darkenPalette(0)
    , needsFullCopy(0)
    , framePalette(0) { }

DisplayDevice::~DisplayDevice() { }

void DisplayDevice::setFramePalette(FramePalette* framePalette_) {
    framePalette = framePalette_;
}

int DisplayDevice::setGlobalPalette() {
    return 0;
}

void DisplayDevice::prePrint() { }

void DisplayDevice::postPrint() { }

void DisplayDevice::printString(
    int, int, const char*, int, int, int) { }

double DisplayDevice::print(const char*, double y, int, int, int) {
    return y;
}

class StaticOutputBackend : public DisplayBackend {
public:
    virtual DisplayEventStats processEvents() {
        return DisplayEventStats();
    }

    virtual PixelSize outputSize() const {
        return PixelSize(20, 15);
    }

    virtual void present(const DisplayPresentation&) {
    }
};

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

double getTime() {
    return 100.0;
}

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

static int copySourceRenderer(ScreenRenderContext& context) {
    for (int y = 0; y < context.sourceHeight(); ++y)
        memcpy(context.destinationLine(y), context.sourceLine(y),
            context.sourceWidth());

    return 0;
}

static int retryRenderer(ScreenRenderContext& context) {
    context.requestScreenChange(+1, 0);
    return 1;
}

class StaticSelection : public PresentationScreenSelection {
    ScreenEntry* currentValue;

public:
    explicit StaticSelection(ScreenEntry& current)
        : currentValue(&current) {
    }

    virtual ScreenEntry* current() {
        return currentValue;
    }

    virtual void change(int /*by*/, int /*doSave*/) {
    }
};

class RetryingSelection : public PresentationScreenSelection {
    ScreenEntry* firstValue;
    ScreenEntry* secondValue;
    int changed;

public:
    int calls;
    int by;
    int doSave;

    RetryingSelection(ScreenEntry& first, ScreenEntry& second)
        : firstValue(&first)
        , secondValue(&second)
        , changed(0)
        , calls(0)
        , by(0)
        , doSave(0) {
    }

    virtual ScreenEntry* current() {
        return changed ? secondValue : firstValue;
    }

    virtual void change(int by_, int doSave_) {
        calls++;
        by = by_;
        doSave = doSave_;
        changed = 1;
    }
};

class DisplayConsumerHarness : public CthughaDisplay {
    PresentationScreenSelection& selection;

public:
    int presented;

    DisplayConsumerHarness(PresentationScreenSelection& selection_,
        DisplayDevice& device, DisplayRuntime& runtime)
        : CthughaDisplay(device, runtime)
        , selection(selection_)
        , presented(0) {
        fps = 60.0;
        rollingFps = 60.0;
    }

    virtual void operator()() {
        presented++;
        const IndexedDisplayFrame& frame = composePresentationFrame(selection);
        buffer = buffer0;
        bufferWidth = frame.pitch();
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

static void testPresentComposesCompletedIndexedFrameForConsumer() {
    ScreenEntry classicEntry(copySourceRenderer, "Up", "Classic 2x",
        xy(1, 1));
    StaticSelection selection(classicEntry);
    IndexedFrameFixture fixture(4, 3, 6);
    FramePalette framePalette;
    IndexedFrame source(fixture.frame().pixels, fixture.width(), fixture.height(),
        fixture.pitch(), &framePalette);
    DisplayDevice device;
    StaticOutputBackend backend;
    DisplayRuntime runtime(backend);
    DisplayConsumerHarness display(selection, device, runtime);

    display.present(source);

    const IndexedDisplayFrame& frame = display.indexedDisplayFrame();
    assert(display.presented == 1);
    assert(device.framePalette == &framePalette);
    assert(frame.framePalette() == &framePalette);
    assert(display.buffer == frame.pixels());
    assert(display.bufferWidth == frame.pitch());
    assertClassicMirroredSource(frame, fixture);
}

static void testPresentHonorsComposerRetryPath() {
    ScreenEntry retryEntry(retryRenderer, "retry", "Retry", xy(1, 1));
    ScreenEntry fallbackEntry(copySourceRenderer, "Up", "Classic 2x",
        xy(1, 1));
    RetryingSelection selection(retryEntry, fallbackEntry);
    IndexedFrameFixture source(4, 3, 6);
    DisplayDevice device;
    StaticOutputBackend backend;
    DisplayRuntime runtime(backend);
    DisplayConsumerHarness display(selection, device, runtime);

    display.present(source.frame());

    assert(selection.calls == 1);
    assert(selection.by == +1);
    assert(selection.doSave == 0);
    assertClassicMirroredSource(display.indexedDisplayFrame(), source);
}

int main() {
    testPresentComposesCompletedIndexedFrameForConsumer();
    testPresentHonorsComposerRetryPath();
    return 0;
}
