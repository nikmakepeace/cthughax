/** @file
 * Unit coverage for module-owned runtime configuration contributors.
 */

#include "RuntimeConfigRegistry.h"

#include "AudioProcessing.h"
#include "AutoChangeSettings.h"
#include "DisplayPresentationOptions.h"
#include "Option.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }

class FakeRandomSource : public RandomSource {
public:
    virtual int uniformInt(int) { return 0; }
};

static void testDisplayContributorSnapshotsDisplayOwnedSettings() {
    Config config;
    DisplayPresentationSettings settings;
    settings.maxFramesPerSecond.setValue(72);
    settings.showFPS.setValue(1);
    settings.zoom.setValue(2);
    DisplayRuntimeConfigContributor contributor(settings);

    contributor.contribute(config);

    assert(config.display.maxFramesPerSecond == 72);
    assert(config.display.showFpsEnabled == 1);
    assert(config.display.zoomMode == 2);
}

static void testAudioContributorSnapshotsAudioOwnedProcessing() {
    Config config;
    FakeRandomSource randomSource;
    AudioProcessingState state(randomSource);
    state.changeTo("Filter2");
    AudioRuntimeConfigContributor contributor(state);

    contributor.contribute(config);

    assert(config.scene.audioProcessing == "Filter2");
}

static void testApplicationContributorSnapshotsApplicationOwnedSettings() {
    Config config;
    AutoChangeConfig autoChange;
    autoChange.quietMs = 2500;
    autoChange.waitMinMs = 3000;
    autoChange.waitRandomMs = 4000;
    autoChange.cumulativeFireLevel = 500;
    autoChange.locked = 1;
    autoChange.changeLittle = 1;
    OwnedAutoChangeSettings autoChangeSettings(autoChange);
    OptionTime quietMessageOption("quiet-message", 0);
    quietMessageOption.setValue(1234);
    ApplicationRuntimeConfigContributor contributor(
        autoChangeSettings, quietMessageOption);

    contributor.contribute(config);

    assert(config.autoChange.quietMs == 2500);
    assert(config.autoChange.waitMinMs == 3000);
    assert(config.autoChange.waitRandomMs == 4000);
    assert(config.autoChange.cumulativeFireLevel == 500);
    assert(config.autoChange.locked == 1);
    assert(config.autoChange.changeLittle == 1);
    assert(config.messages.quietMessageMs == 1234);
}

static void testRegistryComposesModuleContributors() {
    Config baseline;
    baseline.scene.audioProcessing = "startup-audio";
    baseline.display.showFpsEnabled = 0;
    RuntimeConfigRegistry registry(baseline);

    DisplayPresentationSettings displaySettings;
    displaySettings.maxFramesPerSecond.setValue(90);
    displaySettings.showFPS.setValue(1);
    displaySettings.zoom.setValue(2);
    DisplayRuntimeConfigContributor displayContributor(displaySettings);
    registry.addContributor(displayContributor);

    FakeRandomSource randomSource;
    AudioProcessingState audioState(randomSource);
    audioState.changeTo("FFT");
    AudioRuntimeConfigContributor audioContributor(audioState);
    registry.addContributor(audioContributor);

    AutoChangeConfig autoChange;
    autoChange.quietMs = 1500;
    OwnedAutoChangeSettings autoChangeSettings(autoChange);
    OptionTime quietMessageOption("quiet-message", 0);
    quietMessageOption.setValue(750);
    ApplicationRuntimeConfigContributor appContributor(
        autoChangeSettings, quietMessageOption);
    registry.addContributor(appContributor);

    Config current = registry.currentConfig();

    assert(current.display.maxFramesPerSecond == 90);
    assert(current.display.showFpsEnabled == 1);
    assert(current.display.zoomMode == 2);
    assert(current.scene.audioProcessing == "FFT");
    assert(current.autoChange.quietMs == 1500);
    assert(current.messages.quietMessageMs == 750);
}

int main() {
    testDisplayContributorSnapshotsDisplayOwnedSettings();
    testAudioContributorSnapshotsAudioOwnedProcessing();
    testApplicationContributorSnapshotsApplicationOwnedSettings();
    testRegistryComposesModuleContributors();
    return 0;
}
