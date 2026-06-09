/** @file
 * Module-owned runtime configuration contributors.
 */

#include "RuntimeConfigRegistry.h"

#include "AudioProcessing.h"
#include "AutoChangeSettings.h"
#include "DisplayPresentationOptions.h"
#include "Option.h"

#include <cstring>

namespace {

static std::string persistedName(const char* name) {
    if (name == NULL || strcmp(name, "unknown") == 0)
        return "";

    return name;
}

}

DisplayRuntimeConfigContributor::DisplayRuntimeConfigContributor(
    const DisplayPresentationSettings& displaySettings_)
    : displaySettings(displaySettings_) { }

void DisplayRuntimeConfigContributor::contribute(Config& config) const {
    config.display.maxFramesPerSecond = int(displaySettings.maxFramesPerSecond);
    config.display.showFpsEnabled = int(displaySettings.showFPS);
    config.display.zoomMode = int(displaySettings.zoom);
}

AudioRuntimeConfigContributor::AudioRuntimeConfigContributor(
    const AudioProcessingState& audioProcessingState_)
    : audioProcessingState(audioProcessingState_) { }

void AudioRuntimeConfigContributor::contribute(Config& config) const {
    config.scene.audioProcessing = persistedName(audioProcessingState.text());
}

ApplicationRuntimeConfigContributor::ApplicationRuntimeConfigContributor(
    const AutoChangeSettings& autoChangeSettings_,
    const Option& quietMessageOption_)
    : autoChangeSettings(autoChangeSettings_)
    , quietMessageOption(quietMessageOption_) { }

void ApplicationRuntimeConfigContributor::contribute(Config& config) const {
    config.autoChange = autoChangeSettings.config();
    config.messages.quietMessageMs = int(quietMessageOption);
}
