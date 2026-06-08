#include "Configuration.h"
#include "EffectControl.h"
#include "LegacySceneEffectControlCatalog.h"
#include "LegacySceneEffectControlTarget.h"
#include "ProcessServices.h"
#include "Scene.h"
#include "SceneChoiceSelection.h"
#include "SceneDependencies.h"

#include <assert.h>
#include <string>
#include <vector>

int cth_log_enabled(int) {
    return 0;
}

int cth_log(int, const char*, ...) {
    return 0;
}

int cth_log_context(int, const char*, const char*, ...) {
    return 0;
}

int cth_log_error(const char*, ...) {
    return 0;
}

int cth_log_errno(int, const char*, ...) {
    return 0;
}

static int eventSequence = 0;

WaveConfig::WaveConfig()
    : waveScale(0)
    , table(0)
    , object(0)
    , bufferWidth(0)
    , bufferHeight(0) { }

WaveConfig::WaveConfig(int waveScale_, int table_, WObject* object_,
    int bufferWidth_, int bufferHeight_)
    : waveScale(waveScale_)
    , table(table_)
    , object(object_)
    , bufferWidth(bufferWidth_)
    , bufferHeight(bufferHeight_) { }

int WaveConfig::sameAs(const WaveConfig& other) const {
    return waveScale == other.waveScale
        && table == other.table
        && object == other.object
        && bufferWidth == other.bufferWidth
        && bufferHeight == other.bufferHeight;
}

class DummyRandomSource : public RandomSource {
public:
    virtual int uniformInt(int) { return 0; }
};

class DummySceneGeometry : public SceneGeometry {
public:
    virtual int width() const { return 320; }
    virtual int height() const { return 200; }
};

class RecordingSceneChoice : public SceneChoice {
    std::string nameValue;
    int useValue;

public:
    explicit RecordingSceneChoice(const char* name_)
        : nameValue(name_)
        , useValue(1) { }

    virtual const char* name() const { return nameValue.c_str(); }
    virtual int sameName(const char* other) const {
        return nameValue == ((other != 0) ? other : "");
    }
    virtual int inUse() const { return useValue; }
    virtual void setUse(int inUse) { useValue = inUse; }
};

class RecordingSceneOptionSelection : public SceneOptionSelection {
public:
    std::string catalogNameValue;
    std::vector<SceneChoice*> choices;
    int value;
    int randomCalls;

    RecordingSceneOptionSelection()
        : catalogNameValue("selection")
        , choices()
        , value(0)
        , randomCalls(0) { }

    explicit RecordingSceneOptionSelection(const char* catalogName_)
        : catalogNameValue(catalogName_)
        , choices()
        , value(0)
        , randomCalls(0) { }

    virtual const char* catalogName() const {
        return catalogNameValue.c_str();
    }
    virtual const char* currentName() const { return "selection"; }
    virtual int currentValue() const { return value; }
    virtual int entryCount() const {
        return choices.empty() ? 10 : int(choices.size());
    }
    virtual int choiceCount() const { return int(choices.size()); }
    virtual SceneChoice* choiceAt(int index) {
        if (index < 0 || index >= int(choices.size()))
            return 0;
        return choices[index];
    }
    virtual const SceneChoice* choiceAt(int index) const {
        if (index < 0 || index >= int(choices.size()))
            return 0;
        return choices[index];
    }
    virtual int resolveValue(const char* text, int* selection) const {
        for (int i = 0; i < int(choices.size()); i++) {
            if (choices[i] != 0 && choices[i]->sameName(text)) {
                if (selection != 0)
                    *selection = i;
                return 1;
            }
        }
        return SceneOptionSelection::resolveValue(text, selection);
    }
    virtual void change(int by) { value += by; }
    virtual void change(const char*, RandomSource&) { value = 3; }
    virtual int changeRandom(RandomSource& randomSource) {
        randomCalls++;
        int previousValue = value;
        value = randomSource.uniformInt(10);
        return value != previousValue;
    }
    virtual void setValue(int index) { value = index; }
};

class RecordingVisualCatalogs : public SceneVisualCatalogs {
    SceneSettings settingsValue;

public:
    int stateValue;
    int currentSettingsCalls;
    int currentImageCalls;
    int activateCalls;
    SceneSelectionTarget activatedTarget;
    int activatedIndex;
    int toggleLockCalls;
    SceneSelectionTarget lockedTarget;
    int toggleChoiceUseCalls;
    SceneSelectionTarget choiceUseTarget;
    int choiceUseIndex;
    unsigned int activateResponse;

