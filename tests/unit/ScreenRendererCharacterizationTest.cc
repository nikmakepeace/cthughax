#include "AudioAnalyzer.h"
#include "AudioFrame.h"
#include "CthughaDisplay.h"
#include "IndexedFrameTestFixtures.h"
#include "Screen.h"
#include "ScreenRenderContext.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static const unsigned char kDestinationSentinel = 0xa5;

CthughaDisplay* cthughaDisplay = 0;
xy draw_size(0, 0);
double now = 0.0;
double deltaT = 0.0;

AudioMetrics audioMetrics;
AcousticContext acousticContext;

AudioMetrics::AudioMetrics()
    : amplitude(0)
    , amplitudeLeft(0)
    , amplitudeRight(0)
    , noisy(0) {
}

AcousticContext::AcousticContext()
    : intensityValue(0.0)
    , lastAmplitudeValue(0)
    , attackLevelValue(0)
    , fireValue(0)
    , cumulativeFireLevelValue(0) {
}

double AcousticContext::intensity() const {
    return intensityValue;
}

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

static char2 emptyProcessedWaveData[1024];

char2* audioFrameProcessedWaveData() {
    return emptyProcessedWaveData;
}

int cth_log_enabled(int /*lvl*/) {
    return 0;
}

int cth_log(int /*lvl*/, const char* /*fmt*/, ...) {
    return 0;
}

int cth_log_context(int /*lvl*/, const char* /*context*/, const char* /*fmt*/, ...) {
    return 0;
}

int cth_log_error(const char* /*fmt*/, ...) {
    return 0;
}

int cth_log_errno(int /*errnum*/, const char* /*fmt*/, ...) {
    return 0;
}

CthughaDisplay::CthughaDisplay()
    : sourceFrame(0)
    , indexedDisplayFrameValue()
    , buffer0(0)
    , displayStart(0.0)
    , frames(0)
    , visualLatencyEstimate(0.0)
    , buffer(0)
    , bufferWidth(0)
    , expandedBuffer(0)
    , expandedBufferWidth(0)
    , needsClear(0)
    , fps(60.0) {
}

CthughaDisplay::~CthughaDisplay() {
}

const unsigned char* CthughaDisplay::sourcePixels() const {
    assert(sourceFrame != 0);
    assert(sourceFrame->valid());
    return sourceFrame->pixels;
}

int CthughaDisplay::sourceWidth() const {
    assert(sourceFrame != 0);
    return sourceFrame->width;
}

int CthughaDisplay::sourceHeight() const {
    assert(sourceFrame != 0);
    return sourceFrame->height;
}

int CthughaDisplay::sourcePitch() const {
    assert(sourceFrame != 0);
    return sourceFrame->pitch;
}

int CthughaDisplay::sourceSize() const {
    return sourceWidth() * sourceHeight();
}

const IndexedDisplayFrame& CthughaDisplay::indexedDisplayFrame() const {
    return indexedDisplayFrameValue;
}

int CthughaDisplay::displayFrameWidth() const {
    return indexedDisplayFrameValue.valid() ? indexedDisplayFrameValue.width()
                                           : 2 * sourceWidth();
}

int CthughaDisplay::displayFrameHeight() const {
    return indexedDisplayFrameValue.valid() ? indexedDisplayFrameValue.height()
                                            : 2 * sourceHeight();
}

static ScreenEntry& requiredScreenEntry(int index, const char* name) {
    ScreenEntry* entry = screenByIndex(index);
    assert(entry != 0);
    assert(strcmp(entry->Name(), name) == 0);
    return *entry;
}

static int renderScreenEntry(ScreenEntry& entry, const IndexedFrame& source,
    IndexedDisplayFrame& destination) {
    xy output = entry.outputSize(source.width, source.height);
    preparePaddedDestination(destination, output.x, output.y, output.x + 3,
        kDestinationSentinel);

    ScreenRenderContext context(source, destination, now, deltaT, 60.0);
    assert(cthughaDisplay == 0);
    assert(currentScreenRenderContext() == 0);
    return entry.render(context);
}

static void assertRowEquals(const IndexedDisplayFrame& destination, int y,
    const unsigned char* expected, int width) {
    int mismatch = firstIndexedRowMismatch(destination.line(y), expected, width);
    if (mismatch != -1) {
        fprintf(stderr, "row %d mismatch at x=%d: got %u expected %u\n", y,
            mismatch, unsigned(destination.line(y)[mismatch]),
            unsigned(expected[mismatch]));
    }
    assert(mismatch == -1);
}

static void assertDestinationRemainderUnchanged(const IndexedDisplayFrame& destination,
    int filledWidth, int filledHeight) {
    for (int y = 0; y < destination.height(); ++y) {
        const unsigned char* row = destination.line(y);
        int firstProtected = y < filledHeight ? filledWidth : 0;
        for (int x = firstProtected; x < destination.pitch(); ++x)
            assert(row[x] == kDestinationSentinel);
    }
}

