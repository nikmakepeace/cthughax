/** @file
 * Transitional runtime configuration contributor.
 */

#include "RuntimeConfigRegistry.h"

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

LegacyRuntimeConfigContributor::LegacyRuntimeConfigContributor() { }

void LegacyRuntimeConfigContributor::contribute(Config& config) const {
    config.scene.presentation = persistedName(screen.currentName());

    config.effectPolicy.useTranslatesEnabled = int(use_translates);
    config.effectPolicy.useObjectsEnabled = int(use_objects);
}
