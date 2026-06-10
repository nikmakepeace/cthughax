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

ControlJsonValue buildControlStateSnapshot(
    const RuntimeConfigRegistry& registry,
    const ControlRuntimeMetricsSnapshot& metrics, int revision);

ControlJsonValue buildControlCatalogSnapshot(
    SceneVisualSelections& selections,
    const ControlDisplayCatalogs& displayCatalogs);

#endif
