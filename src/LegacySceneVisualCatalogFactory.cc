// Legacy visual catalog factory over global visual options.

#include "LegacySceneVisualCatalogs.h"

#include "Border.h"
#include "Flashlight.h"
#include "Image.h"
#include "LegacySceneCatalogAdapters.h"
#include "LegacySceneSelectionAdapters.h"
#include "LegacySceneSelectionSynchronizer.h"
#include "TranslationOptions.h"
#include "display.h"
#include "flames.h"
#include "waves.h"

#include <utility>

LegacySceneVisualCatalogFactory::LegacySceneVisualCatalogFactory(
    SceneVisualSelections& selections_)
    : ownedSelections()
    , selections(selections_)
    , paletteRandomizer(createLegacyScenePaletteRandomizer()) { }

LegacySceneVisualCatalogFactory::LegacySceneVisualCatalogFactory(
    std::unique_ptr<SceneVisualSelections> ownedSelections_)
    : ownedSelections(std::move(ownedSelections_))
    , selections(*ownedSelections)
    , paletteRandomizer(createLegacyScenePaletteRandomizer()) { }

SceneVisualCatalogFactoryResult LegacySceneVisualCatalogFactory::create(
    SceneSelectionState& selectionState) {
    std::unique_ptr<SceneVisualCatalogs> visualCatalogs(
        new LegacySceneVisualCatalogs(
            selectionState, selections, *paletteRandomizer));
    std::unique_ptr<SceneRuntimeControlBridge> controlBridge
        = createLegacySceneSelectionSynchronizer(selections);
    return SceneVisualCatalogFactoryResult(std::move(visualCatalogs),
        std::move(controlBridge), selections);
}

std::unique_ptr<SceneVisualCatalogFactory> createLegacySceneVisualCatalogFactory(
    ImageOption& images) {
    return std::unique_ptr<SceneVisualCatalogFactory>(
        new LegacySceneVisualCatalogFactory(createLegacySceneSelectionAdapters(
            flame, flameGeneral, wave, waveScale, table, object, translation,
            palette, border, flashlight, images)));
}
