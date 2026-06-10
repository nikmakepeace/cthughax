/** @file
 * Unit coverage for control state and catalog snapshots.
 */

#include "ControlSnapshot.h"

#include "ControlDisplayCatalogs.h"
#include "RuntimeConfigRegistry.h"
#include "SceneChoiceSelection.h"

#include <assert.h>
#include <string>
#include <vector>

int cth_log_enabled(int) {
    return 0;
}

int cth_log(int, const char*, ...) {
    return 0;
}

int cth_log_error(const char*, ...) {
    return 0;
}

class FakeChoice : public SceneChoice {
    std::string nameValue;
    int inUseValue;

public:
    FakeChoice(const char* name_, int inUse_)
        : nameValue(name_)
        , inUseValue(inUse_) { }

    virtual const char* name() const { return nameValue.c_str(); }
    virtual int sameName(const char* other) const {
        return nameValue == (other != 0 ? other : "");
    }
    virtual int inUse() const { return inUseValue; }
    virtual void setUse(int inUse) { inUseValue = inUse; }
};

class FakeSelection : public SceneFlameSelection,
                      public SceneGeneralFlameSelection,
                      public SceneWaveSelection,
                      public SceneWaveObjectSelection,
                      public SceneTranslationSelection,
                      public ScenePaletteSelection,
                      public SceneImageSelection {
    std::vector<FakeChoice> choices;
    int currentValueValue;

public:
    FakeSelection()
        : choices()
        , currentValueValue(0) {
        choices.push_back(FakeChoice("first", 1));
        choices.push_back(FakeChoice("second", 0));
    }

    virtual const char* currentName() const {
        return choices[currentValueValue].name();
    }
    virtual int currentValue() const { return currentValueValue; }
    virtual int entryCount() const { return int(choices.size()); }
    virtual int choiceCount() const { return int(choices.size()); }
    virtual SceneChoice* choiceAt(int index) {
        if (index < 0 || index >= int(choices.size()))
            return 0;
        return &choices[index];
    }
    virtual const SceneChoice* choiceAt(int index) const {
        if (index < 0 || index >= int(choices.size()))
            return 0;
        return &choices[index];
    }
    virtual void change(int by) {
        currentValueValue = (currentValueValue + by) % int(choices.size());
    }
    virtual void change(const char*, RandomSource&) { currentValueValue = 1; }
    virtual int changeRandom(RandomSource&) { currentValueValue = 0; return 1; }
    virtual void setValue(int index) { currentValueValue = index; }
    virtual const Flame* currentFlame() { return 0; }
    virtual int encodedValue() const { return currentValueValue; }
    virtual const char* selectionText() const { return currentName(); }
    virtual Wave* currentWave() { return 0; }
    virtual WObject* currentObject() { return 0; }
    virtual TranslationTable currentTranslationTable() {
        return TranslationTable();
    }
    virtual PaletteEntry* currentPaletteEntry() { return 0; }
    virtual const IndexedImage* currentImage() { return 0; }
};

class FakeSelections : public SceneVisualSelections {
    FakeSelection selection;

public:
    virtual SceneFlameSelection& flame() { return selection; }
    virtual SceneGeneralFlameSelection& generalFlame() { return selection; }
    virtual SceneWaveSelection& wave() { return selection; }
    virtual SceneOptionSelection& waveScale() { return selection; }
    virtual SceneOptionSelection& table() { return selection; }
    virtual SceneOptionSelection& object() { return selection; }
    virtual SceneTranslationSelection& translation() { return selection; }
    virtual ScenePaletteSelection& palette() { return selection; }
    virtual SceneOptionSelection& border() { return selection; }
    virtual SceneOptionSelection& flashlight() { return selection; }
    virtual SceneImageSelection& images() { return selection; }
};