    RecordingVisualCatalogs()
        : settingsValue()
        , stateValue(0)
        , currentSettingsCalls(0)
        , currentImageCalls(0)
        , activateCalls(0)
        , activatedTarget(SceneSelectionFlame)
        , activatedIndex(-1)
        , toggleLockCalls(0)
        , lockedTarget(SceneSelectionFlame)
        , toggleChoiceUseCalls(0)
        , choiceUseTarget(SceneSelectionFlame)
        , choiceUseIndex(-1)
        , activateResponse(SceneNoChange) { }

    virtual const SceneSettings& currentSettings(SceneGeometry&) {
        currentSettingsCalls++;
        settingsValue.borderMode = stateValue;
        return settingsValue;
    }

    virtual const IndexedImage* currentImage() {
        currentImageCalls++;
        return 0;
    }

    virtual void applyStartupConfig(
        const SceneConfig&, RandomSource&) { }

    virtual unsigned int change(
        SceneSelectionTarget, int, RandomSource&) {
        return SceneNoChange;
    }

    virtual unsigned int change(
        SceneSelectionTarget, const char*, RandomSource&) {
        return SceneNoChange;
    }

    virtual unsigned int activate(SceneSelectionTarget target, int index) {
        activateCalls++;
        activatedTarget = target;
        activatedIndex = index;
        return activateResponse;
    }

    virtual void toggleLock(SceneSelectionTarget target) {
        toggleLockCalls++;
        lockedTarget = target;
    }

    virtual void toggleChoiceUse(SceneSelectionTarget target, int index) {
        toggleChoiceUseCalls++;
        choiceUseTarget = target;
        choiceUseIndex = index;
    }

    virtual unsigned int randomPalette(RandomSource&) {
        return SceneNoChange;
    }

    virtual unsigned int addRandomPalette(RandomSource&) {
        return SceneNoChange;
    }
};

class SyncingEffectControlCatalog : public SceneEffectControlCatalog {
    RecordingVisualCatalogs& visualCatalogs;

public:
    int syncCalls;
    int syncControlsCalls;
    const EffectControl* sceneOption;
    RecordingSceneOptionSelection sceneSelection;
    int changeByCalls;
    int changeByOrder;
    unsigned int syncResponse;
    unsigned int syncControlsResponse;
    unsigned int changeByResponse;

    explicit SyncingEffectControlCatalog(
        RecordingVisualCatalogs& visualCatalogs_)
        : visualCatalogs(visualCatalogs_)
        , syncCalls(0)
        , syncControlsCalls(0)
        , sceneOption(0)
        , sceneSelection()
        , changeByCalls(0)
        , changeByOrder(0)
        , syncResponse(SceneNoChange)
        , syncControlsResponse(SceneNoChange)
        , changeByResponse(SceneNoChange) { }

    virtual unsigned int syncFromControls() {
        syncCalls++;
        visualCatalogs.stateValue = 7;
        return syncResponse;
    }
    virtual unsigned int syncControlsFromSelections() {
        syncControlsCalls++;
        visualCatalogs.stateValue = 9;
        return syncControlsResponse;
    }

    virtual SceneOptionSelection* selectionFor(EffectControl& option) {
        return const_cast<SceneOptionSelection*>(
            static_cast<const SyncingEffectControlCatalog*>(this)
                ->selectionFor(option));
    }
    virtual const SceneOptionSelection* selectionFor(
        const EffectControl& option) const {
        return (&option == sceneOption) ? &sceneSelection : 0;
    }
    virtual unsigned int change(SceneOptionSelection& selection, int,
        RandomSource&) {
        assert(&selection == &sceneSelection);
        changeByCalls++;
        changeByOrder = ++eventSequence;
        return changeByResponse;
    }
    virtual unsigned int change(SceneOptionSelection&, const char*,
        RandomSource&) {
        return SceneNoChange;
    }
    virtual unsigned int activate(SceneOptionSelection&, int) {
        return SceneNoChange;
    }
    virtual void toggleLock(SceneOptionSelection&) { }
    virtual void toggleChoiceUse(SceneOptionSelection&, int) { }
};

class RecordingEffectRegistry : public SceneEffectRegistry {
public:
    int saveCalls;
    int restoreCalls;
    int saveOrder;

    RecordingEffectRegistry()
        : saveCalls(0)
        , restoreCalls(0)
        , saveOrder(0) { }

