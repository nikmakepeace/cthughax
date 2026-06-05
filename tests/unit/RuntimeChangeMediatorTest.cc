/** @file
 * Unit coverage for runtime command routing through subsystem control ports.
 */

#include "EffectControl.h"
#include "EffectControlPolicy.h"
#include "RuntimeAudioControls.h"
#include "RuntimeAutoChangeControls.h"
#include "RuntimeChangeMediator.h"
#include "RuntimeDisplayControls.h"
#include "RuntimeEffectControls.h"
#include "RuntimePersistence.h"
#include "RuntimeShutdown.h"
#include "Scene.h"

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

static void resetSceneRecord() {
    recordSceneCommand("");
    currentSceneOption = 0;
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

    virtual bool closeRequested() const {
        return closeRequests != 0;
    }
};

class RecordingRuntimeDisplayControls : public RuntimeDisplayControls {
public:
    int presentationByCalls;
    int lastPresentationBy;
    int presentationToCalls;
    const char* lastPresentationTo;
    int zoomByCalls;
    int lastZoomBy;
    int zoomToCalls;
    const char* lastZoomTo;
    int fpsToggles;
    int displayEffectByCalls;
    int displayEffectToCalls;
    int displayActivateCalls;
    int displayOptionByCalls;
    int displayOptionToCalls;
    int handlesDisplayEffectControl;
    int handlesDisplayOption;
    EffectControl* lastDisplayEffectControl;
    Option* lastDisplayOption;
    int lastDisplayValue;
    const char* lastDisplayText;
    RuntimeChangeSet displayResponse;

    RecordingRuntimeDisplayControls()
        : presentationByCalls(0)
        , lastPresentationBy(0)
        , presentationToCalls(0)
        , lastPresentationTo(0)
        , zoomByCalls(0)
        , lastZoomBy(0)
        , zoomToCalls(0)
        , lastZoomTo(0)
        , fpsToggles(0)
        , displayEffectByCalls(0)
        , displayEffectToCalls(0)
        , displayActivateCalls(0)
        , displayOptionByCalls(0)
        , displayOptionToCalls(0)
        , handlesDisplayEffectControl(0)
        , handlesDisplayOption(0)
        , lastDisplayEffectControl(0)
        , lastDisplayOption(0)
        , lastDisplayValue(0)
        , lastDisplayText(0)
        , displayResponse() { }

    virtual void changePresentationBy(int by) {
        presentationByCalls++;
        lastPresentationBy = by;
    }

    virtual void changePresentationTo(const char* to) {
        presentationToCalls++;
        lastPresentationTo = to;
    }

    virtual void changeZoomBy(int by) {
        zoomByCalls++;
        lastZoomBy = by;
    }

    virtual void changeZoomTo(const char* to) {
        zoomToCalls++;
        lastZoomTo = to;
    }

    virtual void toggleFpsOverlay() {
        fpsToggles++;
    }

    virtual int changeDisplayEffectControlBy(
        EffectControl& control, int by, RuntimeChangeSet& changes) {
        displayEffectByCalls++;
        lastDisplayEffectControl = &control;
        lastDisplayValue = by;
        if (!handlesDisplayEffectControl)
            return 0;
        changes.merge(displayResponse);
        return 1;
    }

    virtual int changeDisplayEffectControlTo(
        EffectControl& control, const char* to, RuntimeChangeSet& changes) {
        displayEffectToCalls++;
        lastDisplayEffectControl = &control;
        lastDisplayText = to;
        if (!handlesDisplayEffectControl)
            return 0;
        changes.merge(displayResponse);
        return 1;
    }

    virtual int activateDisplayEffectControl(
        EffectControl& control, int index, RuntimeChangeSet& changes) {
        displayActivateCalls++;
        lastDisplayEffectControl = &control;
        lastDisplayValue = index;
        if (!handlesDisplayEffectControl)
            return 0;
        changes.merge(displayResponse);
        return 1;
    }

    virtual int changeDisplayOptionBy(
        Option& option, int by, RuntimeChangeSet& changes) {
        displayOptionByCalls++;
        lastDisplayOption = &option;
        lastDisplayValue = by;
        if (!handlesDisplayOption)
            return 0;
        changes.merge(displayResponse);
        return 1;
    }

    virtual int changeDisplayOptionTo(
        Option& option, const char* to, RuntimeChangeSet& changes) {
        displayOptionToCalls++;
        lastDisplayOption = &option;
        lastDisplayText = to;
        if (!handlesDisplayOption)
            return 0;
        changes.merge(displayResponse);
        return 1;
    }
};

