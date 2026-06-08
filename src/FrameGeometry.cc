// Frame Generator geometry value.

#include "FrameGeometry.h"

#include "Configuration.h"
#include "imath.h"

FrameGeometry::FrameGeometry()
    : sizeValue(160, 100)
    , hiddenBorderRowsValue(3) { }

FrameGeometry::FrameGeometry(const PixelSize& size, int hiddenBorderRows)
    : sizeValue(size)
    , hiddenBorderRowsValue(hiddenBorderRows < 0 ? 0 : hiddenBorderRows) { }

FrameGeometry FrameGeometry::fromDisplayConfig(const DisplayConfig& config) {
    return FrameGeometry(PixelSize(config.bufferWidth, config.bufferHeight));
}

PixelSize FrameGeometry::size() const {
    return sizeValue;
}

int FrameGeometry::width() const {
    return sizeValue.width;
}

int FrameGeometry::height() const {
    return sizeValue.height;
}

int FrameGeometry::hiddenBorderRows() const {
    return hiddenBorderRowsValue;
}

int FrameGeometry::hiddenBorderByteCount() const {
    return hiddenBorderRows() * width();
}

int FrameGeometry::maxDimension() const {
    return max(width(), height());
}

int FrameGeometry::valid() const {
    return width() > 0 && height() > 0;
}
