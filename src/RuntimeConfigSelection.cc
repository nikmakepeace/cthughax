/** @file
 * UI text helpers for selections stored in runtime Config snapshots.
 */

#include "RuntimeConfigSelection.h"

namespace {

static const std::string& selectionValue(
    const Config& config, RuntimeConfigSelectionField field) {
    switch (field) {
    case RuntimeConfigSelectionDisplay:
        return config.scene.presentation;
    case RuntimeConfigSelectionFlame:
        return config.scene.flame;
    case RuntimeConfigSelectionGeneralFlame:
        return config.scene.generalFlame;
    case RuntimeConfigSelectionBorder:
        return config.scene.border;
    case RuntimeConfigSelectionTranslation:
        return config.scene.translation;
    case RuntimeConfigSelectionWave:
        return config.scene.wave;
    case RuntimeConfigSelectionAudioProcessing:
        return config.scene.audioProcessing;
    case RuntimeConfigSelectionTable:
        return config.scene.table;
    case RuntimeConfigSelectionWaveScale:
        return config.scene.waveScale;
    case RuntimeConfigSelectionPalette:
        return config.scene.palette;
    case RuntimeConfigSelectionImage:
        return config.scene.image;
    case RuntimeConfigSelectionObject:
        return config.scene.object;
    case RuntimeConfigSelectionFlashlight:
        return config.scene.flashlight;
    }

    return config.scene.flame;
}

}

std::string runtimeConfigSelectionText(
    const Config& config, RuntimeConfigSelectionField field) {
    const std::string& value = selectionValue(config, field);
    if (!value.empty())
        return value;

    return "unknown";
}

std::string runtimeConfigSelectionTextOrFallback(const Config& config,
    RuntimeConfigSelectionField field, const char* fallback) {
    const std::string& value = selectionValue(config, field);
    if (!value.empty())
        return value;

    if (fallback != NULL && fallback[0] != '\0')
        return fallback;

    return "unknown";
}
