// Legacy visual catalog adapter over explicit Scene selection ports.

#include "LegacySceneVisualCatalogs.h"

#include "Configuration.h"
#include "Flame.h"
#include "LegacySceneCatalogAdapters.h"
#include "LegacySceneControlMirror.h"
#include "PaletteEntry.h"
#include "SceneTypedVisualCatalogs.h"

static void syncLegacyControlsFromSelections(
    LegacySceneControlMirror& mirror);

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

LegacySceneVisualCatalogs::LegacySceneVisualCatalogs(
    SceneSelectionState& selectionState_, SceneVisualSelections& selections_,
    LegacySceneControlMirror& controlMirror_,
    ScenePaletteRandomizer& paletteRandomizer_)
    : selectionState(selectionState_)
    , selections(selections_)
    , controlMirror(controlMirror_)
    , paletteRandomizer(paletteRandomizer_) { }

Wave* LegacySceneVisualCatalogs::selectRunnableWave(
    const WaveConfig& config, int& selectionChanged) {
    int nEntries = selections.wave().entryCount();

    for (int i = 0; i < nEntries; i++) {
        Wave* selectedWave = selections.wave().currentWave();
        if (selectedWave == 0 || selectedWave->canRun(config))
            return selectedWave;

        selections.wave().change(+1);
        selectionChanged = 1;
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
    int waveSelectionChanged = 0;
    settings.wave = selectRunnableWave(
        settings.waveConfig, waveSelectionChanged);
    if (waveSelectionChanged)
        syncLegacyControlsFromSelections(controlMirror);
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
    LegacySceneControlMirror& mirror) {
    mirror.syncControlsFromSelections();
}

static unsigned int syncLegacyControlsAndReturn(
    LegacySceneControlMirror& mirror, unsigned int result) {
    syncLegacyControlsFromSelections(mirror);
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
    syncLegacyControlsFromSelections(controlMirror);
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

    return syncLegacyControlsAndReturn(controlMirror, result);
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

    return syncLegacyControlsAndReturn(controlMirror, result);
}

unsigned int LegacySceneVisualCatalogs::activate(
    SceneSelectionTarget target, int index) {
    sceneSelectionForTarget(selections, target).activate(index);
    return syncLegacyControlsAndReturn(controlMirror,
        forcedChangeForSelection(target));
}

void LegacySceneVisualCatalogs::toggleLock(SceneSelectionTarget target) {
    sceneSelectionForTarget(selections, target).toggleLock();
    syncLegacyControlsFromSelections(controlMirror);
}

void LegacySceneVisualCatalogs::toggleChoiceUse(
    SceneSelectionTarget target, int index) {
    sceneSelectionForTarget(selections, target).toggleChoiceUse(index);
    syncLegacyControlsFromSelections(controlMirror);
}

unsigned int LegacySceneVisualCatalogs::randomPalette(RandomSource& randomSource) {
    int index = paletteRandomizer.randomizeLast(randomSource);
    refreshOwnedPaletteEntry(selections, paletteRandomizer, index);
    selections.palette().setValue(index);
    return syncLegacyControlsAndReturn(controlMirror, ScenePaletteChanged);
}

unsigned int LegacySceneVisualCatalogs::addRandomPalette(RandomSource& randomSource) {
    int index = paletteRandomizer.addRandom(randomSource);
    refreshOwnedPaletteEntry(selections, paletteRandomizer, index);
    selections.palette().setValue(index);
    return syncLegacyControlsAndReturn(controlMirror, ScenePaletteChanged);
}
