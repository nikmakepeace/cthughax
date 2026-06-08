// Factory for native Scene visual catalog services.

#ifndef CTHUGHA_SCENE_VISUAL_CATALOG_SERVICE_FACTORY_H
#define CTHUGHA_SCENE_VISUAL_CATALOG_SERVICE_FACTORY_H

#include "SceneRuntimeDependencies.h"

#include <memory>

class SceneVisualSelections;

/**
 * Scene visual catalog factory over an owned native selection set.
 */
class SceneVisualCatalogServiceFactory : public SceneVisualCatalogFactory {
    std::unique_ptr<SceneVisualSelections> ownedSelections;
    SceneVisualSelections& selections;

public:
    /**
     * Creates a factory around prebuilt Scene visual selections.
     *
     * @param ownedSelections_ Native selection set owned by this factory.
     */
    explicit SceneVisualCatalogServiceFactory(
        std::unique_ptr<SceneVisualSelections> ownedSelections_);

    /** Destroys owned selections after SceneRuntime use. */
    ~SceneVisualCatalogServiceFactory();

    /**
     * Creates visual catalog services over the owned selection set.
     *
     * @param selectionState Storage for the current Scene settings snapshot.
     * @return Scene runtime visual catalog wiring.
     */
    virtual SceneVisualCatalogFactoryResult create(
        SceneSelectionState& selectionState);
};

/**
 * Creates a native Scene visual catalog factory around owned selections.
 *
 * @param selections Native visual selections owned by the returned factory.
 * @return Scene visual catalog factory.
 */
std::unique_ptr<SceneVisualCatalogFactory> createSceneVisualCatalogServiceFactory(
    std::unique_ptr<SceneVisualSelections> selections);

#endif
