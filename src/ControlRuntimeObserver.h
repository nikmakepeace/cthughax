/** @file
 * Runtime-state observer adapters used by the control service.
 */

#ifndef CTHUGHA_CONTROL_RUNTIME_OBSERVER_H
#define CTHUGHA_CONTROL_RUNTIME_OBSERVER_H

#include "RuntimeCommandSink.h"
#include "Scene.h"

class RuntimeStateObserver;

class ControlObservedRuntimeCommandSink : public RuntimeCommandSink {
    RuntimeCommandSink& inner;
    RuntimeStateObserver& observer;

public:
    ControlObservedRuntimeCommandSink(
        RuntimeCommandSink& inner_, RuntimeStateObserver& observer_);

    virtual RuntimeChangeSet apply(const RuntimeCommand& command);
};

class ControlObservedSceneCommandTarget : public SceneCommandTarget {
    SceneCommandTarget& inner;
    RuntimeStateObserver& observer;

    void changed();

public:
    ControlObservedSceneCommandTarget(
        SceneCommandTarget& inner_, RuntimeStateObserver& observer_);

    virtual void restore();
    virtual void restorePreset(int slot);
    virtual void savePreset(int slot);
    virtual void randomPalette();
    virtual void addRandomPalette();
    virtual void changeAll();
    virtual void changeOne();
    virtual void change(SceneSelectionTarget target, int by);
    virtual void change(SceneSelectionTarget target, const char* to);
    virtual void activate(SceneSelectionTarget target, int index);
    virtual void toggleLock(SceneSelectionTarget target);
    virtual void toggleChoiceUse(SceneSelectionTarget target, int index);
};

#endif
