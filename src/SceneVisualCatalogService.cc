// Native Scene visual catalog and selection service.

#include "SceneVisualCatalogService.h"

#include "Configuration.h"
#include "Flame.h"
#include "PaletteEntry.h"
#include "ScenePaletteRandomizer.h"
#include "SceneTypedVisualCatalogs.h"
#include "SceneVisualSelections.h"

static WObject* currentSceneWaveObject(SceneOptionSelection& selection) {
    SceneWaveObjectSelection* objectSelection
        = dynamic_cast<SceneWaveObjectSelection*>(&selection);
    return (objectSelection != 0) ? objectSelection->currentObject() : 0;
}

static SceneOptionSelection& sceneSelectionForTarget(
    SceneVisualSelections& selections, SceneSelectionTarget target) {
    switch (target) {
    case SceneSelectionFlame:
        return selections.flame();
    case SceneSelectionGeneralFlame:
        return selections.generalFlame();
    case SceneSelectionWave:
        return selections.wave();
    case SceneSelectionWaveScale:
        return selections.waveScale();
    case SceneSelectionObject:
        return selections.object();
    case SceneSelectionTranslation:
        return selections.translation();
    case SceneSelectionBorder:
        return selections.border();
    case SceneSelectionFlashlight:
        return selections.flashlight();
    case SceneSelectionPalette:
        return selections.palette();
    case SceneSelectionTable:
        return selections.table();
    case SceneSelectionImage:
        return selections.images();
    }

    return selections.flame();
}

static unsigned int forcedChangeForSelection(SceneSelectionTarget target) {
    switch (target) {
    case SceneSelectionGeneralFlame:
        return SceneFlameChanged;
    case SceneSelectionImage:
        return SceneImageChanged;
    default:
        return SceneNoChange;
    }
}

static void refreshOwnedPaletteEntry(SceneVisualSelections& selections,
    ScenePaletteRandomizer& paletteRandomizer, int index) {
    if (index < 0)
        return;

    ScenePaletteChoiceSelection* paletteSelection
        = dynamic_cast<ScenePaletteChoiceSelection*>(&selections.palette());
    PaletteEntry* paletteEntry = paletteRandomizer.paletteEntry(index);
    if (paletteSelection == 0 || paletteEntry == 0)
        return;

    if (index < paletteSelection->entryCount())
        paletteSelection->replacePaletteEntry(index, *paletteEntry,
            paletteEntry->inUse());
    else if (index == paletteSelection->entryCount())
        paletteSelection->appendPaletteEntry(*paletteEntry,
            paletteEntry->inUse());
}

static void applyStartupChoice(SceneOptionSelection& selection,
    const std::string& choice, RandomSource& randomSource) {
    selection.change(choice.c_str(), randomSource);
}

SceneVisualCatalogService::SceneVisualCatalogService(
    SceneSelectionState& selectionState_, SceneVisualSelections& selections_,
    ScenePaletteRandomizer& paletteRandomizer_)
    : selectionState(selectionState_)
    , selections(selections_)
    , paletteRandomizer(paletteRandomizer_) { }

Wave* SceneVisualCatalogService::selectRunnableWave(
    const WaveConfig& config) {
    int nEntries = selections.wave().entryCount();

    for (int i = 0; i < nEntries; i++) {
        Wave* selectedWave = selections.wave().currentWave();
        if (selectedWave == 0 || selectedWave->canRun(config))
            return selectedWave;

        selections.wave().change(+1);
    }

    return 0;
}

const SceneSettings& SceneVisualCatalogService::currentSettings(
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

const IndexedImage* SceneVisualCatalogService::currentImage() {
    return selections.images().currentImage();
}

void SceneVisualCatalogService::applyStartupConfig(
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

unsigned int SceneVisualCatalogService::change(SceneSelectionTarget target,
    int by, RandomSource& randomSource) {
    switch (target) {
    case SceneSelectionFlame:
        selections.flame().change(by);
        break;
    case SceneSelectionGeneralFlame:
        selections.generalFlame().changeRandom(randomSource);
        return SceneFlameChanged;
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

    return SceneNoChange;
}

unsigned int SceneVisualCatalogService::change(SceneSelectionTarget target,
    const char* to, RandomSource& randomSource) {
    switch (target) {
    case SceneSelectionFlame:
        selections.flame().change(to, randomSource);
        break;
    case SceneSelectionGeneralFlame:
        selections.generalFlame().change(to, randomSource);
        return SceneFlameChanged;
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

    return SceneNoChange;
}

unsigned int SceneVisualCatalogService::activate(
    SceneSelectionTarget target, int index) {
    sceneSelectionForTarget(selections, target).activate(index);
    return forcedChangeForSelection(target);
}

void SceneVisualCatalogService::toggleLock(SceneSelectionTarget target) {
    sceneSelectionForTarget(selections, target).toggleLock();
}

void SceneVisualCatalogService::toggleChoiceUse(
    SceneSelectionTarget target, int index) {
    sceneSelectionForTarget(selections, target).toggleChoiceUse(index);
}

unsigned int SceneVisualCatalogService::randomPalette(
    RandomSource& randomSource) {
    int index = paletteRandomizer.randomizeLast(randomSource,
        selections.palette().currentValue());
    refreshOwnedPaletteEntry(selections, paletteRandomizer, index);
    selections.palette().setValue(index);
    return ScenePaletteChanged;
}

unsigned int SceneVisualCatalogService::addRandomPalette(
    RandomSource& randomSource) {
    int index = paletteRandomizer.addRandom(randomSource);
    refreshOwnedPaletteEntry(selections, paletteRandomizer, index);
    selections.palette().setValue(index);
    return ScenePaletteChanged;
}
