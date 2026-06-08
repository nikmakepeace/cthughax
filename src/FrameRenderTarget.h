// Mutable indexed render target for Frame Generator filters.

#ifndef __FRAME_RENDER_TARGET_H
#define __FRAME_RENDER_TARGET_H

#include "cthugha.h"
#include "FrameStorageLayout.h"

/**
 * Mutable active/passive indexed pixel target for one Frame Generator store.
 *
 * This is the narrow render-target surface used by legacy filter, flame,
 * translate, wave, border, image, text, and frame-publication code while
 * storage ownership lives in FrameStore. It has no process-wide aliases.
 */
class FrameRenderTarget {
    friend class FrameStore;

public:
    /** Creates a target with historical default dimensions and no pixels. */
    FrameRenderTarget();

    /** Releases active/passive pixel allocations. */
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

    /** Swaps active and passive allocations. */
    void swapBuffers();

    /**
     * Clears active and passive visible plus hidden pixel storage.
     */
    void clear();

    /** @return Mutable first visible pixel of the active buffer. */
    unsigned char* activePixels();

    /**
     * @param y Visible row index.
     * @return Mutable active row start for y.
     */
    unsigned char* activeRow(int y);

    /** @return Mutable first visible pixel of the passive buffer. */
    unsigned char* passivePixels();

    /**
     * @param y Visible row index.
     * @return Mutable passive row start for y.
     */
    unsigned char* passiveRow(int y);

    /** @return Immutable first visible pixel of the active buffer. */
    const unsigned char* activePixels() const;

    /**
     * @param y Visible row index.
     * @return Immutable active row start for y.
     */
    const unsigned char* activeRow(int y) const;

    /** @return Immutable first visible pixel of the passive buffer. */
    const unsigned char* passivePixels() const;

    /**
     * @param y Visible row index.
     * @return Immutable passive row start for y.
     */
    const unsigned char* passiveRow(int y) const;

    /**
     * @return First hidden row above the active visible image.
     */
    unsigned char* activeTopHiddenRows();

    /**
     * @return First hidden row below the active visible image.
     */
    unsigned char* activeBottomHiddenRows();

    /** @return Nonzero when visible rows have no padding. */
    int visibleRowsArePacked() const;

    /**
     * Maps a visible x/y coordinate to a byte offset from activePixels().
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
     * @return Storage offset from activePixels()/passivePixels().
     */
    int visibleLinearOffset(int linearOffset) const;

private:
    FrameRenderTarget(const FrameRenderTarget&) = delete;
    FrameRenderTarget& operator=(const FrameRenderTarget&) = delete;

    unsigned char* activeAllocation;
    unsigned char* passiveAllocation;
    unsigned char* activeBuffer; /* visible pixels next on screen */
    unsigned char* passiveBuffer; /* visible pixels current on screen */
    FrameStorageLayout layoutValue;

    int allocationByteCount() const;
    unsigned char* visiblePixels(unsigned char* allocation) const;

    /**
     * Allocates active/passive pixel memory for the current dimensions.
     *
     * Must run after option parsing has applied the final buffer width/height.
     * The allocation includes hidden rows above and below the visible pixels so
     * flame feedback can sample outside the displayed image.
     */
    void allocatePixels();

};

#endif
