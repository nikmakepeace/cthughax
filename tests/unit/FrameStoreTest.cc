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

    FrameBufferView destination = store.destination();
    FrameBufferView source = store.source();
    assert(destination.valid());
    assert(source.valid());
    assert(destination.width() == 7);
    assert(destination.height() == 3);
    assert(destination.pitch() == 7);
    assert(source.width() == 7);
    assert(source.height() == 3);
    assert(store.layout().topHiddenByteCount() == 21);
    assert(store.destinationTopHiddenRows() == destination.pixels() - 21);
    assert(store.destinationBottomHiddenRows() == destination.pixels() + 21);
}

static void testFrameStoreOwnsPaddedStorageLayout() {
    FrameStore store;
    FrameStorageLayout layout(PixelSize(5, 3), 8, 2);

    store.resize(layout);

    FrameBufferView destination = store.destination();
    FrameBufferView source = store.source();
    FrameRenderTarget& target = store.renderTarget();

    assert(destination.valid());
    assert(source.valid());
    assert(store.layout().pitch() == 8);
    assert(store.layout().visibleStorageByteCount() == 24);
    assert(store.layout().topHiddenByteCount() == 16);
    assert(store.layout().bottomHiddenByteCount() == 16);
    assert(store.layout().allocationByteCount() == 56);
    assert(destination.width() == 5);
    assert(destination.height() == 3);
    assert(destination.pitch() == 8);
    assert(source.pitch() == 8);
    assert(target.width() == 5);
    assert(target.height() == 3);
    assert(target.pitch() == 8);
    assert(target.visibleStorageByteCount() == 24);
    assert(target.hiddenBorderByteCount() == 16);
    assert(!target.visibleRowsArePacked());
    assert(target.destinationTopHiddenRows() == destination.pixels() - 16);
    assert(target.destinationBottomHiddenRows() == destination.pixels() + 24);
    assert(target.destinationRow(2) == destination.pixels() + 16);
    assert(target.sourceRow(1) == source.pixels() + 8);
    assert(target.visibleOffset(4, 2) == 20);
    assert(target.visibleLinearOffset(-1) == -4);
    assert(target.visibleLinearOffset(-5) == -8);
    assert(target.visibleLinearOffset(15) == 24);
}

static void testFrameStoreSwapsSourceAndDestinationBuffers() {
    FrameStore store;
    store.resize(FrameGeometry(PixelSize(4, 2)));

    unsigned char* destination = store.destination().pixels();
    unsigned char* source = store.source().pixels();
    destination[0] = 11;
    source[0] = 29;

    store.swapSourceAndDestination();

    assert(store.destination().pixels() == source);
    assert(store.source().pixels() == destination);
    assert(store.destination().pixels()[0] == 29);
    assert(store.source().pixels()[0] == 11);
}

static void testFrameStoreClearsVisibleAndHiddenStorage() {
    FrameStore store;
    FrameStorageLayout layout(PixelSize(5, 2), 8, 2);
    store.resize(layout);

    int allocationBytes = layout.allocationByteCount();
    memset(store.destinationTopHiddenRows(), 0x7f, allocationBytes);
    memset(store.renderTarget().sourcePixels(), 0x55,
        layout.visibleStorageByteCount());

    store.clear();

    const unsigned char* destinationAllocation = store.destinationTopHiddenRows();
    for (int i = 0; i < allocationBytes; ++i)
        assert(destinationAllocation[i] == 0);

    const unsigned char* source = store.renderTarget().sourcePixels();
    for (int i = 0; i < layout.visibleStorageByteCount(); ++i)
        assert(source[i] == 0);
}

int main() {
    testFrameGeometryReportsDerivedValues();
    testFrameStoreResizesAndExposesBufferViews();
    testFrameStoreOwnsPaddedStorageLayout();
    testFrameStoreSwapsSourceAndDestinationBuffers();
    testFrameStoreClearsVisibleAndHiddenStorage();
    return 0;
}
