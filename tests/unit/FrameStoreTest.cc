#include "FrameStore.h"

#include <assert.h>
#include <string.h>

static void testFrameGeometryReportsDerivedValues() {
    FrameGeometry geometry(PixelSize(8, 5));

    assert(geometry.valid());
    assert(geometry.size() == PixelSize(8, 5));
    assert(geometry.width() == 8);
    assert(geometry.height() == 5);
    assert(geometry.maxDimension() == 8);
}

static void testFrameStoreResizesAndExposesBufferViews() {
    FrameStore store;
    FrameGeometry geometry(PixelSize(7, 3));

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
    assert(store.layout().topHiddenByteCount() == 21);
    assert(store.activeTopHiddenRows() == active.pixels() - 21);
    assert(store.activeBottomHiddenRows() == active.pixels() + 21);
}

static void testFrameStoreOwnsPaddedStorageLayout() {
    FrameStore store;
    FrameStorageLayout layout(PixelSize(5, 3), 8, 2);

    store.resize(layout);

    FrameBufferView active = store.active();
    FrameBufferView passive = store.passive();
    FrameRenderTarget& target = store.renderTarget();

    assert(active.valid());
    assert(passive.valid());
    assert(store.layout().pitch() == 8);
    assert(store.layout().visibleStorageByteCount() == 24);
    assert(store.layout().topHiddenByteCount() == 16);
    assert(store.layout().bottomHiddenByteCount() == 16);
    assert(store.layout().allocationByteCount() == 56);
    assert(active.width() == 5);
    assert(active.height() == 3);
    assert(active.pitch() == 8);
    assert(passive.pitch() == 8);
    assert(target.width() == 5);
    assert(target.height() == 3);
    assert(target.pitch() == 8);
    assert(target.visibleStorageByteCount() == 24);
    assert(target.hiddenBorderByteCount() == 16);
    assert(!target.visibleRowsArePacked());
    assert(target.activeTopHiddenRows() == active.pixels() - 16);
    assert(target.activeBottomHiddenRows() == active.pixels() + 24);
    assert(target.activeRow(2) == active.pixels() + 16);
    assert(target.passiveRow(1) == passive.pixels() + 8);
    assert(target.visibleOffset(4, 2) == 20);
    assert(target.visibleLinearOffset(-1) == -4);
    assert(target.visibleLinearOffset(-5) == -8);
    assert(target.visibleLinearOffset(15) == 24);
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
    FrameStorageLayout layout(PixelSize(5, 2), 8, 2);
    store.resize(layout);

    int allocationBytes = layout.allocationByteCount();
    memset(store.activeTopHiddenRows(), 0x7f, allocationBytes);
    memset(store.renderTarget().passivePixels(), 0x55,
        layout.visibleStorageByteCount());

    store.clear();

    const unsigned char* activeAllocation = store.activeTopHiddenRows();
    for (int i = 0; i < allocationBytes; ++i)
        assert(activeAllocation[i] == 0);

    const unsigned char* passive = store.renderTarget().passivePixels();
    for (int i = 0; i < layout.visibleStorageByteCount(); ++i)
        assert(passive[i] == 0);
}

int main() {
    testFrameGeometryReportsDerivedValues();
    testFrameStoreResizesAndExposesBufferViews();
    testFrameStoreOwnsPaddedStorageLayout();
    testFrameStoreSwapsActiveAndPassiveBuffers();
    testFrameStoreClearsVisibleAndHiddenStorage();
    return 0;
}
