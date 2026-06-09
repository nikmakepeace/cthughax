// Indexed image ownership, placement, and loader option integration.

#include "Image.h"
#include "cthugha.h"
#include "Configuration.h"
#include "EffectChoiceLoader.h"
#include "ImagePlacement.h"
#include "ProcessServices.h"
#include "pcx.h"
#include "png.h"

static EffectChoiceList& imageEntries() {
    static ImageEntry none("none", "");
    static EffectChoiceList entries(&none);
    return entries;
}

static int minInt(int lhs, int rhs) {
    return lhs < rhs ? lhs : rhs;
}

static const char* imagePath[] = { "./", "./resources/img/", CTH_LIBDIR "/img/", "" };

struct ImageFileFormat {
    const char* extension;
    EffectChoiceContextLoader loader;
};

static EffectChoice* loadPcxImage(FILE* file, const char* name, const char* dir,
    const char* totalName, void* context) {
    return read_pcx_image(file, name, dir, totalName,
        *static_cast<const ImageLoadTarget*>(context));
}

static EffectChoice* loadPngImage(FILE* file, const char* name, const char* dir,
    const char* totalName, void* context) {
    return read_png_image(file, name, dir, totalName,
        *static_cast<const ImageLoadTarget*>(context));
}

static const ImageFileFormat imageFileFormats[] = {
    { ".pcx", loadPcxImage },
    { ".png", loadPngImage },
    { 0, 0 }
};

static int chooseImageLeft(int imageSize, int bufferSize, RandomSource& randomSource) {
    if (imageSize <= 0 || bufferSize <= 0)
        return 0;

    if (imageSize > bufferSize)
        return -(randomSource.uniformInt(imageSize - bufferSize + 1));

    return randomSource.uniformInt(bufferSize - imageSize + 1);
}

IndexedImage::IndexedImage(const char* name, int width, int height,
    ColorPalette* palette)
    : nameValue(0)
    , pixelsValue(0)
    , paletteValue(palette)
    , widthValue(width)
    , heightValue(height) {
    const char* imageName = (name != 0) ? name : "";
    nameValue = new char[strlen(imageName) + 1];
    strcpy(nameValue, imageName);

    if (widthValue > 0 && heightValue > 0)
        pixelsValue = new unsigned char[widthValue * heightValue];
}

IndexedImage::~IndexedImage() {
    delete[] nameValue;
    nameValue = 0;
    delete[] pixelsValue;
    pixelsValue = 0;
    delete paletteValue;
    paletteValue = 0;
}

const char* IndexedImage::name() const {
    return nameValue;
}

int IndexedImage::width() const {
    return widthValue;
}

int IndexedImage::height() const {
    return heightValue;
}

int IndexedImage::size() const {
    return widthValue * heightValue;
}

const unsigned char* IndexedImage::pixels() const {
    return pixelsValue;
}

unsigned char* IndexedImage::mutablePixels() {
    return pixelsValue;
}

void IndexedImage::setPalette(ColorPalette* palette) {
    if (paletteValue == palette)
        return;

    delete paletteValue;
    paletteValue = palette;
}

const ColorPalette* IndexedImage::palette() const {
    return paletteValue;
}

ImagePlacement::ImagePlacement()
    : left(0)
    , top(0)
    , sourceX(0)
    , sourceY(0)
    , destinationX(0)
    , destinationY(0)
    , width(0)
    , height(0) { }

ImagePlacement::ImagePlacement(int left_, int top_, int imageWidth, int imageHeight,
    int bufferWidth, int bufferHeight)
    : left(left_)
    , top(top_)
    , sourceX((left_ < 0) ? -left_ : 0)
    , sourceY((top_ < 0) ? -top_ : 0)
    , destinationX((left_ > 0) ? left_ : 0)
    , destinationY((top_ > 0) ? top_ : 0)
    , width(0)
    , height(0) {
    width = minInt(imageWidth - sourceX, bufferWidth - destinationX);
    height = minInt(imageHeight - sourceY, bufferHeight - destinationY);
    if (width < 0)
        width = 0;
    if (height < 0)
        height = 0;
}

int ImagePlacement::visible() const {
    return width > 0 && height > 0;
}

ImagePlacementStrategy::~ImagePlacementStrategy() { }

ImagePlacement RandomLegalImagePlacementStrategy::choose(const IndexedImage& image,
    int bufferWidth, int bufferHeight, RandomSource& randomSource) const {
    int left = chooseImageLeft(image.width(), bufferWidth, randomSource);
    int top = chooseImageLeft(image.height(), bufferHeight, randomSource);

    return ImagePlacement(left, top, image.width(), image.height(), bufferWidth,
        bufferHeight);
}

ImageEntry::ImageEntry(const char* name, const char* desc, IndexedImage* image)
    : EffectChoice(name, desc)
    , imageValue(image) { }

ImageEntry::~ImageEntry() {
    delete imageValue;
    imageValue = 0;
}

const IndexedImage* ImageEntry::image() const {
    return imageValue;
}

ImageOption::ImageOption(int buffer, const char* name)
    : EffectControl(buffer, name, imageEntries(), EFFECT_CONTROL_AUTO_CHANGE) { }

ImageEntry* ImageOption::currentImageEntry() {
    return static_cast<ImageEntry*>(current());
}

const IndexedImage* ImageOption::currentImage() {
    ImageEntry* entry = currentImageEntry();
    return (entry != 0) ? entry->image() : 0;
}

int ImageOption::loadImages(const PathConfig& pathConfig, int targetWidth,
    int targetHeight) {
    int result = 0;
    ImageLoadTarget target(targetWidth, targetHeight);

    for (const ImageFileFormat* format = imageFileFormats; format->extension != 0;
         format++) {
        result |= loadEffectChoices(*this, pathConfig, imagePath, "/img/",
            format->extension, format->loader, &target);
    }

    return result;
}
