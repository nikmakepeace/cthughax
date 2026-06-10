#include "AudioAnalyzer.h"
#include "AudioFrame.h"
#include "IndexedFrameTestFixtures.h"
#include "Screen.h"
#include "ScreenRenderContext.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static const unsigned char kDestinationSentinel = 0xa5;

double now = 0.0;
double deltaT = 0.0;

AudioMetrics::AudioMetrics()
    : amplitude(0)
    , amplitudeLeft(0)
    , amplitudeRight(0)
    , lowPass150HzAmplitude(0)
    , lowPass150HzAmplitudeLeft(0)
    , lowPass150HzAmplitudeRight(0)
    , noisy(0) {
}

AcousticContext::AcousticContext(LogSink* log_)
    : log(log_)
    , intensityValue(0.0)
    , lastAmplitudeValue(0)
    , attackLevelValue(0)
    , fireValue(0)
    , cumulativeFireLevelValue(0) {
}

double AcousticContext::intensity() const {
    return intensityValue;
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
    return entry.render(context);
}

static int renderScreenEntry(ScreenEntry& entry, const IndexedFrame& source,
    IndexedDisplayFrame& destination, double contextDeltaTime) {
    xy output = entry.outputSize(source.width, source.height);
    preparePaddedDestination(destination, output.x, output.y, output.x + 3,
        kDestinationSentinel);

    ScreenRenderContext context(source, destination, now, contextDeltaTime, 60.0);
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

static unsigned char sourceFixtureAt(const IndexedFrameFixture& source, int x, int y) {
    return source.line(y)[x];
}

static void assertRenderedVisibleAreasDiffer(const IndexedDisplayFrame& left,
    const IndexedDisplayFrame& right) {
    assert(left.width() == right.width());
    assert(left.height() == right.height());

    for (int y = 0; y < left.height(); ++y) {
        const unsigned char* leftRow = left.line(y);
        const unsigned char* rightRow = right.line(y);
        for (int x = 0; x < left.width(); ++x) {
            if (leftRow[x] != rightRow[x])
                return;
        }
    }

    assert(!"expected rendered visible pixels to differ");
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

static void testHorizontalSplitRenderersMatchCurrentLinePermutations() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;

    ScreenEntry& twoHorizontal = requiredScreenEntry(2, "2hor");
    int result = renderScreenEntry(twoHorizontal, source.frame(), destination);
    assert(result == 0);

    const int twoHorizontalRows[] = { 2, 3, 3, 2 };
    for (int y = 0; y < source.height(); ++y)
        assertRowEquals(destination, y, source.line(twoHorizontalRows[y]), source.width());
    assertDestinationRemainderUnchanged(destination, source.width(), source.height());

    ScreenEntry& reverseTwoHorizontal = requiredScreenEntry(3, "r2hor");
    result = renderScreenEntry(reverseTwoHorizontal, source.frame(), destination);
    assert(result == 0);

    const int reverseTwoHorizontalRows[] = { 3, 2, 2, 3 };
    for (int y = 0; y < source.height(); ++y)
        assertRowEquals(destination, y, source.line(reverseTwoHorizontalRows[y]),
            source.width());
    assertDestinationRemainderUnchanged(destination, source.width(), source.height());
}

static void testVerticalMirrorRenderersMatchCurrentColumnMapping() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;
    unsigned char expected[4];

    ScreenEntry& twoVertical = requiredScreenEntry(5, "2verd");
    int result = renderScreenEntry(twoVertical, source.frame(), destination);
    assert(result == 0);

    for (int y = 0; y < source.height(); ++y) {
        expected[0] = sourceFixtureAt(source, y, 0);
        expected[1] = sourceFixtureAt(source, y, 1);
        expected[2] = sourceFixtureAt(source, y, 2);
        expected[3] = sourceFixtureAt(source, y, 1);
        assertRowEquals(destination, y, expected, 4);
    }
    assertDestinationRemainderUnchanged(destination, source.width(), source.height());

    ScreenEntry& reverseTwoVertical = requiredScreenEntry(6, "r2verd");
    result = renderScreenEntry(reverseTwoVertical, source.frame(), destination);
    assert(result == 0);

    for (int y = 0; y < source.height(); ++y) {
        int sourceColumn = source.width() - y - 1;
        expected[0] = sourceFixtureAt(source, sourceColumn, 1);
        expected[1] = sourceFixtureAt(source, sourceColumn, 2);
        expected[2] = sourceFixtureAt(source, sourceColumn, 1);
        expected[3] = sourceFixtureAt(source, sourceColumn, 0);
        assertRowEquals(destination, y, expected, 4);
    }
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

static void testFourKaleidoscopeUsesCurrentQuadrantMapping() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;
    ScreenEntry& entry = requiredScreenEntry(7, "4kal");

    int result = renderScreenEntry(entry, source.frame(), destination);

    assert(result == 0);
    assert(destination.width() == 2 * source.width());
    assert(destination.height() == 2 * source.height());

    unsigned char expected[4];
    for (int y = 0; y < source.height() / 2; ++y) {
        expected[0] = sourceFixtureAt(source, y, 0);
        expected[1] = sourceFixtureAt(source, y, 1);
        expected[2] = sourceFixtureAt(source, y, 2);
        expected[3] = sourceFixtureAt(source, y, 1);
        assertRowEquals(destination, y, expected, 4);
    }
    for (int y = source.height() / 2; y < source.height(); ++y) {
        int sourceColumn = source.height() - y - 1;
        expected[0] = sourceFixtureAt(source, sourceColumn, 1);
        expected[1] = sourceFixtureAt(source, sourceColumn, 2);
        expected[2] = sourceFixtureAt(source, sourceColumn, 1);
        expected[3] = sourceFixtureAt(source, sourceColumn, 0);
        assertRowEquals(destination, y, expected, 4);
    }

    assertDestinationRemainderUnchanged(destination, source.width(), source.height());
}

