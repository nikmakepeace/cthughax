/** @file
 * Transitional runtime configuration contributor.
 */

#include "RuntimeConfigRegistry.h"

#include "AutoChangeSettings.h"
#include "AudioProcessing.h"
#include "DisplayPresentationOptions.h"
#include "Option.h"
#include "Screen.h"
#include "TranslationOption.h"
#include "WaveOptions.h"

#include <cstring>

namespace {

static std::string persistedName(const char* name) {
    if (name == NULL || strcmp(name, "unknown") == 0)
        return "";

    return name;
}

}

LegacyRuntimeConfigContributor::LegacyRuntimeConfigContributor(
    const AutoChangeSettings& autoChangeSettings_,
    const AudioProcessingState& audioProcessingState_,
    const Option& quietMessageOption_)
    : autoChangeSettings(autoChangeSettings_)
    , audioProcessingState(audioProcessingState_)
    , quietMessageOption(quietMessageOption_) { }

void LegacyRuntimeConfigContributor::contribute(Config& config) const {
    config.scene.presentation = persistedName(screen.currentName());
    config.scene.audioProcessing = persistedName(audioProcessingState.text());

    config.display.maxFramesPerSecond = int(maxFramesPerSecond);
    config.display.showFpsEnabled = int(showFPS);
    config.display.zoomMode = int(zoom);

    config.autoChange = autoChangeSettings.config();

    config.messages.quietMessageMs = int(quietMessageOption);

    config.effectPolicy.useTranslatesEnabled = int(use_translates);
    config.effectPolicy.useObjectsEnabled = int(use_objects);
}
