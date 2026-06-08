#include "SceneImageCatalogLoader.h"
#include "ScenePaletteCatalogLoader.h"
#include "SceneWaveObjectCatalogLoader.h"

#include "Configuration.h"
#include "EffectChoiceLoader.h"
#include "Image.h"
#include "PaletteEntry.h"
#include "ProcessServices.h"
#include "SceneImageCatalog.h"
#include "ScenePaletteCatalog.h"
#include "SceneWaveObjectCatalog.h"
#include "WaveObject.h"
#include "pcx.h"
#include "png.h"

#include <assert.h>
#include <cstdio>
#include <stdarg.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

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

IndexedImage* read_pcx_indexed_image(FILE*, const char* name, const char*,
    const char*, const ImageLoadTarget&) {
    ColorPalette* palette = new ColorPalette();
    palette->setColor(1, 10, 20, 30);

    IndexedImage* image = new IndexedImage(name, 1, 1, palette);
    image->mutablePixels()[0] = 7;
    return image;
}

IndexedImage* read_png_indexed_image(FILE*, const char*, const char*,
    const char*, const ImageLoadTarget&) {
    return 0;
}

PaletteEntry* read_palette_entry(FILE*, const char* name, const char*,
    const char*) {
    PaletteEntry* palette = new PaletteEntry(name, "");
    palette->colors().setColor(0, 255, 255, 255);
    palette->colors().setColor(2, 40, 50, 60);
    return palette;
}

class SilentLogSink : public LogSink {
public:
    virtual int enabled(int) const { return 0; }

protected:
    virtual void write(
        int, const char*, int, const char*, va_list) { }
};

static int catalogIndexByName(
    const SceneImageCatalog& catalog, const char* name) {
    for (int i = 0; i < catalog.entryCount(); i++)
        if (strcmp(catalog.nameAt(i), name) == 0)
            return i;

    return -1;
}

static int catalogIndexByName(
    const SceneWaveObjectCatalog& catalog, const char* name) {
    for (int i = 0; i < catalog.entryCount(); i++)
        if (strcmp(catalog.nameAt(i), name) == 0)
            return i;

    return -1;
}

static int catalogIndexByName(
    const ScenePaletteCatalog& catalog, const char* name) {
    for (int i = 0; i < catalog.entryCount(); i++) {
        const PaletteEntry* entry = catalog.paletteAt(i);
        if (entry != 0 && strcmp(entry->Name(), name) == 0)
            return i;
    }

    return -1;
}

static std::string createTemporaryObjectRoot() {
    char pathTemplate[] = "/tmp/cthughanix-scene-objects-XXXXXX";
    char* path = mkdtemp(pathTemplate);
    assert(path != 0);

    std::string root(path);
    std::string objectDir = root + "/obj";
    assert(mkdir(objectDir.c_str(), 0700) == 0);
    return root;
}

static void writeObjectFile(const std::string& root, const char* name) {
    std::string path = root + "/obj/" + name;
    FILE* file = fopen(path.c_str(), "w");
    assert(file != 0);
    assert(fputs(
        "# ignored\n"
        "10,20,30 - 11,22,33\n"
        "13,24,31 - 10,20,30\n",
        file) >= 0);
    fclose(file);
}

static void removeTemporaryObjectRoot(const std::string& root) {
    unlink((root + "/obj/fixture.obj").c_str());
    rmdir((root + "/obj").c_str());
    rmdir(root.c_str());
}

static std::string createTemporaryImageRoot() {
    char pathTemplate[] = "/tmp/cthughanix-scene-images-XXXXXX";
    char* path = mkdtemp(pathTemplate);
    assert(path != 0);

    std::string root(path);
    std::string imageDir = root + "/img";
    assert(mkdir(imageDir.c_str(), 0700) == 0);
    return root;
}

static void writeImageFile(const std::string& root, const char* name) {
    std::string path = root + "/img/" + name;
    FILE* file = fopen(path.c_str(), "w");
    assert(file != 0);
    assert(fputs("stub image\n", file) >= 0);
    fclose(file);
}

static void removeTemporaryImageRoot(const std::string& root) {
    unlink((root + "/img/fixture.pcx").c_str());
    rmdir((root + "/img").c_str());
    rmdir(root.c_str());
}

