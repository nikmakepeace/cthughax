#include "FrameStore.h"

#include <assert.h>
#include <string.h>

static void testFrameGeometryReportsDerivedValues() {
    FrameGeometry geometry(PixelSize(8, 5), 4);

    assert(geometry.valid());
    assert(geometry.size() == PixelSize(8, 5));
    assert(geometry.width() == 8);
    assert(geometry.height() == 5);
    assert(geometry.hiddenBorderRows() == 4);
    assert(geometry.hiddenBorderByteCount() == 32);
    assert(geometry.maxDimension() == 8);
}

static void testFrameStoreResizesAndExposesBufferViews() {
    FrameStore store;
    FrameGeometry geometry(PixelSize(7, 3), 3);

    store.resize(geometry);

    FrameBufferView active = store.active();
    FrameBufferView passive = store.passive();
    assert(active.valid());
    assert(passive.valid());
    assert(active.width() == 7);
    assert(active.height() == 3);
    assert(active.pitch() == 7);
    assert(passive.width() == 7);
    assert(passive.height() == 3);
    assert(store.geometry().hiddenBorderByteCount() == 21);
    assert(store.activeTopHiddenRows() == active.pixels() - 21);
    assert(store.activeBottomHiddenRows() == active.pixels() + 21);
}

static void testFrameStoreSwapsActiveAndPassiveBuffers() {
    FrameStore store;
    store.resize(FrameGeometry(PixelSize(4, 2)));

    unsigned char* active = store.active().pixels();
    unsigned char* passive = store.passive().pixels();
    active[0] = 11;
    passive[0] = 29;

    store.swapActivePassive();

    assert(store.active().pixels() == passive);
    assert(store.passive().pixels() == active);
    assert(store.active().pixels()[0] == 29);
    assert(store.passive().pixels()[0] == 11);
}

static void testFrameStoreClearsVisibleAndHiddenStorage() {
    FrameStore store;
    FrameGeometry geometry(PixelSize(5, 2), 2);
    store.resize(geometry);

    int allocationBytes = geometry.width() * geometry.height()
        + 2 * geometry.hiddenBorderByteCount();
    memset(store.activeTopHiddenRows(), 0x7f, allocationBytes);
    memset(store.compatibilityBuffer().passivePixels(), 0x55,
        geometry.width() * geometry.height());

    store.clear();

    const unsigned char* activeAllocation = store.activeTopHiddenRows();
    for (int i = 0; i < allocationBytes; ++i)
        assert(activeAllocation[i] == 0);

    const unsigned char* passive = store.compatibilityBuffer().passivePixels();
    for (int i = 0; i < geometry.width() * geometry.height(); ++i)
        assert(passive[i] == 0);
}

int main() {
    testFrameGeometryReportsDerivedValues();
    testFrameStoreResizesAndExposesBufferViews();
    testFrameStoreSwapsActiveAndPassiveBuffers();
    testFrameStoreClearsVisibleAndHiddenStorage();
    return 0;
}
