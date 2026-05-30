#ifndef __IMAGE_H
#define __IMAGE_H

#include "ColorPalette.h"
#include "CoreOption.h"

class CthughaBuffer;

class IndexedImage {
    char* nameValue;
    unsigned char* pixelsValue;
    int widthValue;
    int heightValue;

    IndexedImage(const IndexedImage&);
    IndexedImage& operator=(const IndexedImage&);

public:
    IndexedImage(const char* name, int width, int height);
    ~IndexedImage();

    const char* name() const;
    int width() const;
    int height() const;
    int size() const;
    const unsigned char* pixels() const;
    unsigned char* mutablePixels();
};

class ImagePlacement {
public:
    int left;
    int top;
    int sourceX;
    int sourceY;
    int destinationX;
    int destinationY;
    int width;
    int height;

    ImagePlacement();
    ImagePlacement(int left_, int top_, int imageWidth, int imageHeight,
        int bufferWidth, int bufferHeight);

    int visible() const;
};

class ImagePlacementStrategy {
public:
    virtual ~ImagePlacementStrategy();
    virtual ImagePlacement choose(const IndexedImage& image, int bufferWidth,
        int bufferHeight) const = 0;
};

class RandomLegalImagePlacementStrategy : public ImagePlacementStrategy {
public:
    ImagePlacement choose(const IndexedImage& image, int bufferWidth,
        int bufferHeight) const;
};

class ImageEntry : public CoreOptionEntry {
    IndexedImage* imageValue;
    ColorPalette* paletteValue;

    ImageEntry(const ImageEntry&);
    ImageEntry& operator=(const ImageEntry&);

public:
    ImageEntry(const char* name, const char* desc, IndexedImage* image = 0,
        ColorPalette* palette = 0);
    ~ImageEntry();

    const IndexedImage* image() const;
    const ColorPalette* palette() const;
};

class ImageOption : public CoreOption {
public:
    ImageOption(int buffer, const char* name);

    ImageEntry* currentImageEntry();
    const IndexedImage* currentImage();
    int loadImages();
};

#endif
