// Factory for the temporary legacy Scene selection synchronizer.

#ifndef CTHUGHA_LEGACY_SCENE_SELECTION_SYNCHRONIZER_H
#define CTHUGHA_LEGACY_SCENE_SELECTION_SYNCHRONIZER_H

#include "SceneRuntimeDependencies.h"
#include "SceneVisualSelections.h"

#include <memory>

/**
 * Compatibility synchronizer for legacy visual control mirrors.
 *
 * Native Scene selections own the current visual state. This temporary bridge
 * pushes those values back into legacy controls that older rendering/catalog
 * code still reads while native visual owners are being introduced.
 */
std::unique_ptr<SceneRuntimeControlBridge> createLegacySceneSelectionSynchronizer(
    SceneVisualSelections& selections);

#endif
