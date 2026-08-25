/** @file
 * Read-only live metrics exposed to the generic control IPC layer.
 */

#include "ControlRuntimeMetrics.h"

#include "AudioAnalyzer.h"

ControlRuntimeMetricsSnapshot::ControlRuntimeMetricsSnapshot()
    : cumulativeFireLevel(0)
    , fireSensitivity(100) { }

ControlRuntimeMetricsSnapshot::ControlRuntimeMetricsSnapshot(
    int cumulativeFireLevel_, int fireSensitivity_)
    : cumulativeFireLevel(cumulativeFireLevel_)
    , fireSensitivity(fireSensitivity_) { }

AcousticControlRuntimeMetrics::AcousticControlRuntimeMetrics(
    const AcousticContext& acousticContext_)
    : acousticContext(acousticContext_) { }

ControlRuntimeMetricsSnapshot AcousticControlRuntimeMetrics::snapshot() const {
    return ControlRuntimeMetricsSnapshot(acousticContext.cumulativeFireLevel(),
        acousticContext.fireSensitivity());
}
