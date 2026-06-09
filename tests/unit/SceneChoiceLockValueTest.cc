#include "SceneChoiceSelection.h"

#include <cassert>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }

static void testLockOwnsNativeState() {
    SceneChoiceLockValue lock;

    assert(lock.enabled() == 0);
    lock.change("on");
    assert(lock.enabled() == 1);
    lock.change("off");
    assert(lock.enabled() == 0);
}

static void testLockAcceptsLegacyYesNoText() {
    SceneChoiceLockValue lock;

    lock.change("yes");
    assert(lock.enabled() == 1);
    lock.change("0");
    assert(lock.enabled() == 0);
    lock.change("1");
    assert(lock.enabled() == 1);
    lock.change("no");
    assert(lock.enabled() == 0);
}

int main() {
    testLockOwnsNativeState();
    testLockAcceptsLegacyYesNoText();
    return 0;
}
