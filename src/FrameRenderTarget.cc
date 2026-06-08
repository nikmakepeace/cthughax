#include "cthugha.h"
#include "FrameRenderTarget.h"
#include "imath.h"

static const int hiddenBorderRowsPerSide = 3; // Unit: rows; used above and below visible pixels by BorderFilter and flame feedback.

FrameRenderTarget::FrameRenderTarget()
    : activeAllocation(0)
    , passiveAllocation(0)
    , activeBuffer(0)
    , passiveBuffer(0)
    , layoutValue() { }

FrameRenderTarget::~FrameRenderTarget() {
    delete[] activeAllocation;
    delete[] passiveAllocation;
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

    delete[] activeAllocation;
    delete[] passiveAllocation;

    activeAllocation = new unsigned char[allocationByteCount()];
    passiveAllocation = new unsigned char[allocationByteCount()];
    activeBuffer = visiblePixels(activeAllocation);
    passiveBuffer = visiblePixels(passiveAllocation);

    memset(activeAllocation, 0, allocationByteCount());
    memset(passiveAllocation, 0, allocationByteCount());
}

void FrameRenderTarget::swapBuffers() {
    unsigned char* allocation = activeAllocation;
    activeAllocation = passiveAllocation;
    passiveAllocation = allocation;

    unsigned char* t = activeBuffer;
    activeBuffer = passiveBuffer;
    passiveBuffer = t;
}

void FrameRenderTarget::clear() {
    if (activeAllocation != 0)
        memset(activeAllocation, 0, allocationByteCount());
    if (passiveAllocation != 0)
        memset(passiveAllocation, 0, allocationByteCount());
}

unsigned char* FrameRenderTarget::activePixels() {
    return activeBuffer;
}

unsigned char* FrameRenderTarget::activeRow(int y) {
    return activeBuffer == 0 ? 0 : activeBuffer + y * pitch();
}

unsigned char* FrameRenderTarget::passivePixels() {
    return passiveBuffer;
}

unsigned char* FrameRenderTarget::passiveRow(int y) {
    return passiveBuffer == 0 ? 0 : passiveBuffer + y * pitch();
}

const unsigned char* FrameRenderTarget::activePixels() const {
    return activeBuffer;
}

const unsigned char* FrameRenderTarget::activeRow(int y) const {
    return activeBuffer == 0 ? 0 : activeBuffer + y * pitch();
}

const unsigned char* FrameRenderTarget::passivePixels() const {
    return passiveBuffer;
}

const unsigned char* FrameRenderTarget::passiveRow(int y) const {
    return passiveBuffer == 0 ? 0 : passiveBuffer + y * pitch();
}

unsigned char* FrameRenderTarget::activeTopHiddenRows() {
    return activeAllocation;
}

unsigned char* FrameRenderTarget::activeBottomHiddenRows() {
    return activeAllocation == 0 ? 0 : activeAllocation + layoutValue.bottomHiddenByteOffset();
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
