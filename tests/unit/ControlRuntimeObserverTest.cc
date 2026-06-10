/** @file
 * Unit coverage for control dirty-state observer adapters.
 */

#include "ControlRuntimeObserver.h"
#include "RuntimeStateObserver.h"

#include <assert.h>

class CountingObserver : public RuntimeStateObserver {
public:
    int changes;

    CountingObserver()
        : changes(0) { }

    virtual void runtimeStateChanged() {
        changes++;
    }
};

class FakeRuntimeCommandSink : public RuntimeCommandSink {
public:
    RuntimeChangeSet response;
    int calls;

    FakeRuntimeCommandSink()
        : response()
        , calls(0) { }

    virtual RuntimeChangeSet apply(const RuntimeCommand&) {
        calls++;
        return response;
    }
};

class FakeSceneCommandTarget : public SceneCommandTarget {
public:
    int changes;
    int saves;

    FakeSceneCommandTarget()
        : changes(0)
        , saves(0) { }

    virtual void restore() { changes++; }
    virtual void restorePreset(int) { changes++; }
    virtual void savePreset(int) { saves++; }
    virtual void randomPalette() { changes++; }
    virtual void addRandomPalette() { changes++; }
    virtual void changeAll() { changes++; }
    virtual void changeOne() { changes++; }
    virtual void change(SceneSelectionTarget, int) { changes++; }
    virtual void change(SceneSelectionTarget, const char*) { changes++; }
    virtual void activate(SceneSelectionTarget, int) { changes++; }
    virtual void toggleLock(SceneSelectionTarget) { changes++; }
    virtual void toggleChoiceUse(SceneSelectionTarget, int) { changes++; }
};

static void testRuntimeSinkObserverMarksOnlyRealChanges() {
    FakeRuntimeCommandSink inner;
    CountingObserver observer;
    ControlObservedRuntimeCommandSink observed(inner, observer);

    observed.apply(RuntimeCommand::writeIni());
    assert(inner.calls == 1);
    assert(observer.changes == 0);

    inner.response.sceneChanges = 1;
    observed.apply(RuntimeCommand::changeAll());
    assert(inner.calls == 2);
    assert(observer.changes == 1);
}

static void testSceneCommandObserverMarksSceneChanges() {
    FakeSceneCommandTarget inner;
    CountingObserver observer;
    ControlObservedSceneCommandTarget observed(inner, observer);

    observed.change(SceneSelectionFlame, 1);
    observed.activate(SceneSelectionPalette, 2);
    observed.toggleLock(SceneSelectionImage);

    assert(inner.changes == 3);
    assert(observer.changes == 3);
}

static void testSceneCommandObserverDoesNotMarkPresetSaves() {
    FakeSceneCommandTarget inner;
    CountingObserver observer;
    ControlObservedSceneCommandTarget observed(inner, observer);

    observed.savePreset(1);

    assert(inner.saves == 1);
    assert(observer.changes == 0);
}

int main() {
    testRuntimeSinkObserverMarksOnlyRealChanges();
    testSceneCommandObserverMarksSceneChanges();
    testSceneCommandObserverDoesNotMarkPresetSaves();
    return 0;
}
