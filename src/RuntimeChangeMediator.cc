/** @file
 * Concrete runtime command mediator.
 */

#include "RuntimeChangeMediator.h"

#include "RuntimeAudioControls.h"
#include "RuntimeAutoChangeControls.h"
#include "RuntimeCommandTargets.h"
#include "RuntimeDisplayControls.h"
#include "RuntimeEffectControls.h"
#include "RuntimeFrameGeneratorControls.h"
#include "RuntimePersistence.h"
#include "RuntimeShutdown.h"
#include "Scene.h"

namespace {

static SceneSelectionTarget sceneSelectionTargetFromRuntime(
    RuntimeSceneTarget target) {
    switch (target) {
    case RuntimeSceneFlame:
        return SceneSelectionFlame;
    case RuntimeSceneGeneralFlame:
        return SceneSelectionGeneralFlame;
    case RuntimeSceneWave:
        return SceneSelectionWave;
    case RuntimeSceneWaveScale:
        return SceneSelectionWaveScale;
    case RuntimeSceneObject:
        return SceneSelectionObject;
    case RuntimeSceneTranslation:
        return SceneSelectionTranslation;
    case RuntimeSceneBorder:
        return SceneSelectionBorder;
    case RuntimeSceneFlashlight:
        return SceneSelectionFlashlight;
    case RuntimeScenePalette:
        return SceneSelectionPalette;
    case RuntimeSceneTable:
        return SceneSelectionTable;
    case RuntimeSceneImage:
        return SceneSelectionImage;
    }

    return SceneSelectionFlame;
}

}

RuntimeChangeMediator::RuntimeChangeMediator(SceneCommandTarget& sceneCommands_,
    RuntimePersistence& runtimePersistence_, RuntimeShutdown& runtimeShutdown_,
    RuntimeDisplayControls& displayControls_, RuntimeAudioControls& audioControls_,
    RuntimeAutoChangeControls& autoChangeControls_,
    RuntimeFrameGeneratorControls& frameGeneratorControls_,
    RuntimeEffectControls& effectControls_)
    : sceneCommands(sceneCommands_)
    , runtimePersistence(runtimePersistence_)
    , runtimeShutdown(runtimeShutdown_)
    , displayControls(displayControls_)
    , audioControls(audioControls_)
    , autoChangeControls(autoChangeControls_)
    , frameGeneratorControls(frameGeneratorControls_)
    , effectControls(effectControls_) { }

RuntimeChangeSet RuntimeChangeMediator::applySceneBy(RuntimeSceneTarget target, int by) {
    RuntimeChangeSet changes;

    sceneCommands.change(sceneSelectionTargetFromRuntime(target), by);
    changes.sceneChanges = 1;
    return changes;
}

