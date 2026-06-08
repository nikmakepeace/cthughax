// Compatibility bindings from legacy EffectControls to Scene selections.

#ifndef CTHUGHA_LEGACY_SCENE_EFFECT_CONTROL_BINDINGS_H
#define CTHUGHA_LEGACY_SCENE_EFFECT_CONTROL_BINDINGS_H

class SceneOptionSelection;
class SceneVisualSelections;
class EffectControl;

/**
 * Adapter-level lookup table for legacy EffectControl command routing.
 *
 * Native Scene selections should not implement legacy EffectControl identity
 * APIs. This table is the temporary compatibility surface that maps older UI
 * and keymap commands to the Scene-owned selection that now holds visual state.
 */
class LegacySceneEffectControlBindings {
public:
    /** Destroys the compatibility binding table. */
    virtual ~LegacySceneEffectControlBindings() { }

    /**
     * Finds the Scene selection controlled by a legacy EffectControl.
     *
     * @param option Legacy control identity from UI/keymap routing.
     * @return Matching Scene selection, or NULL when the control is not visual.
     */
    virtual SceneOptionSelection* selectionFor(EffectControl& option) = 0;

    /**
     * Finds the Scene selection controlled by a legacy EffectControl.
     *
     * @param option Legacy control identity from UI/keymap routing.
     * @return Matching Scene selection, or NULL when the control is not visual.
     */
    virtual const SceneOptionSelection* selectionFor(
        const EffectControl& option) const = 0;

    /** Synchronizes all bound selections from their legacy control values. */
    virtual void syncFromControls() = 0;
};

LegacySceneEffectControlBindings* legacySceneEffectControlBindings(
    SceneVisualSelections& selections);
const LegacySceneEffectControlBindings* legacySceneEffectControlBindings(
    const SceneVisualSelections& selections);

#endif