    virtual void saveAll() {
        saveCalls++;
        saveOrder = ++eventSequence;
    }
    virtual void restoreAll() { restoreCalls++; }
    virtual void changeAll(RandomSource&) { }
    virtual void changeOne(RandomSource&) { }
};

class RecordingPresetCatalog : public ScenePresetCatalog {
public:
    int restoreCalls;
    int saveCalls;

    RecordingPresetCatalog()
        : restoreCalls(0)
        , saveCalls(0) { }

    virtual void restore(int) { restoreCalls++; }
    virtual void save(int) { saveCalls++; }
};

static void testRestorePushesSceneSelectionsBeforeReadingSceneSettings() {
    Scene scene;
    DummySceneGeometry geometry;
    RecordingVisualCatalogs visualCatalogs;
    SyncingEffectControlCatalog effectControls(visualCatalogs);
    RecordingEffectRegistry effectRegistry;
    RecordingPresetCatalog presets;
    DummyRandomSource randomSource;

    SceneCommandDependencies dependencies(
        visualCatalogs, effectControls, effectRegistry, presets);
    SceneCommands commands(scene, geometry, dependencies, randomSource);

    commands.restore();

    assert(effectRegistry.restoreCalls == 1);
    assert(effectControls.syncCalls == 0);
    assert(effectControls.syncControlsCalls == 1);
    assert(visualCatalogs.currentSettingsCalls == 1);
    assert(scene.settings().borderMode == 9);
}

static EffectControl& testSceneOption() {
    static OffEntry offEntry("off");
    static EffectChoiceList entries(&offEntry);
    static EffectControl option(0, "scene-option", entries);
    return option;
}

static void testEffectControlSaveUsesExplicitRegistryBeforeCatalogChange() {
    eventSequence = 0;

    Scene scene;
    DummySceneGeometry geometry;
    RecordingVisualCatalogs visualCatalogs;
    SyncingEffectControlCatalog effectControls(visualCatalogs);
    RecordingEffectRegistry effectRegistry;
    RecordingPresetCatalog presets;
    DummyRandomSource randomSource;
    EffectControl& option = testSceneOption();
    effectControls.sceneOption = &option;
    effectControls.changeByResponse = SceneImageChanged;

    SceneCommandDependencies dependencies(
        visualCatalogs, effectControls, effectRegistry, presets);
    SceneCommands commands(scene, geometry, dependencies, randomSource);
    SceneCommandsEffectControlOwner target(
        commands, effectControls, effectRegistry, randomSource);

    target.changeEffectControlBy(option, 1, 1);

    assert(effectRegistry.saveCalls == 1);
    assert(effectControls.changeByCalls == 1);
    assert(effectRegistry.saveOrder > 0);
    assert(effectControls.changeByOrder > effectRegistry.saveOrder);
    assert(visualCatalogs.currentImageCalls == 1);
}

static void testChangeOneUsesSyncReturnedImageChangeForCue() {
    Scene scene;
    DummySceneGeometry geometry;
    RecordingVisualCatalogs visualCatalogs;
    SyncingEffectControlCatalog effectControls(visualCatalogs);
    RecordingEffectRegistry effectRegistry;
    RecordingPresetCatalog presets;
    DummyRandomSource randomSource;
    effectControls.syncControlsResponse = SceneImageChanged;

    SceneCommandDependencies dependencies(
        visualCatalogs, effectControls, effectRegistry, presets);
    SceneCommands commands(scene, geometry, dependencies, randomSource);

    commands.changeOne();

    assert(effectControls.syncCalls == 0);
    assert(effectControls.syncControlsCalls == 1);
    assert(visualCatalogs.currentImageCalls == 1);
}

static void testSceneCommandsExposeNativeSelectionActions() {
    Scene scene;
    DummySceneGeometry geometry;
    RecordingVisualCatalogs visualCatalogs;
    SyncingEffectControlCatalog effectControls(visualCatalogs);
    RecordingEffectRegistry effectRegistry;
    RecordingPresetCatalog presets;
    DummyRandomSource randomSource;

    SceneCommandDependencies dependencies(
        visualCatalogs, effectControls, effectRegistry, presets);
    SceneCommands commands(scene, geometry, dependencies, randomSource);

    commands.activate(SceneSelectionImage, 4);

    assert(visualCatalogs.activateCalls == 1);
    assert(visualCatalogs.activatedTarget == SceneSelectionImage);
    assert(visualCatalogs.activatedIndex == 4);
    assert(visualCatalogs.currentImageCalls == 1);

    commands.toggleLock(SceneSelectionPalette);
    commands.toggleChoiceUse(SceneSelectionWave, 3);

    assert(visualCatalogs.toggleLockCalls == 1);
    assert(visualCatalogs.lockedTarget == SceneSelectionPalette);
    assert(visualCatalogs.toggleChoiceUseCalls == 1);
    assert(visualCatalogs.choiceUseTarget == SceneSelectionWave);
    assert(visualCatalogs.choiceUseIndex == 3);
}

