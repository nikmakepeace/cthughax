#include "RuntimeShutdown.h"

#include <assert.h>

static void testRuntimeCloseStateRecordsCloseRequests() {
    RuntimeCloseState closeState;

    assert(!closeState.closeRequested());
    closeState.requestClose();
    assert(closeState.closeRequested());
    closeState.requestClose();
    assert(closeState.closeRequested());
}

int main() {
    testRuntimeCloseStateRecordsCloseRequests();
    return 0;
}