class RecordingRuntimeAudioControls : public RuntimeAudioControls {
public:
    int soundByCalls;
    int lastSoundBy;
    int soundToCalls;
    const char* lastSoundTo;
    int audioOptionByCalls;
    int audioOptionToCalls;
    int handlesAudioOption;
    Option* lastAudioOption;
    int lastAudioOptionValue;
    const char* lastAudioOptionText;
    RuntimeChangeSet audioOptionResponse;

    RecordingRuntimeAudioControls()
        : soundByCalls(0)
        , lastSoundBy(0)
        , soundToCalls(0)
        , lastSoundTo(0)
        , audioOptionByCalls(0)
        , audioOptionToCalls(0)
        , handlesAudioOption(0)
        , lastAudioOption(0)
        , lastAudioOptionValue(0)
        , lastAudioOptionText(0)
        , audioOptionResponse() { }

    virtual void changeSoundProcessingBy(int by) {
        soundByCalls++;
        lastSoundBy = by;
    }

    virtual void changeSoundProcessingTo(const char* to) {
        soundToCalls++;
        lastSoundTo = to;
    }

    virtual int changeAudioOptionBy(
        Option& option, int by, RuntimeChangeSet& changes) {
        audioOptionByCalls++;
        lastAudioOption = &option;
        lastAudioOptionValue = by;
        if (!handlesAudioOption)
            return 0;
        changes.merge(audioOptionResponse);
        return 1;
    }

    virtual int changeAudioOptionTo(
        Option& option, const char* to, RuntimeChangeSet& changes) {
        audioOptionToCalls++;
        lastAudioOption = &option;
        lastAudioOptionText = to;
        if (!handlesAudioOption)
            return 0;
        changes.merge(audioOptionResponse);
        return 1;
    }
};

class RecordingRuntimeAutoChangeControls : public RuntimeAutoChangeControls {
public:
    int lockToggles;
    int autoChangeOptionByCalls;
    int autoChangeOptionToCalls;
    int handlesAutoChangeOption;
    Option* lastAutoChangeOption;
    int lastAutoChangeOptionValue;
    const char* lastAutoChangeOptionText;
    RuntimeChangeSet autoChangeOptionResponse;

    RecordingRuntimeAutoChangeControls()
        : lockToggles(0)
        , autoChangeOptionByCalls(0)
        , autoChangeOptionToCalls(0)
        , handlesAutoChangeOption(0)
        , lastAutoChangeOption(0)
        , lastAutoChangeOptionValue(0)
        , lastAutoChangeOptionText(0)
        , autoChangeOptionResponse() { }

    virtual void toggleLock() {
        lockToggles++;
    }

    virtual int changeAutoChangeOptionBy(
        Option& option, int by, RuntimeChangeSet& changes) {
        autoChangeOptionByCalls++;
        lastAutoChangeOption = &option;
        lastAutoChangeOptionValue = by;
        if (!handlesAutoChangeOption)
            return 0;
        changes.merge(autoChangeOptionResponse);
        return 1;
    }

    virtual int changeAutoChangeOptionTo(
        Option& option, const char* to, RuntimeChangeSet& changes) {
        autoChangeOptionToCalls++;
        lastAutoChangeOption = &option;
        lastAutoChangeOptionText = to;
        if (!handlesAutoChangeOption)
            return 0;
        changes.merge(autoChangeOptionResponse);
        return 1;
    }
};

class RecordingRuntimeEffectControls : public RuntimeEffectControls {
public:
    int effectByCalls;
    int effectToCalls;
    int activateCalls;
    int lockToggles;
    int choiceUseToggles;
    EffectControl* lastEffectControl;
    int lastValue;
    const char* lastText;
    RuntimeChangeSet effectChangeResponse;

    RecordingRuntimeEffectControls()
        : effectByCalls(0)
        , effectToCalls(0)
        , activateCalls(0)
        , lockToggles(0)
        , choiceUseToggles(0)
        , lastEffectControl(0)
        , lastValue(0)
        , lastText(0)
        , effectChangeResponse() { }

    virtual RuntimeChangeSet changeEffectControlBy(
        EffectControl& control, int by) {
        effectByCalls++;
        lastEffectControl = &control;
        lastValue = by;
        return effectChangeResponse;
    }

    virtual RuntimeChangeSet changeEffectControlTo(
        EffectControl& control, const char* to) {
        effectToCalls++;
        lastEffectControl = &control;
        lastText = to;
        return effectChangeResponse;
    }

    virtual RuntimeChangeSet activateEffectControl(
        EffectControl& control, int index) {
        activateCalls++;
        lastEffectControl = &control;
        lastValue = index;
        return effectChangeResponse;
    }

    virtual void toggleEffectControlLock(EffectControl& control) {
        lockToggles++;
        lastEffectControl = &control;
    }

    virtual void toggleEffectChoiceUse(EffectControl& control, int index) {
        choiceUseToggles++;
        lastEffectControl = &control;
        lastValue = index;
    }
};

