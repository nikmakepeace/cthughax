/** @file
 * Runtime command value and change-set helpers.
 */

#include "RuntimeCommandSink.h"

RuntimeCommand::RuntimeCommand(RuntimeCommandType type_)
    : type(type_)
    , sceneTarget(RuntimeSceneFlame)
    , value(0)
    , text(0)
    , effectControlTarget(0)
    , optionTarget(0)
    , paletteMetadataTarget(0) { }

RuntimeCommand RuntimeCommand::requestClose() {
    return RuntimeCommand(RuntimeCommandRequestClose);
}

RuntimeCommand RuntimeCommand::changeSceneBy(RuntimeSceneTarget target, int by) {
    RuntimeCommand command(RuntimeCommandChangeSceneBy);
    command.sceneTarget = target;
    command.value = by;
    return command;
}

RuntimeCommand RuntimeCommand::changeSceneTo(RuntimeSceneTarget target, const char* to) {
    RuntimeCommand command(RuntimeCommandChangeSceneTo);
    command.sceneTarget = target;
    command.text = to;
    return command;
}

RuntimeCommand RuntimeCommand::activateScene(
    RuntimeSceneTarget target, int index) {
    RuntimeCommand command(RuntimeCommandActivateScene);
    command.sceneTarget = target;
    command.value = index;
    return command;
}

RuntimeCommand RuntimeCommand::toggleSceneLock(RuntimeSceneTarget target) {
    RuntimeCommand command(RuntimeCommandToggleSceneLock);
    command.sceneTarget = target;
    return command;
}

RuntimeCommand RuntimeCommand::toggleSceneChoiceUse(
    RuntimeSceneTarget target, int index) {
    RuntimeCommand command(RuntimeCommandToggleSceneChoiceUse);
    command.sceneTarget = target;
    command.value = index;
    return command;
}

RuntimeCommand RuntimeCommand::changeScreenBy(int by) {
    RuntimeCommand command(RuntimeCommandChangeScreenBy);
    command.value = by;
    return command;
}

RuntimeCommand RuntimeCommand::changeScreenTo(const char* to) {
    RuntimeCommand command(RuntimeCommandChangeScreenTo);
    command.text = to;
    return command;
}

RuntimeCommand RuntimeCommand::changeZoomBy(int by) {
    RuntimeCommand command(RuntimeCommandChangeZoomBy);
    command.value = by;
    return command;
}

RuntimeCommand RuntimeCommand::changeZoomTo(const char* to) {
    RuntimeCommand command(RuntimeCommandChangeZoomTo);
    command.text = to;
    return command;
}

RuntimeCommand RuntimeCommand::changeMaxFpsTo(int to) {
    RuntimeCommand command(RuntimeCommandChangeMaxFpsTo);
    command.value = to;
    return command;
}

RuntimeCommand RuntimeCommand::changeSoundProcessingBy(int by) {
    RuntimeCommand command(RuntimeCommandChangeSoundProcessingBy);
    command.value = by;
    return command;
}

RuntimeCommand RuntimeCommand::changeSoundProcessingTo(const char* to) {
    RuntimeCommand command(RuntimeCommandChangeSoundProcessingTo);
    command.text = to;
    return command;
}

RuntimeCommand RuntimeCommand::changeFireSensitivityTo(int sensitivity) {
    RuntimeCommand command(RuntimeCommandChangeFireSensitivityTo);
    command.value = sensitivity;
    return command;
}

RuntimeCommand RuntimeCommand::changeFireSourceTo(const char* to) {
    RuntimeCommand command(RuntimeCommandChangeFireSourceTo);
    command.text = to;
    return command;
}

RuntimeCommand RuntimeCommand::toggleAutoChangeLock() {
    return RuntimeCommand(RuntimeCommandToggleAutoChangeLock);
}

RuntimeCommand RuntimeCommand::changeAutoChangeLockTo(int locked) {
    RuntimeCommand command(RuntimeCommandChangeAutoChangeLockTo);
    command.value = locked;
    return command;
}

RuntimeCommand RuntimeCommand::changeAutoChangeCumulativeFireLevelTo(
    int threshold) {
    RuntimeCommand command(
        RuntimeCommandChangeAutoChangeCumulativeFireLevelTo);
    command.value = threshold;
    return command;
}

RuntimeCommand RuntimeCommand::writeIni() {
    return RuntimeCommand(RuntimeCommandWriteIni);
}

RuntimeCommand RuntimeCommand::stopAndContinue() {
    return RuntimeCommand(RuntimeCommandStopAndContinue);
}

