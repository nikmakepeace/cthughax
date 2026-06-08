// Legacy global Scene palette randomizer factory.

#ifndef CTHUGHA_LEGACY_SCENE_PALETTE_RANDOMIZER_H
#define CTHUGHA_LEGACY_SCENE_PALETTE_RANDOMIZER_H

#include "ScenePaletteRandomizer.h"

#include <memory>

/**
 * Creates the temporary random-palette bridge backed by the global palette.
 *
 * This is intentionally separate from Scene visual catalog ownership so the
 * remaining legacy global dependency is isolated to random/add-random palette
 * behavior until palette persistence moves to native visual owners.
 */
std::unique_ptr<ScenePaletteRandomizer> createLegacyScenePaletteRandomizer();

#endif
