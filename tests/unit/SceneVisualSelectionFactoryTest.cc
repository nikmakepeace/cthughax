#include "SceneVisualSelectionFactory.h"

#include "EffectChoiceLoader.h"
#include "Flame.h"
#include "Image.h"
#include "SceneImageCatalog.h"
#include "ScenePaletteCatalog.h"
#include "SceneTranslationCatalog.h"
#include "SceneVisualSelections.h"
#include "SceneWaveObjectCatalog.h"
#include "Wave.h"
#include "pcx.h"
#include "png.h"

#include <cassert>
#include <memory>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }

const int nFlameCatalogEntries = 0;
const int nWaveCatalogEntries = 0;

const char* Flame::name() const { return ""; }
const char* Wave::name() const { return ""; }

const Flame* flameByIndex(int) { return 0; }
Wave* waveByIndex(int) { return 0; }

EffectChoice* read_pcx_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

EffectChoice* read_png_image(FILE*, const char*, const char*, const char*,
    const ImageLoadTarget&) {
    return 0;
}

int loadEffectChoices(EffectControl&, const PathConfig&, const char*[],
    const char*, const char*, EffectChoiceContextLoader, void*) {
    return 0;
}

static void seedSelection(SceneOptionSelectionSeed& seed, const char* name,
    int value, int lockValue) {
    seed = SceneOptionSelectionSeed(name, value, lockValue);
}

static void assertSelectionState(
    const SceneOptionSelection& selection, int value, int lockValue) {
    assert(selection.currentValue() == value);
    assert(selection.lockEnabled() == lockValue);
}

static SceneVisualSelectionSeeds populatedSeeds() {
    SceneVisualSelectionSeeds seeds;
    seedSelection(seeds.flame, "flame", 1, 1);
    seedSelection(seeds.generalFlame, "flame-general", 2, 0);
    seedSelection(seeds.wave, "wave", 3, 1);
    seedSelection(seeds.waveScale, "wave-scale", 1, 0);
    seedSelection(seeds.table, "table", 4, 1);
    seedSelection(seeds.object, "object", 0, 0);
    seedSelection(seeds.translation, "translation", 0, 1);
    seedSelection(seeds.palette, "palette", 0, 0);
    seedSelection(seeds.border, "border", 2, 1);
    seedSelection(seeds.flashlight, "flashlight", 1, 0);
    seedSelection(seeds.images, "image", 0, 1);
    return seeds;
}

static void testFactorySeedsSelectionsFromNativeValues() {
    SceneVisualSelectionSeeds seeds = populatedSeeds();
    SceneWaveObjectCatalog waveObjects;
    SceneImageCatalog imageCatalog;
    ScenePaletteCatalog paletteCatalog;
    SceneTranslationCatalog translations;

    std::unique_ptr<SceneVisualSelections> selections
        = createSceneVisualSelections(seeds, waveObjects, imageCatalog,
            paletteCatalog, translations);

    assertSelectionState(selections->flame(), 1, 1);
    assertSelectionState(selections->generalFlame(), 2, 0);
    assertSelectionState(selections->wave(), 3, 1);
    assertSelectionState(selections->waveScale(), 1, 0);
    assertSelectionState(selections->table(), 4, 1);
    assertSelectionState(selections->object(), 0, 0);
    assertSelectionState(selections->translation(), 0, 1);
    assertSelectionState(selections->palette(), 0, 0);
    assertSelectionState(selections->border(), 2, 1);
    assertSelectionState(selections->flashlight(), 1, 0);
    assertSelectionState(selections->images(), 0, 1);
}

static void testSelectionsDoNotMutateSeedValues() {
    SceneVisualSelectionSeeds seeds = populatedSeeds();
    SceneWaveObjectCatalog waveObjects;
    SceneImageCatalog imageCatalog;
    ScenePaletteCatalog paletteCatalog;
    SceneTranslationCatalog translations;

    std::unique_ptr<SceneVisualSelections> selections
        = createSceneVisualSelections(seeds, waveObjects, imageCatalog,
            paletteCatalog, translations);

    selections->flame().setValue(2);
    selections->flame().toggleLock();

    assert(seeds.flame.selectedValue == 1);
    assert(seeds.flame.lockEnabled == 1);
}

int main() {
    testFactorySeedsSelectionsFromNativeValues();
    testSelectionsDoNotMutateSeedValues();
    return 0;
}
