/** @file
 * Unit coverage for native Scene random-palette mutation.
 */

#include "ScenePaletteRandomizer.h"

#include "PaletteEntry.h"
#include "ProcessServices.h"
#include "SceneTypedVisualCatalogs.h"
#include "pcx.h"
#include "png.h"

#include <assert.h>
#include <cstring>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
char* cthugha_mode_text() { return (char*)""; }
int cth_init(int*, char**) { return 0; }
int cth_main() { return 0; }

EffectChoice* read_pcx_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

EffectChoice* read_png_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

class CountingRandomSource : public RandomSource {
    int value;

public:
    CountingRandomSource()
        : value(0) { }

    virtual int uniformInt(int exclusiveMax) {
        if (exclusiveMax <= 1)
            return 0;

        return value++ % exclusiveMax;
    }
};

static void enterTempPaletteDirectory(char originalCwd[PATH_MAX],
    char tempTemplate[]) {
    char* tempDir = mkdtemp(tempTemplate);
    assert(tempDir != NULL);
    assert(getcwd(originalCwd, PATH_MAX) != NULL);
    assert(chdir(tempDir) == 0);
    assert(mkdir("resources", 0777) == 0);
    assert(mkdir("resources/map", 0777) == 0);
}

static void writePlaceholderPalette(const char* path) {
    FILE* file = fopen(path, "wb");
    assert(file != NULL);
    fputs("0 0 0\n", file);
    fclose(file);
}

static void assertFileExists(const char* path) {
    assert(access(path, F_OK) == 0);
}

static void testRandomizerReplacesSelectedRandomPaletteInSceneSelection() {
    char originalCwd[PATH_MAX];
    char tempTemplate[] = "/tmp/cthughanix-scene-randomizer-replace-XXXXXX";
    enterTempPaletteDirectory(originalCwd, tempTemplate);

    PaletteEntry warm("warm", "Warm Palette");
    PaletteEntry randomOne("random.1", "random");
    randomOne.colors().setColor(0, 99, 98, 97);
    std::strcpy(randomOne.sourcePath, "resources/map/random.1.map");

    ScenePaletteChoiceCatalog* catalog
        = new ScenePaletteChoiceCatalog("palette", new SceneChoiceLockValue());
    catalog->addChoice(warm, 1);
    catalog->addChoice(randomOne, 1);
    ScenePaletteChoiceSelection selection(catalog, 1);
    ScenePaletteRandomizer randomizer;
    CountingRandomSource randomSource;

    int changedIndex = randomizer.randomizeLast(selection, randomSource);

    assert(changedIndex == 1);
    assert(selection.entryCount() == 2);
    assert(selection.currentValue() == 1);
    assert(randomizer.lastRandomIndex() == 1);
    assert(randomizer.lastRandomSelectionIndex() == 1);

    PaletteEntry* selected = selection.currentPaletteEntry();
    assert(selected != NULL);
    assert(std::strcmp(selected->Name(), "random.1") == 0);
    assert(std::strcmp(selected->sourcePath, "resources/map/random.1.map") == 0);
    assert(selected->colors().component(0, 0) == 1);
    assertFileExists("resources/map/random.1.map");

    assert(chdir(originalCwd) == 0);
}

static void testRandomizerAddsAfterSceneAndPersistedRandomPalettes() {
    char originalCwd[PATH_MAX];
    char tempTemplate[] = "/tmp/cthughanix-scene-randomizer-add-XXXXXX";
    enterTempPaletteDirectory(originalCwd, tempTemplate);
    writePlaceholderPalette("resources/map/random.0.map");
    writePlaceholderPalette("resources/map/random.4.map");

    PaletteEntry warm("warm", "Warm Palette");
    PaletteEntry randomTwo("random.2", "random");
    ScenePaletteChoiceCatalog* catalog
        = new ScenePaletteChoiceCatalog("palette", new SceneChoiceLockValue());
    catalog->addChoice(warm, 1);
    catalog->addChoice(randomTwo, 1);
    ScenePaletteChoiceSelection selection(catalog, 0);
    ScenePaletteRandomizer randomizer;
    CountingRandomSource randomSource;

    int addedIndex = randomizer.addRandom(selection, randomSource);

    assert(addedIndex == 2);
    assert(selection.entryCount() == 3);
    assert(randomizer.lastRandomIndex() == 5);
    assert(randomizer.lastRandomSelectionIndex() == 2);

    selection.setValue(addedIndex);
    PaletteEntry* selected = selection.currentPaletteEntry();
    assert(selected != NULL);
    assert(std::strcmp(selected->Name(), "random.5") == 0);
    assert(std::strcmp(selected->sourcePath, "resources/map/random.5.map") == 0);
    assertFileExists("resources/map/random.5.map");

    assert(chdir(originalCwd) == 0);
}

static void testRandomizerUsesLastSceneSlotWhenCurrentPaletteIsNotRandom() {
    char originalCwd[PATH_MAX];
    char tempTemplate[] = "/tmp/cthughanix-scene-randomizer-last-XXXXXX";
    enterTempPaletteDirectory(originalCwd, tempTemplate);

    PaletteEntry warm("warm", "Warm Palette");
    ScenePaletteChoiceCatalog* catalog
        = new ScenePaletteChoiceCatalog("palette", new SceneChoiceLockValue());
    catalog->addChoice(warm, 1);
    ScenePaletteChoiceSelection selection(catalog, 0);
    ScenePaletteRandomizer randomizer;
    CountingRandomSource randomSource;

    int addedIndex = randomizer.addRandom(selection, randomSource);
    assert(addedIndex == 1);
    selection.setValue(0);

    int changedIndex = randomizer.randomizeLast(selection, randomSource);

    assert(changedIndex == 1);
    assert(selection.entryCount() == 2);
    assert(randomizer.lastRandomIndex() == 0);
    assert(randomizer.lastRandomSelectionIndex() == 1);

    selection.setValue(changedIndex);
    PaletteEntry* selected = selection.currentPaletteEntry();
    assert(selected != NULL);
    assert(std::strcmp(selected->Name(), "random.0") == 0);
    assertFileExists("resources/map/random.0.map");

    assert(chdir(originalCwd) == 0);
}

int main() {
    testRandomizerReplacesSelectedRandomPaletteInSceneSelection();
    testRandomizerAddsAfterSceneAndPersistedRandomPalettes();
    testRandomizerUsesLastSceneSlotWhenCurrentPaletteIsNotRandom();
    return 0;
}