RuntimeCommand RuntimeCommand::toggleShowFps() {
    return RuntimeCommand(RuntimeCommandToggleShowFps);
}

RuntimeCommand RuntimeCommand::restoreScene() {
    return RuntimeCommand(RuntimeCommandRestoreScene);
}

RuntimeCommand RuntimeCommand::savePreset(int slot) {
    RuntimeCommand command(RuntimeCommandSavePreset);
    command.value = slot;
    return command;
}

RuntimeCommand RuntimeCommand::restorePreset(int slot) {
    RuntimeCommand command(RuntimeCommandRestorePreset);
    command.value = slot;
    return command;
}

RuntimeCommand RuntimeCommand::changeAll() {
    return RuntimeCommand(RuntimeCommandChangeAll);
}

RuntimeCommand RuntimeCommand::changeOne() {
    return RuntimeCommand(RuntimeCommandChangeOne);
}

RuntimeCommand RuntimeCommand::randomPalette() {
    return RuntimeCommand(RuntimeCommandRandomPalette);
}

RuntimeCommand RuntimeCommand::addRandomPalette() {
    return RuntimeCommand(RuntimeCommandAddRandomPalette);
}

RuntimeCommand RuntimeCommand::changeEffectControlBy(
    RuntimeEffectControlTarget& target, int by) {
    RuntimeCommand command(RuntimeCommandChangeEffectControlBy);
    command.effectControlTarget = &target;
    command.value = by;
    return command;
}

RuntimeCommand RuntimeCommand::changeEffectControlTo(
    RuntimeEffectControlTarget& target, const char* to) {
    RuntimeCommand command(RuntimeCommandChangeEffectControlTo);
    command.effectControlTarget = &target;
    command.text = to;
    return command;
}

RuntimeCommand RuntimeCommand::activateEffectControl(
    RuntimeEffectControlTarget& target, int index) {
    RuntimeCommand command(RuntimeCommandActivateEffectControl);
    command.effectControlTarget = &target;
    command.value = index;
    return command;
}

RuntimeCommand RuntimeCommand::toggleEffectChoiceUse(
    RuntimeEffectControlTarget& target, int index) {
    RuntimeCommand command(RuntimeCommandToggleEffectChoiceUse);
    command.effectControlTarget = &target;
    command.value = index;
    return command;
}

RuntimeCommand RuntimeCommand::changeOptionBy(RuntimeOptionTarget& target, int by) {
    RuntimeCommand command(RuntimeCommandChangeOptionBy);
    command.optionTarget = &target;
    command.value = by;
    return command;
}

RuntimeCommand RuntimeCommand::changeOptionTo(
    RuntimeOptionTarget& target, const char* to) {
    RuntimeCommand command(RuntimeCommandChangeOptionTo);
    command.optionTarget = &target;
    command.text = to;
    return command;
}

RuntimeCommand RuntimeCommand::toggleEffectControlLock(
    RuntimeEffectControlTarget& target) {
    RuntimeCommand command(RuntimeCommandToggleEffectControlLock);
    command.effectControlTarget = &target;
    return command;
}

RuntimeCommand RuntimeCommand::savePaletteMetadata(
    RuntimePaletteMetadataTarget& target) {
    RuntimeCommand command(RuntimeCommandSavePaletteMetadata);
    command.paletteMetadataTarget = &target;
    return command;
}

RuntimeCommand RuntimeCommand::revertPaletteMetadata(
    RuntimePaletteMetadataTarget& target) {
    RuntimeCommand command(RuntimeCommandRevertPaletteMetadata);
    command.paletteMetadataTarget = &target;
    return command;
}

RuntimeChangeSet::RuntimeChangeSet()
    : sceneChanges(0)
    , displayChanged(0)
    , audioProcessingChanged(0)
    , autoChangeChanged(0)
    , persistenceRequested(0)
    , fpsChanged(0)
    , closeRequested(0)
    , uiChanged(0) { }

int RuntimeChangeSet::any() const {
    return sceneChanges || displayChanged || audioProcessingChanged
        || autoChangeChanged || persistenceRequested || fpsChanged
        || closeRequested || uiChanged;
}

void RuntimeChangeSet::merge(const RuntimeChangeSet& other) {
    sceneChanges |= other.sceneChanges;
    displayChanged |= other.displayChanged;
    audioProcessingChanged |= other.audioProcessingChanged;
    autoChangeChanged |= other.autoChangeChanged;
    persistenceRequested |= other.persistenceRequested;
    fpsChanged |= other.fpsChanged;
    closeRequested |= other.closeRequested;
    uiChanged |= other.uiChanged;
}
