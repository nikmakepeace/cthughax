// Owned collection of Scene visual selection ports.

#include "SceneVisualSelectionSet.h"

SceneVisualSelectionSet::SceneVisualSelectionSet(SceneFlameSelection* flame_,
    SceneGeneralFlameSelection* generalFlame_, SceneWaveSelection* wave_,
    SceneOptionSelection* waveScale_, SceneOptionSelection* table_,
    SceneOptionSelection* object_, SceneTranslationSelection* translation_,
    ScenePaletteSelection* palette_, SceneOptionSelection* border_,
    SceneOptionSelection* flashlight_, SceneImageSelection* images_)
    : flameValue(flame_)
    , generalFlameValue(generalFlame_)
    , waveValue(wave_)
    , waveScaleValue(waveScale_)
    , tableValue(table_)
    , objectValue(object_)
    , translationValue(translation_)
    , paletteValue(palette_)
    , borderValue(border_)
    , flashlightValue(flashlight_)
    , imagesValue(images_) { }

SceneFlameSelection& SceneVisualSelectionSet::flame() {
    return *flameValue;
}

SceneGeneralFlameSelection& SceneVisualSelectionSet::generalFlame() {
    return *generalFlameValue;
}

SceneWaveSelection& SceneVisualSelectionSet::wave() {
    return *waveValue;
}

SceneOptionSelection& SceneVisualSelectionSet::waveScale() {
    return *waveScaleValue;
}

SceneOptionSelection& SceneVisualSelectionSet::table() {
    return *tableValue;
}

SceneOptionSelection& SceneVisualSelectionSet::object() {
    return *objectValue;
}

SceneTranslationSelection& SceneVisualSelectionSet::translation() {
    return *translationValue;
}

ScenePaletteSelection& SceneVisualSelectionSet::palette() {
    return *paletteValue;
}

SceneOptionSelection& SceneVisualSelectionSet::border() {
    return *borderValue;
}

SceneOptionSelection& SceneVisualSelectionSet::flashlight() {
    return *flashlightValue;
}

SceneImageSelection& SceneVisualSelectionSet::images() {
    return *imagesValue;
}
