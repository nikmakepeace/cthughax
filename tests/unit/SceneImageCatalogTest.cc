#include "SceneImageCatalog.h"

#include "EffectChoiceLoader.h"
#include "Image.h"
#include "ProcessServices.h"
#include "pcx.h"
#include "png.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

int loadEffectChoices(EffectControl&, const PathConfig&, const char*[],
    const char*, const char*, EffectChoiceLoader) {
    return 0;
}

int loadEffectChoices(EffectControl&, const PathConfig&, const char*[],
    const char*, const char*, EffectChoiceContextLoader, void*) {
    return 0;
}

EffectChoice* read_pcx_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

EffectChoice* read_png_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

static void testCatalogOwnsCopiedIndexedImages() {
    ColorPalette* palette = new ColorPalette();
    palette->setColor(1, 10, 20, 30);
    IndexedImage image("source", 2, 2, palette);
    image.mutablePixels()[0] = 1;
    image.mutablePixels()[1] = 2;
    image.mutablePixels()[2] = 3;
    image.mutablePixels()[3] = 4;

    SceneImageCatalog catalog;
    catalog.addChoice("fixture", &image, 1);

    image.mutablePixels()[0] = 9;
    palette->setColor(1, 90, 80, 70);

    assert(catalog.entryCount() == 1);
    assert(strcmp(catalog.nameAt(0), "fixture") == 0);
    assert(catalog.inUseAt(0) == 1);

    const IndexedImage* copy = catalog.imageAt(0);
    assert(copy != 0);
    assert(strcmp(copy->name(), "source") == 0);
    assert(copy->width() == 2);
    assert(copy->height() == 2);
    assert(copy->pixels()[0] == 1);
    assert(copy->pixels()[1] == 2);
    assert(copy->pixels()[2] == 3);
    assert(copy->pixels()[3] == 4);
    assert(copy->palette() != 0);
    assert(copy->palette()->component(1, 0) == 10);
    assert(copy->palette()->component(1, 1) == 20);
    assert(copy->palette()->component(1, 2) == 30);
}

static void testOutOfRangeEntriesAreEmpty() {
    SceneImageCatalog catalog;

    assert(catalog.entryCount() == 0);
    assert(strcmp(catalog.nameAt(-1), "") == 0);
    assert(strcmp(catalog.nameAt(0), "") == 0);
    assert(catalog.imageAt(-1) == 0);
    assert(catalog.imageAt(0) == 0);
    assert(catalog.inUseAt(-1) == 0);
    assert(catalog.inUseAt(0) == 0);
}

int main() {
    testCatalogOwnsCopiedIndexedImages();
    testOutOfRangeEntriesAreEmpty();
    return 0;
}
