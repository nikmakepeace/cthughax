#include "IniFiles.h"
#include "RuntimeCommand.h"
#include "RuntimeConfigRegistry.h"
#include "RuntimePersistence.h"

#include <assert.h>

ContinuationIniConfig::ContinuationIniConfig()
    : scene()
    , showFpsEnabled(0) { }

static int iniWrites = 0;
static Config lastConfig;
int write_ini(const Config& config) {
    iniWrites++;
    lastConfig = config;
    return 0;
}

static int continuationWrites = 0;
static ContinuationIniConfig lastContinuationConfig;
int write_continuation_ini(const ContinuationIniConfig& config) {
    continuationWrites++;
    lastContinuationConfig = config;
    return 0;
}

class RuntimeWaveContributor : public RuntimeConfigContributor {
public:
    virtual void contribute(Config& config) const {
        config.scene.wave = "runtime-wave";
        config.display.showFpsEnabled = 1;
    }
};

static void testWriteCurrentConfigUsesRegistrySnapshot() {
    Config baseline;
    baseline.scene.wave = "startup-wave";
    baseline.display.showFpsEnabled = 0;
    RuntimeConfigRegistry registry(baseline);
    RuntimeWaveContributor contributor;
    registry.addContributor(contributor);
    IniRuntimePersistence persistence(registry);

    iniWrites = 0;
    assert(persistence.writeCurrentConfig() == 0);

    assert(iniWrites == 1);
    assert(lastConfig.scene.wave == "runtime-wave");
    assert(lastConfig.display.showFpsEnabled == 1);
}

static void testWriteContinuationMapsRuntimeContinuationState() {
    Config baseline;
    RuntimeConfigRegistry registry(baseline);
    IniRuntimePersistence persistence(registry);
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

    continuationWrites = 0;
    assert(persistence.writeContinuation(continuation) == 0);

    assert(continuationWrites == 1);
    assert(lastContinuationConfig.scene.flame == "saved-flame");
    assert(lastContinuationConfig.scene.generalFlame == "saved-general");
    assert(lastContinuationConfig.scene.wave == "saved-wave");
    assert(lastContinuationConfig.scene.waveScale == "saved-scale");
    assert(lastContinuationConfig.scene.object == "saved-object");
    assert(lastContinuationConfig.scene.translation == "saved-translation");
    assert(lastContinuationConfig.scene.palette == "saved-palette");
    assert(lastContinuationConfig.scene.border == "saved-border");
    assert(lastContinuationConfig.scene.flashlight == "saved-flashlight");
    assert(lastContinuationConfig.scene.table == "saved-table");
    assert(lastContinuationConfig.scene.image == "saved-image");
    assert(lastContinuationConfig.scene.presentation == "saved-screen");
    assert(lastContinuationConfig.scene.audioProcessing == "saved-audio");
    assert(lastContinuationConfig.showFpsEnabled == 1);
}

int main() {
    testWriteCurrentConfigUsesRegistrySnapshot();
    testWriteContinuationMapsRuntimeContinuationState();
    return 0;
}
