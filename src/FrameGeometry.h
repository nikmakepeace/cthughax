// Frame Generator geometry value and Scene geometry port.

#ifndef CTHUGHA_FRAME_GEOMETRY_H
#define CTHUGHA_FRAME_GEOMETRY_H

#include "DisplayGeometry.h"
#include "SceneGeometry.h"

struct DisplayConfig;

/**
 * Logical indexed-frame geometry owned by the Frame Generator module.
 *
 * FrameGeometry deliberately describes dimensions and derived storage sizes
 * only. Pixel memory lives in FrameStore, and Scene receives this object only
 * through the narrower SceneGeometry interface.
 */
class FrameGeometry : public SceneGeometry {
    PixelSize sizeValue;
    int hiddenBorderRowsValue;

public:
    /** Creates the historical default 160x100 frame geometry. */
    FrameGeometry();

    /**
     * Creates explicit frame geometry.
     *
     * @param size Visible indexed-frame dimensions.
     * @param hiddenBorderRows Hidden rows stored above and below the frame.
     */
    explicit FrameGeometry(const PixelSize& size, int hiddenBorderRows = 3);

    /**
     * Creates geometry from startup display configuration.
     *
     * @param config Startup display/buffer configuration.
     * @return Geometry using configured buffer width and height.
     */
    static FrameGeometry fromDisplayConfig(const DisplayConfig& config);

    /** @return Visible indexed-frame dimensions. */
    PixelSize size() const;

    /** @return Visible frame width in pixels. */
    virtual int width() const;

    /** @return Visible frame height in pixels. */
    virtual int height() const;

    /** @return Hidden border height on each vertical side, in pixel rows. */
    int hiddenBorderRows() const;

    /** @return Hidden border storage on each vertical side, in bytes. */
    int hiddenBorderByteCount() const;

    /** @return Larger of width() and height(). */
    int maxDimension() const;

    /** @return Nonzero when the geometry can address visible pixels. */
    int valid() const;
};

#endif
