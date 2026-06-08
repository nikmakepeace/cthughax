// Legacy EffectControl command target facade for SceneCommands.

#ifndef CTHUGHA_LEGACY_SCENE_EFFECT_CONTROL_TARGET_H
#define CTHUGHA_LEGACY_SCENE_EFFECT_CONTROL_TARGET_H

#include "RuntimeCommandTargets.h"

class EffectControl;
class RandomSource;
class SceneCommands;
class SceneEffectControlCatalog;
class SceneEffectRegistry;

class SceneCommandsEffectControlOwner : public RuntimeEffectControlOwner {
    SceneCommands& sceneCommands;
    SceneEffectControlCatalog& effectControls;
    SceneEffectRegistry& effectRegistry;
    RandomSource& randomSource;

public:
    SceneCommandsEffectControlOwner(SceneCommands& sceneCommands_,
        SceneEffectControlCatalog& effectControls_,
        SceneEffectRegistry& effectRegistry_, RandomSource& randomSource_);

    virtual int ownsEffectControl(const EffectControl& option) const;
    virtual void changeEffectControlBy(
        EffectControl& option, int by, int doSave = 0);
    virtual void changeEffectControlTo(
        EffectControl& option, const char* to, int doSave = 0);
    virtual void activateEffectControl(EffectControl& option, int index);

    /** Toggles Scene-owned lock state for a migrated visual control. */
    virtual void toggleEffectControlLock(EffectControl& option);

    /** Toggles Scene-owned choice availability for a migrated visual control. */
    virtual void toggleEffectChoiceUse(EffectControl& option, int index);
};

#endif
