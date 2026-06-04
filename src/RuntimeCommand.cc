#include "RuntimeCommandSink.h"

RuntimeContinuationState::RuntimeContinuationState()
    : flame()
    , generalFlame()
    , wave()
    , waveScale()
    , object()
    , translation()
    , palette()
    , border()
    , flashlight()
    , table()
    , image()
    , presentation()
    , audioProcessing()
    , showFpsEnabled(0) { }

RuntimeCommand::RuntimeCommand(RuntimeCommandType type_)
    : type(type_)
    , sceneTarget(RuntimeSceneFlame)
    , value(0)
    , text(0)
    , effectControl(0)
    , option(0)
    , paletteMetadataTarget(0)
    , continuation() { }

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

RuntimeCommand RuntimeCommand::toggleAutoChangeLock() {
    return RuntimeCommand(RuntimeCommandToggleAutoChangeLock);
}

RuntimeCommand RuntimeCommand::resetAudioFrame() {
    return RuntimeCommand(RuntimeCommandResetAudioFrame);
}

RuntimeCommand RuntimeCommand::writeIni() {
    return RuntimeCommand(RuntimeCommandWriteIni);
}

RuntimeCommand RuntimeCommand::stopAndContinue(
    const RuntimeContinuationState& continuation) {
    RuntimeCommand command(RuntimeCommandStopAndContinue);
    command.continuation = continuation;
    return command;
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

RuntimeCommand RuntimeCommand::changeEffectControlBy(EffectControl& option, int by) {
    RuntimeCommand command(RuntimeCommandChangeEffectControlBy);
    command.effectControl = &option;
    command.value = by;
    return command;
}

RuntimeCommand RuntimeCommand::changeEffectControlTo(EffectControl& option, const char* to) {
    RuntimeCommand command(RuntimeCommandChangeEffectControlTo);
    command.effectControl = &option;
    command.text = to;
    return command;
}

RuntimeCommand RuntimeCommand::activateEffectControl(EffectControl& option, int index) {
    RuntimeCommand command(RuntimeCommandActivateEffectControl);
    command.effectControl = &option;
    command.value = index;
    return command;
}

RuntimeCommand RuntimeCommand::changeOptionBy(Option& option, int by) {
    RuntimeCommand command(RuntimeCommandChangeOptionBy);
    command.option = &option;
    command.value = by;
    return command;
}

RuntimeCommand RuntimeCommand::changeOptionTo(Option& option, const char* to) {
    RuntimeCommand command(RuntimeCommandChangeOptionTo);
    command.option = &option;
    command.text = to;
    return command;
}

RuntimeCommand RuntimeCommand::toggleEffectControlLock(EffectControl& option) {
    RuntimeCommand command(RuntimeCommandToggleEffectControlLock);
    command.effectControl = &option;
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
    , audioResetRequested(0)
    , persistenceRequested(0)
    , fpsChanged(0)
    , closeRequested(0)
    , uiChanged(0) { }

int RuntimeChangeSet::any() const {
    return sceneChanges || displayChanged || audioProcessingChanged
        || autoChangeChanged || audioResetRequested || persistenceRequested
        || fpsChanged || closeRequested || uiChanged;
}

void RuntimeChangeSet::merge(const RuntimeChangeSet& other) {
    sceneChanges |= other.sceneChanges;
    displayChanged |= other.displayChanged;
    audioProcessingChanged |= other.audioProcessingChanged;
    autoChangeChanged |= other.autoChangeChanged;
    audioResetRequested |= other.audioResetRequested;
    persistenceRequested |= other.persistenceRequested;
    fpsChanged |= other.fpsChanged;
    closeRequested |= other.closeRequested;
    uiChanged |= other.uiChanged;
}