class MediatorHarness {
public:
    RecordingRuntimePersistence persistence;
    RecordingRuntimeShutdown shutdown;
    RecordingRuntimeDisplayControls displayControls;
    RecordingRuntimeAudioControls audioControls;
    RecordingRuntimeAutoChangeControls autoChangeControls;
    RecordingRuntimeEffectControls effectControls;
    RuntimeChangeMediator mediator;

    MediatorHarness()
        : persistence()
        , shutdown()
        , displayControls()
        , audioControls()
        , autoChangeControls()
        , effectControls()
        , mediator(fakeSceneCommands(), persistence, shutdown, displayControls,
              audioControls, autoChangeControls, effectControls) { }
};

static void testRoutesSceneCommandsThroughSceneCommands() {
    MediatorHarness harness;
    resetSceneRecord();

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

static void testRoutesDisplayCommandsThroughDisplayControls() {
    MediatorHarness harness;

    RuntimeChangeSet screenBy
        = harness.mediator.apply(RuntimeCommand::changeScreenBy(2));
    assert(screenBy.displayChanged == 1);
    assert(harness.displayControls.presentationByCalls == 1);
    assert(harness.displayControls.lastPresentationBy == 2);

    RuntimeChangeSet screenTo
        = harness.mediator.apply(RuntimeCommand::changeScreenTo("up"));
    assert(screenTo.displayChanged == 1);
    assert(harness.displayControls.presentationToCalls == 1);
    assert(strcmp(harness.displayControls.lastPresentationTo, "up") == 0);

    RuntimeChangeSet zoomBy
        = harness.mediator.apply(RuntimeCommand::changeZoomBy(1));
    assert(zoomBy.displayChanged == 1);
    assert(harness.displayControls.zoomByCalls == 1);
    assert(harness.displayControls.lastZoomBy == 1);

    RuntimeChangeSet zoomTo
        = harness.mediator.apply(RuntimeCommand::changeZoomTo("2"));
    assert(zoomTo.displayChanged == 1);
    assert(harness.displayControls.zoomToCalls == 1);
    assert(strcmp(harness.displayControls.lastZoomTo, "2") == 0);

    RuntimeChangeSet fps = harness.mediator.apply(RuntimeCommand::toggleShowFps());
    assert(fps.fpsChanged == 1);
    assert(harness.displayControls.fpsToggles == 1);
}

static void testReportsNonSceneRuntimeChanges() {
    MediatorHarness harness;

    RuntimeChangeSet close = harness.mediator.apply(RuntimeCommand::requestClose());
    assert(close.closeRequested == 1);
    assert(harness.shutdown.closeRequests == 1);

    RuntimeChangeSet sound
        = harness.mediator.apply(RuntimeCommand::changeSoundProcessingBy(2));
    assert(sound.audioProcessingChanged == 1);
    assert(harness.audioControls.soundByCalls == 1);
    assert(harness.audioControls.lastSoundBy == 2);

    RuntimeChangeSet soundTo
        = harness.mediator.apply(RuntimeCommand::changeSoundProcessingTo("fft"));
    assert(soundTo.audioProcessingChanged == 1);
    assert(harness.audioControls.soundToCalls == 1);
    assert(strcmp(harness.audioControls.lastSoundTo, "fft") == 0);

    RuntimeChangeSet autoChange
        = harness.mediator.apply(RuntimeCommand::toggleAutoChangeLock());
    assert(autoChange.autoChangeChanged == 1);
    assert(harness.autoChangeControls.lockToggles == 1);

    RuntimeChangeSet persist = harness.mediator.apply(RuntimeCommand::writeIni());
    assert(persist.persistenceRequested == 1);
    assert(harness.persistence.currentWrites == 1);
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

static void testEffectControlCommandsRouteByOwner() {
    MediatorHarness harness;
    resetSceneRecord();

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
    assert(harness.displayControls.displayEffectByCalls == 0);
    assert(harness.effectControls.effectByCalls == 0);

    currentSceneOption = 0;
    harness.displayControls.handlesDisplayEffectControl = 1;
    harness.displayControls.displayResponse.displayChanged = 1;
    RuntimeChangeSet displayChange
        = harness.mediator.apply(RuntimeCommand::changeEffectControlBy(effect, 1));
    assert(displayChange.sceneChanges == 0);
    assert(displayChange.displayChanged == 1);
    assert(harness.displayControls.displayEffectByCalls == 1);
    assert(harness.displayControls.lastDisplayEffectControl == &effect);
    assert(harness.displayControls.lastDisplayValue == 1);
    assert(harness.effectControls.effectByCalls == 0);

    harness.displayControls.handlesDisplayEffectControl = 0;
    harness.effectControls.effectChangeResponse.uiChanged = 1;
    RuntimeChangeSet catalogTextChange
        = harness.mediator.apply(RuntimeCommand::changeEffectControlTo(effect, "beta"));
    assert(catalogTextChange.uiChanged == 1);
    assert(harness.displayControls.displayEffectToCalls == 1);
    assert(harness.effectControls.effectToCalls == 1);
    assert(harness.effectControls.lastEffectControl == &effect);
    assert(strcmp(harness.effectControls.lastText, "beta") == 0);

    RuntimeChangeSet catalogActivation
        = harness.mediator.apply(RuntimeCommand::activateEffectControl(effect, 0));
    assert(catalogActivation.uiChanged == 1);
    assert(harness.displayControls.displayActivateCalls == 1);
    assert(harness.effectControls.activateCalls == 1);
    assert(harness.effectControls.lastValue == 0);

    currentSceneOption = &effect;
    RuntimeChangeSet sceneActivation
        = harness.mediator.apply(RuntimeCommand::activateEffectControl(effect, 1));
    assert(sceneActivation.sceneChanges == 1);
    assert(strcmp(sceneRecord.name, "activate") == 0);
    assert(sceneRecord.option == &effect);
    assert(sceneRecord.value == 1);
    assert(harness.effectControls.activateCalls == 1);

    currentSceneOption = 0;
}

static void testOptionCommandsRouteThroughOwningControls() {
    OptionInt option("integer", 0, 10);

    {
        MediatorHarness harness;
        harness.displayControls.handlesDisplayOption = 1;
        harness.displayControls.displayResponse.displayChanged = 1;

        RuntimeChangeSet changes
            = harness.mediator.apply(RuntimeCommand::changeOptionBy(option, 3));
        assert(changes.displayChanged == 1);
        assert(harness.displayControls.displayOptionByCalls == 1);
        assert(harness.displayControls.lastDisplayOption == &option);
        assert(harness.displayControls.lastDisplayValue == 3);
        assert(harness.audioControls.audioOptionByCalls == 0);
        assert(harness.autoChangeControls.autoChangeOptionByCalls == 0);
    }

    {
        MediatorHarness harness;
        harness.audioControls.handlesAudioOption = 1;
        harness.audioControls.audioOptionResponse.audioProcessingChanged = 1;

        RuntimeChangeSet changes
            = harness.mediator.apply(RuntimeCommand::changeOptionTo(option, "7"));
        assert(changes.audioProcessingChanged == 1);
        assert(harness.displayControls.displayOptionToCalls == 1);
        assert(harness.audioControls.audioOptionToCalls == 1);
        assert(harness.audioControls.lastAudioOption == &option);
        assert(strcmp(harness.audioControls.lastAudioOptionText, "7") == 0);
        assert(harness.autoChangeControls.autoChangeOptionToCalls == 0);
    }

    {
        MediatorHarness harness;
        harness.autoChangeControls.handlesAutoChangeOption = 1;
        harness.autoChangeControls.autoChangeOptionResponse.autoChangeChanged = 1;

        RuntimeChangeSet changes
            = harness.mediator.apply(RuntimeCommand::changeOptionBy(option, 4));
        assert(changes.autoChangeChanged == 1);
        assert(harness.displayControls.displayOptionByCalls == 1);
        assert(harness.audioControls.audioOptionByCalls == 1);
        assert(harness.autoChangeControls.autoChangeOptionByCalls == 1);
        assert(harness.autoChangeControls.lastAutoChangeOption == &option);
        assert(harness.autoChangeControls.lastAutoChangeOptionValue == 4);
    }
}

static void testEffectControlStateCommandsRouteThroughEffectControls() {
    MediatorHarness harness;
    EffectChoice first("alpha", "");
    EffectChoiceList entries(&first);
    EffectControl effect(-1, "effect-lock", entries);

    harness.mediator.apply(RuntimeCommand::toggleEffectControlLock(effect));
    assert(harness.effectControls.lockToggles == 1);
    assert(harness.effectControls.lastEffectControl == &effect);

    harness.mediator.apply(RuntimeCommand::toggleEffectChoiceUse(effect, 0));
    assert(harness.effectControls.choiceUseToggles == 1);
    assert(harness.effectControls.lastEffectControl == &effect);
    assert(harness.effectControls.lastValue == 0);
}

int main() {
    testRoutesSceneCommandsThroughSceneCommands();
    testRoutesDisplayCommandsThroughDisplayControls();
    testReportsNonSceneRuntimeChanges();
    testWriteIniDelegatesToPersistence();
    testStopAndContinuePersistsSnapshotBeforeClosing();
    testRoutesPaletteMetadataCommandsThroughTarget();
    testEffectControlCommandsRouteByOwner();
    testOptionCommandsRouteThroughOwningControls();
    testEffectControlStateCommandsRouteThroughEffectControls();
    return 0;
}
