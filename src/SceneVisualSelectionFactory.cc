// Native construction for Scene visual selections.

#include "SceneVisualSelectionFactory.h"

#include "SceneBuiltInChoiceCatalogs.h"
#include "SceneChoiceSelection.h"
#include "SceneGeneralFlameSelectionValue.h"
#include "SceneTypedVisualCatalogs.h"
#include "SceneVisualSelectionSet.h"

SceneOptionSelectionSeed::SceneOptionSelectionSeed()
    : catalogName()
    , selectedValue(0)
    , lockEnabled(0) { }

SceneOptionSelectionSeed::SceneOptionSelectionSeed(
    const char* catalogName_, int selectedValue_, int lockEnabled_)
    : catalogName((catalogName_ != 0) ? catalogName_ : "")
    , selectedValue(selectedValue_)
    , lockEnabled(lockEnabled_) { }

SceneVisualSelectionSeeds::SceneVisualSelectionSeeds()
    : flame("flame", 0, 0)
    , generalFlame("flame-general", 0, 0)
    , wave("wave", 0, 0)
    , waveScale("wave-scale", 0, 0)
    , table("table", 0, 0)
    , object("object", 0, 0)
    , translation("translation", 0, 0)
    , palette("palette", 0, 0)
    , border("border", 0, 0)
    , flashlight("flashlight", 0, 0)
    , images("image", 0, 0) { }

static SceneChoiceLock* createSeedLock(const SceneOptionSelectionSeed& seed) {
    return new SceneChoiceLockValue(seed.lockEnabled);
}

std::unique_ptr<SceneVisualSelections> createSceneVisualSelections(
    const SceneVisualSelectionSeeds& seeds,
    const SceneWaveObjectCatalog& waveObjects,
    const SceneImageCatalog& imageCatalog,
    const ScenePaletteCatalog& paletteCatalog,
    const SceneTranslationCatalog& translations) {
    return std::unique_ptr<SceneVisualSelections>(new SceneVisualSelectionSet(
        new SceneFlameChoiceSelection(
            createSceneFlameChoiceCatalog(seeds.flame.catalogName.c_str(),
                createSeedLock(seeds.flame)),
            seeds.flame.selectedValue),
        new SceneGeneralFlameSelectionValue(
            seeds.generalFlame.catalogName.c_str(),
            createSeedLock(seeds.generalFlame),
            seeds.generalFlame.selectedValue),
        new SceneWaveChoiceSelection(
            createSceneWaveChoiceCatalog(seeds.wave.catalogName.c_str(),
                createSeedLock(seeds.wave)),
            seeds.wave.selectedValue),
        new SceneChoiceSelection(
            createSceneWaveScaleChoiceCatalog(
                seeds.waveScale.catalogName.c_str(),
                createSeedLock(seeds.waveScale)),
            seeds.waveScale.selectedValue),
        new SceneChoiceSelection(
            createSceneTableChoiceCatalog(seeds.table.catalogName.c_str(),
                createSeedLock(seeds.table)),
            seeds.table.selectedValue),
        new SceneWaveObjectChoiceSelection(
            createSceneWaveObjectChoiceCatalog(
                seeds.object.catalogName.c_str(), createSeedLock(seeds.object),
                waveObjects),
            seeds.object.selectedValue),
        new SceneTranslationChoiceSelection(
            createSceneTranslationChoiceCatalog(
                seeds.translation.catalogName.c_str(),
                createSeedLock(seeds.translation), translations),
            seeds.translation.selectedValue),
        new ScenePaletteChoiceSelection(
            createScenePaletteChoiceCatalog(
                seeds.palette.catalogName.c_str(),
                createSeedLock(seeds.palette), paletteCatalog),
            seeds.palette.selectedValue),
        new SceneChoiceSelection(
            createSceneBorderChoiceCatalog(seeds.border.catalogName.c_str(),
                createSeedLock(seeds.border)),
            seeds.border.selectedValue),
        new SceneChoiceSelection(
            createSceneFlashlightChoiceCatalog(
                seeds.flashlight.catalogName.c_str(),
                createSeedLock(seeds.flashlight)),
            seeds.flashlight.selectedValue),
        new SceneImageChoiceSelection(
            createSceneImageChoiceCatalog(seeds.images.catalogName.c_str(),
                createSeedLock(seeds.images), imageCatalog),
            seeds.images.selectedValue)));
}
