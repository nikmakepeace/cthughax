// Native construction for Scene visual selections.

#ifndef CTHUGHA_SCENE_VISUAL_SELECTION_FACTORY_H
#define CTHUGHA_SCENE_VISUAL_SELECTION_FACTORY_H

#include <memory>
#include <string>

class SceneImageCatalog;
class ScenePaletteCatalog;
class SceneTranslationCatalog;
class SceneVisualSelections;
class SceneWaveObjectCatalog;

/**
 * Initial value, lock, and catalog-name seed for one Scene visual selection.
 */
class SceneOptionSelectionSeed {
public:
    std::string catalogName;
    int selectedValue;
    int lockEnabled;

    /** Creates an empty unlocked seed at selection index zero. */
    SceneOptionSelectionSeed();

    /**
     * Creates a selection seed.
     *
     * @param catalogName_ Stable Scene catalog name.
     * @param selectedValue_ Initial selected index.
     * @param lockEnabled_ Nonzero when random changes start locked.
     */
    SceneOptionSelectionSeed(
        const char* catalogName_, int selectedValue_, int lockEnabled_);
};

/**
 * Initial seed values for every Scene visual selection.
 */
class SceneVisualSelectionSeeds {
public:
    SceneOptionSelectionSeed flame;
    SceneOptionSelectionSeed generalFlame;
    SceneOptionSelectionSeed wave;
    SceneOptionSelectionSeed waveScale;
    SceneOptionSelectionSeed table;
    SceneOptionSelectionSeed object;
    SceneOptionSelectionSeed translation;
    SceneOptionSelectionSeed palette;
    SceneOptionSelectionSeed border;
    SceneOptionSelectionSeed flashlight;
    SceneOptionSelectionSeed images;

    /** Creates seeds with the standard Scene catalog names and zero values. */
    SceneVisualSelectionSeeds();
};

/**
 * Builds native Scene visual selections from explicit seed values.
 *
 * @param seeds Initial catalog names, selected values, and locks.
 * @param waveObjects Native wave-object catalog.
 * @param imageCatalog Native image catalog.
 * @param paletteCatalog Native palette catalog.
 * @param translations Native translation catalog.
 * @return Owned native visual selections.
 */
std::unique_ptr<SceneVisualSelections> createSceneVisualSelections(
    const SceneVisualSelectionSeeds& seeds,
    const SceneWaveObjectCatalog& waveObjects,
    const SceneImageCatalog& imageCatalog,
    const ScenePaletteCatalog& paletteCatalog,
    const SceneTranslationCatalog& translations);

#endif
