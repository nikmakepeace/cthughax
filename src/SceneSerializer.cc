// Runtime configuration contributor for current Scene selections.

#include "SceneSerializer.h"

#include "Scene.h"

#include <cstring>

namespace {

static std::string persistedName(const char* name) {
    if (name == 0 || std::strcmp(name, "unknown") == 0)
        return "";

    return name;
}

}

SceneSerializer::SceneSerializer(const Scene& scene_)
    : scene(scene_) { }

void SceneSerializer::contribute(Config& config) const {
    const SceneSettings& settings = scene.settings();

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
    config.scene.image = persistedName(settings.imageName);
}
