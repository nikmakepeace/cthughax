/** @file
 * Runtime shutdown request port and close-state implementation.
 */

#include "RuntimeShutdown.h"

RuntimeCloseState::RuntimeCloseState()
    : closeRequestedValue(false) { }

void RuntimeCloseState::requestClose() {
    closeRequestedValue = true;
}

bool RuntimeCloseState::closeRequested() const {
    return closeRequestedValue;
}
