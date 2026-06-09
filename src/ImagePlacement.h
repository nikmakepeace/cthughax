// Indexed image placement strategy used by frame filters.

#ifndef CTHUGHA_IMAGE_PLACEMENT_H
#define CTHUGHA_IMAGE_PLACEMENT_H

class IndexedImage;
class RandomSource;

/**
 * Clipped source/destination rectangle for drawing an image into a buffer.
 *
 * left/top preserve the requested placement. The source/destination/size fields
 * describe the visible copy rectangle after clipping against the target buffer.
 */
class ImagePlacement {
public:
    /** Requested image left edge relative to the target buffer, in pixels. */
    int left;

    /** Requested image top edge relative to the target buffer, in pixels. */
    int top;

    /** Source x coordinate inside the image, in pixels. */
    int sourceX;

    /** Source y coordinate inside the image, in pixels. */
    int sourceY;

    /** Destination x coordinate inside the buffer, in pixels. */
    int destinationX;

    /** Destination y coordinate inside the buffer, in pixels. */
    int destinationY;

    /** Visible copy width in pixels. */
    int width;

    /** Visible copy height in pixels. */
    int height;

    ImagePlacement();

    /**
     * Computes the clipped visible rectangle.
     *
     * @param left_ Requested image left edge relative to the buffer, in pixels.
     * @param top_ Requested image top edge relative to the buffer, in pixels.
     * @param imageWidth Source image width in pixels.
     * @param imageHeight Source image height in pixels.
     * @param bufferWidth Target buffer width in pixels.
     * @param bufferHeight Target buffer height in pixels.
     */
    ImagePlacement(int left_, int top_, int imageWidth, int imageHeight,
        int bufferWidth, int bufferHeight);

    /** @return Nonzero when width and height describe visible pixels. */
    int visible() const;
};

/**
 * Chooses where an image should be drawn in a target buffer.
 */
class ImagePlacementStrategy {
public:
    virtual ~ImagePlacementStrategy();

    /**
     * @param image Image to place.
     * @param bufferWidth Target buffer width in pixels.
     * @param bufferHeight Target buffer height in pixels.
     * @param randomSource Random source for strategies that randomize placement.
     * @return Clipped image placement.
     */
    virtual ImagePlacement choose(const IndexedImage& image, int bufferWidth,
        int bufferHeight, RandomSource& randomSource) const = 0;
};

/**
 * Random placement strategy that keeps at least part of the image visible.
 */
class RandomLegalImagePlacementStrategy : public ImagePlacementStrategy {
public:
    /**
     * @param image Image to place.
     * @param bufferWidth Target buffer width in pixels.
     * @param bufferHeight Target buffer height in pixels.
     * @param randomSource Random source used to choose the placement.
     * @return Random legal placement, clipped to the target buffer.
     */
    ImagePlacement choose(const IndexedImage& image, int bufferWidth,
        int bufferHeight, RandomSource& randomSource) const;
};

#endif
