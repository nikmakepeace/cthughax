#include "IndexedDisplayFrame.h"

IndexedDisplayFrame::IndexedDisplayFrame()
    : pixelsValue(0)
    , widthValue(0)
    , heightValue(0)
    , pitchValue(0)
    , capacityByteCountValue(0)
    , framePaletteValue(0) {
}

IndexedDisplayFrame::~IndexedDisplayFrame() {
    delete[] pixelsValue;
}

void IndexedDisplayFrame::reset() {
    delete[] pixelsValue;
    pixelsValue = 0;
    widthValue = 0;
    heightValue = 0;
    pitchValue = 0;
    capacityByteCountValue = 0;
    framePaletteValue = 0;
}

void IndexedDisplayFrame::resize(int width, int height) {
    resize(width, height, width);
}

void IndexedDisplayFrame::resize(int width, int height, int pitch) {
    if (width <= 0 || height <= 0 || pitch < width) {
        reset();
        return;
    }

    int byteCount = pitch * height;
    if (byteCount > capacityByteCountValue) {
        delete[] pixelsValue;
        pixelsValue = new unsigned char[byteCount];
        capacityByteCountValue = byteCount;
    }

    widthValue = width;
    heightValue = height;
    pitchValue = pitch;
}

int IndexedDisplayFrame::valid() const {
    return pixelsValue != 0 && widthValue > 0 && heightValue > 0 && pitchValue >= widthValue;
}
