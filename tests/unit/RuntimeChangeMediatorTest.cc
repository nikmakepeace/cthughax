#include "AudioFrame.h"
#include "AudioProcessor.h"
#include "AutoChanger.h"
#include "CthughaDisplay.h"
#include "EffectControl.h"
#include "EffectControlPolicy.h"
#include "RuntimeChangeMediator.h"
#include "RuntimePersistence.h"
#include "RuntimeShutdown.h"
#include "Scene.h"
#include "Screen.h"

#include <assert.h>
#include <string.h>

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

void effectControlPolicyObserve(EffectControl&) { }
void configureEffectPolicy(const EffectPolicy&) { }

static EffectChoice audioNone("none", "");
static EffectChoiceList audioEntries(&audioNone);
AudioProcessingOption audioProcessing("sound-processing", audioEntries);
static int audioProcessingBy = 0;
static const char* audioProcessingTo = 0;

AudioProcessingOption::AudioProcessingOption(const char* name, EffectChoiceList& entries_)
    : Option(name)
    , entries(entries_)
    , initialEntry() { }

void AudioProcessingOption::setInitialEntry(const char*) { }
void AudioProcessingOption::changeToInitial() { }
void AudioProcessingOption::change(int by) { audioProcessingBy = by; }
void AudioProcessingOption::change(const char* to) { audioProcessingTo = to; }
const char* AudioProcessingOption::text() const { return "none"; }
int AudioProcessingOption::entryCount() const { return 1; }
int AudioProcessingOption::optNr(const char*) const { return 0; }
int AudioProcessingOption::process() { return 0; }

OptionOnOff lock("lock", 0);
OptionInt zoom("zoom", 0, 8);
OptionOnOff showFPS("show-fps", 0);

static EffectChoice screenFirst("first", "");
static EffectChoice screenSecond("second", "");
static EffectChoice* screenChoiceArray[] = { &screenFirst, &screenSecond };
EffectChoiceList screenEntries(screenChoiceArray, 2);
EffectControl screen(-1, "display", screenEntries);

static int audioFrameChanged = 0;
void audioFrameChange() {
    audioFrameChanged++;
}

struct SceneCommandRecord {
    const char* name;
    int value;
    const char* text;
    EffectControl* option;
};

static SceneCommandRecord sceneRecord = { "", 0, 0, 0 };
static const EffectControl* currentSceneOption = 0;

static void recordSceneCommand(
    const char* name, int value = 0, const char* text = 0, EffectControl* option = 0) {
    sceneRecord.name = name;
    sceneRecord.value = value;
    sceneRecord.text = text;
    sceneRecord.option = option;
}

static SceneCommands& fakeSceneCommands() {
    static char storage[sizeof(SceneCommands)];
    return *reinterpret_cast<SceneCommands*>(storage);
}

int SceneCommands::isSceneOption(const EffectControl& option) const {
    return &option == currentSceneOption;
}

void SceneCommands::change(EffectControl& option, int by, int) {
    recordSceneCommand("change-by", by, 0, &option);
}

void SceneCommands::change(EffectControl& option, const char* to, int) {
    recordSceneCommand("change-to", 0, to, &option);
}

void SceneCommands::activate(EffectControl& option, int index) {
    recordSceneCommand("activate", index, 0, &option);
}

void SceneCommands::changeFlame(int by) { recordSceneCommand("flame-by", by); }
void SceneCommands::changeFlame(const char* to) { recordSceneCommand("flame-to", 0, to); }
void SceneCommands::changeGeneralFlame() { recordSceneCommand("general-flame"); }
void SceneCommands::changeWave(int by) { recordSceneCommand("wave-by", by); }
void SceneCommands::changeWave(const char* to) { recordSceneCommand("wave-to", 0, to); }
void SceneCommands::changeWaveScale(int by) { recordSceneCommand("wave-scale-by", by); }
void SceneCommands::changeWaveScale(const char* to) { recordSceneCommand("wave-scale-to", 0, to); }
void SceneCommands::changeObject(int by) { recordSceneCommand("object-by", by); }
void SceneCommands::changeObject(const char* to) { recordSceneCommand("object-to", 0, to); }
void SceneCommands::changeTranslation(int by) { recordSceneCommand("translation-by", by); }
void SceneCommands::changeTranslation(const char* to) { recordSceneCommand("translation-to", 0, to); }
void SceneCommands::changeBorder(int by) { recordSceneCommand("border-by", by); }
void SceneCommands::changeBorder(const char* to) { recordSceneCommand("border-to", 0, to); }
void SceneCommands::changeFlashlight(int by) { recordSceneCommand("flashlight-by", by); }
void SceneCommands::changeFlashlight(const char* to) { recordSceneCommand("flashlight-to", 0, to); }
void SceneCommands::changePalette(int by) { recordSceneCommand("palette-by", by); }
void SceneCommands::changePalette(const char* to) { recordSceneCommand("palette-to", 0, to); }
void SceneCommands::randomPalette() { recordSceneCommand("random-palette"); }
void SceneCommands::addRandomPalette() { recordSceneCommand("add-random-palette"); }
void SceneCommands::changeTable(int by) { recordSceneCommand("table-by", by); }
void SceneCommands::changeTable(const char* to) { recordSceneCommand("table-to", 0, to); }
void SceneCommands::changeImage(int by) { recordSceneCommand("image-by", by); }
void SceneCommands::changeImage(const char* to) { recordSceneCommand("image-to", 0, to); }
void SceneCommands::changeAll() { recordSceneCommand("change-all"); }
void SceneCommands::changeOne() { recordSceneCommand("change-one"); }
void SceneCommands::restore() { recordSceneCommand("restore"); }
void SceneCommands::restorePreset(int slot) { recordSceneCommand("restore-preset", slot); }
void SceneCommands::savePreset(int slot) { recordSceneCommand("save-preset", slot); }

