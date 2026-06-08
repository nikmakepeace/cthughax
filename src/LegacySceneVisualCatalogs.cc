// Legacy visual catalog adapter over explicit Scene selection ports.

#include "LegacySceneVisualCatalogs.h"

#include "Configuration.h"
#include "Border.h"
#include "Flame.h"
#include "Flashlight.h"
#include "Image.h"
#include "LegacySceneCatalogAdapters.h"
#include "LegacySceneEffectControlCatalog.h"
#include "LegacySceneSelectionAdapters.h"
#include "TranslationOptions.h"
#include "display.h"
#include "flames.h"
#include "waves.h"

#include <utility>

LegacySceneVisualCatalogs::LegacySceneVisualCatalogs(
    SceneSelectionState& selectionState_, SceneVisualSelections& selections_,
    SceneWaveObjectSource& waveObjects_,
    ScenePaletteRandomizer& paletteRandomizer_)
    : selectionState(selectionState_)
    , selections(selections_)
    , waveObjects(waveObjects_)
    , paletteRandomizer(paletteRandomizer_) { }

Wave* LegacySceneVisualCatalogs::selectRunnableWave(const WaveConfig& config) {
    int nEntries = selections.wave().entryCount();

    for (int i = 0; i < nEntries; i++) {
        Wave* selectedWave = selections.wave().currentWave();
        if (selectedWave == 0 || selectedWave->canRun(config))
            return selectedWave;

        selections.wave().change(+1);
    }

    return 0;
}

const SceneSettings& LegacySceneVisualCatalogs::currentSettings(
    SceneGeometry& geometry) {
    SceneSettings settings;

    settings.flame = selections.flame().currentFlame();
    settings.generalFlame = selections.generalFlame().encodedValue();
    settings.flameName = (settings.flame != 0) ? settings.flame->name() : "unknown";
    settings.generalFlameName = selections.generalFlame().selectionText();

    settings.waveConfig = WaveConfig(selections.waveScale().currentValue(),
        selections.table().currentValue(), waveObjects.currentObject(),
        geometry.width(), geometry.height());
    settings.wave = selectRunnableWave(settings.waveConfig);
    settings.waveName = (settings.wave != 0) ? settings.wave->name() : "unknown";
    settings.waveScaleName = selections.waveScale().currentName();
    settings.tableName = selections.table().currentName();
    settings.objectName = selections.object().currentName();

    settings.translationTable = selections.translation().currentTranslationTable();
    settings.translateIndex = selections.translation().currentValue();
    settings.translationName = selections.translation().currentName();

    settings.palette = selections.palette().currentPaletteEntry();
    settings.paletteIndex = selections.palette().currentValue();
    settings.paletteName = selections.palette().currentName();

    settings.borderMode = selections.border().currentValue();
    settings.flashlightEnabled = selections.flashlight().currentValue() != 0;
    settings.borderName = selections.border().currentName();
    settings.flashlightName = selections.flashlight().currentName();
    settings.imageName = selections.images().currentName();

    selectionState.update(settings);
    return selectionState.settings();
}

const IndexedImage* LegacySceneVisualCatalogs::currentImage() {
    return selections.images().currentImage();
}

static void applyStartupChoice(SceneOptionSelection& selection,
    const std::string& choice,
    RandomSource& randomSource) {
    selection.change(choice.c_str(), randomSource);
}

void LegacySceneVisualCatalogs::applyStartupConfig(
    const SceneConfig& config, RandomSource& randomSource) {
    applyStartupChoice(selections.waveScale(), config.waveScale, randomSource);
    applyStartupChoice(selections.table(), config.table, randomSource);
    applyStartupChoice(selections.object(), config.object, randomSource);
    applyStartupChoice(selections.wave(), config.wave, randomSource);
    applyStartupChoice(selections.flame(), config.flame, randomSource);
    applyStartupChoice(
        selections.generalFlame(), config.generalFlame, randomSource);
    applyStartupChoice(selections.translation(), config.translation, randomSource);
    applyStartupChoice(selections.palette(), config.palette, randomSource);
    applyStartupChoice(selections.border(), config.border, randomSource);
    applyStartupChoice(selections.flashlight(), config.flashlight, randomSource);
    applyStartupChoice(selections.images(), config.image, randomSource);
}

