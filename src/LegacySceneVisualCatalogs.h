// Legacy visual catalog adapter for SceneRuntime.

#ifndef CTHUGHA_LEGACY_SCENE_VISUAL_CATALOGS_H
#define CTHUGHA_LEGACY_SCENE_VISUAL_CATALOGS_H

#include "LegacySceneCatalogAdapters.h"
#include "SceneRuntimeDependencies.h"
#include "SceneVisualSelections.h"

#include <memory>

class ImageOption;

/**
 * Compatibility visual catalog adapter over legacy global EffectControls.
 */
class LegacySceneVisualCatalogs : public SceneVisualCatalogs {
    SceneSelectionState& selectionState;
    SceneVisualSelections& selections;
    SceneWaveObjectSource& waveObjects;
    ScenePaletteRandomizer& paletteRandomizer;

    Wave* selectRunnableWave(const WaveConfig& config);

public:
    LegacySceneVisualCatalogs(SceneSelectionState& selectionState_,
        SceneVisualSelections& selections_,
        SceneWaveObjectSource& waveObjects_,
        ScenePaletteRandomizer& paletteRandomizer_);

    virtual const SceneSettings& currentSettings(SceneGeometry& geometry);
    virtual const IndexedImage* currentImage();

    virtual void applyStartupConfig(
        const SceneConfig& config, RandomSource& randomSource);
    virtual unsigned int change(
        SceneSelectionTarget target, int by, RandomSource& randomSource);
    virtual unsigned int change(SceneSelectionTarget target, const char* to,
        RandomSource& randomSource);
    virtual unsigned int randomPalette(RandomSource& randomSource);
    virtual unsigned int addRandomPalette(RandomSource& randomSource);
};

class LegacySceneVisualCatalogFactory : public SceneVisualCatalogFactory {
    std::unique_ptr<SceneVisualSelections> ownedSelections;
    SceneVisualSelections& selections;
    std::unique_ptr<SceneWaveObjectSource> waveObjects;
    std::unique_ptr<ScenePaletteRandomizer> paletteRandomizer;

public:
    explicit LegacySceneVisualCatalogFactory(SceneVisualSelections& selections_);
    explicit LegacySceneVisualCatalogFactory(
        std::unique_ptr<SceneVisualSelections> ownedSelections_);

    virtual SceneVisualCatalogFactoryResult create(
        SceneSelectionState& selectionState);
};

/**
 * Creates the temporary legacy visual catalog factory.
 *
 * The caller supplies only the non-global image option owned by the Frame
 * Generator. The factory implementation quarantines the remaining global
 * EffectControl-backed visual catalogs until native Scene visual catalogs
 * replace them.
 *
 * @param images Image option owned by FrameGeneratorRuntime scene binding.
 * @return Scene visual catalog factory for the current legacy catalogs.
 */
std::unique_ptr<SceneVisualCatalogFactory> createLegacySceneVisualCatalogFactory(
    ImageOption& images);

#endif
