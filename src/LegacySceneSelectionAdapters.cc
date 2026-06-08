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

class LegacySceneEffectChoiceSelection : public SceneChoiceSelection {
protected:
    EffectChoice* currentEffectChoice();
    const EffectChoice* currentEffectChoice() const;

public:
    LegacySceneEffectChoiceSelection(
        SceneChoiceCatalog* catalog, int selectedValue);
};

class LegacySceneControlBackedSelection
    : public LegacySceneEffectChoiceSelection {
    EffectControl& option;

protected:
    virtual void selectionChanged();
    virtual void syncSelectedValue(int value);

public:
    explicit LegacySceneControlBackedSelection(EffectControl& option_);

    int isOption(const EffectControl& option_) const;
    void syncFromControl();
    void syncControlFromSelection();
};

class LegacySceneFlameSelection : public LegacySceneEffectChoiceSelection,
    public SceneFlameSelection {
public:
    LegacySceneFlameSelection(SceneChoiceCatalog* catalog, int selectedValue);

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

class LegacySceneWaveSelection : public LegacySceneEffectChoiceSelection,
    public SceneWaveSelection {
public:
    LegacySceneWaveSelection(SceneChoiceCatalog* catalog, int selectedValue);

    virtual Wave* currentWave();
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
    EffectControl& waveControl;
    EffectControl& waveScaleControl;
    EffectControl& tableControl;
    EffectControl& objectControl;
    EffectControl& translationControl;
    EffectControl& paletteControl;
    EffectControl& borderControl;
    EffectControl& flashlightControl;
    EffectControl& imagesControl;
    LegacySceneFlameSelection flameValue;
    LegacySceneGeneralFlameSelection generalFlameValue;
    LegacySceneWaveSelection waveValue;
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

LegacySceneControlBackedSelection::LegacySceneControlBackedSelection(
    EffectControl& option_)
    : LegacySceneEffectChoiceSelection(
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

int LegacySceneControlBackedSelection::isOption(
    const EffectControl& option_) const {
    return &option_ == &option;
}

void LegacySceneControlBackedSelection::syncFromControl() {
    syncSelectedValue(int(option));
}

void LegacySceneControlBackedSelection::syncControlFromSelection() {
    option.setValue(currentValue());
}

LegacySceneFlameSelection::LegacySceneFlameSelection(
    SceneChoiceCatalog* catalog, int selectedValue)
    : LegacySceneEffectChoiceSelection(catalog, selectedValue) { }

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

LegacySceneWaveSelection::LegacySceneWaveSelection(
    SceneChoiceCatalog* catalog, int selectedValue)
    : LegacySceneEffectChoiceSelection(catalog, selectedValue) { }

Wave* LegacySceneWaveSelection::currentWave() {
    WaveEntry* entry = dynamic_cast<WaveEntry*>(currentEffectChoice());
    return (entry != 0) ? &entry->wave() : 0;
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
    , waveControl(wave_)
    , waveScaleControl(waveScale_)
    , tableControl(table_)
    , objectControl(object_)
    , translationControl(translation_)
    , paletteControl(palette_)
    , borderControl(border_)
    , flashlightControl(flashlight_)
    , imagesControl(images_)
    , flameValue(new SceneEffectChoiceCatalog(flame_.name(),
          flame_.choiceList(), flame_.lock), int(flame_))
    , generalFlameValue(generalFlame_)
    , waveValue(new SceneEffectChoiceCatalog(wave_.name(),
          wave_.choiceList(), wave_.lock), int(wave_))
    , waveScaleValue(new SceneEffectChoiceCatalog(waveScale_.name(),
          waveScale_.choiceList(), waveScale_.lock), int(waveScale_))
    , tableValue(new SceneEffectChoiceCatalog(table_.name(),
          table_.choiceList(), table_.lock), int(table_))
    , objectValue(new SceneEffectChoiceCatalog(object_.name(),
          object_.choiceList(), object_.lock), int(object_))
    , translationValue(new SceneEffectChoiceCatalog(translation_.name(),
          translation_.choiceList(), translation_.lock), int(translation_))
    , paletteValue(new SceneEffectChoiceCatalog(palette_.name(),
          palette_.choiceList(), palette_.lock), int(palette_))
    , borderValue(new SceneEffectChoiceCatalog(border_.name(),
          border_.choiceList(), border_.lock), int(border_))
    , flashlightValue(new SceneEffectChoiceCatalog(flashlight_.name(),
          flashlight_.choiceList(), flashlight_.lock), int(flashlight_))
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
    if (generalFlameValue.isOption(option))
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
    generalFlameValue.syncFromControl();
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
    generalFlameValue.syncControlFromSelection();
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
