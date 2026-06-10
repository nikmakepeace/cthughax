/** @file
 * Unit coverage for runtime command routing through subsystem control ports.
 */

#include "EffectControl.h"
#include "RuntimeAudioControls.h"
#include "RuntimeAutoChangeControls.h"
#include "RuntimeChangeMediator.h"
#include "RuntimeCommandTargets.h"
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

struct SceneCommandRecord {
    const char* name;
    int value;
    const char* text;
    EffectControl* option;
    SceneSelectionTarget target;
};

static SceneCommandRecord sceneRecord = { "", 0, 0, 0, SceneSelectionFlame };

static void recordSceneCommand(
    const char* name, int value = 0, const char* text = 0,
    EffectControl* option = 0,
    SceneSelectionTarget target = SceneSelectionFlame) {
    sceneRecord.name = name;
    sceneRecord.value = value;
    sceneRecord.text = text;
    sceneRecord.option = option;
    sceneRecord.target = target;
}

static void resetSceneRecord() {
    recordSceneCommand("");
}

static void recordSceneSelectionBy(SceneSelectionTarget target, int by) {
    switch (target) {
    case SceneSelectionFlame:
        recordSceneCommand("flame-by", by);
        break;
    case SceneSelectionGeneralFlame:
        recordSceneCommand("general-flame");
        break;
    case SceneSelectionWave:
        recordSceneCommand("wave-by", by);
        break;
    case SceneSelectionWaveScale:
        recordSceneCommand("wave-scale-by", by);
        break;
    case SceneSelectionObject:
        recordSceneCommand("object-by", by);
        break;
    case SceneSelectionTranslation:
        recordSceneCommand("translation-by", by);
        break;
    case SceneSelectionBorder:
        recordSceneCommand("border-by", by);
        break;
    case SceneSelectionFlashlight:
        recordSceneCommand("flashlight-by", by);
        break;
    case SceneSelectionPalette:
        recordSceneCommand("palette-by", by);
        break;
    case SceneSelectionTable:
        recordSceneCommand("table-by", by);
        break;
    case SceneSelectionImage:
        recordSceneCommand("image-by", by);
        break;
    }
}

static void recordSceneSelectionTo(SceneSelectionTarget target, const char* to) {
    switch (target) {
    case SceneSelectionFlame:
        recordSceneCommand("flame-to", 0, to);
        break;
    case SceneSelectionGeneralFlame:
        recordSceneCommand("general-flame");
        break;
    case SceneSelectionWave:
        recordSceneCommand("wave-to", 0, to);
        break;
    case SceneSelectionWaveScale:
        recordSceneCommand("wave-scale-to", 0, to);
        break;
    case SceneSelectionObject:
        recordSceneCommand("object-to", 0, to);
        break;
    case SceneSelectionTranslation:
        recordSceneCommand("translation-to", 0, to);
        break;
    case SceneSelectionBorder:
        recordSceneCommand("border-to", 0, to);
        break;
    case SceneSelectionFlashlight:
        recordSceneCommand("flashlight-to", 0, to);
        break;
    case SceneSelectionPalette:
        recordSceneCommand("palette-to", 0, to);
        break;
    case SceneSelectionTable:
        recordSceneCommand("table-to", 0, to);
        break;
    case SceneSelectionImage:
        recordSceneCommand("image-to", 0, to);
        break;
    }
}

class RecordingSceneCommandTarget : public SceneCommandTarget {
public:
    virtual void restore() { recordSceneCommand("restore"); }
    virtual void restorePreset(int slot) {
        recordSceneCommand("restore-preset", slot);
    }
    virtual void savePreset(int slot) { recordSceneCommand("save-preset", slot); }
    virtual void randomPalette() { recordSceneCommand("random-palette"); }
    virtual void addRandomPalette() { recordSceneCommand("add-random-palette"); }
    virtual void changeAll() { recordSceneCommand("change-all"); }
    virtual void changeOne() { recordSceneCommand("change-one"); }
    virtual void change(SceneSelectionTarget target, int by) {
        recordSceneSelectionBy(target, by);
    }
    virtual void change(SceneSelectionTarget target, const char* to) {
        recordSceneSelectionTo(target, to);
    }
    virtual void activate(SceneSelectionTarget target, int index) {
        recordSceneCommand("activate-scene", index, 0, 0, target);
    }
    virtual void toggleLock(SceneSelectionTarget target) {
        recordSceneCommand("toggle-scene-lock", 0, 0, 0, target);
    }
    virtual void toggleChoiceUse(SceneSelectionTarget target, int index) {
        recordSceneCommand("toggle-scene-choice-use", index, 0, 0, target);
    }
};

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

