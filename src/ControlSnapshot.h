/** @file
 * Builds JSON control state and catalog snapshots from app-owned runtime data.
 */

#ifndef CTHUGHA_CONTROL_SNAPSHOT_H
#define CTHUGHA_CONTROL_SNAPSHOT_H

#include "ControlProtocol.h"
#include "ControlRuntimeMetrics.h"

class ControlDisplayCatalogs;
class RuntimeConfigRegistry;
class SceneVisualSelections;

class ControlExtraLockState {
public:
    virtual ~ControlExtraLockState() { }

    /** @return Nonzero when the supplied non-scene target is locked. */
    virtual int targetLocked(const char* target) const = 0;
};

ControlJsonValue buildControlStateSnapshot(
    const RuntimeConfigRegistry& registry,
    SceneVisualSelections& selections,
    const ControlExtraLockState& extraLocks,
    const ControlRuntimeMetricsSnapshot& metrics, int revision);

ControlJsonValue buildControlCatalogSnapshot(
    SceneVisualSelections& selections,
    const ControlDisplayCatalogs& displayCatalogs);

#endif
