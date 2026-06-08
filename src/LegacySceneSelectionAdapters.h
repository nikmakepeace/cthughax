// Factory for legacy EffectControl-backed Scene visual selection adapters.

#ifndef CTHUGHA_LEGACY_SCENE_SELECTION_ADAPTERS_H
#define CTHUGHA_LEGACY_SCENE_SELECTION_ADAPTERS_H

#include "LegacySceneControlMirror.h"
#include "SceneVisualSelections.h"

#include <memory>

class EffectControl;
class SceneImageCatalog;
class ScenePaletteCatalog;
class SceneTranslationCatalog;
class SceneWaveObjectCatalog;

/**
 * Owned legacy selection adapter plus its explicit control mirror.
 *
 * The mirror points into the selection adapter object owned by selections; it
 * is exposed separately so Scene wiring does not need to discover it through
 * RTTI or legacy EffectControl identity.
 */
class LegacySceneSelectionAdapterSet {
public:
    std::unique_ptr<SceneVisualSelections> selections;
    LegacySceneControlMirror& controlMirror;

    LegacySceneSelectionAdapterSet(
        std::unique_ptr<SceneVisualSelections> selections_,
        LegacySceneControlMirror& controlMirror_);
    ~LegacySceneSelectionAdapterSet();
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
 * Wraps prebuilt native selections with legacy control lookup/sync bindings.
 *
 * @param selections Owned native selection set to expose through the adapter.
 */
std::unique_ptr<LegacySceneSelectionAdapterSet>
createLegacySceneSelectionAdapters(
    EffectControl& flame, EffectControl& generalFlame, EffectControl& wave,
    EffectControl& waveScale, EffectControl& table, EffectControl& object,
    EffectControl& translation, EffectControl& palette, EffectControl& border,
    EffectControl& flashlight, EffectControl& images,
    std::unique_ptr<SceneVisualSelections> selections);

#endif