static std::string createTemporaryPaletteRoot() {
    char pathTemplate[] = "/tmp/cthughanix-scene-palettes-XXXXXX";
    char* path = mkdtemp(pathTemplate);
    assert(path != 0);

    std::string root(path);
    std::string paletteDir = root + "/map";
    assert(mkdir(paletteDir.c_str(), 0700) == 0);
    return root;
}

static void writePaletteFile(const std::string& root, const char* name) {
    std::string path = root + "/map/" + name;
    FILE* file = fopen(path.c_str(), "w");
    assert(file != 0);
    assert(fputs("stub palette\n", file) >= 0);
    fclose(file);
}

static void removeTemporaryPaletteRoot(const std::string& root) {
    unlink((root + "/map/fixture-scene-palette.map").c_str());
    rmdir((root + "/map").c_str());
    rmdir(root.c_str());
}

static void testLoadsWaveObjectsFromPathConfig() {
    std::string root = createTemporaryObjectRoot();
    writeObjectFile(root, "fixture.obj");

    PathConfig pathConfig;
    pathConfig.extraLibraryPath = root;
    SilentLogSink log;
    SceneWaveObjectCatalog catalog;

    loadSceneWaveObjectCatalog(catalog, pathConfig, 1, log);

    int builtInIndex = catalogIndexByName(catalog, "bigH");
    assert(builtInIndex >= 0);
    assert(catalog.objectAt(builtInIndex) != 0);

    int fixtureIndex = catalogIndexByName(catalog, "fixture");
    assert(fixtureIndex >= 0);
    assert(catalog.inUseAt(fixtureIndex) == 1);

    const WObject* object = catalog.objectAt(fixtureIndex);
    assert(object != 0);
    assert(object[0][0][0] == 0);
    assert(object[0][0][1] == 0);
    assert(object[0][0][2] == 0);
    assert(object[0][1][0] == 1);
    assert(object[0][1][1] == 2);
    assert(object[0][1][2] == 3);

    removeTemporaryObjectRoot(root);
}

static void testLoadsImagesFromPathConfig() {
    std::string root = createTemporaryImageRoot();
    writeImageFile(root, "fixture.pcx");

    PathConfig pathConfig;
    pathConfig.extraLibraryPath = root;
    SilentLogSink log;
    SceneImageCatalog catalog;

    loadSceneImageCatalog(catalog, pathConfig, 1, 320, 200, log);

    int noneIndex = catalogIndexByName(catalog, "none");
    assert(noneIndex >= 0);
    assert(catalog.imageAt(noneIndex) == 0);

    int index = catalogIndexByName(catalog, "fixture");
    assert(index >= 0);
    assert(catalog.inUseAt(index) == 1);

    const IndexedImage* copied = catalog.imageAt(index);
    assert(copied != 0);
    assert(strcmp(copied->name(), "fixture") == 0);
    assert(copied->pixels()[0] == 7);
    assert(copied->palette()->component(1, 0) == 10);

    removeTemporaryImageRoot(root);
}

static void testLoadsPalettesFromPathConfig() {
    std::string root = createTemporaryPaletteRoot();
    writePaletteFile(root, "fixture-scene-palette.map");

    PathConfig pathConfig;
    pathConfig.extraLibraryPath = root;
    SilentLogSink log;
    ScenePaletteCatalog catalog;

    loadScenePaletteCatalog(catalog, pathConfig, "", log);

    int generalIndex = catalogIndexByName(catalog, "general");
    assert(generalIndex >= 0);

    int index = catalogIndexByName(catalog, "fixture-scene-palette");
    assert(index >= 0);
    assert(catalog.inUseAt(index) == 1);
    assert(catalog.paletteAt(index)->colors().component(2, 0) == 40);
    assert(catalog.paletteAt(index)->colors().component(2, 1) == 50);
    assert(catalog.paletteAt(index)->colors().component(2, 2) == 60);

    removeTemporaryPaletteRoot(root);
}

int main() {
    testLoadsWaveObjectsFromPathConfig();
    testLoadsImagesFromPathConfig();
    testLoadsPalettesFromPathConfig();
    return 0;
}
