// Legacy EffectControl-backed Scene visual selection adapters.

#include "LegacySceneSelectionAdapters.h"

#include "EffectControl.h"
#include "Image.h"
#include "LegacySceneEffectControlBindings.h"
#include "SceneChoiceListCatalog.h"
#include "SceneChoiceSelection.h"
#include "SceneEffectChoiceCatalog.h"
#include "SceneGeneralFlameSelectionValue.h"
#include "SceneTypedVisualCatalogs.h"
#include "TranslationOptions.h"
#include "display.h"
#include "flames.h"
#include "waves.h"

#include <memory>

namespace {

class LegacySceneEffectChoiceSelection : public SceneChoiceSelection {
protected:
    EffectChoice* currentEffectChoice();
    const EffectChoice* currentEffectChoice() const;

public:
    LegacySceneEffectChoiceSelection(
        SceneChoiceCatalog* catalog, int selectedValue);
};

class LegacySceneTranslationSelection : public LegacySceneEffectChoiceSelection,
    public SceneTranslationSelection {
public:
    LegacySceneTranslationSelection(
        SceneChoiceCatalog* catalog, int selectedValue);

    virtual TranslationTable currentTranslationTable();
};

class LegacyScenePaletteSelection : public LegacySceneEffectChoiceSelection,
    public ScenePaletteSelection {
public:
    LegacyScenePaletteSelection(SceneChoiceCatalog* catalog, int selectedValue);

    virtual PaletteEntry* currentPaletteEntry();
};

class LegacySceneImageSelection : public LegacySceneEffectChoiceSelection,
    public SceneImageSelection {
public:
    LegacySceneImageSelection(SceneChoiceCatalog* catalog, int selectedValue);

    virtual const IndexedImage* currentImage();
};

class LegacySceneSelectionAdapters : public SceneVisualSelections,
    public LegacySceneEffectControlBindings {
    EffectControl& flameControl;
    EffectControl& generalFlameControl;
    EffectControl& waveControl;
    EffectControl& waveScaleControl;
    EffectControl& tableControl;
    EffectControl& objectControl;
    EffectControl& translationControl;
    EffectControl& paletteControl;
    EffectControl& borderControl;
    EffectControl& flashlightControl;
    EffectControl& imagesControl;
    SceneFlameChoiceSelection flameValue;
    SceneGeneralFlameSelectionValue generalFlameValue;
    SceneWaveChoiceSelection waveValue;
    SceneChoiceSelection waveScaleValue;
    SceneChoiceSelection tableValue;
    SceneChoiceSelection objectValue;
    LegacySceneTranslationSelection translationValue;
    LegacyScenePaletteSelection paletteValue;
    SceneChoiceSelection borderValue;
    SceneChoiceSelection flashlightValue;
    LegacySceneImageSelection imagesValue;

public:
    LegacySceneSelectionAdapters(EffectControl& flame_,
        EffectControl& generalFlame_, EffectControl& wave_,
        EffectControl& waveScale_, EffectControl& table_,
        EffectControl& object_, EffectControl& translation_,
        EffectControl& palette_, EffectControl& border_,
        EffectControl& flashlight_, EffectControl& images_);

    virtual SceneFlameSelection& flame();
    virtual SceneGeneralFlameSelection& generalFlame();
    virtual SceneWaveSelection& wave();
    virtual SceneOptionSelection& waveScale();
    virtual SceneOptionSelection& table();
    virtual SceneOptionSelection& object();
    virtual SceneTranslationSelection& translation();
    virtual ScenePaletteSelection& palette();
    virtual SceneOptionSelection& border();
    virtual SceneOptionSelection& flashlight();
    virtual SceneImageSelection& images();

    virtual SceneOptionSelection* selectionFor(EffectControl& option);
    virtual const SceneOptionSelection* selectionFor(
        const EffectControl& option) const;
    virtual void syncFromControls();
    virtual void syncControlsFromSelections();
};

static void addLegacyChoiceAliases(
    SceneChoiceListEntry& entry, EffectChoice& choice) {
    if (choice.sameName("yes"))
        entry.addAlias("yes");
    if (choice.sameName("no"))
        entry.addAlias("no");
    if (choice.sameName("1"))
        entry.addAlias("1");
    if (choice.sameName("0"))
        entry.addAlias("0");
}

static SceneChoiceCatalog* createOwnedSceneChoiceCatalog(
    EffectControl& option) {
    SceneChoiceListCatalog* catalog = new SceneChoiceListCatalog(
        option.name(), new SceneEffectChoiceLock(option.lock));

    int nEntries = option.getNEntries();
    for (int i = 0; i < nEntries; i++) {
        EffectChoice* choice = option[i];
        if (choice != 0) {
            SceneChoiceListEntry& entry
                = catalog->addChoice(choice->Name(), choice->inUse());
            addLegacyChoiceAliases(entry, *choice);
        }
    }

    return catalog;
}

static int legacyChoiceInUse(EffectControl& option, int index, int defaultUse) {
    EffectChoice* choice = option[index];
    return (choice != 0) ? choice->inUse() : defaultUse;
}

static SceneChoiceCatalog* createSceneFlameChoiceCatalog(
    EffectControl& option) {
    SceneFlameChoiceCatalog* catalog = new SceneFlameChoiceCatalog(
        option.name(), new SceneEffectChoiceLock(option.lock));

    for (int i = 0; i < nFlameCatalogEntries; i++) {
        const Flame* flame = flameByIndex(i);
        if (flame != 0)
            catalog->addChoice(
                flame, flame->name(), legacyChoiceInUse(option, i, i != 0));
    }

    return catalog;
}

static SceneChoiceCatalog* createSceneWaveChoiceCatalog(
    EffectControl& option) {
    SceneWaveChoiceCatalog* catalog = new SceneWaveChoiceCatalog(
        option.name(), new SceneEffectChoiceLock(option.lock));

    for (int i = 0; i < nWaveCatalogEntries; i++) {
        Wave* wave = waveByIndex(i);
        if (wave != 0)
            catalog->addChoice(
                wave, wave->name(), legacyChoiceInUse(option, i, i < 33));
    }

    return catalog;
}

LegacySceneEffectChoiceSelection::LegacySceneEffectChoiceSelection(
    SceneChoiceCatalog* catalog, int selectedValue)
    : SceneChoiceSelection(catalog, selectedValue) { }

EffectChoice* LegacySceneEffectChoiceSelection::currentEffectChoice() {
    SceneEffectChoice* choice
        = dynamic_cast<SceneEffectChoice*>(currentChoice());
    return (choice != 0) ? &choice->effectChoice() : 0;
}

const EffectChoice* LegacySceneEffectChoiceSelection::currentEffectChoice()
    const {
    const SceneEffectChoice* choice
        = dynamic_cast<const SceneEffectChoice*>(currentChoice());
    return (choice != 0) ? &choice->effectChoice() : 0;
}

LegacySceneTranslationSelection::LegacySceneTranslationSelection(
    SceneChoiceCatalog* catalog, int selectedValue)
    : LegacySceneEffectChoiceSelection(catalog, selectedValue) { }

TranslationTable LegacySceneTranslationSelection::currentTranslationTable() {
    TranslateEntry* entry = dynamic_cast<TranslateEntry*>(currentEffectChoice());
    return (entry != 0) ? entry->table() : TranslationTable();
}

LegacyScenePaletteSelection::LegacyScenePaletteSelection(
    SceneChoiceCatalog* catalog, int selectedValue)
    : LegacySceneEffectChoiceSelection(catalog, selectedValue) { }

PaletteEntry* LegacyScenePaletteSelection::currentPaletteEntry() {
    return dynamic_cast<PaletteEntry*>(currentEffectChoice());
}

LegacySceneImageSelection::LegacySceneImageSelection(
    SceneChoiceCatalog* catalog, int selectedValue)
    : LegacySceneEffectChoiceSelection(catalog, selectedValue) { }

const IndexedImage* LegacySceneImageSelection::currentImage() {
    ImageEntry* entry = dynamic_cast<ImageEntry*>(currentEffectChoice());
    return (entry != 0) ? entry->image() : 0;
}

LegacySceneSelectionAdapters::LegacySceneSelectionAdapters(EffectControl& flame_,
    EffectControl& generalFlame_, EffectControl& wave_,
    EffectControl& waveScale_, EffectControl& table_, EffectControl& object_,
    EffectControl& translation_, EffectControl& palette_,
    EffectControl& border_, EffectControl& flashlight_, EffectControl& images_)
    : flameControl(flame_)
    , generalFlameControl(generalFlame_)
    , waveControl(wave_)
    , waveScaleControl(waveScale_)
    , tableControl(table_)
    , objectControl(object_)
    , translationControl(translation_)
    , paletteControl(palette_)
    , borderControl(border_)
    , flashlightControl(flashlight_)
    , imagesControl(images_)
    , flameValue(createSceneFlameChoiceCatalog(flame_), int(flame_))
    , generalFlameValue(generalFlame_.name(),
          new SceneEffectChoiceLock(generalFlame_.lock), int(generalFlame_))
    , waveValue(createSceneWaveChoiceCatalog(wave_), int(wave_))
    , waveScaleValue(createOwnedSceneChoiceCatalog(waveScale_),
          int(waveScale_))
    , tableValue(createOwnedSceneChoiceCatalog(table_), int(table_))
    , objectValue(createOwnedSceneChoiceCatalog(object_), int(object_))
    , translationValue(new SceneEffectChoiceCatalog(translation_.name(),
          translation_.choiceList(), translation_.lock), int(translation_))
    , paletteValue(new SceneEffectChoiceCatalog(palette_.name(),
          palette_.choiceList(), palette_.lock), int(palette_))
    , borderValue(createOwnedSceneChoiceCatalog(border_), int(border_))
    , flashlightValue(createOwnedSceneChoiceCatalog(flashlight_),
          int(flashlight_))
    , imagesValue(new SceneEffectChoiceCatalog(images_.name(),
          images_.choiceList(), images_.lock), int(images_)) { }

SceneFlameSelection& LegacySceneSelectionAdapters::flame() {
    return flameValue;
}

SceneGeneralFlameSelection& LegacySceneSelectionAdapters::generalFlame() {
    return generalFlameValue;
}

SceneWaveSelection& LegacySceneSelectionAdapters::wave() {
    return waveValue;
}

SceneOptionSelection& LegacySceneSelectionAdapters::waveScale() {
    return waveScaleValue;
}

SceneOptionSelection& LegacySceneSelectionAdapters::table() {
    return tableValue;
}

SceneOptionSelection& LegacySceneSelectionAdapters::object() {
    return objectValue;
}

SceneTranslationSelection& LegacySceneSelectionAdapters::translation() {
    return translationValue;
}

ScenePaletteSelection& LegacySceneSelectionAdapters::palette() {
    return paletteValue;
}

SceneOptionSelection& LegacySceneSelectionAdapters::border() {
    return borderValue;
}

SceneOptionSelection& LegacySceneSelectionAdapters::flashlight() {
    return flashlightValue;
}

SceneImageSelection& LegacySceneSelectionAdapters::images() {
    return imagesValue;
}

SceneOptionSelection* LegacySceneSelectionAdapters::selectionFor(
    EffectControl& option) {
    return const_cast<SceneOptionSelection*>(
        static_cast<const LegacySceneSelectionAdapters*>(this)->selectionFor(
            option));
}

const SceneOptionSelection* LegacySceneSelectionAdapters::selectionFor(
    const EffectControl& option) const {
    if (&option == &flameControl)
        return &flameValue;
    if (&option == &generalFlameControl)
        return &generalFlameValue;
    if (&option == &waveControl)
        return &waveValue;
    if (&option == &waveScaleControl)
        return &waveScaleValue;
    if (&option == &objectControl)
        return &objectValue;
    if (&option == &translationControl)
        return &translationValue;
    if (&option == &borderControl)
        return &borderValue;
    if (&option == &flashlightControl)
        return &flashlightValue;
    if (&option == &paletteControl)
        return &paletteValue;
    if (&option == &tableControl)
        return &tableValue;
    if (&option == &imagesControl)
        return &imagesValue;

    return 0;
}

void LegacySceneSelectionAdapters::syncFromControls() {
    flameValue.setValue(int(flameControl));
    generalFlameValue.setValue(int(generalFlameControl));
    waveValue.setValue(int(waveControl));
    waveScaleValue.setValue(int(waveScaleControl));
    tableValue.setValue(int(tableControl));
    objectValue.setValue(int(objectControl));
    translationValue.setValue(int(translationControl));
    paletteValue.setValue(int(paletteControl));
    borderValue.setValue(int(borderControl));
    flashlightValue.setValue(int(flashlightControl));
    imagesValue.setValue(int(imagesControl));
}

void LegacySceneSelectionAdapters::syncControlsFromSelections() {
    flameControl.setValue(flameValue.currentValue());
    generalFlameControl.setValue(generalFlameValue.currentValue());
    waveControl.setValue(waveValue.currentValue());
    waveScaleControl.setValue(waveScaleValue.currentValue());
    tableControl.setValue(tableValue.currentValue());
    objectControl.setValue(objectValue.currentValue());
    translationControl.setValue(translationValue.currentValue());
    paletteControl.setValue(paletteValue.currentValue());
    borderControl.setValue(borderValue.currentValue());
    flashlightControl.setValue(flashlightValue.currentValue());
    imagesControl.setValue(imagesValue.currentValue());
}

}

LegacySceneEffectControlBindings* legacySceneEffectControlBindings(
    SceneVisualSelections& selections) {
    return dynamic_cast<LegacySceneEffectControlBindings*>(&selections);
}

const LegacySceneEffectControlBindings* legacySceneEffectControlBindings(
    const SceneVisualSelections& selections) {
    return dynamic_cast<const LegacySceneEffectControlBindings*>(&selections);
}

std::unique_ptr<SceneVisualSelections> createLegacySceneSelectionAdapters(
    EffectControl& flame, EffectControl& generalFlame, EffectControl& wave,
    EffectControl& waveScale, EffectControl& table, EffectControl& object,
    EffectControl& translation, EffectControl& palette, EffectControl& border,
    EffectControl& flashlight, EffectControl& images) {
    return std::unique_ptr<SceneVisualSelections>(
        new LegacySceneSelectionAdapters(flame, generalFlame, wave, waveScale,
            table, object, translation, palette, border, flashlight, images));
}
