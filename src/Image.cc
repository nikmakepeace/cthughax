#include "Image.h"
#include "cthugha.h"
#include "imath.h"

CoreOptionEntry* read_pcx_image(FILE* file, const char* name, const char* dir,
    const char* totalName);
CoreOptionEntry* read_png_image(FILE* file, const char* name, const char* dir,
    const char* totalName);

static CoreOptionEntryList& imageEntries() {
    static ImageEntry none("none", "");
    static CoreOptionEntryList entries(&none);
    return entries;
}

static const char* imagePath[] = { "./", "./pcx/", CTH_LIBDIR "/pcx/", "" };

static int chooseImageLeft(int imageSize, int bufferSize) {
    if (imageSize <= 0 || bufferSize <= 0)
        return 0;

    if (imageSize > bufferSize)
        return -(Random(imageSize - bufferSize + 1));

    return Random(bufferSize - imageSize + 1);
}

IndexedImage::IndexedImage(const char* name, int width, int height)
    : nameValue(0)
    , pixelsValue(0)
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
    width = min(imageWidth - sourceX, bufferWidth - destinationX);
    height = min(imageHeight - sourceY, bufferHeight - destinationY);
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
    int bufferWidth, int bufferHeight) const {
    int left = chooseImageLeft(image.width(), bufferWidth);
    int top = chooseImageLeft(image.height(), bufferHeight);

    return ImagePlacement(left, top, image.width(), image.height(), bufferWidth,
        bufferHeight);
}

ImageEntry::ImageEntry(const char* name, const char* desc, IndexedImage* image,
    ColorPalette* palette)
    : CoreOptionEntry(name, desc)
    , imageValue(image)
    , paletteValue(palette) { }

ImageEntry::~ImageEntry() {
    delete imageValue;
    imageValue = 0;
    delete paletteValue;
    paletteValue = 0;
}

const IndexedImage* ImageEntry::image() const {
    return imageValue;
}

const ColorPalette* ImageEntry::palette() const {
    return paletteValue;
}

ImageOption::ImageOption(int buffer, const char* name)
    : CoreOption(buffer, name, imageEntries()) { }

ImageEntry* ImageOption::currentImageEntry() {
    return static_cast<ImageEntry*>(current());
}

const IndexedImage* ImageOption::currentImage() {
    ImageEntry* entry = currentImageEntry();
    return (entry != 0) ? entry->image() : 0;
}

int ImageOption::loadImages() {
    int result = 0;

    result |= load(imagePath, "/pcx/", ".pcx", read_pcx_image);
    result |= load(imagePath, "/pcx/", ".png", read_png_image);

    return result;
}
