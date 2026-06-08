// Legacy visual catalog factory over global visual options.

#include "LegacySceneVisualCatalogFactory.h"

#include "LegacyGlobalSceneSelectionFactory.h"
#include "SceneImageCatalog.h"
#include "ScenePaletteCatalog.h"
#include "SceneTranslationCatalog.h"
#include "SceneVisualCatalogService.h"
#include "SceneVisualSelections.h"
#include "SceneWaveObjectCatalog.h"

#include <utility>

LegacySceneVisualCatalogFactory::LegacySceneVisualCatalogFactory(
    std::unique_ptr<SceneVisualSelections> ownedSelections_)
    : ownedSelections(std::move(ownedSelections_))
    , selections(*ownedSelections) { }

LegacySceneVisualCatalogFactory::~LegacySceneVisualCatalogFactory() { }

SceneVisualCatalogFactoryResult LegacySceneVisualCatalogFactory::create(
    SceneSelectionState& selectionState) {
    std::unique_ptr<SceneVisualCatalogs> visualCatalogs(
        new SceneVisualCatalogService(selectionState, selections));
    return SceneVisualCatalogFactoryResult(std::move(visualCatalogs),
        selections);
}

std::unique_ptr<SceneVisualCatalogFactory> createLegacySceneVisualCatalogFactory(
    ImageOption& images, const SceneWaveObjectCatalog& waveObjects,
    const SceneImageCatalog& imageCatalog,
    const ScenePaletteCatalog& paletteCatalog,
    const SceneTranslationCatalog& translations) {
    return std::unique_ptr<SceneVisualCatalogFactory>(
        new LegacySceneVisualCatalogFactory(
            createLegacyGlobalSceneVisualSelections(images, waveObjects,
                imageCatalog, paletteCatalog, translations)));
}
