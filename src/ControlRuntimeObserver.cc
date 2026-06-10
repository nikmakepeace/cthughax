/** @file
 * Runtime-state observer adapters used by the control service.
 */

#include "ControlRuntimeObserver.h"

#include "RuntimeStateObserver.h"

ControlObservedRuntimeCommandSink::ControlObservedRuntimeCommandSink(
    RuntimeCommandSink& inner_, RuntimeStateObserver& observer_)
    : inner(inner_)
    , observer(observer_) { }

RuntimeChangeSet ControlObservedRuntimeCommandSink::apply(
    const RuntimeCommand& command) {
    RuntimeChangeSet changes = inner.apply(command);
    if (changes.any())
        observer.runtimeStateChanged();
    return changes;
}

ControlObservedSceneCommandTarget::ControlObservedSceneCommandTarget(
    SceneCommandTarget& inner_, RuntimeStateObserver& observer_)
    : inner(inner_)
    , observer(observer_) { }

void ControlObservedSceneCommandTarget::changed() {
    observer.runtimeStateChanged();
}

void ControlObservedSceneCommandTarget::restore() {
    inner.restore();
    changed();
}

void ControlObservedSceneCommandTarget::restorePreset(int slot) {
    inner.restorePreset(slot);
    changed();
}

void ControlObservedSceneCommandTarget::savePreset(int slot) {
    inner.savePreset(slot);
}

void ControlObservedSceneCommandTarget::randomPalette() {
    inner.randomPalette();
    changed();
}

void ControlObservedSceneCommandTarget::addRandomPalette() {
    inner.addRandomPalette();
    changed();
}

void ControlObservedSceneCommandTarget::changeAll() {
    inner.changeAll();
    changed();
}

void ControlObservedSceneCommandTarget::changeOne() {
    inner.changeOne();
    changed();
}

void ControlObservedSceneCommandTarget::change(
    SceneSelectionTarget target, int by) {
    inner.change(target, by);
    changed();
}

void ControlObservedSceneCommandTarget::change(
    SceneSelectionTarget target, const char* to) {
    inner.change(target, to);
    changed();
}

void ControlObservedSceneCommandTarget::activate(
    SceneSelectionTarget target, int index) {
    inner.activate(target, index);
    changed();
}

void ControlObservedSceneCommandTarget::toggleLock(SceneSelectionTarget target) {
    inner.toggleLock(target);
    changed();
}

void ControlObservedSceneCommandTarget::toggleChoiceUse(
    SceneSelectionTarget target, int index) {
    inner.toggleChoiceUse(target, index);
    changed();
}
