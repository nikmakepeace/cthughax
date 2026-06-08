// Legacy EffectControl-backed Scene visual selection adapters.

#include "LegacySceneSelectionAdapters.h"

#include "EffectControl.h"
#include "SceneDependencies.h"
#include "SceneVisualSelections.h"

#include <memory>
#include <utility>

/**
 * Adapter-level mirror for legacy visual EffectControls.
 *
 * Native Scene selections should not implement legacy EffectControl identity
 * APIs. This private mirror copies the Scene-owned selection values into
 * legacy controls still read by older catalog/display code.
 */
class LegacySceneControlMirror {
public:
    /** Destroys the compatibility mirror. */
    virtual ~LegacySceneControlMirror() { }

    /** Synchronizes all legacy control values from their bound selections. */
    virtual void syncControlsFromSelections() = 0;
};

namespace {

static void syncControlFromSelection(
    EffectControl& control, SceneOptionSelection& selection) {
    control.setValue(selection.currentValue());
    control.lock.setValue(selection.lockEnabled());
}

class LegacySceneSelectionMirror : public LegacySceneControlMirror {
    EffectControl& flameControl;
    EffectControl& generalFlameControl;
    EffectControl& waveControl;
    EffectControl& waveScaleControl;
    EffectControl& tableControl;
    EffectControl& objectControl;
    EffectControl& translationControl;
    EffectControl& paletteControl;
    EffectControl& borderControl;
    EffectControl& flashlightControl;
    EffectControl& imagesControl;
    SceneVisualSelections& selections;

public:
    LegacySceneSelectionMirror(EffectControl& flame_,
        EffectControl& generalFlame_, EffectControl& wave_,
        EffectControl& waveScale_, EffectControl& table_,
        EffectControl& object_, EffectControl& translation_,
        EffectControl& palette_, EffectControl& border_,
        EffectControl& flashlight_, EffectControl& images_,
        SceneVisualSelections& selections_);

    virtual void syncControlsFromSelections();
};

LegacySceneSelectionMirror::LegacySceneSelectionMirror(EffectControl& flame_,
    EffectControl& generalFlame_, EffectControl& wave_,
    EffectControl& waveScale_, EffectControl& table_, EffectControl& object_,
    EffectControl& translation_, EffectControl& palette_,
    EffectControl& border_, EffectControl& flashlight_, EffectControl& images_,
    SceneVisualSelections& selections_)
    : flameControl(flame_)
    , generalFlameControl(generalFlame_)
    , waveControl(wave_)
    , waveScaleControl(waveScale_)
    , tableControl(table_)
    , objectControl(object_)
    , translationControl(translation_)
    , paletteControl(palette_)
    , borderControl(border_)
    , flashlightControl(flashlight_)
    , imagesControl(images_)
    , selections(selections_) { }

void LegacySceneSelectionMirror::syncControlsFromSelections() {
    syncControlFromSelection(flameControl, selections.flame());
    syncControlFromSelection(generalFlameControl, selections.generalFlame());
    syncControlFromSelection(waveControl, selections.wave());
    syncControlFromSelection(waveScaleControl, selections.waveScale());
    syncControlFromSelection(tableControl, selections.table());
    syncControlFromSelection(objectControl, selections.object());
    syncControlFromSelection(translationControl, selections.translation());
    syncControlFromSelection(paletteControl, selections.palette());
    syncControlFromSelection(borderControl, selections.border());
    syncControlFromSelection(flashlightControl, selections.flashlight());
    syncControlFromSelection(imagesControl, selections.images());
}

class LegacySceneSelectionSynchronizer : public SceneSelectionSynchronizer {
    SceneVisualSelections& selections;
    LegacySceneControlMirror& mirror;
    int syncedImageValue;

    unsigned int imageChangeFrom(int previousImageValue) {
        return (selections.images().currentValue() != previousImageValue)
            ? SceneImageChanged
            : SceneNoChange;
    }

    void syncBoundControlsFromSelections() {
        mirror.syncControlsFromSelections();
        syncedImageValue = selections.images().currentValue();
    }

public:
    LegacySceneSelectionSynchronizer(SceneVisualSelections& selections_,
        LegacySceneControlMirror& mirror_)
        : selections(selections_)
        , mirror(mirror_)
        , syncedImageValue(selections_.images().currentValue()) { }

    virtual unsigned int syncControlsFromSelections() {
        int previousImageValue = syncedImageValue;

        syncBoundControlsFromSelections();

        return imageChangeFrom(previousImageValue);
    }
};

}

class LegacySceneSelectionAdapterState {
public:
    std::unique_ptr<SceneVisualSelections> selections;
    std::unique_ptr<LegacySceneControlMirror> controlMirror;

    LegacySceneSelectionAdapterState(
        std::unique_ptr<SceneVisualSelections> selections_,
        std::unique_ptr<LegacySceneControlMirror> controlMirror_)
        : selections(std::move(selections_))
        , controlMirror(std::move(controlMirror_)) { }
};

LegacySceneSelectionAdapterSet::LegacySceneSelectionAdapterSet(
    std::unique_ptr<LegacySceneSelectionAdapterState> state_)
    : state(std::move(state_)) { }

LegacySceneSelectionAdapterSet::~LegacySceneSelectionAdapterSet() { }

SceneVisualSelections& LegacySceneSelectionAdapterSet::selections() {
    return *state->selections;
}

std::unique_ptr<SceneSelectionSynchronizer>
LegacySceneSelectionAdapterSet::createSelectionSynchronizer() {
    return std::unique_ptr<SceneSelectionSynchronizer>(
        new LegacySceneSelectionSynchronizer(
            *state->selections, *state->controlMirror));
}

std::unique_ptr<LegacySceneSelectionAdapterSet>
createLegacySceneSelectionAdapters(
    EffectControl& flame, EffectControl& generalFlame, EffectControl& wave,
    EffectControl& waveScale, EffectControl& table, EffectControl& object,
    EffectControl& translation, EffectControl& palette, EffectControl& border,
    EffectControl& flashlight, EffectControl& images,
    std::unique_ptr<SceneVisualSelections> selections) {
    std::unique_ptr<LegacySceneControlMirror> controlMirror(
        new LegacySceneSelectionMirror(
        flame, generalFlame, wave, waveScale, table, object, translation,
        palette, border, flashlight, images, *selections));
    return std::unique_ptr<LegacySceneSelectionAdapterSet>(
        new LegacySceneSelectionAdapterSet(
            std::unique_ptr<LegacySceneSelectionAdapterState>(
                new LegacySceneSelectionAdapterState(
                    std::move(selections), std::move(controlMirror)))));
}
