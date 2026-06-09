#include "AudioFrame.h"
#include "CthughaDisplay.h"
#include "DisplayDevice.h"
#include "DisplayPresentationOptions.h"
#include "DisplayRuntime.h"
#include "FramePalette.h"
#include "IndexedFrameTestFixtures.h"
#include "InputQueue.h"
#include "ProcessServices.h"
#include "Screen.h"
#include "FrameRenderContext.h"

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
    virtual DisplayEventStats processEvents(InputEventSink&) {
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

static int rejectRenderer(ScreenRenderContext&) {
    return 1;
}

static const AudioMetrics* observedAudioMetrics = 0;
static const char2* observedRawAudio = 0;
static const char2* observedProcessedAudio = 0;
static const AcousticContext* observedAcousticContext = 0;

static int observeAudioRenderer(ScreenRenderContext& context) {
    observedAudioMetrics = context.audioMetrics();
    observedRawAudio = context.rawAudioData();
    observedProcessedAudio = context.processedWaveData();
    observedAcousticContext = context.acousticContext();
    return copySourceRenderer(context);
}

class FakeSecondsClock : public SecondsClock {
public:
    double value;

    explicit FakeSecondsClock(double value_)
        : value(value_) { }

    virtual double nowSeconds() const { return value; }
};

static FakeSecondsClock displayClock(100.0);
static DisplayPresentationSettings displaySettings;

class StaticSelection : public PresentationScreenSelection {
    ScreenEntry* currentValue;

public:
    explicit StaticSelection(ScreenEntry& current)
        : currentValue(&current) {
    }

    virtual ScreenEntry* current() {
        return currentValue;
    }

    void set(ScreenEntry& current) {
        currentValue = &current;
    }
};

class DisplayConsumerHarness : public CthughaDisplay {
    PresentationScreenSelection& selection;

public:
    int presented;

    DisplayConsumerHarness(PresentationScreenSelection& selection_,
        DisplayDevice& device, DisplayRuntime& runtime)
        : CthughaDisplay(device, runtime, displayClock, displaySettings)
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

static void testPresentFallsBackToLastRenderedScreenAfterRejection() {
    ScreenEntry previousEntry(copySourceRenderer, "previous", "Previous",
        xy(1, 1));
    ScreenEntry rejectEntry(rejectRenderer, "reject", "Reject", xy(1, 1),
        xy(1, 1));
    StaticSelection selection(previousEntry);
    IndexedFrameFixture source(4, 3, 6);
    DisplayDevice device;
    StaticOutputBackend backend;
    DisplayRuntime runtime(backend);
    DisplayConsumerHarness display(selection, device, runtime);

    display.present(source.frame());

    assertClassicMirroredSource(display.indexedDisplayFrame(), source);

    selection.set(rejectEntry);
    display.present(source.frame());

    assert(selection.current() == &rejectEntry);
    assertClassicMirroredSource(display.indexedDisplayFrame(), source);
}

static void testPresentPassesAudioContextToScreenRenderer() {
    ScreenEntry audioEntry(observeAudioRenderer, "audio", "Audio",
        xy(1, 1), xy(1, 1));
    StaticSelection selection(audioEntry);
    IndexedFrameFixture source(4, 3, 6);
    DisplayDevice device;
    StaticOutputBackend backend;
    DisplayRuntime runtime(backend);
    DisplayConsumerHarness display(selection, device, runtime);
    AudioFrame audioFrame;
    audioFrame.metrics.amplitude = 42;

    FrameRenderContext frameContext;
    frameContext.audioFrame = &audioFrame;
    frameContext.rawAudioData = audioFrame.raw;
    frameContext.processedWaveData = audioFrame.processedWaveData;
    frameContext.audioMetrics = &audioFrame.metrics;
    frameContext.acousticContext = (const AcousticContext*)0x1234;
    observedAudioMetrics = 0;
    observedRawAudio = 0;
    observedProcessedAudio = 0;
    observedAcousticContext = 0;

    display.present(source.frame(), frameContext);

    assert(observedAudioMetrics == &audioFrame.metrics);
    assert(observedRawAudio == audioFrame.raw);
    assert(observedProcessedAudio == audioFrame.processedWaveData);
    assert(observedAcousticContext == (const AcousticContext*)0x1234);
    assert(observedAudioMetrics->amplitude == 42);
}

int main() {
    testPresentComposesCompletedIndexedFrameForConsumer();
    testPresentFallsBackToLastRenderedScreenAfterRejection();
    testPresentPassesAudioContextToScreenRenderer();
    return 0;
}
