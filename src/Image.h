#ifndef __IMAGE_H
#define __IMAGE_H

#include "EffectControl.h"
#include "IndexedImage.h"

struct PathConfig;

/**
 * Image option entry owning optional indexed image pixels.
 */
class ImageEntry : public EffectChoice {
    IndexedImage* imageValue;

    ImageEntry(const ImageEntry&);
    ImageEntry& operator=(const ImageEntry&);

public:
    /**
     * Creates an image option entry.
     *
     * @param name Option/display name.
     * @param desc Human-readable description.
     * @param image Owned indexed image, or NULL.
     */
    ImageEntry(const char* name, const char* desc, IndexedImage* image = 0);
    ~ImageEntry();

    /** @return Owned image pointer, or NULL. */
    const IndexedImage* image() const;
};

/**
 * EffectControl that loads/selects indexed image entries.
 */
class ImageOption : public EffectControl {
public:
    ImageOption(int buffer, const char* name);

    /** @return Currently selected image entry, or NULL. */
    ImageEntry* currentImageEntry();

    /** @return Currently selected indexed image, or NULL. */
    const IndexedImage* currentImage();

    /**
     * Loads supported image formats from configured image paths.
     *
     * @param targetWidth Display/buffer width in pixels.
     * @param targetHeight Display/buffer height in pixels.
     * @return Combined loader result from the configured image catalog loaders.
     */
    int loadImages(const PathConfig& pathConfig, int targetWidth,
        int targetHeight);
};

#endif
