/** @file
 * Unit coverage for translation on padded FrameStore layouts.
 */

#include "FrameStore.h"
#include "FrameStageBuffer.h"
#include "Translate.h"
#include "FrameGeneratorContext.h"

#include <assert.h>
#include <stdarg.h>
#include <vector>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

static unsigned char patternValue(int x, int y) {
    return (unsigned char)((x * 17 + y * 43 + 29) & 0xff);
}

static void fillSourcePattern(FrameRenderTarget& target) {
    for (int y = 0; y < target.height(); y++) {
        unsigned char* row = target.sourceRow(y);
        for (int x = 0; x < target.width(); x++)
            row[x] = patternValue(x, y);
        for (int x = target.width(); x < target.pitch(); x++)
            row[x] = 0xdd;
    }
}

static void fillDestinationBlank(FrameRenderTarget& target) {
    for (int y = 0; y < target.height(); y++) {
        unsigned char* row = target.destinationRow(y);
        for (int x = 0; x < target.width(); x++)
            row[x] = 0;
        for (int x = target.width(); x < target.pitch(); x++)
            row[x] = 0xee;
    }
}

static void assertVisibleMatches(const FrameRenderTarget& expected,
    const FrameRenderTarget& actual) {
    for (int y = 0; y < expected.height(); y++) {
        const unsigned char* expectedRow = expected.destinationRow(y);
        const unsigned char* actualRow = actual.destinationRow(y);
        for (int x = 0; x < expected.width(); x++)
            assert(expectedRow[x] == actualRow[x]);
    }
}

static void assertDestinationPaddingUntouched(const FrameRenderTarget& target) {
    for (int y = 0; y < target.height(); y++) {
        const unsigned char* row = target.destinationRow(y);
        for (int x = target.width(); x < target.pitch(); x++)
            assert(row[x] == 0xee);
    }
}

static void assertMirrorYResult(const FrameRenderTarget& target) {
    for (int y = 0; y < target.height(); y++) {
        const unsigned char* row = target.destinationRow(y);
        int sourceY = target.height() - 1 - y;
        for (int x = 0; x < target.width(); x++) {
            unsigned char expected = (x == 0 && sourceY == 0)
                ? 0
                : patternValue(x, sourceY);
            assert(row[x] == expected);
        }
    }
}

static std::vector<int> makeMirrorYTable(int width, int height) {
    std::vector<int> table(width * height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++)
            table[y * width + x] = (height - 1 - y) * width + x;
    }
    return table;
}

static void testTranslateUsesPitchedRows() {
    FrameStore packedStore;
    FrameStore paddedStore;
    packedStore.resize(FrameStorageLayout(PixelSize(6, 4), 6, 3));
    paddedStore.resize(FrameStorageLayout(PixelSize(6, 4), 10, 3));

    FrameRenderTarget& packed = packedStore.renderTarget();
    FrameRenderTarget& padded = paddedStore.renderTarget();
    fillSourcePattern(packed);
    fillSourcePattern(padded);
    fillDestinationBlank(packed);
    fillDestinationBlank(padded);

    std::vector<int> table = makeMirrorYTable(packed.width(), packed.height());
    TranslationTable translationTable("mirror-y", table.data(),
        packed.width(), packed.height());
    Translate translate(translationTable);
    FrameGeneratorContext context;

    FrameStageBuffer packedStageBuffer(packed);
    FrameStageBuffer paddedStageBuffer(padded);
    translate.execute(packedStageBuffer, context);
    translate.execute(paddedStageBuffer, context);

    assertMirrorYResult(packed);
    assertMirrorYResult(padded);
    assertVisibleMatches(packed, padded);
    assertDestinationPaddingUntouched(padded);
}

static void testTranslateIndexZeroMapsBlackWithoutMutatingSource() {
    FrameStore store;
    store.resize(FrameStorageLayout(PixelSize(3, 2), 5, 3));
    FrameRenderTarget& target = store.renderTarget();

    fillSourcePattern(target);
    fillDestinationBlank(target);
    unsigned char sourceTopLeft = target.sourcePixels()[0];
    assert(sourceTopLeft != 0);

    std::vector<int> table(target.size(), 0);
    TranslationTable translationTable("all-black", table.data(),
        target.width(), target.height());
    Translate translate(translationTable);
    FrameGeneratorContext context;

    FrameStageBuffer stageBuffer(target);
    translate.execute(stageBuffer, context);

    for (int y = 0; y < target.height(); y++) {
        const unsigned char* row = target.destinationRow(y);
        for (int x = 0; x < target.width(); x++)
            assert(row[x] == 0);
    }

    assert(target.sourcePixels()[0] == sourceTopLeft);
}

int main() {
    testTranslateUsesPitchedRows();
    testTranslateIndexZeroMapsBlackWithoutMutatingSource();
    return 0;
}
