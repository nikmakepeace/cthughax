/** @file
 * Transitional runtime configuration contributor.
 */

#include "RuntimeConfigRegistry.h"

#include "AutoChangeSettings.h"
#include "AudioProcessing.h"
#include "CthughaDisplay.h"
#include "Image.h"
#include "Scene.h"
#include "Screen.h"
#include "TranslationOptions.h"
#include "VideoDirector.h"
#include "waves.h"

#include <cstring>

namespace {

static std::string persistedName(const char* name) {
    if (name == NULL || strcmp(name, "unknown") == 0)
        return "";

    return name;
}

}

LegacyRuntimeConfigContributor::LegacyRuntimeConfigContributor(
    SceneCommands& sceneCommands_,
    const AutoChangeSettings& autoChangeSettings_,
    const AudioProcessingState& audioProcessingState_)
    : sceneCommands(sceneCommands_)
    , autoChangeSettings(autoChangeSettings_)
    , audioProcessingState(audioProcessingState_) { }

void LegacyRuntimeConfigContributor::contribute(Config& config) const {
    const SceneSettings& settings = sceneCommands.sceneState().settings();

    config.scene.flame = persistedName(settings.flameName);
    config.scene.generalFlame = persistedName(settings.generalFlameName);
    config.scene.wave = persistedName(settings.waveName);
    config.scene.waveScale = persistedName(settings.waveScaleName);
    config.scene.object = persistedName(settings.objectName);
    config.scene.translation = persistedName(settings.translationName);
    config.scene.palette = persistedName(settings.paletteName);
    config.scene.border = persistedName(settings.borderName);
    config.scene.flashlight = persistedName(settings.flashlightName);
    config.scene.table = persistedName(settings.tableName);
    config.scene.image = persistedName(sceneCommands.imageOption().currentName());
    config.scene.presentation = persistedName(screen.currentName());
    config.scene.audioProcessing = persistedName(audioProcessingState.text());

    config.display.maxFramesPerSecond = int(maxFramesPerSecond);
    config.display.showFpsEnabled = int(showFPS);
    config.display.zoomMode = int(zoom);

    config.autoChange = autoChangeSettings.config();

    config.messages.quietMessageMs = int(changeMsgTime);

    config.effectPolicy.useTranslatesEnabled = int(use_translates);
    config.effectPolicy.useObjectsEnabled = int(use_objects);
}