static void assertUpLikeOutput(const IndexedDisplayFrame& destination,
    const IndexedFrameFixture& source) {
    for (int y = 0; y < source.height(); ++y)
        assertRowEquals(destination, y, source.line(y), source.width());

    assertDestinationRemainderUnchanged(destination, source.width(), source.height());
}

static void testSourceUsesSourceSizedOutputAndPaddedPitches() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;
    ScreenEntry& entry = requiredScreenEntry(15, "Source");

    int result = renderScreenEntry(entry, source.frame(), destination);

    assert(result == 0);
    assert(destination.width() == source.width());
    assert(destination.height() == source.height());
    assert(destination.pitch() == source.width() + 3);
    assertUpLikeOutput(destination, source);
}

static void testUpCopiesVisibleRowsAndLeavesUnfilledLegacyOutput() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;
    ScreenEntry& entry = requiredScreenEntry(0, "Up");

    int result = renderScreenEntry(entry, source.frame(), destination);

    assert(result == 0);
    assert(destination.width() == 2 * source.width());
    assert(destination.height() == 2 * source.height());
    assertUpLikeOutput(destination, source);
}

static void testDownReversesVisibleRowsAndLeavesUnfilledLegacyOutput() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;
    ScreenEntry& entry = requiredScreenEntry(1, "Down");

    int result = renderScreenEntry(entry, source.frame(), destination);

    assert(result == 0);
    for (int y = 0; y < source.height(); ++y)
        assertRowEquals(destination, y, source.line(source.height() - y - 1),
            source.width());
    assertDestinationRemainderUnchanged(destination, source.width(), source.height());
}

static void testScaleYRendererMatchesUpBeforeCompletion() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;
    ScreenEntry& entry = requiredScreenEntry(13, "scaley");

    int result = renderScreenEntry(entry, source.frame(), destination);

    assert(result == 0);
    assert(destination.width() == 2 * source.width());
    assert(destination.height() == source.height());
    assertUpLikeOutput(destination, source);
}

static void testScaleXRendererMatchesUpBeforeCompletion() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;
    ScreenEntry& entry = requiredScreenEntry(14, "scalex");

    int result = renderScreenEntry(entry, source.frame(), destination);

    assert(result == 0);
    assert(destination.width() == source.width());
    assert(destination.height() == 2 * source.height());
    assertUpLikeOutput(destination, source);
}

static void testFourHorizontalKaleidoscopeUsesCurrentLowerHalfMapping() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;
    ScreenEntry& entry = requiredScreenEntry(4, "4hor");

    int result = renderScreenEntry(entry, source.frame(), destination);

    assert(result == 0);
    assert(destination.width() == 2 * source.width());
    assert(destination.height() == 2 * source.height());

    const int sourceRows[] = { 2, 3, 3, 2 };
    unsigned char expected[4];
    for (int y = 0; y < source.height(); ++y) {
        const unsigned char* sourceRow = source.line(sourceRows[y]);
        expected[0] = sourceRow[0];
        expected[1] = sourceRow[1];
        expected[2] = sourceRow[2];
        expected[3] = sourceRow[1];
        assertRowEquals(destination, y, expected, 4);
    }

    assertDestinationRemainderUnchanged(destination, source.width(), source.height());
}

static void testGeometryRetryMovesToNextScreenEntry() {
    IndexedFrameFixture source(8, 3, 11);
    IndexedDisplayFrame destination;
    ScreenEntry& entry = requiredScreenEntry(5, "2verd");

    xy output = entry.outputSize(source.width(), source.height());
    preparePaddedDestination(destination, output.x, output.y, output.x + 3,
        kDestinationSentinel);

    CountingSelectionController controller;
    ScreenRenderContext context(source.frame(), destination, now, deltaT, 60.0,
        &controller);
    assert(cthughaDisplay == 0);
    assert(currentScreenRenderContext() == 0);
    screen.change("2verd", 0);

    int result = entry.render(context);

    assert(result == 1);
    assert(controller.calls == 1);
    assert(controller.by == +1);
    assert(controller.doSave == 0);
    assertDestinationRemainderUnchanged(destination, 0, 0);
}

int main() {
    testSourceUsesSourceSizedOutputAndPaddedPitches();
    testUpCopiesVisibleRowsAndLeavesUnfilledLegacyOutput();
    testDownReversesVisibleRowsAndLeavesUnfilledLegacyOutput();
    testScaleYRendererMatchesUpBeforeCompletion();
    testScaleXRendererMatchesUpBeforeCompletion();
    testFourHorizontalKaleidoscopeUsesCurrentLowerHalfMapping();
    testGeometryRetryMovesToNextScreenEntry();
    return 0;
}
