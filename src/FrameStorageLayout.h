// Frame Generator indexed-pixel storage layout.

#ifndef CTHUGHA_FRAME_STORAGE_LAYOUT_H
#define CTHUGHA_FRAME_STORAGE_LAYOUT_H

#include "DisplayGeometry.h"

/**
 * Describes the byte layout of one source or destination indexed frame
 * allocation.
 *
 * FrameGeometry describes the logical visible frame. FrameStorageLayout is the
 * storage policy owned by FrameStore: row pitch, hidden rows, allocation size,
 * and offsets from the allocation base to the visible image.
 */
class FrameStorageLayout {
    PixelSize visibleSizeValue;
    int rowPitchValue;
    int topHiddenRowsValue;
    int bottomHiddenRowsValue;

public:
    /** Creates the historical 160x100, tightly packed, three-border-row layout. */
    FrameStorageLayout()
        : visibleSizeValue(160, 100)
        , rowPitchValue(160)
        , topHiddenRowsValue(3)
        , bottomHiddenRowsValue(3) { }

    /**
     * Creates an explicit indexed-frame storage layout.
     *
     * @param visibleSize Visible indexed-frame dimensions.
     * @param rowPitch Bytes between adjacent row starts. Values narrower than
     *        visible width are clamped to visible width.
     * @param topHiddenRows Hidden rows stored before the visible image.
     * @param bottomHiddenRows Hidden rows stored after the visible image.
     */
    FrameStorageLayout(const PixelSize& visibleSize, int rowPitch,
        int topHiddenRows, int bottomHiddenRows)
        : visibleSizeValue(visibleSize)
        , rowPitchValue(rowPitch < visibleSize.width ? visibleSize.width : rowPitch)
        , topHiddenRowsValue(topHiddenRows < 0 ? 0 : topHiddenRows)
        , bottomHiddenRowsValue(bottomHiddenRows < 0 ? 0 : bottomHiddenRows) { }

    /**
     * Creates a layout with the same hidden-row count above and below.
     *
     * @param visibleSize Visible indexed-frame dimensions.
     * @param rowPitch Bytes between adjacent row starts.
     * @param hiddenRows Hidden rows stored on each vertical side.
     */
    FrameStorageLayout(const PixelSize& visibleSize, int rowPitch, int hiddenRows)
        : FrameStorageLayout(visibleSize, rowPitch, hiddenRows, hiddenRows) { }

    /** @return Visible indexed-frame dimensions. */
    PixelSize visibleSize() const { return visibleSizeValue; }

    /** @return Visible frame width in pixels. */
    int width() const { return visibleSizeValue.width; }

    /** @return Visible frame height in pixels. */
    int height() const { return visibleSizeValue.height; }

    /** @return Bytes between adjacent row starts. */
    int pitch() const { return rowPitchValue; }

    /** @return Hidden rows stored above the visible image. */
    int topHiddenRows() const { return topHiddenRowsValue; }

    /** @return Hidden rows stored below the visible image. */
    int bottomHiddenRows() const { return bottomHiddenRowsValue; }

    /** @return Nonzero when the layout can address visible pixels. */
    int valid() const {
        return width() > 0 && height() > 0 && pitch() >= width();
    }

    /** @return Visible pixel count, excluding row padding. */
    int visiblePixelCount() const { return width() * height(); }

    /** @return Bytes occupied by visible rows, including row padding. */
    int visibleStorageByteCount() const { return pitch() * height(); }

    /** @return Bytes occupied by top hidden rows, including row padding. */
    int topHiddenByteCount() const { return pitch() * topHiddenRows(); }

    /** @return Bytes occupied by bottom hidden rows, including row padding. */
    int bottomHiddenByteCount() const { return pitch() * bottomHiddenRows(); }

    /** @return Offset from allocation base to first visible pixel. */
    int visibleByteOffset() const { return topHiddenByteCount(); }

    /** @return Offset from allocation base to first bottom hidden-row byte. */
    int bottomHiddenByteOffset() const {
        return visibleByteOffset() + visibleStorageByteCount();
    }

    /** @return Total source/destination allocation size in bytes. */
    int allocationByteCount() const {
        return topHiddenByteCount() + visibleStorageByteCount()
            + bottomHiddenByteCount();
    }

    /**
     * Maps a visible x/y coordinate to a byte offset from the first visible row.
     *
     * @param x Visible x coordinate.
     * @param y Visible y coordinate.
     * @return Storage offset from the visible image start.
     */
    int visibleOffset(int x, int y) const { return y * pitch() + x; }

    /**
     * Maps a packed visible-stream offset to pitched storage.
     *
     * Offsets may be negative or beyond the visible pixel count when a renderer
     * intentionally samples hidden rows. Row padding is skipped; only visible
     * bytes within each row participate in the packed stream.
     *
     * @param linearOffset Offset in a width-packed visible/hidden row stream.
     * @return Storage offset from the visible image start.
     */
    int visibleLinearOffset(int linearOffset) const {
        if (rowPitchValue == visibleSizeValue.width)
            return linearOffset;

        int row = linearOffset / width();
        int x = linearOffset % width();
        if (x < 0) {
            x += width();
            row--;
        }
        return visibleOffset(x, row);
    }
};

#endif
