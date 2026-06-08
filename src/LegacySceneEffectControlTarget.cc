// Legacy EffectControl command target facade for SceneCommands.

#include "LegacySceneEffectControlTarget.h"

#include "LegacySceneEffectControlCatalog.h"
#include "Scene.h"
#include "SceneDependencies.h"

RuntimeEffectControlOwner* SceneEffectControlCatalog::createEffectControlOwner(
    SceneCommands& sceneCommands, SceneEffectRegistry& effectRegistry,
    RandomSource& randomSource) {
    return new SceneCommandsEffectControlOwner(
        sceneCommands, *this, effectRegistry, randomSource);
}

SceneCommandsEffectControlOwner::SceneCommandsEffectControlOwner(
    SceneCommands& sceneCommands_, SceneEffectControlCatalog& effectControls_,
    SceneEffectRegistry& effectRegistry_, RandomSource& randomSource_)
    : sceneCommands(sceneCommands_)
    , effectControls(effectControls_)
    , effectRegistry(effectRegistry_)
    , randomSource(randomSource_) { }

int SceneCommandsEffectControlOwner::ownsEffectControl(
    const EffectControl& option) const {
    return effectControls.isSceneOption(option);
}

void SceneCommandsEffectControlOwner::changeEffectControlBy(
    EffectControl& option, int by, int doSave) {
    if (doSave && effectControls.isSceneOption(option))
        effectRegistry.saveAll();
    sceneCommands.refreshFromOptionsAndMaybeCueImage(
        effectControls.change(option, by, randomSource));
}

void SceneCommandsEffectControlOwner::changeEffectControlTo(
    EffectControl& option, const char* to, int doSave) {
    if (doSave && effectControls.isSceneOption(option))
        effectRegistry.saveAll();
    sceneCommands.refreshFromOptionsAndMaybeCueImage(
        effectControls.change(option, to, randomSource));
}

void SceneCommandsEffectControlOwner::activateEffectControl(
    EffectControl& option, int index) {
    sceneCommands.refreshFromOptionsAndMaybeCueImage(
        effectControls.activate(option, index));
}

void SceneCommandsEffectControlOwner::toggleEffectControlLock(
    EffectControl& option) {
    effectControls.toggleLock(option);
}

void SceneCommandsEffectControlOwner::toggleEffectChoiceUse(
    EffectControl& option, int index) {
    effectControls.toggleChoiceUse(option, index);
}
