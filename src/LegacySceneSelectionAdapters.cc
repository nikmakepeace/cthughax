// Legacy EffectControl-backed Scene visual selection adapters.

#include "LegacySceneSelectionAdapters.h"

#include "EffectControl.h"
#include "Image.h"
#include "LegacySceneEffectControlBindings.h"
#include "ProcessServices.h"
#include "SceneChoiceSelection.h"
#include "SceneEffectChoiceCatalog.h"
#include "TranslationOptions.h"
#include "display.h"
#include "flames.h"
#include "waves.h"

#include <cstdlib>
#include <memory>

namespace {

static const int generalFlameStates = 9 * 9 * 9 * 9 * 9;

static int modInt(int value, int modulo) {
    int result = value % modulo;
    return result < 0 ? result + modulo : result;
}

class LegacySceneControlBackedSelection : public SceneChoiceSelection {
    EffectControl& option;

protected:
    virtual void selectionChanged();
    virtual void syncSelectedValue(int value);
    EffectChoice* currentEffectChoice();
    const EffectChoice* currentEffectChoice() const;

public:
    explicit LegacySceneControlBackedSelection(EffectControl& option_);

    int isOption(const EffectControl& option_) const;
    void syncFromControl();
};

class LegacySceneFlameSelection : public LegacySceneControlBackedSelection,
    public SceneFlameSelection {
public:
    explicit LegacySceneFlameSelection(EffectControl& flameOption_);

    virtual const Flame* currentFlame();
};

class LegacySceneGeneralFlameSelection : public LegacySceneControlBackedSelection,
    public SceneGeneralFlameSelection {
protected:
    virtual void syncSelectedValue(int value);

public:
    explicit LegacySceneGeneralFlameSelection(
        EffectControl& generalFlameControl_);

    virtual const char* currentName() const;
    virtual int currentValue() const;
    virtual int entryCount() const;
    virtual void change(int by);
    virtual void change(
        const char* to, RandomSource& randomSource);
    virtual void setValue(int index);
    virtual int encodedValue() const;
    virtual const char* selectionText() const;
    virtual int changeRandom(RandomSource& randomSource);
};

class LegacySceneWaveSelection : public LegacySceneControlBackedSelection,
    public SceneWaveSelection {
public:
    explicit LegacySceneWaveSelection(EffectControl& waveOption_);

    virtual Wave* currentWave();
};

class LegacySceneTranslationSelection : public LegacySceneControlBackedSelection,
    public SceneTranslationSelection {
public:
    explicit LegacySceneTranslationSelection(
        EffectControl& translationOption_);

    virtual TranslationTable currentTranslationTable();
};

class LegacyScenePaletteSelection : public LegacySceneControlBackedSelection,
    public ScenePaletteSelection {
public:
    explicit LegacyScenePaletteSelection(EffectControl& paletteOption_);

    virtual PaletteEntry* currentPaletteEntry();
};

class LegacySceneImageSelection : public LegacySceneControlBackedSelection,
    public SceneImageSelection {
public:
    explicit LegacySceneImageSelection(EffectControl& imageOption_);

    virtual const IndexedImage* currentImage();
};

class LegacySceneSelectionAdapters : public SceneVisualSelections,
    public LegacySceneEffectControlBindings {
    LegacySceneFlameSelection flameValue;
    LegacySceneGeneralFlameSelection generalFlameValue;
    LegacySceneWaveSelection waveValue;
    LegacySceneControlBackedSelection waveScaleValue;
    LegacySceneControlBackedSelection tableValue;
    LegacySceneControlBackedSelection objectValue;
    LegacySceneTranslationSelection translationValue;
    LegacyScenePaletteSelection paletteValue;
    LegacySceneControlBackedSelection borderValue;
    LegacySceneControlBackedSelection flashlightValue;
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
};

LegacySceneControlBackedSelection::LegacySceneControlBackedSelection(
    EffectControl& option_)
    : SceneChoiceSelection(
          new SceneEffectChoiceCatalog(option_.name(), option_.choiceList(),
              option_.lock),
          int(option_))
    , option(option_) { }

void LegacySceneControlBackedSelection::selectionChanged() {
    option.setValue(currentValue());
}

void LegacySceneControlBackedSelection::syncSelectedValue(int value) {
    setSelectedValue(value);
}

EffectChoice* LegacySceneControlBackedSelection::currentEffectChoice() {
    SceneEffectChoice* choice
        = dynamic_cast<SceneEffectChoice*>(currentChoice());
    return (choice != 0) ? &choice->effectChoice() : 0;
}

const EffectChoice* LegacySceneControlBackedSelection::currentEffectChoice()
    const {
    const SceneEffectChoice* choice
        = dynamic_cast<const SceneEffectChoice*>(currentChoice());
    return (choice != 0) ? &choice->effectChoice() : 0;
}

int LegacySceneControlBackedSelection::isOption(
    const EffectControl& option_) const {
    return &option_ == &option;
}

void LegacySceneControlBackedSelection::syncFromControl() {
    syncSelectedValue(int(option));
}

LegacySceneFlameSelection::LegacySceneFlameSelection(
    EffectControl& flameOption_)
    : LegacySceneControlBackedSelection(flameOption_) { }

const Flame* LegacySceneFlameSelection::currentFlame() {
    FlameEntry* entry = dynamic_cast<FlameEntry*>(currentEffectChoice());
    return (entry != 0) ? &entry->flame() : 0;
}

LegacySceneGeneralFlameSelection::LegacySceneGeneralFlameSelection(
    EffectControl& generalFlameControl_)
    : LegacySceneControlBackedSelection(generalFlameControl_) { }

void LegacySceneGeneralFlameSelection::syncSelectedValue(int value) {
    setSelectedValue(modInt(value, generalFlameStates));
}

const char* LegacySceneGeneralFlameSelection::currentName() const {
    return selectionText();
}

int LegacySceneGeneralFlameSelection::currentValue() const {
    return encodedValue();
}

int LegacySceneGeneralFlameSelection::entryCount() const {
    return generalFlameStates;
}

void LegacySceneGeneralFlameSelection::change(int by) {
    setValue(modInt(encodedValue() + by, generalFlameStates));
}

void LegacySceneGeneralFlameSelection::change(
    const char* to, RandomSource& randomSource) {
    char* pos;

    if ((to == 0) || (to[0] == '\0'))
        return;

    to = applySelectionLockPrefix(to);

    int newValue = std::strtol(to, &pos, 0);
    if (pos == to) {
        CTH_WARN("Unknown entry `%s' for option `%s'\n", to,
            optionName());
        changeRandom(randomSource);
        return;
    }

    setValue(modInt(newValue, generalFlameStates));
}

void LegacySceneGeneralFlameSelection::setValue(int index) {
    setSelectedValue(modInt(index, generalFlameStates));
    selectionChanged();
}

int LegacySceneGeneralFlameSelection::encodedValue() const {
    return SceneChoiceSelection::currentValue();
}

const char* LegacySceneGeneralFlameSelection::selectionText() const {
    static char str[32];

    if (selectionLock().enabled())
        std::snprintf(str, sizeof(str), "locked:%d", encodedValue());
    else
        std::snprintf(str, sizeof(str), "%d", encodedValue());

    return str;
}

int LegacySceneGeneralFlameSelection::changeRandom(
    RandomSource& randomSource) {
    if (selectionLock().enabled())
        return 0;

    int previousValue = encodedValue();
    setValue(randomSource.uniformInt(generalFlameStates));
    return encodedValue() != previousValue;
}

LegacySceneWaveSelection::LegacySceneWaveSelection(EffectControl& waveOption_)
    : LegacySceneControlBackedSelection(waveOption_) { }

Wave* LegacySceneWaveSelection::currentWave() {
    WaveEntry* entry = dynamic_cast<WaveEntry*>(currentEffectChoice());
    return (entry != 0) ? &entry->wave() : 0;
}

LegacySceneTranslationSelection::LegacySceneTranslationSelection(
    EffectControl& translationOption_)
    : LegacySceneControlBackedSelection(translationOption_) { }

TranslationTable LegacySceneTranslationSelection::currentTranslationTable() {
    TranslateEntry* entry = dynamic_cast<TranslateEntry*>(currentEffectChoice());
    return (entry != 0) ? entry->table() : TranslationTable();
}

LegacyScenePaletteSelection::LegacyScenePaletteSelection(
    EffectControl& paletteOption_)
    : LegacySceneControlBackedSelection(paletteOption_) { }

PaletteEntry* LegacyScenePaletteSelection::currentPaletteEntry() {
    return dynamic_cast<PaletteEntry*>(currentEffectChoice());
}

LegacySceneImageSelection::LegacySceneImageSelection(EffectControl& imageOption_)
    : LegacySceneControlBackedSelection(imageOption_) { }

const IndexedImage* LegacySceneImageSelection::currentImage() {
    ImageEntry* entry = dynamic_cast<ImageEntry*>(currentEffectChoice());
    return (entry != 0) ? entry->image() : 0;
}

LegacySceneSelectionAdapters::LegacySceneSelectionAdapters(EffectControl& flame_,
    EffectControl& generalFlame_, EffectControl& wave_,
    EffectControl& waveScale_, EffectControl& table_, EffectControl& object_,
    EffectControl& translation_, EffectControl& palette_,
    EffectControl& border_, EffectControl& flashlight_, EffectControl& images_)
    : flameValue(flame_)
    , generalFlameValue(generalFlame_)
    , waveValue(wave_)
    , waveScaleValue(waveScale_)
    , tableValue(table_)
    , objectValue(object_)
    , translationValue(translation_)
    , paletteValue(palette_)
    , borderValue(border_)
    , flashlightValue(flashlight_)
    , imagesValue(images_) { }

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
    if (flameValue.isOption(option))
        return &flameValue;
    if (generalFlameValue.isOption(option))
        return &generalFlameValue;
    if (waveValue.isOption(option))
        return &waveValue;
    if (waveScaleValue.isOption(option))
        return &waveScaleValue;
    if (objectValue.isOption(option))
        return &objectValue;
    if (translationValue.isOption(option))
        return &translationValue;
    if (borderValue.isOption(option))
        return &borderValue;
    if (flashlightValue.isOption(option))
        return &flashlightValue;
    if (paletteValue.isOption(option))
        return &paletteValue;
    if (tableValue.isOption(option))
        return &tableValue;
    if (imagesValue.isOption(option))
        return &imagesValue;

    return 0;
}

void LegacySceneSelectionAdapters::syncFromControls() {
    flameValue.syncFromControl();
    generalFlameValue.syncFromControl();
    waveValue.syncFromControl();
    waveScaleValue.syncFromControl();
    tableValue.syncFromControl();
    objectValue.syncFromControl();
    translationValue.syncFromControl();
    paletteValue.syncFromControl();
    borderValue.syncFromControl();
    flashlightValue.syncFromControl();
    imagesValue.syncFromControl();
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
