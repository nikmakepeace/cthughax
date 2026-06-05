/** @file
 * Runtime shutdown request port and legacy close-flag implementation.
 */

#include "RuntimeShutdown.h"

#include "cthugha.h"

void CthughaRuntimeShutdown::requestClose() {
    cthugha_close++;
}
