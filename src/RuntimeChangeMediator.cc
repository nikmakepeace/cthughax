/** @file
 * Concrete runtime command mediator.
 */

#include "RuntimeChangeMediator.h"

#include "RuntimeAudioControls.h"
#include "RuntimeAutoChangeControls.h"
#include "RuntimeDisplayControls.h"
#include "RuntimeEffectControls.h"
#include "RuntimePersistence.h"
#include "RuntimeShutdown.h"
#include "Scene.h"

RuntimeChangeMediator::RuntimeChangeMediator(SceneCommands& sceneCommands_,
    RuntimePersistence& runtimePersistence_, RuntimeShutdown& runtimeShutdown_,
    RuntimeDisplayControls& displayControls_, RuntimeAudioControls& audioControls_,
    RuntimeAutoChangeControls& autoChangeControls_,
    RuntimeEffectControls& effectControls_)
    : sceneCommands(sceneCommands_)
    , runtimePersistence(runtimePersistence_)
    , runtimeShutdown(runtimeShutdown_)
    , displayControls(displayControls_)
    , audioControls(audioControls_)
    , autoChangeControls(autoChangeControls_)
    , effectControls(effectControls_) { }

RuntimeChangeSet RuntimeChangeMediator::applySceneBy(RuntimeSceneTarget target, int by) {
    RuntimeChangeSet changes;

    switch (target) {
    case RuntimeSceneFlame:
        sceneCommands.changeFlame(by);
        break;
    case RuntimeSceneGeneralFlame:
        sceneCommands.changeGeneralFlame();
        break;
    case RuntimeSceneWave:
        sceneCommands.changeWave(by);
        break;
    case RuntimeSceneWaveScale:
        sceneCommands.changeWaveScale(by);
        break;
    case RuntimeSceneObject:
        sceneCommands.changeObject(by);
        break;
    case RuntimeSceneTranslation:
        sceneCommands.changeTranslation(by);
        break;
    case RuntimeSceneBorder:
        sceneCommands.changeBorder(by);
        break;
    case RuntimeSceneFlashlight:
        sceneCommands.changeFlashlight(by);
        break;
    case RuntimeScenePalette:
        sceneCommands.changePalette(by);
        break;
    case RuntimeSceneTable:
        sceneCommands.changeTable(by);
        break;
    case RuntimeSceneImage:
        sceneCommands.changeImage(by);
        break;
    }

    changes.sceneChanges = 1;
    return changes;
}

RuntimeChangeSet RuntimeChangeMediator::applySceneTo(
    RuntimeSceneTarget target, const char* to) {
    RuntimeChangeSet changes;

    switch (target) {
    case RuntimeSceneFlame:
        sceneCommands.changeFlame(to);
        break;
    case RuntimeSceneGeneralFlame:
        sceneCommands.changeGeneralFlame();
        break;
    case RuntimeSceneWave:
        sceneCommands.changeWave(to);
        break;
    case RuntimeSceneWaveScale:
        sceneCommands.changeWaveScale(to);
        break;
    case RuntimeSceneObject:
        sceneCommands.changeObject(to);
        break;
    case RuntimeSceneTranslation:
        sceneCommands.changeTranslation(to);
        break;
    case RuntimeSceneBorder:
        sceneCommands.changeBorder(to);
        break;
    case RuntimeSceneFlashlight:
        sceneCommands.changeFlashlight(to);
        break;
    case RuntimeScenePalette:
        sceneCommands.changePalette(to);
        break;
    case RuntimeSceneTable:
        sceneCommands.changeTable(to);
        break;
    case RuntimeSceneImage:
        sceneCommands.changeImage(to);
        break;
    }

    changes.sceneChanges = 1;
    return changes;
}

RuntimeChangeSet RuntimeChangeMediator::applyNonSceneEffectControlBy(
    EffectControl& control, int by) {
    RuntimeChangeSet changes;

    if (displayControls.changeDisplayEffectControlBy(control, by, changes))
        return changes;

    changes.merge(effectControls.changeEffectControlBy(control, by));
    return changes;
}

RuntimeChangeSet RuntimeChangeMediator::applyNonSceneEffectControlTo(
    EffectControl& control, const char* to) {
    RuntimeChangeSet changes;

    if (displayControls.changeDisplayEffectControlTo(control, to, changes))
        return changes;

    changes.merge(effectControls.changeEffectControlTo(control, to));
    return changes;
}

RuntimeChangeSet RuntimeChangeMediator::activateNonSceneEffectControl(
    EffectControl& control, int index) {
    RuntimeChangeSet changes;

    if (displayControls.activateDisplayEffectControl(control, index, changes))
        return changes;

    changes.merge(effectControls.activateEffectControl(control, index));
    return changes;
}

RuntimeChangeSet RuntimeChangeMediator::changeOwnedOptionBy(
    Option& option, int by) {
    RuntimeChangeSet changes;

    if (displayControls.changeDisplayOptionBy(option, by, changes))
        return changes;
    if (audioControls.changeAudioOptionBy(option, by, changes))
        return changes;
    if (autoChangeControls.changeAutoChangeOptionBy(option, by, changes))
        return changes;

    return changes;
}

RuntimeChangeSet RuntimeChangeMediator::changeOwnedOptionTo(
    Option& option, const char* to) {
    RuntimeChangeSet changes;

    if (displayControls.changeDisplayOptionTo(option, to, changes))
        return changes;
    if (audioControls.changeAudioOptionTo(option, to, changes))
        return changes;
    if (autoChangeControls.changeAutoChangeOptionTo(option, to, changes))
        return changes;

    return changes;
}

