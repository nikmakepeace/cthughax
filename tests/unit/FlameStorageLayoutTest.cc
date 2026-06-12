/** @file
 * Unit coverage for flame kernels on padded FrameStore layouts.
 */

#include "Flame.h"
#include "FrameStore.h"
#include "FrameStageBuffer.h"
#include "FrameGeneratorContext.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

void flame_clear(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_down(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_upslow(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_upsubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_upfast(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_leftslow(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_leftsubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_leftfast(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_rightslow(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_rightsubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_rightfast(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_water(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_watersubtle(FrameStageBuffer& buffer,
    const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_skyline(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_weird(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_zzz(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_fade(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
    FlameRuntime& runtime);
void flame_general_subtle(FrameStageBuffer& buffer,
    const FrameGeneratorContext& context, FlameRuntime& runtime);
void flame_general_slow(FrameStageBuffer& buffer,
    const FrameGeneratorContext& context, FlameRuntime& runtime);

static unsigned char patternValue(int linearOffset, int salt) {
    return (unsigned char)((linearOffset * 37 + salt * 19 + 251) & 0xff);
}

static void fillVisibleStream(FrameRenderTarget& target) {
    int hidden = target.hiddenBorderRows();
    int first = -hidden * target.width();
    int last = target.size() + hidden * target.width();

    for (int offset = first; offset < last; offset++) {
        int storage = target.visibleLinearOffset(offset);
        target.destinationPixels()[storage] = patternValue(offset, 3);
        target.sourcePixels()[storage] = patternValue(offset, 9);
    }
}

static void copySourceVisibleStreamToDestination(FrameRenderTarget& target) {
    int hidden = target.hiddenBorderRows();
    int first = -hidden * target.width();
    int last = target.size() + hidden * target.width();

    for (int offset = first; offset < last; offset++) {
        int storage = target.visibleLinearOffset(offset);
        target.destinationPixels()[storage] = target.sourcePixels()[storage];
    }
}

static void poisonDestinationPadding(FrameRenderTarget& target) {
    for (int y = 0; y < target.height(); y++) {
        unsigned char* row = target.destinationRow(y);
        for (int x = target.width(); x < target.pitch(); x++)
            row[x] = 0xee;
    }
}

static void poisonSourcePadding(FrameRenderTarget& target) {
    for (int y = 0; y < target.height(); y++) {
        unsigned char* row = target.sourceRow(y);
        for (int x = target.width(); x < target.pitch(); x++)
            row[x] = 0xee;
    }
}

static void assertVisibleMatches(const FrameRenderTarget& expected,
    const FrameRenderTarget& actual) {
    assert(expected.width() == actual.width());
    assert(expected.height() == actual.height());

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

static void assertSourceVisibleStreamUnchanged(const FrameRenderTarget& target) {
    int hidden = target.hiddenBorderRows();
    int first = -hidden * target.width();
    int last = target.size() + hidden * target.width();

    for (int offset = first; offset < last; offset++) {
        int storage = target.visibleLinearOffset(offset);
        assert(target.sourcePixels()[storage] == patternValue(offset, 9));
    }
}

static void assertSourcePaddingUntouched(const FrameRenderTarget& target) {
    for (int y = 0; y < target.height(); y++) {
        const unsigned char* row = target.sourceRow(y);
        for (int x = target.width(); x < target.pitch(); x++)
            assert(row[x] == 0xee);
    }
}

typedef void (*FlameKernel)(FrameStageBuffer& buffer,
    const FrameGeneratorContext& context, FlameRuntime& runtime);

static void runKernelOnPackedAndPaddedStores(FlameKernel kernel) {
    FrameStore packedStore;
    FrameStore paddedStore;
    packedStore.resize(FrameStorageLayout(PixelSize(7, 5), 7, 3));
    paddedStore.resize(FrameStorageLayout(PixelSize(7, 5), 11, 3));

    FrameRenderTarget& packedTarget = packedStore.renderTarget();
    FrameRenderTarget& paddedTarget = paddedStore.renderTarget();
    fillVisibleStream(packedTarget);
    fillVisibleStream(paddedTarget);
    copySourceVisibleStreamToDestination(packedTarget);
    copySourceVisibleStreamToDestination(paddedTarget);
    poisonDestinationPadding(paddedTarget);
    poisonSourcePadding(paddedTarget);
    FrameStageBuffer packed(packedTarget);
    FrameStageBuffer padded(paddedTarget);

    FrameGeneratorContext context;
    FlameLookupTables tables;
    FlameRuntime runtime(0, tables);

    kernel(packed, context, runtime);
    kernel(padded, context, runtime);

    assertVisibleMatches(packedTarget, paddedTarget);
    assertDestinationPaddingUntouched(paddedTarget);
    assertSourceVisibleStreamUnchanged(packedTarget);
    assertSourceVisibleStreamUnchanged(paddedTarget);
    assertSourcePaddingUntouched(paddedTarget);
}

static void testFlameClearSkipsPadding() {
    FrameStore store;
    store.resize(FrameStorageLayout(PixelSize(7, 5), 11, 3));
    FrameRenderTarget& target = store.renderTarget();
    fillVisibleStream(target);
    copySourceVisibleStreamToDestination(target);
    poisonDestinationPadding(target);
    FrameStageBuffer stageBuffer(target);

    FrameGeneratorContext context;
    FlameLookupTables tables;
    FlameRuntime runtime(0, tables);
    flame_clear(stageBuffer, context, runtime);

    for (int y = 0; y < target.height(); y++) {
        const unsigned char* row = target.destinationRow(y);
        for (int x = 0; x < target.width(); x++)
            assert(row[x] == 0);
    }
    assertDestinationPaddingUntouched(target);
}

static void testFlameKernelsUseVisibleStreamNotPadding() {
    static FlameKernel kernels[] = {
        flame_down,
        flame_upslow,
        flame_upsubtle,
        flame_upfast,
        flame_leftslow,
        flame_leftsubtle,
        flame_leftfast,
        flame_rightslow,
        flame_rightsubtle,
        flame_rightfast,
        flame_water,
        flame_watersubtle,
        flame_skyline,
        flame_weird,
        flame_zzz,
        flame_fade,
        flame_general_subtle,
        flame_general_slow,
    };

    for (unsigned int i = 0; i < sizeof(kernels) / sizeof(kernels[0]); i++)
        runKernelOnPackedAndPaddedStores(kernels[i]);
}

static void assertKernelDoesNotMutateSource(FlameKernel kernel) {
    FrameStore store;
    store.resize(FrameStorageLayout(PixelSize(7, 5), 7, 3));
    FrameRenderTarget& target = store.renderTarget();
    fillVisibleStream(target);
    copySourceVisibleStreamToDestination(target);
    FrameStageBuffer stageBuffer(target);

    FrameGeneratorContext context;
    FlameLookupTables tables;
    FlameRuntime runtime(0, tables);
    kernel(stageBuffer, context, runtime);

    assertSourceVisibleStreamUnchanged(target);
}

static void testFormerSwapStyleFlamesDoNotMutateSource() {
    assertKernelDoesNotMutateSource(flame_upslow);
    assertKernelDoesNotMutateSource(flame_fade);
}

int main() {
    testFlameClearSkipsPadding();
    testFlameKernelsUseVisibleStreamNotPadding();
    testFormerSwapStyleFlamesDoNotMutateSource();
    return 0;
}
