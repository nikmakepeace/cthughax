// Legacy visual catalog factory for SceneRuntime.

#ifndef CTHUGHA_LEGACY_SCENE_VISUAL_CATALOG_FACTORY_H
#define CTHUGHA_LEGACY_SCENE_VISUAL_CATALOG_FACTORY_H

#include "SceneRuntimeDependencies.h"

#include <memory>

class ImageOption;
class LegacySceneSelectionAdapterSet;
class SceneImageCatalog;
class ScenePaletteCatalog;
class ScenePaletteRandomizer;
class SceneTranslationCatalog;
class SceneWaveObjectCatalog;

/**
 * Temporary Scene visual factory backed by legacy global visual controls.
 *
 * This factory creates native Scene visual catalog services and pairs them with
 * the legacy selection synchronizer required while old controls still mirror
 * Scene selections.
 */
class LegacySceneVisualCatalogFactory : public SceneVisualCatalogFactory {
    std::unique_ptr<LegacySceneSelectionAdapterSet> ownedAdapters;
    SceneVisualSelections& selections;
    std::unique_ptr<ScenePaletteRandomizer> paletteRandomizer;

public:
    /**
     * Creates a factory around prebuilt legacy-backed selections.
     *
     * @param ownedAdapters_ Selection adapter set owned by the factory.
     */
    explicit LegacySceneVisualCatalogFactory(
        std::unique_ptr<LegacySceneSelectionAdapterSet> ownedAdapters_);

    /** Destroys owned adapter and randomizer objects after SceneRuntime use. */
    ~LegacySceneVisualCatalogFactory();

    /**
     * Creates visual catalogs, control bridge, and selection registry input.
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
 * EffectControl-backed visual catalogs until native Scene visual catalogs
 * replace them.
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
