/** @file
 * Read-only live metrics exposed to the generic control IPC layer.
 */

#include "ControlRuntimeMetrics.h"

#include "AudioAnalyzer.h"

ControlRuntimeMetricsSnapshot::ControlRuntimeMetricsSnapshot()
    : fire(0)
    , cumulativeFireLevel(0)
    , fireSensitivity(100) { }

ControlRuntimeMetricsSnapshot::ControlRuntimeMetricsSnapshot(int fire_,
    int cumulativeFireLevel_, int fireSensitivity_)
    : fire(fire_)
    , cumulativeFireLevel(cumulativeFireLevel_)
    , fireSensitivity(fireSensitivity_) { }

AcousticControlRuntimeMetrics::AcousticControlRuntimeMetrics(
    const AcousticContext& acousticContext_)
    : acousticContext(acousticContext_) { }

ControlRuntimeMetricsSnapshot AcousticControlRuntimeMetrics::snapshot() const {
    return ControlRuntimeMetricsSnapshot(acousticContext.fire(),
        acousticContext.cumulativeFireLevel(),
        acousticContext.fireSensitivity());
}