unsigned int LegacySceneVisualCatalogs::change(SceneSelectionTarget target, int by,
    RandomSource& randomSource) {
    switch (target) {
    case SceneSelectionFlame:
        selections.flame().change(by);
        return SceneNoChange;
    case SceneSelectionGeneralFlame:
        selections.generalFlame().changeRandom(randomSource);
        return SceneFlameChanged;
    case SceneSelectionWave:
        selections.wave().change(by);
        return SceneNoChange;
    case SceneSelectionWaveScale:
        selections.waveScale().change(by);
        return SceneNoChange;
    case SceneSelectionObject:
        selections.object().change(by);
        return SceneNoChange;
    case SceneSelectionTranslation:
        selections.translation().change(by);
        return SceneNoChange;
    case SceneSelectionBorder:
        selections.border().change(by);
        return SceneNoChange;
    case SceneSelectionFlashlight:
        selections.flashlight().change(by);
        return SceneNoChange;
    case SceneSelectionPalette:
        selections.palette().change(by);
        return SceneNoChange;
    case SceneSelectionTable:
        selections.table().change(by);
        return SceneNoChange;
    case SceneSelectionImage:
        selections.images().change(by);
        return SceneNoChange;
    }

    return SceneNoChange;
}

unsigned int LegacySceneVisualCatalogs::change(SceneSelectionTarget target,
    const char* to, RandomSource& randomSource) {
    switch (target) {
    case SceneSelectionFlame:
        selections.flame().change(to, randomSource);
        return SceneNoChange;
    case SceneSelectionGeneralFlame:
        selections.generalFlame().change(to, randomSource);
        return SceneFlameChanged;
    case SceneSelectionWave:
        selections.wave().change(to, randomSource);
        return SceneNoChange;
    case SceneSelectionWaveScale:
        selections.waveScale().change(to, randomSource);
        return SceneNoChange;
    case SceneSelectionObject:
        selections.object().change(to, randomSource);
        return SceneNoChange;
    case SceneSelectionTranslation:
        selections.translation().change(to, randomSource);
        return SceneNoChange;
    case SceneSelectionBorder:
        selections.border().change(to, randomSource);
        return SceneNoChange;
    case SceneSelectionFlashlight:
        selections.flashlight().change(to, randomSource);
        return SceneNoChange;
    case SceneSelectionPalette:
        selections.palette().change(to, randomSource);
        return SceneNoChange;
    case SceneSelectionTable:
        selections.table().change(to, randomSource);
        return SceneNoChange;
    case SceneSelectionImage:
        selections.images().change(to, randomSource);
        return SceneNoChange;
    }

    return SceneNoChange;
}

unsigned int LegacySceneVisualCatalogs::randomPalette(RandomSource& randomSource) {
    selections.palette().setValue(paletteRandomizer.randomizeLast(randomSource));
    return ScenePaletteChanged;
}

unsigned int LegacySceneVisualCatalogs::addRandomPalette(RandomSource& randomSource) {
    selections.palette().setValue(paletteRandomizer.addRandom(randomSource));
    return ScenePaletteChanged;
}

LegacySceneVisualCatalogFactory::LegacySceneVisualCatalogFactory(
    SceneVisualSelections& selections_)
    : ownedSelections()
    , selections(selections_)
    , waveObjects(createLegacySceneWaveObjectSource())
    , paletteRandomizer(createLegacyScenePaletteRandomizer()) { }

LegacySceneVisualCatalogFactory::LegacySceneVisualCatalogFactory(
    std::unique_ptr<SceneVisualSelections> ownedSelections_)
    : ownedSelections(std::move(ownedSelections_))
    , selections(*ownedSelections)
    , waveObjects(createLegacySceneWaveObjectSource())
    , paletteRandomizer(createLegacyScenePaletteRandomizer()) { }

SceneVisualCatalogFactoryResult LegacySceneVisualCatalogFactory::create(
    SceneSelectionState& selectionState) {
    std::unique_ptr<SceneVisualCatalogs> visualCatalogs(
        new LegacySceneVisualCatalogs(
            selectionState, selections, *waveObjects, *paletteRandomizer));
    std::unique_ptr<SceneRuntimeControlBridge> controlBridge
        = createLegacySceneEffectControlCatalog(selections);
    return SceneVisualCatalogFactoryResult(std::move(visualCatalogs),
        std::move(controlBridge), selections);
}

std::unique_ptr<SceneVisualCatalogFactory> createLegacySceneVisualCatalogFactory(
    ImageOption& images) {
    return std::unique_ptr<SceneVisualCatalogFactory>(
        new LegacySceneVisualCatalogFactory(createLegacySceneSelectionAdapters(
            flame, flameGeneral, wave, waveScale, table, object, translation,
            palette, border, flashlight, images)));
}
