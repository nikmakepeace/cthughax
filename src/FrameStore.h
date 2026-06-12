// Owned indexed frame storage for the Frame Generator module.

#ifndef CTHUGHA_FRAME_STORE_H
#define CTHUGHA_FRAME_STORE_H

#include "FrameRenderTarget.h"
#include "FrameGeometry.h"

/**
 * Borrowed view of one visible indexed pixel buffer.
 */
class FrameBufferView {
    unsigned char* pixelsValue;
    PixelSize sizeValue;
    int pitchValue;

public:
    /** Creates an empty invalid view. */
    FrameBufferView();

    /**
     * Creates a visible indexed-buffer view.
     *
     * @param pixels First visible pixel, or NULL.
     * @param size Visible dimensions.
     * @param pitch Bytes between row starts.
     */
    FrameBufferView(unsigned char* pixels, const PixelSize& size, int pitch);

    /** @return First visible pixel, or NULL. */
    unsigned char* pixels() const;

    /** @return Visible dimensions. */
    PixelSize size() const;

    /** @return Visible frame width in pixels. */
    int width() const;

    /** @return Visible frame height in pixels. */
    int height() const;

    /** @return Bytes between row starts. */
    int pitch() const;

    /** @return Nonzero when pixels and dimensions are usable. */
    int valid() const;
};

/**
 * Owns source/destination indexed frame storage for Frame Generator rendering.
 *
 * The store owns the render target passed to filter, flame, translate, wave,
 * border, image, text, and frame-publication stages. Pixel storage is never
 * reached through process-wide aliases.
 */
class FrameStore {
    FrameGeometry geometryValue;
    FrameStorageLayout layoutValue;
    FrameRenderTarget bufferValue;

public:
    /** Creates storage with the default geometry but no allocated pixels. */
    FrameStore();

    /**
     * Resizes owned source/destination storage to a geometry.
     *
     * @param geometry New frame geometry.
     */
    void resize(const FrameGeometry& geometry);

    /**
     * Resizes owned source/destination storage to an explicit layout.
     *
     * @param layout New storage layout, including pitch and hidden rows.
     */
    void resize(const FrameStorageLayout& layout);

    /** @return Current frame geometry. */
    const FrameGeometry& geometry() const;

    /** @return Current source/destination storage layout. */
    const FrameStorageLayout& layout() const;

    /** @return Mutable visible destination buffer view. */
    FrameBufferView destination();

    /** @return Mutable visible source buffer view. */
    FrameBufferView source();

    /** Swaps source and destination buffers. */
    void swapSourceAndDestination();

    /** Clears source and destination visible plus hidden storage. */
    void clear();

    /** @return First hidden row above the destination visible image. */
    unsigned char* destinationTopHiddenRows();

    /** @return First hidden row below the destination visible image. */
    unsigned char* destinationBottomHiddenRows();

    /** @return Mutable render target used by generator stages. */
    FrameRenderTarget& renderTarget();

    /** @return Immutable render target used by generator stages. */
    const FrameRenderTarget& renderTarget() const;
};

#endif
