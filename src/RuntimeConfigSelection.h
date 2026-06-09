/** @file
 * UI text helpers for selections stored in runtime Config snapshots.
 */

#ifndef CTHUGHA_RUNTIME_CONFIG_SELECTION_H
#define CTHUGHA_RUNTIME_CONFIG_SELECTION_H

#include "Configuration.h"

#include <string>

/** Runtime Config selection fields that UI surfaces can display. */
enum RuntimeConfigSelectionField {
    RuntimeConfigSelectionDisplay,
    RuntimeConfigSelectionFlame,
    RuntimeConfigSelectionGeneralFlame,
    RuntimeConfigSelectionBorder,
    RuntimeConfigSelectionTranslation,
    RuntimeConfigSelectionWave,
    RuntimeConfigSelectionAudioProcessing,
    RuntimeConfigSelectionTable,
    RuntimeConfigSelectionWaveScale,
    RuntimeConfigSelectionPalette,
    RuntimeConfigSelectionImage,
    RuntimeConfigSelectionObject,
    RuntimeConfigSelectionFlashlight
};

/**
 * Returns display text for one selected runtime Config field.
 *
 * @param config Config snapshot to read.
 * @param field Selection field to display.
 * @return Selection text, or "unknown" when the snapshot has no value.
 */
std::string runtimeConfigSelectionText(
    const Config& config, RuntimeConfigSelectionField field);

/**
 * Returns display text for one selected runtime Config field with fallback.
 *
 * @param config Config snapshot to read.
 * @param field Selection field to display.
 * @param fallback Text to use when the snapshot has no value.
 * @return Selection text, fallback text, or "unknown".
 */
std::string runtimeConfigSelectionTextOrFallback(const Config& config,
    RuntimeConfigSelectionField field, const char* fallback);

#endif
