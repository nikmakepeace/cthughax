#include "IniFiles.h"
#include "AutoChangeSettings.h"
#include "ProcessServices.h"
#include "RuntimeConfigRegistry.h"
#include "RuntimePersistence.h"

#include <assert.h>

ContinuationIniConfig::ContinuationIniConfig()
    : scene()
    , showFpsEnabled(0) { }

class StubLogSink : public LogSink {
public:
    virtual int enabled(int) const {
        return 0;
    }

protected:
    virtual void write(int, const char*, int, const char*, va_list) { }
};

static StubLogSink testLogSink;

static int iniWrites = 0;
static Config lastConfig;
static LogSink* lastIniLogSink = NULL;
int write_ini(const Config& config, LogSink& log) {
    iniWrites++;
    lastConfig = config;
    lastIniLogSink = &log;
    return 0;
}

static int continuationWrites = 0;
static ContinuationIniConfig lastContinuationConfig;
static LogSink* lastContinuationLogSink = NULL;
int write_continuation_ini(const ContinuationIniConfig& config, LogSink& log) {
    continuationWrites++;
    lastContinuationConfig = config;
    lastContinuationLogSink = &log;
    return 0;
}

class RuntimeWaveContributor : public RuntimeConfigContributor {
public:
    virtual void contribute(Config& config) const {
        config.scene.wave = "runtime-wave";
        config.display.showFpsEnabled = 1;
    }
};

class RuntimeAutoChangeContributor : public RuntimeConfigContributor {
    const AutoChangeSettings& settings;

public:
    explicit RuntimeAutoChangeContributor(const AutoChangeSettings& settings_)
        : settings(settings_) { }

    virtual void contribute(Config& config) const {
        config.autoChange = settings.config();
    }
};

static void testWriteCurrentConfigUsesRegistrySnapshot() {
    Config baseline;
    baseline.scene.wave = "startup-wave";
    baseline.display.showFpsEnabled = 0;
    baseline.audioAnalysis.minNoise = 123;
    RuntimeConfigRegistry registry(baseline);
    RuntimeWaveContributor contributor;
    registry.addContributor(contributor);
    IniRuntimePersistence persistence(registry, testLogSink);

    iniWrites = 0;
    lastIniLogSink = NULL;
    assert(persistence.writeCurrentConfig() == 0);

    assert(iniWrites == 1);
    assert(lastIniLogSink == &testLogSink);
    assert(lastConfig.scene.wave == "runtime-wave");
    assert(lastConfig.display.showFpsEnabled == 1);
    assert(lastConfig.audioAnalysis.minNoise == 123);
}

static void testWriteCurrentConfigReadsOwnedAutoChangeSettings() {
    Config baseline;
    RuntimeConfigRegistry registry(baseline);
    AutoChangeConfig runtimeConfig;
    runtimeConfig.quietMs = 2500;
    runtimeConfig.waitMinMs = 3000;
    runtimeConfig.waitRandomMs = 4000;
    runtimeConfig.cumulativeFireLevel = 500;
    runtimeConfig.locked = 1;
    runtimeConfig.changeLittle = 1;
    OwnedAutoChangeSettings settings(runtimeConfig);
    RuntimeAutoChangeContributor contributor(settings);
    registry.addContributor(contributor);
    IniRuntimePersistence persistence(registry, testLogSink);

    iniWrites = 0;
    lastIniLogSink = NULL;
    assert(persistence.writeCurrentConfig() == 0);

    assert(iniWrites == 1);
    assert(lastIniLogSink == &testLogSink);
    assert(lastConfig.autoChange.quietMs == 2500);
    assert(lastConfig.autoChange.waitMinMs == 3000);
    assert(lastConfig.autoChange.waitRandomMs == 4000);
    assert(lastConfig.autoChange.cumulativeFireLevel == 500);
    assert(lastConfig.autoChange.locked == 1);
    assert(lastConfig.autoChange.changeLittle == 1);
}

static void testWriteContinuationMapsCurrentRegistryConfig() {
    Config baseline;
    baseline.scene.flame = "saved-flame";
    baseline.scene.generalFlame = "saved-general";
    baseline.scene.wave = "startup-wave";
    baseline.scene.waveScale = "saved-scale";
    baseline.scene.object = "saved-object";
    baseline.scene.translation = "saved-translation";
    baseline.scene.palette = "saved-palette";
    baseline.scene.border = "saved-border";
    baseline.scene.flashlight = "saved-flashlight";
    baseline.scene.table = "saved-table";
    baseline.scene.image = "saved-image";
    baseline.scene.presentation = "saved-screen";
    baseline.scene.audioProcessing = "saved-audio";
    baseline.display.showFpsEnabled = 0;
    RuntimeConfigRegistry registry(baseline);
    RuntimeWaveContributor contributor;
    registry.addContributor(contributor);
    IniRuntimePersistence persistence(registry, testLogSink);

    continuationWrites = 0;
    lastContinuationLogSink = NULL;
    assert(persistence.writeContinuation() == 0);

    assert(continuationWrites == 1);
    assert(lastContinuationLogSink == &testLogSink);
    assert(lastContinuationConfig.scene.flame == "saved-flame");
    assert(lastContinuationConfig.scene.generalFlame == "saved-general");
    assert(lastContinuationConfig.scene.wave == "runtime-wave");
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
    testWriteCurrentConfigReadsOwnedAutoChangeSettings();
    testWriteContinuationMapsCurrentRegistryConfig();
    return 0;
}
