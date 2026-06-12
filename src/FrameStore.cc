// Owned indexed frame storage for the Frame Generator module.

#include "FrameStore.h"

static const int defaultHiddenBorderRows = 3;

FrameBufferView::FrameBufferView()
    : pixelsValue(0)
    , sizeValue()
    , pitchValue(0) { }

FrameBufferView::FrameBufferView(unsigned char* pixels, const PixelSize& size,
    int pitch)
    : pixelsValue(pixels)
    , sizeValue(size)
    , pitchValue(pitch) { }

unsigned char* FrameBufferView::pixels() const {
    return pixelsValue;
}

PixelSize FrameBufferView::size() const {
    return sizeValue;
}

int FrameBufferView::width() const {
    return sizeValue.width;
}

int FrameBufferView::height() const {
    return sizeValue.height;
}

int FrameBufferView::pitch() const {
    return pitchValue;
}

int FrameBufferView::valid() const {
    return pixelsValue != 0 && sizeValue.valid() && pitchValue >= sizeValue.width;
}

FrameStore::FrameStore()
    : geometryValue()
    , layoutValue(geometryValue.size(), geometryValue.width(),
          defaultHiddenBorderRows)
    , bufferValue() { }

void FrameStore::resize(const FrameGeometry& geometry) {
    resize(FrameStorageLayout(geometry.size(), geometry.width(),
        defaultHiddenBorderRows));
}

void FrameStore::resize(const FrameStorageLayout& layout) {
    layoutValue = layout;
    geometryValue = FrameGeometry(layout.visibleSize());
    bufferValue.setLayout(layoutValue);
    bufferValue.allocatePixels();
}

const FrameGeometry& FrameStore::geometry() const {
    return geometryValue;
}

const FrameStorageLayout& FrameStore::layout() const {
    return layoutValue;
}

FrameBufferView FrameStore::destination() {
    return FrameBufferView(bufferValue.destinationPixels(), geometryValue.size(),
        layoutValue.pitch());
}

FrameBufferView FrameStore::source() {
    return FrameBufferView(bufferValue.sourcePixels(), geometryValue.size(),
        layoutValue.pitch());
}

void FrameStore::swapSourceAndDestination() {
    bufferValue.swapSourceAndDestination();
}

void FrameStore::clear() {
    bufferValue.clear();
}

unsigned char* FrameStore::destinationTopHiddenRows() {
    return bufferValue.destinationTopHiddenRows();
}

unsigned char* FrameStore::destinationBottomHiddenRows() {
    return bufferValue.destinationBottomHiddenRows();
}

FrameRenderTarget& FrameStore::renderTarget() {
    return bufferValue;
}

const FrameRenderTarget& FrameStore::renderTarget() const {
    return bufferValue;
}