class RecordingRuntimeOptionTarget : public RuntimeOptionTarget {
public:
    int byCalls;
    int toCalls;
    int lastBy;
    const char* lastTo;
    RuntimeChangeSet byResponse;
    RuntimeChangeSet toResponse;

    RecordingRuntimeOptionTarget()
        : byCalls(0)
        , toCalls(0)
        , lastBy(0)
        , lastTo(0)
        , byResponse()
        , toResponse() { }

    virtual RuntimeChangeSet changeBy(int by) {
        byCalls++;
        lastBy = by;
        return byResponse;
    }

    virtual RuntimeChangeSet changeTo(const char* to) {
        toCalls++;
        lastTo = to;
        return toResponse;
    }
};

class RecordingRuntimeEffectControlTarget : public RuntimeEffectControlTarget {
public:
    int byCalls;
    int toCalls;
    int activateCalls;
    int lockToggles;
    int choiceUseToggles;
    int lastValue;
    const char* lastTo;
    RuntimeChangeSet byResponse;
    RuntimeChangeSet toResponse;
    RuntimeChangeSet activateResponse;

    RecordingRuntimeEffectControlTarget()
        : byCalls(0)
        , toCalls(0)
        , activateCalls(0)
        , lockToggles(0)
        , choiceUseToggles(0)
        , lastValue(0)
        , lastTo(0)
        , byResponse()
        , toResponse()
        , activateResponse() { }

    virtual RuntimeChangeSet changeBy(int by) {
        byCalls++;
        lastValue = by;
        return byResponse;
    }

    virtual RuntimeChangeSet changeTo(const char* to) {
        toCalls++;
        lastTo = to;
        return toResponse;
    }

    virtual RuntimeChangeSet activate(int index) {
        activateCalls++;
        lastValue = index;
        return activateResponse;
    }

    virtual void toggleLock() {
        lockToggles++;
    }

    virtual void toggleChoiceUse(int index) {
        choiceUseToggles++;
        lastValue = index;
    }
};

class RecordingRuntimePersistence : public RuntimePersistence {
public:
    int currentWrites;
    int currentResult;
    int continuationWrites;
    int continuationResult;

    RecordingRuntimePersistence()
        : currentWrites(0)
        , currentResult(0)
        , continuationWrites(0)
        , continuationResult(0) { }

    virtual int writeCurrentConfig() {
        currentWrites++;
        return currentResult;
    }

