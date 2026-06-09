/** @file
 * Runtime persistence port and ini-backed implementation.
 */

#include "RuntimePersistence.h"

#include "IniFiles.h"
#include "ProcessServices.h"
#include "RuntimeConfigRegistry.h"

namespace {

static ContinuationIniConfig continuationIniConfigFromConfig(
    const Config& state) {
    ContinuationIniConfig config;
    config.scene = state.scene;
    config.showFpsEnabled = state.display.showFpsEnabled;
    return config;
}

}

IniRuntimePersistence::IniRuntimePersistence(
    RuntimeConfigRegistry& runtimeConfigRegistry_, LogSink& log_)
    : runtimeConfigRegistry(runtimeConfigRegistry_)
    , log(log_) { }

int IniRuntimePersistence::writeCurrentConfig() {
    return write_ini(runtimeConfigRegistry.currentConfig(), log);
}

int IniRuntimePersistence::writeContinuation() {
    Config current = runtimeConfigRegistry.currentConfig();
    return write_continuation_ini(
        continuationIniConfigFromConfig(current), log);
}
