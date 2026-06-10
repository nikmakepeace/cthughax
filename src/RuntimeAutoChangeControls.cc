/** @file
 * Runtime automatic scene-change control adapter.
 */

#include "RuntimeAutoChangeControls.h"

#include "AutoChangeControls.h"

DefaultRuntimeAutoChangeControls::DefaultRuntimeAutoChangeControls(
    AutoChangeControls& autoChangeControls_, Option& quietMessageOption_)
    : autoChangeControls(autoChangeControls_)
    , quietMessageOption(quietMessageOption_) { }

static int isSameOption(Option& lhs, Option& rhs) {
    return &lhs == &rhs;
}

static void markChanged(RuntimeChangeSet& changes) {
    changes.autoChangeChanged = 1;
}

void DefaultRuntimeAutoChangeControls::toggleLock() {
    autoChangeControls.toggleLock();
}

void DefaultRuntimeAutoChangeControls::changeLockTo(int locked) {
    autoChangeControls.lockedOption().setValue(locked ? 1 : 0);
}

void DefaultRuntimeAutoChangeControls::changeCumulativeFireLevelTo(
    int threshold) {
    autoChangeControls.cumulativeFireLevelOption().setValue(threshold);
}

int DefaultRuntimeAutoChangeControls::changeAutoChangeOptionBy(
    Option& option, int by, RuntimeChangeSet& changes) {
    if (autoChangeControls.changeOptionBy(option, by)) {
        markChanged(changes);
        return 1;
    }

    if (!isSameOption(option, quietMessageOption))
        return 0;

    option.change(by);
    markChanged(changes);
    return 1;
}

int DefaultRuntimeAutoChangeControls::changeAutoChangeOptionTo(
    Option& option, const char* to, RuntimeChangeSet& changes) {
    if (autoChangeControls.changeOptionTo(option, to)) {
        markChanged(changes);
        return 1;
    }

    if (!isSameOption(option, quietMessageOption))
        return 0;

    option.change(to);
    markChanged(changes);
    return 1;
}
