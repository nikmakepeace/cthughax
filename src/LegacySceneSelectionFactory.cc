// Legacy visual selection construction over global visual options.

#include "LegacySceneSelectionAdapters.h"

#include "EffectControl.h"
#include "Image.h"
#include "LegacySceneChoiceLock.h"
#include "SceneBuiltInChoiceCatalogs.h"
#include "SceneChoiceSelection.h"
#include "SceneGeneralFlameSelectionValue.h"
#include "SceneTranslationCatalog.h"
#include "SceneTypedVisualCatalogs.h"
#include "SceneVisualSelectionSet.h"
#include "WaveObject.h"
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
    EffectControl& option) {
    SceneWaveObjectChoiceCatalog* catalog = new SceneWaveObjectChoiceCatalog(
        option.name(), new LegacySceneChoiceLock(option.lock));

    for (int i = 0; i < option.getNEntries(); i++) {
        EffectChoice* choice = option[i];
        if (choice != 0)
            catalog->addChoice(choice->Name(), waveObjectEntryObject(choice),
                choice->inUse());
    }

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
    EffectControl& option) {
    ScenePaletteChoiceCatalog* catalog = new ScenePaletteChoiceCatalog(
        option.name(), new LegacySceneChoiceLock(option.lock));

    for (int i = 0; i < option.getNEntries(); i++) {
        PaletteEntry* entry = dynamic_cast<PaletteEntry*>(option[i]);
        if (entry != 0)
            catalog->addChoice(*entry, entry->inUse());
    }

    return catalog;
}

static SceneChoiceCatalog* createSceneImageChoiceCatalog(EffectControl& option) {
    SceneImageChoiceCatalog* catalog = new SceneImageChoiceCatalog(
        option.name(), new LegacySceneChoiceLock(option.lock));

    for (int i = 0; i < option.getNEntries(); i++) {
        ImageEntry* entry = dynamic_cast<ImageEntry*>(option[i]);
        if (entry != 0)
            catalog->addChoice(entry->Name(), entry->image(),
                entry->inUse());
    }

    return catalog;
}

}

std::unique_ptr<LegacySceneSelectionAdapterSet>
createLegacySceneSelectionAdapters(
    EffectControl& flame, EffectControl& generalFlame, EffectControl& wave,
    EffectControl& waveScale, EffectControl& table, EffectControl& object,
    EffectControl& translation, EffectControl& palette, EffectControl& border,
    EffectControl& flashlight, EffectControl& images,
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
                createSceneWaveObjectChoiceCatalog(object), int(object)),
            new SceneTranslationChoiceSelection(
                createSceneTranslationChoiceCatalog(translation, translations),
                int(translation)),
            new ScenePaletteChoiceSelection(
                createScenePaletteChoiceCatalog(palette), int(palette)),
            new SceneChoiceSelection(
                createSceneBorderChoiceCatalog(border.name(),
                    new LegacySceneChoiceLock(border.lock)),
                int(border)),
            new SceneChoiceSelection(
                createSceneFlashlightChoiceCatalog(flashlight.name(),
                    new LegacySceneChoiceLock(flashlight.lock)),
                int(flashlight)),
            new SceneImageChoiceSelection(
                createSceneImageChoiceCatalog(images), int(images)))));
}
