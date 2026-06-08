// Factory for legacy EffectControl-backed Scene visual selection adapters.

#ifndef CTHUGHA_LEGACY_SCENE_SELECTION_ADAPTERS_H
#define CTHUGHA_LEGACY_SCENE_SELECTION_ADAPTERS_H

#include <memory>

class LegacySceneSelectionAdapterState;
class SceneSelectionSynchronizer;
class SceneVisualSelections;

/**
 * Owned native selections plus their explicit legacy control mirror.
 *
 * The mirror is intentionally separate from the selection set so native Scene
 * selections do not implement legacy EffectControl identity or synchronization
 * behavior.
 */
class LegacySceneSelectionAdapterSet {
    std::unique_ptr<LegacySceneSelectionAdapterState> state;

public:
    /** Creates a handle around private legacy adapter state. */
    explicit LegacySceneSelectionAdapterSet(
        std::unique_ptr<LegacySceneSelectionAdapterState> state_);

    ~LegacySceneSelectionAdapterSet();

    /** @return Native selections owned by this adapter set. */
    SceneVisualSelections& selections();

    /**
     * Creates the temporary one-way synchronizer for these adapters.
     *
     * @return Synchronizer that mirrors Scene selection values into the bound
     *         legacy controls.
     */
    std::unique_ptr<SceneSelectionSynchronizer> createSelectionSynchronizer();
};

#endif
