/** @file
 * Runtime persistence port and ini-backed implementation.
 */

#include "RuntimePersistence.h"

#include "IniFiles.h"
#include "RuntimeCommand.h"
#include "RuntimeConfigRegistry.h"

namespace {

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

}

IniRuntimePersistence::IniRuntimePersistence(
    RuntimeConfigRegistry& runtimeConfigRegistry_)
    : runtimeConfigRegistry(runtimeConfigRegistry_) { }

int IniRuntimePersistence::writeCurrentConfig() {
    return write_ini(runtimeConfigRegistry.currentConfig());
}

int IniRuntimePersistence::writeContinuation(
    const RuntimeContinuationState& continuation) {
    return write_continuation_ini(
        continuationIniConfigFromRuntimeState(continuation));
}
