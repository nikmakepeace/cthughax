// Owned indexed frame storage for the Frame Generator module.

#include "FrameStore.h"

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
    , bufferValue() { }

void FrameStore::resize(const FrameGeometry& geometry) {
    geometryValue = geometry;
    bufferValue.setDimensions(geometry.width(), geometry.height());
    bufferValue.allocatePixels();
}

const FrameGeometry& FrameStore::geometry() const {
    return geometryValue;
}

FrameBufferView FrameStore::active() {
    return FrameBufferView(bufferValue.activePixels(), geometryValue.size(),
        geometryValue.width());
}

FrameBufferView FrameStore::passive() {
    return FrameBufferView(bufferValue.passivePixels(), geometryValue.size(),
        geometryValue.width());
}

void FrameStore::swapActivePassive() {
    bufferValue.swapBuffers();
}

void FrameStore::clear() {
    bufferValue.clear();
}

unsigned char* FrameStore::activeTopHiddenRows() {
    return bufferValue.activeTopHiddenRows();
}

unsigned char* FrameStore::activeBottomHiddenRows() {
    return bufferValue.activeBottomHiddenRows();
}

CthughaBuffer& FrameStore::compatibilityBuffer() {
    return bufferValue;
}

const CthughaBuffer& FrameStore::compatibilityBuffer() const {
    return bufferValue;
}
