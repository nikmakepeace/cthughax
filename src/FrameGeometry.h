// Frame Generator geometry value and Scene geometry port.

#ifndef CTHUGHA_FRAME_GEOMETRY_H
#define CTHUGHA_FRAME_GEOMETRY_H

#include "DisplayGeometry.h"
#include "SceneGeometry.h"

/**
 * Logical indexed-frame geometry owned by the Frame Generator module.
 *
 * FrameGeometry deliberately describes only visible dimensions and values
 * derived from those dimensions. Pixel memory, row pitch, padding, and hidden
 * border rows live in FrameStore/FrameStorageLayout.
 */
class FrameGeometry : public SceneGeometry {
    PixelSize sizeValue;

public:
    /** Creates the historical default 160x100 frame geometry. */
    FrameGeometry();

    /**
     * Creates explicit frame geometry.
     *
     * @param size Visible indexed-frame dimensions.
     */
    explicit FrameGeometry(const PixelSize& size);

    /** @return Visible indexed-frame dimensions. */
    PixelSize size() const;

    /** @return Visible frame width in pixels. */
    virtual int width() const;

    /** @return Visible frame height in pixels. */
    virtual int height() const;

    /** @return Larger of width() and height(). */
    int maxDimension() const;

    /** @return Nonzero when the geometry can address visible pixels. */
    int valid() const;
};

#endif
