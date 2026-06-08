// Frame Generator geometry value.

#include "FrameGeometry.h"

#include "imath.h"

FrameGeometry::FrameGeometry()
    : sizeValue(160, 100) { }

FrameGeometry::FrameGeometry(const PixelSize& size)
    : sizeValue(size) { }

PixelSize FrameGeometry::size() const {
    return sizeValue;
}

int FrameGeometry::width() const {
    return sizeValue.width;
}

int FrameGeometry::height() const {
    return sizeValue.height;
}

int FrameGeometry::maxDimension() const {
    return max(width(), height());
}

int FrameGeometry::valid() const {
    return width() > 0 && height() > 0;
}