    virtual int writeContinuation() {
        continuationWrites++;
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
    int maxFpsToCalls;
    int lastMaxFpsTo;
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
        , maxFpsToCalls(0)
        , lastMaxFpsTo(0)
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

    virtual void changeMaxFpsTo(int to) {
        maxFpsToCalls++;
        lastMaxFpsTo = to;
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
    int fireSensitivityToCalls;
    int lastFireSensitivity;
    int fireSourceToCalls;
    const char* lastFireSource;
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
        , fireSensitivityToCalls(0)
        , lastFireSensitivity(0)
        , fireSourceToCalls(0)
        , lastFireSource(0)
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

    virtual void changeFireSensitivityTo(int sensitivity) {
        fireSensitivityToCalls++;
        lastFireSensitivity = sensitivity;
    }

    virtual void changeFireSourceTo(const char* to) {
        fireSourceToCalls++;
        lastFireSource = to;
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
    int lockToCalls;
    int lastLockValue;
    int cumulativeFireToCalls;
    int lastCumulativeFireValue;
    int autoChangeOptionByCalls;
    int autoChangeOptionToCalls;
    int handlesAutoChangeOption;
    Option* lastAutoChangeOption;
    int lastAutoChangeOptionValue;
    const char* lastAutoChangeOptionText;
    RuntimeChangeSet autoChangeOptionResponse;

    RecordingRuntimeAutoChangeControls()
        : lockToggles(0)
        , lockToCalls(0)
        , lastLockValue(0)
        , cumulativeFireToCalls(0)
        , lastCumulativeFireValue(0)
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

    virtual void changeLockTo(int locked) {
        lockToCalls++;
        lastLockValue = locked;
    }

    virtual void changeCumulativeFireLevelTo(int threshold) {
        cumulativeFireToCalls++;
        lastCumulativeFireValue = threshold;
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
    RecordingSceneCommandTarget sceneCommands;
    RuntimeChangeMediator mediator;
    RoutedRuntimeCommandTargetRouter targetRouter;

    MediatorHarness()
        : persistence()
        , shutdown()
        , displayControls()
        , audioControls()
        , autoChangeControls()
        , effectControls()
        , sceneCommands()
        , mediator(sceneCommands, persistence, shutdown, displayControls,
              audioControls, autoChangeControls, effectControls)
        , targetRouter(mediator, displayControls, audioControls,
              autoChangeControls, effectControls) { }
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

    RuntimeChangeSet activateScene
        = harness.mediator.apply(
            RuntimeCommand::activateScene(RuntimeSceneImage, 5));
    assert(activateScene.sceneChanges == 1);
    assert(strcmp(sceneRecord.name, "activate-scene") == 0);
    assert(sceneRecord.target == SceneSelectionImage);
    assert(sceneRecord.value == 5);

    RuntimeChangeSet toggleLock
        = harness.mediator.apply(
            RuntimeCommand::toggleSceneLock(RuntimeScenePalette));
    assert(toggleLock.uiChanged == 1);
    assert(strcmp(sceneRecord.name, "toggle-scene-lock") == 0);
    assert(sceneRecord.target == SceneSelectionPalette);

    RuntimeChangeSet toggleChoiceUse
        = harness.mediator.apply(
            RuntimeCommand::toggleSceneChoiceUse(RuntimeSceneWave, 3));
    assert(toggleChoiceUse.uiChanged == 1);
    assert(strcmp(sceneRecord.name, "toggle-scene-choice-use") == 0);
    assert(sceneRecord.target == SceneSelectionWave);
    assert(sceneRecord.value == 3);
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

    RuntimeChangeSet maxFps
        = harness.mediator.apply(RuntimeCommand::changeMaxFpsTo(72));
    assert(maxFps.fpsChanged == 1);
    assert(harness.displayControls.maxFpsToCalls == 1);
    assert(harness.displayControls.lastMaxFpsTo == 72);
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

    RuntimeChangeSet fireSensitivity
        = harness.mediator.apply(RuntimeCommand::changeFireSensitivityTo(37));
    assert(fireSensitivity.audioProcessingChanged == 1);
    assert(harness.audioControls.fireSensitivityToCalls == 1);
    assert(harness.audioControls.lastFireSensitivity == 37);

    RuntimeChangeSet fireSource
        = harness.mediator.apply(RuntimeCommand::changeFireSourceTo(
            "low-pass-150hz-amplitude"));
    assert(fireSource.audioProcessingChanged == 1);
    assert(harness.audioControls.fireSourceToCalls == 1);
    assert(strcmp(harness.audioControls.lastFireSource,
        "low-pass-150hz-amplitude") == 0);

    RuntimeChangeSet autoChange
        = harness.mediator.apply(RuntimeCommand::toggleAutoChangeLock());
    assert(autoChange.autoChangeChanged == 1);
    assert(harness.autoChangeControls.lockToggles == 1);

    RuntimeChangeSet autoChangeTo
        = harness.mediator.apply(RuntimeCommand::changeAutoChangeLockTo(1));
    assert(autoChangeTo.autoChangeChanged == 1);
    assert(harness.autoChangeControls.lockToCalls == 1);
    assert(harness.autoChangeControls.lastLockValue == 1);

    RuntimeChangeSet cumulativeFire = harness.mediator.apply(
        RuntimeCommand::changeAutoChangeCumulativeFireLevelTo(420));
    assert(cumulativeFire.autoChangeChanged == 1);
    assert(harness.autoChangeControls.cumulativeFireToCalls == 1);
    assert(harness.autoChangeControls.lastCumulativeFireValue == 420);

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

static void testStopAndContinuePersistsCurrentConfigBeforeClosing() {
    MediatorHarness harness;

    RuntimeChangeSet changes
        = harness.mediator.apply(RuntimeCommand::stopAndContinue());
    assert(changes.persistenceRequested == 1);
    assert(changes.closeRequested == 1);
    assert(harness.persistence.continuationWrites == 1);
    assert(harness.shutdown.closeRequests == 1);

    harness.persistence.continuationResult = 1;
    changes = harness.mediator.apply(RuntimeCommand::stopAndContinue());
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

static void testGenericCommandsCanRouteThroughExplicitTargets() {
    MediatorHarness harness;
    RecordingRuntimeOptionTarget optionTarget;
    RecordingRuntimeEffectControlTarget effectTarget;
    optionTarget.byResponse.audioProcessingChanged = 1;
    optionTarget.toResponse.autoChangeChanged = 1;
    effectTarget.byResponse.displayChanged = 1;
    effectTarget.toResponse.uiChanged = 1;
    effectTarget.activateResponse.sceneChanges = 1;

    RuntimeChangeSet optionBy
        = harness.mediator.apply(RuntimeCommand::changeOptionBy(optionTarget, 8));
    assert(optionBy.audioProcessingChanged == 1);
    assert(optionTarget.byCalls == 1);
    assert(optionTarget.lastBy == 8);
    assert(harness.displayControls.displayOptionByCalls == 0);
    assert(harness.audioControls.audioOptionByCalls == 0);
    assert(harness.autoChangeControls.autoChangeOptionByCalls == 0);

    RuntimeChangeSet optionTo
        = harness.mediator.apply(RuntimeCommand::changeOptionTo(optionTarget, "11"));
    assert(optionTo.autoChangeChanged == 1);
    assert(optionTarget.toCalls == 1);
    assert(strcmp(optionTarget.lastTo, "11") == 0);
    assert(harness.displayControls.displayOptionToCalls == 0);
    assert(harness.audioControls.audioOptionToCalls == 0);
    assert(harness.autoChangeControls.autoChangeOptionToCalls == 0);

    RuntimeChangeSet effectBy
        = harness.mediator.apply(RuntimeCommand::changeEffectControlBy(effectTarget, 4));
    assert(effectBy.displayChanged == 1);
    assert(effectTarget.byCalls == 1);
    assert(effectTarget.lastValue == 4);
    assert(harness.displayControls.displayEffectByCalls == 0);
    assert(harness.effectControls.effectByCalls == 0);

    RuntimeChangeSet effectTo
        = harness.mediator.apply(RuntimeCommand::changeEffectControlTo(effectTarget, "next"));
    assert(effectTo.uiChanged == 1);
    assert(effectTarget.toCalls == 1);
    assert(strcmp(effectTarget.lastTo, "next") == 0);
    assert(harness.displayControls.displayEffectToCalls == 0);
    assert(harness.effectControls.effectToCalls == 0);

    RuntimeChangeSet activate
        = harness.mediator.apply(RuntimeCommand::activateEffectControl(effectTarget, 3));
    assert(activate.sceneChanges == 1);
    assert(effectTarget.activateCalls == 1);
    assert(effectTarget.lastValue == 3);
    assert(harness.displayControls.displayActivateCalls == 0);
    assert(harness.effectControls.activateCalls == 0);

    harness.mediator.apply(RuntimeCommand::toggleEffectControlLock(effectTarget));
    assert(effectTarget.lockToggles == 1);
    assert(harness.effectControls.lockToggles == 0);

    harness.mediator.apply(RuntimeCommand::toggleEffectChoiceUse(effectTarget, 6));
    assert(effectTarget.choiceUseToggles == 1);
    assert(effectTarget.lastValue == 6);
    assert(harness.effectControls.choiceUseToggles == 0);
}

static void testEffectControlCommandsRouteByOwner() {
    MediatorHarness harness;
    resetSceneRecord();

    EffectChoice first("alpha", "");
    EffectChoice second("beta", "");
    EffectChoice* choices[] = { &first, &second };
    EffectChoiceList entries(choices, 2);
    EffectControl effect(-1, "effect", entries);

    harness.displayControls.handlesDisplayEffectControl = 1;
    harness.displayControls.displayResponse.displayChanged = 1;
    RuntimeChangeSet displayChange
        = harness.targetRouter.changeEffectControlBy(effect, 5);
    assert(displayChange.sceneChanges == 0);
    assert(displayChange.displayChanged == 1);
    assert(harness.displayControls.displayEffectByCalls == 1);
    assert(harness.displayControls.lastDisplayEffectControl == &effect);
    assert(harness.displayControls.lastDisplayValue == 5);
    assert(harness.effectControls.effectByCalls == 0);

    harness.displayControls.handlesDisplayEffectControl = 0;
    harness.effectControls.effectChangeResponse.uiChanged = 1;
    RuntimeChangeSet catalogTextChange
        = harness.targetRouter.changeEffectControlTo(effect, "beta");
    assert(catalogTextChange.uiChanged == 1);
    assert(harness.displayControls.displayEffectToCalls == 1);
    assert(harness.effectControls.effectToCalls == 1);
    assert(harness.effectControls.lastEffectControl == &effect);
    assert(strcmp(harness.effectControls.lastText, "beta") == 0);

    RuntimeChangeSet catalogActivation
        = harness.targetRouter.activateEffectControl(effect, 0);
    assert(catalogActivation.uiChanged == 1);
    assert(harness.displayControls.displayActivateCalls == 1);
    assert(harness.effectControls.activateCalls == 1);
    assert(harness.effectControls.lastValue == 0);

    harness.targetRouter.toggleEffectControlLock(effect);
    assert(harness.effectControls.lockToggles == 1);
    assert(harness.effectControls.lastEffectControl == &effect);

    harness.targetRouter.toggleEffectChoiceUse(effect, 1);
    assert(harness.effectControls.choiceUseToggles == 1);
    assert(harness.effectControls.lastEffectControl == &effect);
    assert(harness.effectControls.lastValue == 1);
}

static void testOptionCommandsRouteThroughOwningControls() {
    OptionInt option("integer", 0, 10);

    {
        MediatorHarness harness;
        harness.displayControls.handlesDisplayOption = 1;
        harness.displayControls.displayResponse.displayChanged = 1;

        RuntimeChangeSet changes
            = harness.targetRouter.changeOptionBy(option, 3);
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
            = harness.targetRouter.changeOptionTo(option, "7");
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
            = harness.targetRouter.changeOptionBy(option, 4);
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

    harness.targetRouter.toggleEffectControlLock(effect);
    assert(harness.effectControls.lockToggles == 1);
    assert(harness.effectControls.lastEffectControl == &effect);

    harness.targetRouter.toggleEffectChoiceUse(effect, 0);
    assert(harness.effectControls.choiceUseToggles == 1);
    assert(harness.effectControls.lastEffectControl == &effect);
    assert(harness.effectControls.lastValue == 0);
}

int main() {
    testRoutesSceneCommandsThroughSceneCommands();
    testRoutesDisplayCommandsThroughDisplayControls();
    testReportsNonSceneRuntimeChanges();
    testWriteIniDelegatesToPersistence();
    testStopAndContinuePersistsCurrentConfigBeforeClosing();
    testRoutesPaletteMetadataCommandsThroughTarget();
    testGenericCommandsCanRouteThroughExplicitTargets();
    testEffectControlCommandsRouteByOwner();
    testOptionCommandsRouteThroughOwningControls();
    testEffectControlStateCommandsRouteThroughEffectControls();
    return 0;
}
