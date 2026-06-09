// Owned collection of Scene visual selection ports.

#ifndef CTHUGHA_SCENE_VISUAL_SELECTION_SET_H
#define CTHUGHA_SCENE_VISUAL_SELECTION_SET_H

#include "SceneVisualSelections.h"

#include <memory>

/**
 * Native owner for the complete set of Scene visual selection ports.
 *
 * The set owns already-constructed typed selections and exposes them through
 * the SceneVisualSelections interface without knowing about legacy globals or
 * identity-based control routing.
 */
class SceneVisualSelectionSet : public SceneVisualSelections {
    std::unique_ptr<SceneFlameSelection> flameValue;
    std::unique_ptr<SceneGeneralFlameSelection> generalFlameValue;
    std::unique_ptr<SceneWaveSelection> waveValue;
    std::unique_ptr<SceneOptionSelection> waveScaleValue;
    std::unique_ptr<SceneOptionSelection> tableValue;
    std::unique_ptr<SceneOptionSelection> objectValue;
    std::unique_ptr<SceneTranslationSelection> translationValue;
    std::unique_ptr<ScenePaletteSelection> paletteValue;
    std::unique_ptr<SceneOptionSelection> borderValue;
    std::unique_ptr<SceneOptionSelection> flashlightValue;
    std::unique_ptr<SceneImageSelection> imagesValue;

public:
    /**
     * Creates an owned visual selection set.
     *
     * All arguments transfer ownership and must be non-NULL.
     */
    SceneVisualSelectionSet(SceneFlameSelection* flame_,
        SceneGeneralFlameSelection* generalFlame_, SceneWaveSelection* wave_,
        SceneOptionSelection* waveScale_, SceneOptionSelection* table_,
        SceneOptionSelection* object_,
        SceneTranslationSelection* translation_,
        ScenePaletteSelection* palette_, SceneOptionSelection* border_,
        SceneOptionSelection* flashlight_, SceneImageSelection* images_);

    /** @return Flame selection. */
    virtual SceneFlameSelection& flame();

    /** @return General-flame selection. */
    virtual SceneGeneralFlameSelection& generalFlame();

    /** @return Wave selection. */
    virtual SceneWaveSelection& wave();

    /** @return Wave-scale selection. */
    virtual SceneOptionSelection& waveScale();

    /** @return Wave-table selection. */
    virtual SceneOptionSelection& table();

    /** @return Wave-object selection. */
    virtual SceneOptionSelection& object();

    /** @return Translation selection. */
    virtual SceneTranslationSelection& translation();

    /** @return Palette selection. */
    virtual ScenePaletteSelection& palette();

    /** @return Border selection. */
    virtual SceneOptionSelection& border();

    /** @return Flashlight selection. */
    virtual SceneOptionSelection& flashlight();

    /** @return Image selection. */
    virtual SceneImageSelection& images();
};

#endif
