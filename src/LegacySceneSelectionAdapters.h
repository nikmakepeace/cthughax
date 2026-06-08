// Factory for legacy EffectControl-backed Scene visual selection adapters.

#ifndef CTHUGHA_LEGACY_SCENE_SELECTION_ADAPTERS_H
#define CTHUGHA_LEGACY_SCENE_SELECTION_ADAPTERS_H

#include "SceneVisualSelections.h"

#include <memory>

class EffectControl;
class LegacySceneControlMirror;
class SceneImageCatalog;
class ScenePaletteCatalog;
class SceneSelectionSynchronizer;
class SceneTranslationCatalog;
class SceneWaveObjectCatalog;

/**
 * Owned native selections plus their explicit legacy control mirror.
 *
 * The mirror is intentionally separate from the selection set so native Scene
 * selections do not implement legacy EffectControl identity or synchronization
 * behavior.
 */
class LegacySceneSelectionAdapterSet {
public:
    std::unique_ptr<SceneVisualSelections> selections;

private:
    std::unique_ptr<LegacySceneControlMirror> controlMirror;

public:
    LegacySceneSelectionAdapterSet(
        std::unique_ptr<SceneVisualSelections> selections_,
        std::unique_ptr<LegacySceneControlMirror> controlMirror_);
    ~LegacySceneSelectionAdapterSet();

    /**
     * Creates the temporary one-way synchronizer for these adapters.
     *
     * @return Synchronizer that mirrors Scene selection values into the bound
     *         legacy controls.
     */
    std::unique_ptr<SceneSelectionSynchronizer> createSelectionSynchronizer();
};

/**
 * Builds legacy-backed Scene selections from current global visual controls.
 *
 * This is the compatibility factory used while catalog loading still lives in
 * legacy EffectControl instances.
 */
std::unique_ptr<LegacySceneSelectionAdapterSet>
createLegacySceneSelectionAdapters(
    EffectControl& flame, EffectControl& generalFlame, EffectControl& wave,
    EffectControl& waveScale, EffectControl& table, EffectControl& object,
    EffectControl& translation, EffectControl& palette, EffectControl& border,
    EffectControl& flashlight, EffectControl& images,
    const SceneWaveObjectCatalog& waveObjects,
    const SceneImageCatalog& imageCatalog,
    const ScenePaletteCatalog& paletteCatalog,
    const SceneTranslationCatalog& translations);

/**
 * Wraps prebuilt native selections with a legacy control sync mirror.
 *
 * @param selections Owned native selection set to expose to Scene.
 */
std::unique_ptr<LegacySceneSelectionAdapterSet>
createLegacySceneSelectionAdapters(
    EffectControl& flame, EffectControl& generalFlame, EffectControl& wave,
    EffectControl& waveScale, EffectControl& table, EffectControl& object,
    EffectControl& translation, EffectControl& palette, EffectControl& border,
    EffectControl& flashlight, EffectControl& images,
    std::unique_ptr<SceneVisualSelections> selections);

#endif
