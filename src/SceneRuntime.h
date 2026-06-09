// Scene module ownership root.

#ifndef CTHUGHA_SCENE_RUNTIME_H
#define CTHUGHA_SCENE_RUNTIME_H

#include "Scene.h"
#include "SceneRuntimeDependencies.h"
#include "SceneSerializer.h"

#include <memory>

class RandomSource;
struct EffectPolicy;

/**
 * Owns the Scene state, command facade, selection registry, and persistence
 * contributor as one explicitly wired runtime module.
 */
class SceneRuntime {
    SceneSelectionRegistry sceneEffectRegistryValue;
    SceneSelectionPresetCatalog sceneSelectionPresetCatalogValue;
    SceneSelectionPolicyApplier sceneSelectionPolicyApplierValue;
    SceneSelectionState selectionStateValue;
    SceneVisualCatalogFactoryResult visualCatalogFactoryResultValue;
    Scene sceneValue;
    SceneCommands commandsValue;
    SceneCommandsTarget commandTargetValue;
    SceneSerializer serializerValue;

public:
    SceneRuntime(SceneGeometry& geometry,
        SceneVisualCatalogFactory& visualCatalogFactory,
        RandomSource& randomSource);
    ~SceneRuntime();

    void configureEffectPolicy(const EffectPolicy& policy);
    void applyStartupConfig(const SceneConfig& config);

    Scene& scene();
    const Scene& scene() const;
    SceneSnapshot snapshot() const;

    /**
     * @return Native visual selections owned by the visual catalog factory
     *         result, or NULL when no selection set is available.
     */
    SceneVisualSelections* visualSelections();

    SceneCommandTarget& commandTarget();
    SceneSerializer& serializer();
};

#endif
