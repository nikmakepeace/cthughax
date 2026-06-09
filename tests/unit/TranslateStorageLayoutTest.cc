/** @file
 * Unit coverage for translation on padded FrameStore layouts.
 */

#include "FrameStore.h"
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

static void fillPassive(FrameRenderTarget& target) {
    for (int y = 0; y < target.height(); y++) {
        unsigned char* row = target.passiveRow(y);
        for (int x = 0; x < target.width(); x++)
            row[x] = patternValue(x, y);
        for (int x = target.width(); x < target.pitch(); x++)
            row[x] = 0xee;
    }
}

static void assertVisibleMatches(const FrameRenderTarget& expected,
    const FrameRenderTarget& actual) {
    for (int y = 0; y < expected.height(); y++) {
        const unsigned char* expectedRow = expected.activeRow(y);
        const unsigned char* actualRow = actual.activeRow(y);
        for (int x = 0; x < expected.width(); x++)
            assert(expectedRow[x] == actualRow[x]);
    }
}

static void assertActivePaddingUntouched(const FrameRenderTarget& target) {
    for (int y = 0; y < target.height(); y++) {
        const unsigned char* row = target.activeRow(y);
        for (int x = target.width(); x < target.pitch(); x++)
            assert(row[x] == 0xee);
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
    fillPassive(packed);
    fillPassive(padded);

    std::vector<int> table = makeMirrorYTable(packed.width(), packed.height());
    TranslationTable translationTable("mirror-y", table.data(),
        packed.width(), packed.height());
    Translate translate(translationTable);
    FrameGeneratorContext context;

    translate.execute(packed, context);
    translate.execute(padded, context);

    assertVisibleMatches(packed, padded);
    assertActivePaddingUntouched(padded);
}

int main() {
    testTranslateUsesPitchedRows();
    return 0;
}
