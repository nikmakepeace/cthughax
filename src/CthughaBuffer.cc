#include "cthugha.h"
#include "CthughaBuffer.h"
#include "imath.h"

static const int hiddenBorderRowsPerSide = 3; // Unit: rows; used above and below visible pixels by BorderFilter and flame feedback.

CthughaBuffer CthughaBuffer::buffer;

CthughaBuffer* CthughaBuffer::current = &CthughaBuffer::buffer;

CthughaBuffer::CthughaBuffer()
    : activeAllocation(0)
    , passiveAllocation(0)
    , activeBuffer(0)
    , passiveBuffer(0)
    , widthValue(160)
    , heightValue(100) { }

CthughaBuffer::~CthughaBuffer() {
    delete[] activeAllocation;
    delete[] passiveAllocation;
}

int CthughaBuffer::width() const {
    return widthValue;
}

int CthughaBuffer::height() const {
    return heightValue;
}

int CthughaBuffer::size() const {
    return widthValue * heightValue;
}

int CthughaBuffer::bottom() const {
    return heightValue - 1;
}

int CthughaBuffer::maxDimension() const {
    return max(widthValue, heightValue);
}

int CthughaBuffer::hiddenBorderRows() const {
    return hiddenBorderRowsPerSide;
}

int CthughaBuffer::hiddenBorderByteCount() const {
    return hiddenBorderRows() * width();
}

void CthughaBuffer::setDimensions(int width_, int height_) {
    widthValue = width_;
    heightValue = height_;
}

int CthughaBuffer::allocationByteCount() const {
    return size() + 2 * hiddenBorderByteCount();
}

unsigned char* CthughaBuffer::visiblePixels(unsigned char* allocation) const {
    return allocation == 0 ? 0 : allocation + hiddenBorderByteCount();
}

void CthughaBuffer::allocatePixels() {

    delete[] activeAllocation;
    delete[] passiveAllocation;

    activeAllocation = new unsigned char[allocationByteCount()];
    passiveAllocation = new unsigned char[allocationByteCount()];
    activeBuffer = visiblePixels(activeAllocation);
    passiveBuffer = visiblePixels(passiveAllocation);

    memset(activeAllocation, 0, allocationByteCount());
    memset(passiveAllocation, 0, allocationByteCount());
}

void CthughaBuffer::swapBuffers() {
    unsigned char* allocation = activeAllocation;
    activeAllocation = passiveAllocation;
    passiveAllocation = allocation;

    unsigned char* t = activeBuffer;
    activeBuffer = passiveBuffer;
    passiveBuffer = t;
}

void CthughaBuffer::clear() {
    if (activeAllocation != 0)
        memset(activeAllocation, 0, allocationByteCount());
    if (passiveAllocation != 0)
        memset(passiveAllocation, 0, allocationByteCount());
}

unsigned char* CthughaBuffer::activePixels() {
    return activeBuffer;
}

unsigned char* CthughaBuffer::passivePixels() {
    return passiveBuffer;
}

const unsigned char* CthughaBuffer::activePixels() const {
    return activeBuffer;
}

const unsigned char* CthughaBuffer::passivePixels() const {
    return passiveBuffer;
}

unsigned char* CthughaBuffer::activeTopHiddenRows() {
    return activeBuffer == 0 ? 0 : activeBuffer - hiddenBorderByteCount();
}

unsigned char* CthughaBuffer::activeBottomHiddenRows() {
    return activeBuffer == 0 ? 0 : activeBuffer + size();
}