static void assertGeometryRejection(ScreenEntry& entry) {
    IndexedFrameFixture source(8, 3, 11);
    IndexedDisplayFrame destination;
    xy output = entry.outputSize(source.width(), source.height());
    preparePaddedDestination(destination, output.x, output.y, output.x + 3,
        kDestinationSentinel);

    ScreenRenderContext context(source.frame(), destination, now, deltaT, 60.0);
    screen.change("2verd", 0);

    int result = entry.render(context);

    assert(result == 1);
    assertDestinationRemainderUnchanged(destination, 0, 0);
}

static void testGeometryRejectionDoesNotChooseReplacement() {
    assertGeometryRejection(requiredScreenEntry(5, "2verd"));
    assertGeometryRejection(requiredScreenEntry(6, "r2verd"));
    assertGeometryRejection(requiredScreenEntry(7, "4kal"));
}

static void testScreenEntryRenderUsesExplicitContext() {
    IndexedFrameFixture source(4, 4, 7);
    IndexedDisplayFrame destination;
    ScreenEntry& entry = requiredScreenEntry(0, "Up");
    xy output = entry.outputSize(source.width(), source.height());
    preparePaddedDestination(destination, output.x, output.y, output.x + 3,
        kDestinationSentinel);

    ScreenRenderContext context(source.frame(), destination, now, deltaT, 60.0);
    assert(entry.render(context) == 0);

    assertUpLikeOutput(destination, source);
}

static void testHeightfieldUsesContextDeltaTime() {
    deltaT = 0.0;
    IndexedFrameFixture source(16, 16, 19);
    IndexedDisplayFrame baseline;
    IndexedDisplayFrame advanced;
    ScreenEntry& entry = requiredScreenEntry(8, "hfield");

    int result = renderScreenEntry(entry, source.frame(), baseline, 0.0);
    assert(result == 0);

    for (int i = 0; i < 8; ++i) {
        result = renderScreenEntry(entry, source.frame(), advanced, 0.25);
        assert(result == 0);
    }

    assertRenderedVisibleAreasDiffer(baseline, advanced);
}

int main() {
    testSourceUsesSourceSizedOutputAndPaddedPitches();
    testUpCopiesVisibleRowsAndLeavesUnfilledLegacyOutput();
    testDownReversesVisibleRowsAndLeavesUnfilledLegacyOutput();
    testHorizontalSplitRenderersMatchCurrentLinePermutations();
    testVerticalMirrorRenderersMatchCurrentColumnMapping();
    testScaleYRendererMatchesUpBeforeCompletion();
    testScaleXRendererMatchesUpBeforeCompletion();
    testFourHorizontalKaleidoscopeUsesCurrentLowerHalfMapping();
    testFourKaleidoscopeUsesCurrentQuadrantMapping();
    testGeometryRejectionDoesNotChooseReplacement();
    testScreenEntryRenderUsesExplicitContext();
    testHeightfieldUsesContextDeltaTime();
    return 0;
}