RuntimeChangeSet RuntimeChangeMediator::applySceneTo(
    RuntimeSceneTarget target, const char* to) {
    RuntimeChangeSet changes;

    sceneCommands.change(sceneSelectionTargetFromRuntime(target), to);
    changes.sceneChanges = 1;
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
    case RuntimeCommandActivateScene:
        sceneCommands.activate(
            sceneSelectionTargetFromRuntime(command.sceneTarget),
            command.value);
        changes.sceneChanges = 1;
        break;
    case RuntimeCommandToggleSceneLock:
        sceneCommands.toggleLock(
            sceneSelectionTargetFromRuntime(command.sceneTarget));
        changes.uiChanged = 1;
        break;
    case RuntimeCommandToggleSceneChoiceUse:
        sceneCommands.toggleChoiceUse(
            sceneSelectionTargetFromRuntime(command.sceneTarget),
            command.value);
        changes.uiChanged = 1;
        break;
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
    case RuntimeCommandChangeMaxFpsTo:
        displayControls.changeMaxFpsTo(command.value);
        changes.fpsChanged = 1;
        break;
    case RuntimeCommandChangeSoundProcessingBy:
        audioControls.changeSoundProcessingBy(command.value);
        changes.audioProcessingChanged = 1;
        break;
    case RuntimeCommandChangeSoundProcessingTo:
        audioControls.changeSoundProcessingTo(command.text);
        changes.audioProcessingChanged = 1;
        break;
    case RuntimeCommandChangeFireSensitivityTo:
        audioControls.changeFireSensitivityTo(command.value);
        changes.audioProcessingChanged = 1;
        break;
    case RuntimeCommandChangeFireSourceTo:
        audioControls.changeFireSourceTo(command.text);
        changes.audioProcessingChanged = 1;
        break;
    case RuntimeCommandToggleAutoChangeLock:
        autoChangeControls.toggleLock();
        changes.autoChangeChanged = 1;
        break;
    case RuntimeCommandChangeAutoChangeLockTo:
        autoChangeControls.changeLockTo(command.value);
        changes.autoChangeChanged = 1;
        break;
    case RuntimeCommandChangeAutoChangeChangeLittleTo:
        autoChangeControls.changeLittleTo(command.value);
        changes.autoChangeChanged = 1;
        break;
    case RuntimeCommandChangeAutoChangeCumulativeFireLevelTo:
        autoChangeControls.changeCumulativeFireLevelTo(command.value);
        changes.autoChangeChanged = 1;
        break;
    case RuntimeCommandChangePaletteSmoothingChanceTo:
        frameGeneratorControls.changePaletteSmoothingChanceTo(command.number);
        changes.uiChanged = 1;
        break;
    case RuntimeCommandChangeFilterchainSequenceTo:
        frameGeneratorControls.changeFilterchainSequenceTo(
            command.textList, command.valueList);
        changes.uiChanged = 1;
        break;
    case RuntimeCommandChangeFilterchainEnabledTo:
        frameGeneratorControls.changeFilterchainEnabledTo(
            command.textList, command.valueList);
        changes.uiChanged = 1;
        break;
    case RuntimeCommandChangeSceneLockTo:
        sceneCommands.setLock(
            sceneSelectionTargetFromRuntime(command.sceneTarget),
            command.value);
        changes.uiChanged = 1;
        break;
    case RuntimeCommandChangeScreenLockTo:
        displayControls.changePresentationLockTo(command.value);
        changes.displayChanged = 1;
        break;
    case RuntimeCommandChangeSoundProcessingLockTo:
        audioControls.changeSoundProcessingLockTo(command.value);
        changes.audioProcessingChanged = 1;
        break;
    case RuntimeCommandWriteIni:
        runtimePersistence.writeCurrentConfig();
        changes.persistenceRequested = 1;
        break;
    case RuntimeCommandStopAndContinue:
        changes.persistenceRequested = 1;
        if (runtimePersistence.writeContinuation() == 0) {
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
        if (command.effectControlTarget != 0)
            changes.merge(command.effectControlTarget->changeBy(command.value));
        break;
    case RuntimeCommandChangeEffectControlTo:
        if (command.effectControlTarget != 0)
            changes.merge(command.effectControlTarget->changeTo(command.text));
        break;
    case RuntimeCommandActivateEffectControl:
        if (command.effectControlTarget != 0)
            changes.merge(command.effectControlTarget->activate(command.value));
        break;
    case RuntimeCommandToggleEffectChoiceUse:
        if (command.effectControlTarget != 0)
            command.effectControlTarget->toggleChoiceUse(command.value);
        break;
    case RuntimeCommandChangeOptionBy:
        if (command.optionTarget != 0)
            changes.merge(command.optionTarget->changeBy(command.value));
        break;
    case RuntimeCommandChangeOptionTo:
        if (command.optionTarget != 0)
            changes.merge(command.optionTarget->changeTo(command.text));
        break;
    case RuntimeCommandToggleEffectControlLock:
        if (command.effectControlTarget != 0)
            command.effectControlTarget->toggleLock();
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
