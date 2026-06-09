// Native Scene visual catalog and selection service.

#ifndef CTHUGHA_SCENE_VISUAL_CATALOG_SERVICE_H
#define CTHUGHA_SCENE_VISUAL_CATALOG_SERVICE_H

#include "ScenePaletteRandomizer.h"
#include "SceneRuntimeDependencies.h"

class SceneVisualSelections;

/**
 * Scene visual catalog implementation over explicit native selections.
 *
 * This service reads and mutates Scene-owned selections, builds current
 * `SceneSettings`, emits image/palette forced-change flags, and owns native
 * random-palette mutation for Scene palette selections.
 */
class SceneVisualCatalogService : public SceneVisualCatalogs {
    SceneSelectionState& selectionState;
    SceneVisualSelections& selections;
    ScenePaletteRandomizer paletteRandomizer;

    Wave* selectRunnableWave(const WaveConfig& config);

public:
    /**
     * Creates a visual catalog service over owned selection state.
     *
     * @param selectionState_ Storage for the latest computed settings snapshot.
     * @param selections_ Scene-owned visual selections.
     */
    SceneVisualCatalogService(SceneSelectionState& selectionState_,
        SceneVisualSelections& selections_);

    /** Builds current settings from native visual selections. */
    virtual const SceneSettings& currentSettings(SceneGeometry& geometry);

    /** @return Currently selected image payload, or NULL. */
    virtual const IndexedImage* currentImage();

    /** Applies startup Scene choices to native visual selections. */
    virtual void applyStartupConfig(
        const SceneConfig& config, RandomSource& randomSource);

    /** Changes one Scene visual selection by relative offset. */
    virtual unsigned int change(
        SceneSelectionTarget target, int by, RandomSource& randomSource);

    /** Changes one Scene visual selection by value text. */
    virtual unsigned int change(SceneSelectionTarget target, const char* to,
        RandomSource& randomSource);

    /** Activates one indexed choice on a Scene visual selection. */
    virtual unsigned int activate(SceneSelectionTarget target, int index);

    /** Toggles lock state on a Scene visual selection. */
    virtual void toggleLock(SceneSelectionTarget target);

    /** Toggles use state for one indexed visual choice. */
    virtual void toggleChoiceUse(SceneSelectionTarget target, int index);

    /** Randomizes and selects the latest random palette slot. */
    virtual unsigned int randomPalette(RandomSource& randomSource);

    /** Adds and selects a new random palette slot. */
    virtual unsigned int addRandomPalette(RandomSource& randomSource);
};

#endif
