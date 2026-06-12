#include "FrameRenderTarget.h"
#include "imath.h"

#include <string.h>

static const int hiddenBorderRowsPerSide = 3; // Unit: rows; used above and below visible pixels by BorderFilter and flame feedback.

FrameRenderTarget::FrameRenderTarget()
    : destinationAllocation(0)
    , sourceAllocation(0)
    , destinationBuffer(0)
    , sourceBuffer(0)
    , layoutValue() { }

FrameRenderTarget::~FrameRenderTarget() {
    delete[] destinationAllocation;
    delete[] sourceAllocation;
}

int FrameRenderTarget::width() const {
    return layoutValue.width();
}

int FrameRenderTarget::height() const {
    return layoutValue.height();
}

int FrameRenderTarget::pitch() const {
    return layoutValue.pitch();
}

int FrameRenderTarget::size() const {
    return layoutValue.visiblePixelCount();
}

int FrameRenderTarget::visibleStorageByteCount() const {
    return layoutValue.visibleStorageByteCount();
}

int FrameRenderTarget::bottom() const {
    return height() - 1;
}

int FrameRenderTarget::maxDimension() const {
    return max(width(), height());
}

int FrameRenderTarget::hiddenBorderRows() const {
    return layoutValue.topHiddenRows();
}

int FrameRenderTarget::hiddenBorderByteCount() const {
    return layoutValue.topHiddenByteCount();
}

void FrameRenderTarget::setDimensions(int width_, int height_) {
    setLayout(FrameStorageLayout(PixelSize(width_, height_), width_,
        hiddenBorderRowsPerSide));
}

void FrameRenderTarget::setLayout(const FrameStorageLayout& layout) {
    layoutValue = layout;
}

int FrameRenderTarget::allocationByteCount() const {
    return layoutValue.allocationByteCount();
}

unsigned char* FrameRenderTarget::visiblePixels(unsigned char* allocation) const {
    return allocation == 0 ? 0 : allocation + layoutValue.visibleByteOffset();
}

void FrameRenderTarget::allocatePixels() {

    delete[] destinationAllocation;
    delete[] sourceAllocation;

    destinationAllocation = new unsigned char[allocationByteCount()];
    sourceAllocation = new unsigned char[allocationByteCount()];
    destinationBuffer = visiblePixels(destinationAllocation);
    sourceBuffer = visiblePixels(sourceAllocation);

    memset(destinationAllocation, 0, allocationByteCount());
    memset(sourceAllocation, 0, allocationByteCount());
}

void FrameRenderTarget::swapSourceAndDestination() {
    unsigned char* allocation = destinationAllocation;
    destinationAllocation = sourceAllocation;
    sourceAllocation = allocation;

    unsigned char* t = destinationBuffer;
    destinationBuffer = sourceBuffer;
    sourceBuffer = t;
}

void FrameRenderTarget::copySourceToDestination() {
    if (destinationAllocation != 0 && sourceAllocation != 0)
        memcpy(destinationAllocation, sourceAllocation, allocationByteCount());
}

void FrameRenderTarget::clear() {
    if (destinationAllocation != 0)
        memset(destinationAllocation, 0, allocationByteCount());
    if (sourceAllocation != 0)
        memset(sourceAllocation, 0, allocationByteCount());
}

unsigned char* FrameRenderTarget::destinationPixels() {
    return destinationBuffer;
}

unsigned char* FrameRenderTarget::destinationRow(int y) {
    return destinationBuffer == 0 ? 0 : destinationBuffer + y * pitch();
}

unsigned char* FrameRenderTarget::sourcePixels() {
    return sourceBuffer;
}

unsigned char* FrameRenderTarget::sourceRow(int y) {
    return sourceBuffer == 0 ? 0 : sourceBuffer + y * pitch();
}

const unsigned char* FrameRenderTarget::destinationPixels() const {
    return destinationBuffer;
}

const unsigned char* FrameRenderTarget::destinationRow(int y) const {
    return destinationBuffer == 0 ? 0 : destinationBuffer + y * pitch();
}

const unsigned char* FrameRenderTarget::sourcePixels() const {
    return sourceBuffer;
}

const unsigned char* FrameRenderTarget::sourceRow(int y) const {
    return sourceBuffer == 0 ? 0 : sourceBuffer + y * pitch();
}

unsigned char* FrameRenderTarget::destinationTopHiddenRows() {
    return destinationAllocation;
}

unsigned char* FrameRenderTarget::destinationBottomHiddenRows() {
    return destinationAllocation == 0 ? 0 : destinationAllocation + layoutValue.bottomHiddenByteOffset();
}

int FrameRenderTarget::visibleRowsArePacked() const {
    return pitch() == width();
}

int FrameRenderTarget::visibleOffset(int x, int y) const {
    return layoutValue.visibleOffset(x, y);
}

int FrameRenderTarget::visibleLinearOffset(int linearOffset) const {
    return layoutValue.visibleLinearOffset(linearOffset);
}
