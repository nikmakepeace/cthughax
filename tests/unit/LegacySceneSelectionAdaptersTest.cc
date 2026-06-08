#include "LegacySceneSelectionAdapters.h"

#include "EffectControl.h"
#include "SceneDependencies.h"
#include "SceneVisualSelections.h"

#include <cassert>
#include <memory>

std::unique_ptr<LegacySceneSelectionAdapterSet>
createLegacySceneSelectionAdapters(
    EffectControl& flame, EffectControl& generalFlame, EffectControl& wave,
    EffectControl& waveScale, EffectControl& table, EffectControl& object,
    EffectControl& translation, EffectControl& palette, EffectControl& border,
    EffectControl& flashlight, EffectControl& images,
    std::unique_ptr<SceneVisualSelections> selections);

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }

class StubSceneSelection : public SceneFlameSelection,
                           public SceneGeneralFlameSelection,
                           public SceneWaveSelection,
                           public SceneWaveObjectSelection,
                           public SceneTranslationSelection,
                           public ScenePaletteSelection,
                           public SceneImageSelection {
    int currentValue_;
    int lockEnabled_;

public:
    StubSceneSelection(int currentValue, int lockEnabled)
        : currentValue_(currentValue)
        , lockEnabled_(lockEnabled) { }

    virtual const char* currentName() const { return "stub"; }
    virtual int currentValue() const { return currentValue_; }
    virtual int entryCount() const { return 1; }
    virtual void change(int) { }
    virtual void change(const char*, RandomSource&) { }
    virtual int changeRandom(RandomSource&) { return 0; }
    virtual int lockEnabled() const { return lockEnabled_; }
    virtual void setValue(int index) { currentValue_ = index; }
    virtual const Flame* currentFlame() { return 0; }
    virtual int encodedValue() const { return currentValue_; }
    virtual const char* selectionText() const { return "stub"; }
    virtual Wave* currentWave() { return 0; }
    virtual WObject* currentObject() { return 0; }
    virtual TranslationTable currentTranslationTable() {
        return TranslationTable();
    }
    virtual PaletteEntry* currentPaletteEntry() { return 0; }
    virtual const IndexedImage* currentImage() { return 0; }
};

class StubSceneVisualSelections : public SceneVisualSelections {
    StubSceneSelection flame_;
    StubSceneSelection generalFlame_;
    StubSceneSelection wave_;
    StubSceneSelection waveScale_;
    StubSceneSelection table_;
    StubSceneSelection object_;
    StubSceneSelection translation_;
    StubSceneSelection palette_;
    StubSceneSelection border_;
    StubSceneSelection flashlight_;
    StubSceneSelection images_;

public:
    StubSceneVisualSelections()
        : flame_(1, 1)
        , generalFlame_(2, 0)
        , wave_(3, 1)
        , waveScale_(4, 0)
        , table_(5, 1)
        , object_(6, 0)
        , translation_(7, 1)
        , palette_(8, 0)
        , border_(9, 1)
        , flashlight_(10, 0)
        , images_(11, 1) { }

    virtual SceneFlameSelection& flame() { return flame_; }
    virtual SceneGeneralFlameSelection& generalFlame() {
        return generalFlame_;
    }
    virtual SceneWaveSelection& wave() { return wave_; }
    virtual SceneOptionSelection& waveScale() { return waveScale_; }
    virtual SceneOptionSelection& table() { return table_; }
    virtual SceneOptionSelection& object() { return object_; }
    virtual SceneTranslationSelection& translation() { return translation_; }
    virtual ScenePaletteSelection& palette() { return palette_; }
    virtual SceneOptionSelection& border() { return border_; }
    virtual SceneOptionSelection& flashlight() { return flashlight_; }
    virtual SceneImageSelection& images() { return images_; }
};

static void assertControlState(
    const EffectControl& control, int value, int lockValue) {
    assert(int(control) == value);
    assert(int(control.lock) == lockValue);
}

static void testMirrorCopiesSelectionValuesAndLocks() {
    EffectChoiceList choices;
    EffectControl flame(0, "flame", choices);
    EffectControl generalFlame(0, "flame-general", choices);
    EffectControl wave(0, "wave", choices);
    EffectControl waveScale(0, "wave-scale", choices);
    EffectControl table(0, "table", choices);
    EffectControl object(0, "object", choices);
    EffectControl translation(0, "translation", choices);
    EffectControl palette(0, "palette", choices);
    EffectControl border(0, "border", choices);
    EffectControl flashlight(0, "flashlight", choices);
    EffectControl images(0, "images", choices);

    std::unique_ptr<LegacySceneSelectionAdapterSet> adapters
        = createLegacySceneSelectionAdapters(flame, generalFlame, wave,
            waveScale, table, object, translation, palette, border,
            flashlight, images,
            std::unique_ptr<SceneVisualSelections>(
                new StubSceneVisualSelections()));

    std::unique_ptr<SceneSelectionSynchronizer> synchronizer
        = adapters->createSelectionSynchronizer();

    synchronizer->syncControlsFromSelections();

    assertControlState(flame, 1, 1);
    assertControlState(generalFlame, 2, 0);
    assertControlState(wave, 3, 1);
    assertControlState(waveScale, 4, 0);
    assertControlState(table, 5, 1);
    assertControlState(object, 6, 0);
    assertControlState(translation, 7, 1);
    assertControlState(palette, 8, 0);
    assertControlState(border, 9, 1);
    assertControlState(flashlight, 10, 0);
    assertControlState(images, 11, 1);
}

int main() {
    testMirrorCopiesSelectionValuesAndLocks();
    return 0;
}
