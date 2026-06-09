// Owned indexed-image payload used by Scene and Frame Generator.

#ifndef CTHUGHA_INDEXED_IMAGE_H
#define CTHUGHA_INDEXED_IMAGE_H

#include "ColorPalette.h"

/**
 * Target buffer dimensions for image loaders.
 */
struct ImageLoadTarget {
    /** Target display/buffer width in pixels. */
    int width;

    /** Target display/buffer height in pixels. */
    int height;

    /**
     * @param width_ Target width in pixels.
     * @param height_ Target height in pixels.
     */
    ImageLoadTarget(int width_, int height_)
        : width(width_)
        , height(height_) { }
};

/**
 * Owned 8-bit indexed image.
 */
class IndexedImage {
    char* nameValue;
    unsigned char* pixelsValue;
    ColorPalette* paletteValue;
    int widthValue;
    int heightValue;

    IndexedImage(const IndexedImage&);
    IndexedImage& operator=(const IndexedImage&);

public:
    /**
     * Allocates an indexed image.
     *
     * @param name Image display/option name.
     * @param width Width in pixels.
     * @param height Height in pixels.
     * @param palette Owned source palette for these indices, or NULL.
     */
    IndexedImage(const char* name, int width, int height,
        ColorPalette* palette = 0);
    ~IndexedImage();

    /** @return Image display/option name. */
    const char* name() const;

    /** @return Width in pixels. */
    int width() const;

    /** @return Height in pixels. */
    int height() const;

    /** @return Number of indexed pixels, width * height. */
    int size() const;

    /** @return Read-only 8-bit palette-index pixels. */
    const unsigned char* pixels() const;

    /** @return Mutable 8-bit palette-index pixels for loaders. */
    unsigned char* mutablePixels();

    /**
     * Replaces the image source palette.
     *
     * @param palette Owned 256-color RGB palette for these indices, or NULL.
     */
    void setPalette(ColorPalette* palette);

    /** @return Source palette owned by this indexed image, or NULL. */
    const ColorPalette* palette() const;
};

#endif