class FakeDisplayCatalogs : public ControlDisplayCatalogs {
public:
    virtual int screenChoiceCount() const { return 2; }
    virtual const char* screenChoiceNameAt(int index) const {
        return index == 0 ? "Up" : "Source";
    }
    virtual const char* screenChoiceLabelAt(int index) const {
        return index == 0 ? "Up Display" : "Source size";
    }
    virtual int screenChoiceInUseAt(int) const { return 1; }
};

static Config sampleConfig() {
    Config config;
    config.scene.flame = "fire";
    config.scene.translation = "plasma";
    config.scene.image = "cthugha";
    config.scene.object = "torus";
    config.scene.table = "cos";
    config.scene.waveScale = "wide";
    config.scene.palette = "volcano";
    config.scene.flashlight = "on";
    config.scene.presentation = "Source";
    config.scene.audioProcessing = "FFT";
    config.display.maxFramesPerSecond = 60;
    config.display.showFpsEnabled = 1;
    config.display.zoomMode = 2;
    config.audioAnalysis.fireSensitivity = 73;
    config.audioAnalysis.fireSource = "low-pass-150hz-amplitude";
    config.autoChange.locked = 1;
    config.autoChange.changeLittle = 0;
    config.autoChange.cumulativeFireLevel = 500;
    return config;
}

static void testStateSnapshotUsesRuntimeConfig() {
    RuntimeConfigRegistry registry(sampleConfig());
    ControlRuntimeMetricsSnapshot metrics(9, 123, 73);
    ControlJsonValue state = buildControlStateSnapshot(registry, metrics, 42);

    assert(state.member("type")->asString() == "state");
    assert(state.member("rev")->asNumber() == 42);
    assert(state.member("scene")->member("flame")->asString() == "fire");
    assert(state.member("scene")->member("palette")->asString() == "volcano");
    assert(state.member("display")->member("maxFps")->asNumber() == 60);
    assert(state.member("display")->member("screen")->asString() == "Source");
    assert(state.member("display")->member("showFps")->asBool() == true);
    assert(state.member("audio")->member("processing")->asString() == "FFT");
    assert(state.member("audio")->member("fire")->asNumber() == 9);
    assert(state.member("audio")->member("cumulativeFireLevel")->asNumber() == 123);
    assert(state.member("audio")->member("fireSensitivity")->asNumber() == 73);
    assert(state.member("audio")->member("fireSource")->asString()
        == "low-pass-150hz-amplitude");
    assert(state.member("autoChange")->member("locked")->asBool() == true);
    assert(state.member("autoChange")->member("enabled")->asBool() == false);
    assert(state.member("autoChange")->member("cumulativeFireLevel")->asNumber() == 500);
}

static void testCatalogSnapshotUsesSelections() {
    FakeSelections selections;
    FakeDisplayCatalogs displayCatalogs;
    ControlJsonValue catalogs = buildControlCatalogSnapshot(
        selections, displayCatalogs);

    assert(catalogs.member("type")->asString() == "catalogs");
    const ControlJsonValue* targets = catalogs.member("targets");
    assert(targets != 0);
    const ControlJsonValue* flames = targets->member("scene.flame");
    assert(flames != 0);
    assert(flames->asArray().size() == 2);
    assert(flames->asArray()[0].member("name")->asString() == "first");
    assert(flames->asArray()[0].member("current")->asBool() == true);
    assert(flames->asArray()[1].member("inUse")->asBool() == false);
    assert(targets->member("audio.processing")->asArray().size() == 4);
    assert(targets->member("audio.fireSource")->asArray().size() == 2);
    assert(targets->member("audio.fireSource")->asArray()[1]
        .member("name")->asString()
        == "low-pass-150hz-amplitude");
    assert(targets->member("display.screen")->asArray().size() == 2);
    assert(targets->member("display.screen")->asArray()[1]
        .member("name")->asString()
        == "Source");
}

int main() {
    testStateSnapshotUsesRuntimeConfig();
    testCatalogSnapshotUsesSelections();
    return 0;
}