class RecordingPaletteMetadataTarget : public RuntimePaletteMetadataTarget {
public:
    int saves;
    int reverts;
    int saveResult;

    RecordingPaletteMetadataTarget()
        : saves(0)
        , reverts(0)
        , saveResult(1) {
    }

    virtual int savePaletteMetadata() {
        saves++;
        return saveResult;
    }

    virtual void revertPaletteMetadata() {
        reverts++;
    }
};

class RecordingRuntimePersistence : public RuntimePersistence {
public:
    int currentWrites;
    int currentResult;
    int continuationWrites;
    int continuationResult;
    RuntimeContinuationState lastContinuation;

    RecordingRuntimePersistence()
        : currentWrites(0)
        , currentResult(0)
        , continuationWrites(0)
        , continuationResult(0)
        , lastContinuation() { }

    virtual int writeCurrentConfig() {
        currentWrites++;
        return currentResult;
    }

    virtual int writeContinuation(
        const RuntimeContinuationState& continuation) {
        continuationWrites++;
        lastContinuation = continuation;
        return continuationResult;
    }
};

class RecordingRuntimeShutdown : public RuntimeShutdown {
public:
    int closeRequests;

    RecordingRuntimeShutdown()
        : closeRequests(0) { }

    virtual void requestClose() {
        closeRequests++;
    }
};

class MediatorHarness {
public:
    RecordingRuntimePersistence persistence;
    RecordingRuntimeShutdown shutdown;
    RuntimeChangeMediator mediator;

    MediatorHarness()
        : persistence()
        , shutdown()
        , mediator(fakeSceneCommands(), persistence, shutdown) { }
};

static void testRoutesSceneCommandsThroughSceneCommands() {
    MediatorHarness harness;

    RuntimeChangeSet waveChange
        = harness.mediator.apply(RuntimeCommand::changeSceneBy(RuntimeSceneWave, 3));
    assert(waveChange.sceneChanges == 1);
    assert(strcmp(sceneRecord.name, "wave-by") == 0);
    assert(sceneRecord.value == 3);

    RuntimeChangeSet changeAll = harness.mediator.apply(RuntimeCommand::changeAll());
    assert(changeAll.sceneChanges == 1);
    assert(strcmp(sceneRecord.name, "change-all") == 0);

    RuntimeChangeSet restorePreset
        = harness.mediator.apply(RuntimeCommand::restorePreset(4));
    assert(restorePreset.sceneChanges == 1);
    assert(strcmp(sceneRecord.name, "restore-preset") == 0);
    assert(sceneRecord.value == 4);
}

static void testReportsNonSceneRuntimeChanges() {
    MediatorHarness harness;

    RuntimeChangeSet close = harness.mediator.apply(RuntimeCommand::requestClose());
    assert(close.closeRequested == 1);
    assert(harness.shutdown.closeRequests == 1);

    RuntimeChangeSet sound
        = harness.mediator.apply(RuntimeCommand::changeSoundProcessingBy(2));
    assert(sound.audioProcessingChanged == 1);
    assert(audioProcessingBy == 2);

    RuntimeChangeSet reset
        = harness.mediator.apply(RuntimeCommand::resetAudioFrame());
    assert(reset.audioResetRequested == 1);
    assert(audioFrameChanged == 1);

    RuntimeChangeSet persist = harness.mediator.apply(RuntimeCommand::writeIni());
    assert(persist.persistenceRequested == 1);
    assert(harness.persistence.currentWrites == 1);

    RuntimeChangeSet fps = harness.mediator.apply(RuntimeCommand::toggleShowFps());
    assert(fps.fpsChanged == 1);
    assert(int(showFPS) == 1);
}

