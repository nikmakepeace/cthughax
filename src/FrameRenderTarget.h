// Mutable indexed render target for Frame Generator filters.

#ifndef __FRAME_RENDER_TARGET_H
#define __FRAME_RENDER_TARGET_H

#include "FrameStorageLayout.h"

/**
 * Mutable source/destination indexed pixel target for one Frame Generator
 * store.
 *
 * Source is the current frame available for stages to read. Destination is
 * the scratch/write frame the framework gives to the current stage.
 * Storage ownership lives in FrameStore. It has no process-wide aliases.
 */
class FrameRenderTarget {
    friend class FrameStore;
    friend class FrameFilterchain;

public:
    /** Creates a target with historical default dimensions and no pixels. */
    FrameRenderTarget();

    /** Releases source/destination pixel allocations. */
    ~FrameRenderTarget();

    /** @return Visible frame width in indexed pixels. */
    int width() const;

    /** @return Visible frame height in indexed pixels. */
    int height() const;

    /** @return Bytes between adjacent row starts. */
    int pitch() const;

    /** @return Visible frame pixel count. */
    int size() const;

    /** @return Visible-row storage bytes, including row padding. */
    int visibleStorageByteCount() const;

    /** @return Last visible y coordinate. */
    int bottom() const;

    /** @return Larger of width() and height(). */
    int maxDimension() const;

    /**
     * @return Hidden border height on each vertical side, in pixel rows.
     */
    int hiddenBorderRows() const;

    /**
     * @return Hidden border storage on each vertical side, in bytes.
     */
    int hiddenBorderByteCount() const;

    /**
     * Sets visible dimensions before FrameStore allocates storage.
     *
     * @param width_ Visible width in indexed pixels.
     * @param height_ Visible height in indexed pixels.
     */
    void setDimensions(int width_, int height_);

    /**
     * Sets the storage layout before FrameStore allocates pixels.
     *
     * @param layout Visible size, pitch, hidden rows, and allocation offsets.
     */
    void setLayout(const FrameStorageLayout& layout);

    /**
     * Clears source and destination visible plus hidden pixel storage.
     */
    void clear();

    /** @return Mutable first visible pixel of the destination buffer. */
    unsigned char* destinationPixels();

    /**
     * @param y Visible row index.
     * @return Mutable destination row start for y.
     */
    unsigned char* destinationRow(int y);

    /** @return Mutable first visible pixel of the source buffer. */
    unsigned char* sourcePixels();

    /**
     * @param y Visible row index.
     * @return Mutable source row start for y.
     */
    unsigned char* sourceRow(int y);

    /** @return Immutable first visible pixel of the destination buffer. */
    const unsigned char* destinationPixels() const;

    /**
     * @param y Visible row index.
     * @return Immutable destination row start for y.
     */
    const unsigned char* destinationRow(int y) const;

    /** @return Immutable first visible pixel of the source buffer. */
    const unsigned char* sourcePixels() const;

    /**
     * @param y Visible row index.
     * @return Immutable source row start for y.
     */
    const unsigned char* sourceRow(int y) const;

    /**
     * @return First hidden row above the destination visible image.
     */
    unsigned char* destinationTopHiddenRows();

    /**
     * @return First hidden row below the destination visible image.
     */
    unsigned char* destinationBottomHiddenRows();

    /** @return Nonzero when visible rows have no padding. */
    int visibleRowsArePacked() const;

    /**
     * Maps a visible x/y coordinate to a byte offset from a visible frame.
     *
     * @param x Visible x coordinate.
     * @param y Visible y coordinate.
     * @return Storage offset from the visible image start.
     */
    int visibleOffset(int x, int y) const;

    /**
     * Maps a packed visible-stream offset to a pitched storage offset.
     *
     * @param linearOffset Offset in a width-packed visible/hidden row stream.
     * @return Storage offset from destinationPixels()/sourcePixels().
     */
    int visibleLinearOffset(int linearOffset) const;

private:
    FrameRenderTarget(const FrameRenderTarget&) = delete;
    FrameRenderTarget& operator=(const FrameRenderTarget&) = delete;

    unsigned char* destinationAllocation;
    unsigned char* sourceAllocation;
    unsigned char* destinationBuffer;
    unsigned char* sourceBuffer;
    FrameStorageLayout layoutValue;

    int allocationByteCount() const;
    unsigned char* visiblePixels(unsigned char* allocation) const;

    /** Swaps source and destination allocations. */
    void swapSourceAndDestination();

    /** Copies the full source allocation into destination. */
    void copySourceToDestination();

    /**
     * Allocates source/destination pixel memory for the current dimensions.
     *
     * Must run after option parsing has applied the final buffer width/height.
     * The allocation includes hidden rows above and below the visible pixels so
     * flame feedback can sample outside the displayed image.
     */
    void allocatePixels();

};

#endif
