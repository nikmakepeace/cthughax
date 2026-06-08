// Legacy visual catalog adapter over explicit Scene selection ports.

#include "LegacySceneVisualCatalogs.h"

#include "Configuration.h"
#include "Border.h"
#include "Flame.h"
#include "Flashlight.h"
#include "Image.h"
#include "LegacySceneCatalogAdapters.h"
#include "LegacySceneEffectControlBindings.h"
#include "LegacySceneEffectControlCatalog.h"
#include "LegacySceneSelectionAdapters.h"
#include "TranslationOptions.h"
#include "display.h"
#include "flames.h"
#include "waves.h"

#include <utility>

static void syncLegacyControlsFromSelections(
    SceneVisualSelections& selections);

static WObject* currentSceneWaveObject(SceneOptionSelection& selection) {
    SceneWaveObjectSelection* objectSelection
        = dynamic_cast<SceneWaveObjectSelection*>(&selection);
    return (objectSelection != 0) ? objectSelection->currentObject() : 0;
}

LegacySceneVisualCatalogs::LegacySceneVisualCatalogs(
    SceneSelectionState& selectionState_, SceneVisualSelections& selections_,
    ScenePaletteRandomizer& paletteRandomizer_)
    : selectionState(selectionState_)
    , selections(selections_)
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
        selections.table().currentValue(),
        currentSceneWaveObject(selections.object()),
        geometry.width(), geometry.height());
    settings.wave = selectRunnableWave(settings.waveConfig);
    syncLegacyControlsFromSelections(selections);
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

static void syncLegacyControlsFromSelections(
    SceneVisualSelections& selections) {
    LegacySceneEffectControlBindings* bindings
        = legacySceneEffectControlBindings(selections);
    if (bindings != 0)
        bindings->syncControlsFromSelections();
}

static unsigned int syncLegacyControlsAndReturn(
    SceneVisualSelections& selections, unsigned int result) {
    syncLegacyControlsFromSelections(selections);
    return result;
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
    syncLegacyControlsFromSelections(selections);
}

unsigned int LegacySceneVisualCatalogs::change(SceneSelectionTarget target, int by,
    RandomSource& randomSource) {
    unsigned int result = SceneNoChange;

    switch (target) {
    case SceneSelectionFlame:
        selections.flame().change(by);
        break;
    case SceneSelectionGeneralFlame:
        selections.generalFlame().changeRandom(randomSource);
        result = SceneFlameChanged;
        break;
    case SceneSelectionWave:
        selections.wave().change(by);
        break;
    case SceneSelectionWaveScale:
        selections.waveScale().change(by);
        break;
    case SceneSelectionObject:
        selections.object().change(by);
        break;
    case SceneSelectionTranslation:
        selections.translation().change(by);
        break;
    case SceneSelectionBorder:
        selections.border().change(by);
        break;
    case SceneSelectionFlashlight:
        selections.flashlight().change(by);
        break;
    case SceneSelectionPalette:
        selections.palette().change(by);
        break;
    case SceneSelectionTable:
        selections.table().change(by);
        break;
    case SceneSelectionImage:
        selections.images().change(by);
        break;
    }

    return syncLegacyControlsAndReturn(selections, result);
}

unsigned int LegacySceneVisualCatalogs::change(SceneSelectionTarget target,
    const char* to, RandomSource& randomSource) {
    unsigned int result = SceneNoChange;

    switch (target) {
    case SceneSelectionFlame:
        selections.flame().change(to, randomSource);
        break;
    case SceneSelectionGeneralFlame:
        selections.generalFlame().change(to, randomSource);
        result = SceneFlameChanged;
        break;
    case SceneSelectionWave:
        selections.wave().change(to, randomSource);
        break;
    case SceneSelectionWaveScale:
        selections.waveScale().change(to, randomSource);
        break;
    case SceneSelectionObject:
        selections.object().change(to, randomSource);
        break;
    case SceneSelectionTranslation:
        selections.translation().change(to, randomSource);
        break;
    case SceneSelectionBorder:
        selections.border().change(to, randomSource);
        break;
    case SceneSelectionFlashlight:
        selections.flashlight().change(to, randomSource);
        break;
    case SceneSelectionPalette:
        selections.palette().change(to, randomSource);
        break;
    case SceneSelectionTable:
        selections.table().change(to, randomSource);
        break;
    case SceneSelectionImage:
        selections.images().change(to, randomSource);
        break;
    }

    return syncLegacyControlsAndReturn(selections, result);
}

unsigned int LegacySceneVisualCatalogs::randomPalette(RandomSource& randomSource) {
    selections.palette().setValue(paletteRandomizer.randomizeLast(randomSource));
    return syncLegacyControlsAndReturn(selections, ScenePaletteChanged);
}

unsigned int LegacySceneVisualCatalogs::addRandomPalette(RandomSource& randomSource) {
    selections.palette().setValue(paletteRandomizer.addRandom(randomSource));
    return syncLegacyControlsAndReturn(selections, ScenePaletteChanged);
}

LegacySceneVisualCatalogFactory::LegacySceneVisualCatalogFactory(
    SceneVisualSelections& selections_)
    : ownedSelections()
    , selections(selections_)
    , paletteRandomizer(createLegacyScenePaletteRandomizer()) { }

LegacySceneVisualCatalogFactory::LegacySceneVisualCatalogFactory(
    std::unique_ptr<SceneVisualSelections> ownedSelections_)
    : ownedSelections(std::move(ownedSelections_))
    , selections(*ownedSelections)
    , paletteRandomizer(createLegacyScenePaletteRandomizer()) { }

SceneVisualCatalogFactoryResult LegacySceneVisualCatalogFactory::create(
    SceneSelectionState& selectionState) {
    std::unique_ptr<SceneVisualCatalogs> visualCatalogs(
        new LegacySceneVisualCatalogs(
            selectionState, selections, *paletteRandomizer));
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
