// Scene module ownership root.

#include "SceneRuntime.h"

SceneRuntime::SceneRuntime(SceneGeometry& geometry,
    SceneVisualCatalogFactory& visualCatalogFactory,
    RandomSource& randomSource)
    : sceneEffectRegistryValue()
    , sceneSelectionPresetCatalogValue(sceneEffectRegistryValue)
    , sceneSelectionPolicyApplierValue(sceneEffectRegistryValue,
          sceneSelectionPresetCatalogValue)
    , selectionStateValue()
    , visualCatalogFactoryResultValue(
          visualCatalogFactory.create(selectionStateValue))
    , sceneValue()
    , commandsValue(sceneValue, geometry,
          SceneCommandDependencies(
              *visualCatalogFactoryResultValue.visualCatalogs,
              *visualCatalogFactoryResultValue.selectionSync,
              sceneEffectRegistryValue, sceneSelectionPresetCatalogValue),
          randomSource)
    , commandTargetValue(commandsValue)
    , serializerValue(sceneValue) {
    if (visualCatalogFactoryResultValue.selections != 0)
        sceneEffectRegistryValue.registerSelections(
            *visualCatalogFactoryResultValue.selections);
}

SceneRuntime::~SceneRuntime() { }

void SceneRuntime::configureEffectPolicy(const EffectPolicy& policy) {
    sceneSelectionPolicyApplierValue.configure(policy);
}

void SceneRuntime::applyStartupConfig(const SceneConfig& config) {
    commandsValue.applyStartupConfig(config);
}

Scene& SceneRuntime::scene() {
    return sceneValue;
}

const Scene& SceneRuntime::scene() const {
    return sceneValue;
}

SceneSnapshot SceneRuntime::snapshot() const {
    return sceneValue.snapshot();
}

SceneVisualSelections* SceneRuntime::visualSelections() {
    return visualCatalogFactoryResultValue.selections;
}

SceneCommandTarget& SceneRuntime::commandTarget() {
    return commandTargetValue;
}

SceneSerializer& SceneRuntime::serializer() {
    return serializerValue;
}