static void testSceneSelectionRegistryOwnsSelectionHistoryAndRandomChanges() {
    SceneSelectionRegistry registry;
    RecordingSceneOptionSelection first;
    RecordingSceneOptionSelection omitted;
    RecordingSceneOptionSelection second;
    DummyRandomSource randomSource;

    first.value = 2;
    omitted.value = 4;
    second.value = 6;
    registry.registerSelection(first);
    registry.registerSelection(second);

    registry.saveAll();
    first.value = 7;
    omitted.value = 8;
    second.value = 9;
    registry.restoreAll();

    assert(first.value == 2);
    assert(omitted.value == 8);
    assert(second.value == 6);

    registry.changeAll(randomSource);

    assert(first.randomCalls == 1);
    assert(omitted.randomCalls == 0);
    assert(second.randomCalls == 1);
}

static void testSceneSelectionPresetCatalogOwnsSelectionSnapshots() {
    SceneSelectionRegistry registry;
    SceneSelectionPresetCatalog presets(registry);
    RecordingSceneOptionSelection first;
    RecordingSceneOptionSelection omitted;
    RecordingSceneOptionSelection second;

    first.value = 1;
    omitted.value = 3;
    second.value = 5;
    registry.registerSelection(first);
    registry.registerSelection(second);

    presets.save(2);

    first.value = 7;
    omitted.value = 9;
    second.value = 11;
    presets.restore(2);

    assert(first.value == 1);
    assert(omitted.value == 9);
    assert(second.value == 5);

    first.value = 13;
    second.value = 15;
    presets.restore(3);

    assert(first.value == 0);
    assert(second.value == 0);
}

static void testSceneSelectionPolicyApplierUsesSceneSelections() {
    SceneSelectionRegistry registry;
    SceneSelectionPresetCatalog presets(registry);
    SceneSelectionPolicyApplier applier(registry, presets);
    RecordingSceneOptionSelection palette("palette");
    RecordingSceneOptionSelection border("border.0");
    RecordingSceneOptionSelection wave("wave");
    RecordingSceneChoice red("red");
    RecordingSceneChoice blue("blue");
    RecordingSceneChoice green("green");
    RecordingSceneChoice off("off");
    RecordingSceneChoice frame("frame");
    EffectPolicy policy;

    palette.choices.push_back(&red);
    palette.choices.push_back(&blue);
    palette.choices.push_back(&green);
    border.choices.push_back(&off);
    border.choices.push_back(&frame);
    registry.registerSelection(palette);
    registry.registerSelection(border);
    registry.registerSelection(wave);

    policy.allowedChoices.push_back(EffectChoicePolicy("palette.blue", 0));
    policy.allowedChoices.push_back(EffectChoicePolicy("border.0.frame", 0));
    policy.presets.push_back(EffectPresetPolicy(2, "palette", "green"));
    policy.presets.push_back(EffectPresetPolicy(2, "wave", "7"));
    applier.configure(policy);

    assert(blue.inUse() == 0);
    assert(frame.inUse() == 0);
    assert(presets.value(2, palette) == 2);
    assert(presets.value(2, wave) == 7);

    palette.value = 0;
    border.value = 1;
    wave.value = 3;
    presets.restore(2);

    assert(palette.value == 2);
    assert(border.value == 0);
    assert(wave.value == 7);
}

int main() {
    testRestorePushesSceneSelectionsBeforeReadingSceneSettings();
    testEffectControlSaveUsesExplicitRegistryBeforeCatalogChange();
    testChangeOneUsesSyncReturnedImageChangeForCue();
    testSceneCommandsExposeNativeSelectionActions();
    testSceneSelectionRegistryOwnsSelectionHistoryAndRandomChanges();
    testSceneSelectionPresetCatalogOwnsSelectionSnapshots();
    testSceneSelectionPolicyApplierUsesSceneSelections();
    return 0;
}
