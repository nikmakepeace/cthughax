// Legacy visual selection construction over global visual options.

#include "LegacySceneSelectionAdapters.h"

#include "EffectControl.h"
#include "LegacySceneChoiceLock.h"
#include "PaletteEntry.h"
#include "SceneBuiltInChoiceCatalogs.h"
#include "SceneChoiceSelection.h"
#include "SceneGeneralFlameSelectionValue.h"
#include "SceneImageCatalog.h"
#include "ScenePaletteCatalog.h"
#include "SceneTranslationCatalog.h"
#include "SceneTypedVisualCatalogs.h"
#include "SceneVisualSelectionSet.h"
#include "SceneWaveObjectCatalog.h"
#include "display.h"
#include "flames.h"
#include "waves.h"

namespace {

static SceneChoiceCatalog* createSceneFlameChoiceCatalog(
    EffectControl& option) {
    SceneFlameChoiceCatalog* catalog = new SceneFlameChoiceCatalog(
        option.name(), new LegacySceneChoiceLock(option.lock));

    for (int i = 0; i < nFlameCatalogEntries; i++) {
        const Flame* flame = flameByIndex(i);
        if (flame != 0)
            catalog->addChoice(
                flame, flame->name(), sceneBuiltInFlameChoiceInUse(i));
    }

    return catalog;
}

static SceneChoiceCatalog* createSceneWaveChoiceCatalog(
    EffectControl& option) {
    SceneWaveChoiceCatalog* catalog = new SceneWaveChoiceCatalog(
        option.name(), new LegacySceneChoiceLock(option.lock));

    for (int i = 0; i < nWaveCatalogEntries; i++) {
        Wave* wave = waveByIndex(i);
        if (wave != 0)
            catalog->addChoice(
                wave, wave->name(), sceneBuiltInWaveChoiceInUse(i));
    }

    return catalog;
}

static SceneChoiceCatalog* createSceneWaveObjectChoiceCatalog(
    EffectControl& option, const SceneWaveObjectCatalog& waveObjects) {
    SceneWaveObjectChoiceCatalog* catalog = new SceneWaveObjectChoiceCatalog(
        option.name(), new LegacySceneChoiceLock(option.lock));

    for (int i = 0; i < waveObjects.entryCount(); i++)
        catalog->addChoice(waveObjects.nameAt(i), waveObjects.objectAt(i),
            waveObjects.inUseAt(i));

    return catalog;
}

static SceneChoiceCatalog* createSceneTranslationChoiceCatalog(
    EffectControl& option, const SceneTranslationCatalog& translations) {
    SceneTranslationChoiceCatalog* catalog = new SceneTranslationChoiceCatalog(
        option.name(), new LegacySceneChoiceLock(option.lock));

    for (int i = 0; i < translations.entryCount(); i++)
        catalog->addChoice(translations.tableAt(i), translations.inUseAt(i));

    return catalog;
}

static SceneChoiceCatalog* createScenePaletteChoiceCatalog(
    EffectControl& option, const ScenePaletteCatalog& palettes) {
    ScenePaletteChoiceCatalog* catalog = new ScenePaletteChoiceCatalog(
        option.name(), new LegacySceneChoiceLock(option.lock));

    for (int i = 0; i < palettes.entryCount(); i++) {
        const PaletteEntry* entry = palettes.paletteAt(i);
        if (entry != 0)
            catalog->addChoice(*entry, palettes.inUseAt(i));
    }

    return catalog;
}

static SceneChoiceCatalog* createSceneImageChoiceCatalog(
    EffectControl& option, const SceneImageCatalog& images) {
    SceneImageChoiceCatalog* catalog = new SceneImageChoiceCatalog(
        option.name(), new LegacySceneChoiceLock(option.lock));

    for (int i = 0; i < images.entryCount(); i++)
        catalog->addChoice(images.nameAt(i), images.imageAt(i),
            images.inUseAt(i));

    return catalog;
}

}

std::unique_ptr<LegacySceneSelectionAdapterSet>
createLegacySceneSelectionAdapters(
    EffectControl& flame, EffectControl& generalFlame, EffectControl& wave,
    EffectControl& waveScale, EffectControl& table, EffectControl& object,
    EffectControl& translation, EffectControl& palette, EffectControl& border,
    EffectControl& flashlight, EffectControl& images,
    const SceneWaveObjectCatalog& waveObjects,
    const SceneImageCatalog& imageCatalog,
    const ScenePaletteCatalog& paletteCatalog,
    const SceneTranslationCatalog& translations) {
    return createLegacySceneSelectionAdapters(flame, generalFlame, wave,
        waveScale, table, object, translation, palette, border, flashlight,
        images,
        std::unique_ptr<SceneVisualSelections>(new SceneVisualSelectionSet(
            new SceneFlameChoiceSelection(
                createSceneFlameChoiceCatalog(flame), int(flame)),
            new SceneGeneralFlameSelectionValue(generalFlame.name(),
                new LegacySceneChoiceLock(generalFlame.lock),
                int(generalFlame)),
            new SceneWaveChoiceSelection(
                createSceneWaveChoiceCatalog(wave), int(wave)),
            new SceneChoiceSelection(
                createSceneWaveScaleChoiceCatalog(waveScale.name(),
                    new LegacySceneChoiceLock(waveScale.lock)),
                int(waveScale)),
            new SceneChoiceSelection(
                createSceneTableChoiceCatalog(table.name(),
                    new LegacySceneChoiceLock(table.lock)),
                int(table)),
            new SceneWaveObjectChoiceSelection(
                createSceneWaveObjectChoiceCatalog(object, waveObjects),
                int(object)),
            new SceneTranslationChoiceSelection(
                createSceneTranslationChoiceCatalog(translation, translations),
                int(translation)),
            new ScenePaletteChoiceSelection(
                createScenePaletteChoiceCatalog(palette, paletteCatalog),
                int(palette)),
            new SceneChoiceSelection(
                createSceneBorderChoiceCatalog(border.name(),
                    new LegacySceneChoiceLock(border.lock)),
                int(border)),
            new SceneChoiceSelection(
                createSceneFlashlightChoiceCatalog(flashlight.name(),
                    new LegacySceneChoiceLock(flashlight.lock)),
                int(flashlight)),
            new SceneImageChoiceSelection(
                createSceneImageChoiceCatalog(images, imageCatalog),
                int(images)))));
}
