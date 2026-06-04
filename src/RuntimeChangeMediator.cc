#include "RuntimeChangeMediator.h"

#include "cthugha.h"
#include "AudioFrame.h"
#include "AudioProcessor.h"
#include "AutoChanger.h"
#include "CthughaDisplay.h"
#include "EffectControl.h"
#include "IniFiles.h"
#include "Option.h"
#include "Scene.h"
#include "Screen.h"

static ContinuationIniConfig continuationIniConfigFromRuntimeState(
    const RuntimeContinuationState& state) {
    ContinuationIniConfig config;
    config.scene.flame = state.flame;
    config.scene.generalFlame = state.generalFlame;
    config.scene.wave = state.wave;
    config.scene.waveScale = state.waveScale;
    config.scene.object = state.object;
    config.scene.translation = state.translation;
    config.scene.palette = state.palette;
    config.scene.border = state.border;
    config.scene.flashlight = state.flashlight;
    config.scene.table = state.table;
    config.scene.image = state.image;
    config.scene.presentation = state.presentation;
    config.scene.audioProcessing = state.audioProcessing;
    config.showFpsEnabled = state.showFpsEnabled;
    return config;
}

RuntimeChangeMediator::RuntimeChangeMediator(SceneCommands& sceneCommands_)
    : sceneCommands(sceneCommands_) { }

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

RuntimeChangeSet RuntimeChangeMediator::apply(const RuntimeCommand& command) {
    RuntimeChangeSet changes;

    switch (command.type) {
    case RuntimeCommandRequestClose:
        cthugha_close++;
        changes.closeRequested = 1;
        break;
    case RuntimeCommandChangeSceneBy:
        return applySceneBy(command.sceneTarget, command.value);
    case RuntimeCommandChangeSceneTo:
        return applySceneTo(command.sceneTarget, command.text);
    case RuntimeCommandChangeScreenBy:
        screen.change(command.value, 0);
        changes.displayChanged = 1;
        break;
    case RuntimeCommandChangeScreenTo:
        screen.change(command.text, 0);
        changes.displayChanged = 1;
        break;
    case RuntimeCommandChangeZoomBy:
        zoom.change(command.value);
        changes.displayChanged = 1;
        break;
    case RuntimeCommandChangeZoomTo:
        zoom.change(command.text);
        changes.displayChanged = 1;
        break;
    case RuntimeCommandChangeSoundProcessingBy:
        audioProcessing.change(command.value);
        changes.audioProcessingChanged = 1;
        break;
    case RuntimeCommandChangeSoundProcessingTo:
        audioProcessing.change(command.text);
        changes.audioProcessingChanged = 1;
        break;
    case RuntimeCommandToggleAutoChangeLock:
        lock.change(+1);
        changes.autoChangeChanged = 1;
        break;
    case RuntimeCommandResetAudioFrame:
        audioFrameChange();
        changes.audioResetRequested = 1;
        break;
    case RuntimeCommandWriteIni:
        write_ini();
        changes.persistenceRequested = 1;
        break;
    case RuntimeCommandStopAndContinue:
        changes.persistenceRequested = 1;
        if (write_continuation_ini(
                continuationIniConfigFromRuntimeState(command.continuation))
            == 0) {
            cthugha_close++;
            changes.closeRequested = 1;
        }
        break;
    case RuntimeCommandToggleShowFps:
        showFPS.change(+1);
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
            command.effectControl->change(command.value, 0);
            changes.displayChanged = (command.effectControl == &screen);
        }
        break;
    case RuntimeCommandChangeEffectControlTo:
        if (command.effectControl == 0)
            break;
        if (sceneCommands.isSceneOption(*command.effectControl)) {
            sceneCommands.change(*command.effectControl, command.text, 0);
            changes.sceneChanges = 1;
        } else {
            command.effectControl->change(command.text, 0);
            changes.displayChanged = (command.effectControl == &screen);
        }
        break;
    case RuntimeCommandActivateEffectControl:
        if (command.effectControl == 0)
            break;
        if (sceneCommands.isSceneOption(*command.effectControl)) {
            sceneCommands.activate(*command.effectControl, command.value);
            changes.sceneChanges = 1;
        } else if ((command.value >= 0)
            && (command.value < command.effectControl->getNEntries())) {
            (*command.effectControl)[command.value]->setUse(1);
            command.effectControl->setValue(command.value);
            command.effectControl->change(0, 0);
            changes.displayChanged = (command.effectControl == &screen);
        }
        break;
    case RuntimeCommandChangeOptionBy:
        if (command.option != 0)
            command.option->change(command.value);
        break;
    case RuntimeCommandChangeOptionTo:
        if (command.option != 0)
            command.option->change(command.text);
        break;
    case RuntimeCommandToggleEffectControlLock:
        if (command.effectControl != 0)
            command.effectControl->lock.change(+1);
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
