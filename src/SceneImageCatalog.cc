// Native Scene catalog owner for indexed-image entries.

#include "SceneImageCatalog.h"

#include "IndexedImage.h"

#include <string.h>

namespace {

static IndexedImage* copyIndexedImage(const IndexedImage* image) {
    if (image == 0)
        return 0;

    ColorPalette* palette = 0;
    if (image->palette() != 0) {
        palette = new ColorPalette();
        palette->copyFrom(*image->palette());
    }

    IndexedImage* copy = new IndexedImage(
        image->name(), image->width(), image->height(), palette);
    if (image->pixels() != 0 && copy->mutablePixels() != 0
        && image->size() > 0) {
        memcpy(copy->mutablePixels(), image->pixels(), image->size());
    }

    return copy;
}

}

SceneImageCatalog::Entry::Entry(
    const char* name_, const IndexedImage* image_, int inUse_)
    : nameValue((name_ != 0) ? name_ : "")
    , imageValue(copyIndexedImage(image_))
    , inUseValue(inUse_) { }

SceneImageCatalog::Entry::~Entry() { }

const char* SceneImageCatalog::Entry::name() const {
    return nameValue.c_str();
}

const IndexedImage* SceneImageCatalog::Entry::image() const {
    return imageValue.get();
}

int SceneImageCatalog::Entry::inUse() const {
    return inUseValue;
}

SceneImageCatalog::SceneImageCatalog()
    : entries() { }

void SceneImageCatalog::clear() {
    entries.clear();
}

void SceneImageCatalog::addChoice(
    const char* name, const IndexedImage* image, int inUse) {
    entries.push_back(std::unique_ptr<Entry>(
        new Entry(name, image, inUse)));
}

int SceneImageCatalog::entryCount() const {
    return int(entries.size());
}

const char* SceneImageCatalog::nameAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return "";

    return entries[index]->name();
}

const IndexedImage* SceneImageCatalog::imageAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return 0;

    return entries[index]->image();
}

int SceneImageCatalog::inUseAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return 0;

    return entries[index]->inUse();
}
