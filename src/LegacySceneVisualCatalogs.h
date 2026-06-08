// Legacy visual catalog adapter for SceneRuntime.

#ifndef CTHUGHA_LEGACY_SCENE_VISUAL_CATALOGS_H
#define CTHUGHA_LEGACY_SCENE_VISUAL_CATALOGS_H

#include "LegacySceneCatalogAdapters.h"
#include "LegacySceneSelectionAdapters.h"
#include "SceneRuntimeDependencies.h"
#include "SceneVisualSelections.h"

#include <memory>

class ImageOption;
class LegacySceneControlMirror;
class SceneImageCatalog;
class ScenePaletteCatalog;
class SceneTranslationCatalog;
class SceneWaveObjectCatalog;

/**
 * Compatibility visual catalog adapter over legacy global EffectControls.
 */
class LegacySceneVisualCatalogs : public SceneVisualCatalogs {
    SceneSelectionState& selectionState;
    SceneVisualSelections& selections;
    LegacySceneControlMirror& controlMirror;
    ScenePaletteRandomizer& paletteRandomizer;

    Wave* selectRunnableWave(const WaveConfig& config, int& selectionChanged);

public:
    LegacySceneVisualCatalogs(SceneSelectionState& selectionState_,
        SceneVisualSelections& selections_,
        LegacySceneControlMirror& controlMirror_,
        ScenePaletteRandomizer& paletteRandomizer_);

    virtual const SceneSettings& currentSettings(SceneGeometry& geometry);
    virtual const IndexedImage* currentImage();

    virtual void applyStartupConfig(
        const SceneConfig& config, RandomSource& randomSource);
    virtual unsigned int change(
        SceneSelectionTarget target, int by, RandomSource& randomSource);
    virtual unsigned int change(SceneSelectionTarget target, const char* to,
        RandomSource& randomSource);
    virtual unsigned int activate(SceneSelectionTarget target, int index);
    virtual void toggleLock(SceneSelectionTarget target);
    virtual void toggleChoiceUse(SceneSelectionTarget target, int index);
    virtual unsigned int randomPalette(RandomSource& randomSource);
    virtual unsigned int addRandomPalette(RandomSource& randomSource);
};

class LegacySceneVisualCatalogFactory : public SceneVisualCatalogFactory {
    std::unique_ptr<LegacySceneSelectionAdapterSet> ownedAdapters;
    SceneVisualSelections& selections;
    LegacySceneControlMirror& controlMirror;
    std::unique_ptr<ScenePaletteRandomizer> paletteRandomizer;

public:
    explicit LegacySceneVisualCatalogFactory(
        std::unique_ptr<LegacySceneSelectionAdapterSet> ownedAdapters_);

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