static void testWriteIniDelegatesToPersistence() {
    MediatorHarness harness;
    RuntimeChangeSet persist = harness.mediator.apply(RuntimeCommand::writeIni());

    assert(persist.persistenceRequested == 1);
    assert(harness.persistence.currentWrites == 1);
    assert(harness.persistence.continuationWrites == 0);
    assert(harness.shutdown.closeRequests == 0);
}

static void testStopAndContinuePersistsSnapshotBeforeClosing() {
    MediatorHarness harness;

    RuntimeContinuationState continuation;
    continuation.flame = "saved-flame";
    continuation.generalFlame = "saved-general";
    continuation.wave = "saved-wave";
    continuation.waveScale = "saved-scale";
    continuation.object = "saved-object";
    continuation.translation = "saved-translation";
    continuation.palette = "saved-palette";
    continuation.border = "saved-border";
    continuation.flashlight = "saved-flashlight";
    continuation.table = "saved-table";
    continuation.image = "saved-image";
    continuation.presentation = "saved-screen";
    continuation.audioProcessing = "saved-audio";
    continuation.showFpsEnabled = 1;

    RuntimeChangeSet changes
        = harness.mediator.apply(RuntimeCommand::stopAndContinue(continuation));
    assert(changes.persistenceRequested == 1);
    assert(changes.closeRequested == 1);
    assert(harness.persistence.continuationWrites == 1);
    assert(harness.shutdown.closeRequests == 1);
    assert(harness.persistence.lastContinuation.flame == "saved-flame");
    assert(harness.persistence.lastContinuation.generalFlame == "saved-general");
    assert(harness.persistence.lastContinuation.wave == "saved-wave");
    assert(harness.persistence.lastContinuation.waveScale == "saved-scale");
    assert(harness.persistence.lastContinuation.object == "saved-object");
    assert(harness.persistence.lastContinuation.translation == "saved-translation");
    assert(harness.persistence.lastContinuation.palette == "saved-palette");
    assert(harness.persistence.lastContinuation.border == "saved-border");
    assert(harness.persistence.lastContinuation.flashlight == "saved-flashlight");
    assert(harness.persistence.lastContinuation.table == "saved-table");
    assert(harness.persistence.lastContinuation.image == "saved-image");
    assert(harness.persistence.lastContinuation.presentation == "saved-screen");
    assert(harness.persistence.lastContinuation.audioProcessing == "saved-audio");
    assert(harness.persistence.lastContinuation.showFpsEnabled == 1);

    harness.persistence.continuationResult = 1;
    changes = harness.mediator.apply(RuntimeCommand::stopAndContinue(continuation));
    assert(changes.persistenceRequested == 1);
    assert(changes.closeRequested == 0);
    assert(harness.persistence.continuationWrites == 2);
    assert(harness.shutdown.closeRequests == 1);
}

static void testRoutesPaletteMetadataCommandsThroughTarget() {
    MediatorHarness harness;
    RecordingPaletteMetadataTarget target;

    RuntimeChangeSet save
        = harness.mediator.apply(RuntimeCommand::savePaletteMetadata(target));
    assert(target.saves == 1);
    assert(target.reverts == 0);
    assert(save.persistenceRequested == 1);
    assert(save.uiChanged == 1);

    RuntimeChangeSet revert
        = harness.mediator.apply(RuntimeCommand::revertPaletteMetadata(target));
    assert(target.saves == 1);
    assert(target.reverts == 1);
    assert(revert.persistenceRequested == 0);
    assert(revert.uiChanged == 1);
}

static void testGenericEffectControlCommandsRespectSceneBoundary() {
    MediatorHarness harness;

    EffectChoice first("alpha", "");
    EffectChoice second("beta", "");
    EffectChoice* choices[] = { &first, &second };
    EffectChoiceList entries(choices, 2);
    EffectControl effect(-1, "effect", entries);

    currentSceneOption = &effect;
    RuntimeChangeSet sceneChange
        = harness.mediator.apply(RuntimeCommand::changeEffectControlBy(effect, 5));
    assert(sceneChange.sceneChanges == 1);
    assert(strcmp(sceneRecord.name, "change-by") == 0);
    assert(sceneRecord.option == &effect);
    assert(sceneRecord.value == 5);

    currentSceneOption = 0;
    effect.change("alpha", 0);
    RuntimeChangeSet localChange
        = harness.mediator.apply(RuntimeCommand::changeEffectControlBy(effect, 1));
    assert(localChange.sceneChanges == 0);
    assert(strcmp(effect.currentName(), "beta") == 0);
}

int main() {
    testRoutesSceneCommandsThroughSceneCommands();
    testReportsNonSceneRuntimeChanges();
    testWriteIniDelegatesToPersistence();
    testStopAndContinuePersistsSnapshotBeforeClosing();
    testRoutesPaletteMetadataCommandsThroughTarget();
    testGenericEffectControlCommandsRespectSceneBoundary();
    return 0;
}
