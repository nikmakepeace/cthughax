// Factory for native Scene visual catalog services.

#include "SceneVisualCatalogServiceFactory.h"

#include "SceneVisualCatalogService.h"
#include "SceneVisualSelections.h"

#include <utility>

SceneVisualCatalogServiceFactory::SceneVisualCatalogServiceFactory(
    std::unique_ptr<SceneVisualSelections> ownedSelections_)
    : ownedSelections(std::move(ownedSelections_))
    , selections(*ownedSelections) { }

SceneVisualCatalogServiceFactory::~SceneVisualCatalogServiceFactory() { }

SceneVisualCatalogFactoryResult SceneVisualCatalogServiceFactory::create(
    SceneSelectionState& selectionState) {
    std::unique_ptr<SceneVisualCatalogs> visualCatalogs(
        new SceneVisualCatalogService(selectionState, selections));
    return SceneVisualCatalogFactoryResult(std::move(visualCatalogs),
        selections);
}

std::unique_ptr<SceneVisualCatalogFactory> createSceneVisualCatalogServiceFactory(
    std::unique_ptr<SceneVisualSelections> selections) {
    return std::unique_ptr<SceneVisualCatalogFactory>(
        new SceneVisualCatalogServiceFactory(std::move(selections)));
}
