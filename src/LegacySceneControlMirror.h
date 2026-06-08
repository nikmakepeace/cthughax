// Compatibility mirror from Scene selections to legacy EffectControls.

#ifndef CTHUGHA_LEGACY_SCENE_CONTROL_MIRROR_H
#define CTHUGHA_LEGACY_SCENE_CONTROL_MIRROR_H

class SceneVisualSelections;

/**
 * Adapter-level mirror for legacy visual EffectControls.
 *
 * Native Scene selections should not implement legacy EffectControl identity
 * APIs. This mirror is the temporary compatibility surface that copies the
 * Scene-owned selection values into legacy controls still read by older
 * catalog/display code.
 */
class LegacySceneControlMirror {
public:
    /** Destroys the compatibility mirror. */
    virtual ~LegacySceneControlMirror() { }

    /** Synchronizes all legacy control values from their bound selections. */
    virtual void syncControlsFromSelections() = 0;
};

LegacySceneControlMirror* legacySceneControlMirror(
    SceneVisualSelections& selections);
const LegacySceneControlMirror* legacySceneControlMirror(
    const SceneVisualSelections& selections);

#endif