RuntimeChangeSet RuntimeChangeMediator::apply(const RuntimeCommand& command) {
    RuntimeChangeSet changes;

    switch (command.type) {
    case RuntimeCommandRequestClose:
        runtimeShutdown.requestClose();
        changes.closeRequested = 1;
        break;
    case RuntimeCommandChangeSceneBy:
        return applySceneBy(command.sceneTarget, command.value);
    case RuntimeCommandChangeSceneTo:
        return applySceneTo(command.sceneTarget, command.text);
    case RuntimeCommandChangeScreenBy:
        displayControls.changePresentationBy(command.value);
        changes.displayChanged = 1;
        break;
    case RuntimeCommandChangeScreenTo:
        displayControls.changePresentationTo(command.text);
        changes.displayChanged = 1;
        break;
    case RuntimeCommandChangeZoomBy:
        displayControls.changeZoomBy(command.value);
        changes.displayChanged = 1;
        break;
    case RuntimeCommandChangeZoomTo:
        displayControls.changeZoomTo(command.text);
        changes.displayChanged = 1;
        break;
    case RuntimeCommandChangeSoundProcessingBy:
        audioControls.changeSoundProcessingBy(command.value);
        changes.audioProcessingChanged = 1;
        break;
    case RuntimeCommandChangeSoundProcessingTo:
        audioControls.changeSoundProcessingTo(command.text);
        changes.audioProcessingChanged = 1;
        break;
    case RuntimeCommandToggleAutoChangeLock:
        autoChangeControls.toggleLock();
        changes.autoChangeChanged = 1;
        break;
    case RuntimeCommandWriteIni:
        runtimePersistence.writeCurrentConfig();
        changes.persistenceRequested = 1;
        break;
    case RuntimeCommandStopAndContinue:
        changes.persistenceRequested = 1;
        if (runtimePersistence.writeContinuation(command.continuation) == 0) {
            runtimeShutdown.requestClose();
            changes.closeRequested = 1;
        }
        break;
    case RuntimeCommandToggleShowFps:
        displayControls.toggleFpsOverlay();
        changes.fpsChanged = 1;
        break;
    case RuntimeCommandRestoreScene:
        sceneCommands.restore();
        changes.sceneChanges = 1;
        break;
    case RuntimeCommandSavePreset:
        sceneCommands.savePreset(command.value);
        changes.persistenceRequested = 1;
        break;
    case RuntimeCommandRestorePreset:
        sceneCommands.restorePreset(command.value);
        changes.sceneChanges = 1;
        break;
    case RuntimeCommandChangeAll:
        sceneCommands.changeAll();
        changes.sceneChanges = 1;
        break;
    case RuntimeCommandChangeOne:
        sceneCommands.changeOne();
        changes.sceneChanges = 1;
        break;
    case RuntimeCommandRandomPalette:
        sceneCommands.randomPalette();
        changes.sceneChanges = 1;
        break;
    case RuntimeCommandAddRandomPalette:
        sceneCommands.addRandomPalette();
        changes.sceneChanges = 1;
        break;
    case RuntimeCommandChangeEffectControlBy:
        if (command.effectControl == 0)
            break;
        if (sceneCommands.isSceneOption(*command.effectControl)) {
            sceneCommands.change(*command.effectControl, command.value, 0);
            changes.sceneChanges = 1;
        } else {
            changes.merge(applyNonSceneEffectControlBy(
                *command.effectControl, command.value));
        }
        break;
    case RuntimeCommandChangeEffectControlTo:
        if (command.effectControl == 0)
            break;
        if (sceneCommands.isSceneOption(*command.effectControl)) {
            sceneCommands.change(*command.effectControl, command.text, 0);
            changes.sceneChanges = 1;
        } else {
            changes.merge(applyNonSceneEffectControlTo(
                *command.effectControl, command.text));
        }
        break;
    case RuntimeCommandActivateEffectControl:
        if (command.effectControl == 0)
            break;
        if (sceneCommands.isSceneOption(*command.effectControl)) {
            sceneCommands.activate(*command.effectControl, command.value);
            changes.sceneChanges = 1;
        } else {
            changes.merge(activateNonSceneEffectControl(
                *command.effectControl, command.value));
        }
        break;
    case RuntimeCommandToggleEffectChoiceUse:
        if (command.effectControl != 0)
            effectControls.toggleEffectChoiceUse(
                *command.effectControl, command.value);
        break;
    case RuntimeCommandChangeOptionBy:
        if (command.option != 0)
            changes.merge(changeOwnedOptionBy(*command.option, command.value));
        break;
    case RuntimeCommandChangeOptionTo:
        if (command.option != 0)
            changes.merge(changeOwnedOptionTo(*command.option, command.text));
        break;
    case RuntimeCommandToggleEffectControlLock:
        if (command.effectControl != 0)
            effectControls.toggleEffectControlLock(*command.effectControl);
        break;
    case RuntimeCommandSavePaletteMetadata:
        if (command.paletteMetadataTarget != 0) {
            command.paletteMetadataTarget->savePaletteMetadata();
            changes.persistenceRequested = 1;
            changes.uiChanged = 1;
        }
        break;
    case RuntimeCommandRevertPaletteMetadata:
        if (command.paletteMetadataTarget != 0) {
            command.paletteMetadataTarget->revertPaletteMetadata();
            changes.uiChanged = 1;
        }
        break;
    }

    return changes;
}
