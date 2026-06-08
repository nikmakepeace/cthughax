// Legacy visual catalog factory for SceneRuntime.

#ifndef CTHUGHA_LEGACY_SCENE_VISUAL_CATALOG_FACTORY_H
#define CTHUGHA_LEGACY_SCENE_VISUAL_CATALOG_FACTORY_H

#include "SceneRuntimeDependencies.h"

#include <memory>

class ImageOption;
class SceneImageCatalog;
class ScenePaletteCatalog;
class SceneTranslationCatalog;
class SceneVisualSelections;
class SceneWaveObjectCatalog;

/**
 * Temporary Scene visual factory backed by legacy global visual controls.
 *
 * This factory creates native Scene visual catalog services from startup values
 * read out of the legacy globals. Runtime visual changes stay in the native
 * Scene selections owned here.
 */
class LegacySceneVisualCatalogFactory : public SceneVisualCatalogFactory {
    std::unique_ptr<SceneVisualSelections> ownedSelections;
    SceneVisualSelections& selections;

public:
    /**
     * Creates a factory around prebuilt Scene selections.
     *
     * @param ownedSelections_ Scene visual selections owned by the factory.
     */
    explicit LegacySceneVisualCatalogFactory(
        std::unique_ptr<SceneVisualSelections> ownedSelections_);

    /** Destroys owned selections after SceneRuntime use. */
    ~LegacySceneVisualCatalogFactory();

    /**
     * Creates visual catalogs, no-op sync hook, and selection registry input.
     *
     * @param selectionState Storage for the current Scene settings snapshot.
     * @return Scene runtime visual catalog wiring.
     */
    virtual SceneVisualCatalogFactoryResult create(
        SceneSelectionState& selectionState);
};

/**
 * Creates the temporary legacy visual catalog factory.
 *
 * The caller supplies only the non-global image option owned by Application.
 * The factory implementation quarantines the remaining global
 * EffectControl-backed startup reads until native Scene visual loaders replace
 * them.
 *
 * @param images Image option owned by Application.
 * @param waveObjects Native Scene-owned wave-object catalog.
 * @param imageCatalog Native Scene-owned image catalog.
 * @param paletteCatalog Native Scene-owned palette catalog.
 * @param translations Native Scene-owned translation catalog.
 * @return Scene visual catalog factory for the current legacy catalogs.
 */
std::unique_ptr<SceneVisualCatalogFactory> createLegacySceneVisualCatalogFactory(
    ImageOption& images, const SceneWaveObjectCatalog& waveObjects,
    const SceneImageCatalog& imageCatalog,
    const ScenePaletteCatalog& paletteCatalog,
    const SceneTranslationCatalog& translations);

#endif
