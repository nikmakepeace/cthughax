/** @file
 * Legacy/global-backed runtime configuration contributor.
 */

#include "RuntimeConfigRegistry.h"

#include "AudioAnalyzer.h"
#include "AudioProcessor.h"
#include "AutoChanger.h"
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
    SceneCommands& sceneCommands_)
    : sceneCommands(sceneCommands_) { }

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
    config.scene.audioProcessing = persistedName(audioProcessing.text());

    config.display.maxFramesPerSecond = int(maxFramesPerSecond);
    config.display.showFpsEnabled = int(showFPS);
    config.display.zoomMode = int(zoom);

    config.autoChange.quietMs = int(changeQuiet);
    config.autoChange.waitMinMs = int(changeWaitMin);
    config.autoChange.waitRandomMs = int(changeWaitRandom);
    config.autoChange.cumulativeFireLevel = int(changeCumulativeFireLevel);
    config.autoChange.locked = int(lock);
    config.autoChange.changeLittle = int(change_little);
    config.autoChange.minNoise = int(sound_minnoise);

    config.messages.quietMessageMs = int(changeMsgTime);

    config.effectPolicy.useTranslatesEnabled = int(use_translates);
    config.effectPolicy.useObjectsEnabled = int(use_objects);
}
