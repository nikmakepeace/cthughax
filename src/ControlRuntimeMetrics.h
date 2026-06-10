/** @file
 * Read-only live metrics exposed to the generic control IPC layer.
 */

#ifndef CTHUGHA_CONTROL_RUNTIME_METRICS_H
#define CTHUGHA_CONTROL_RUNTIME_METRICS_H

class AcousticContext;

struct ControlRuntimeMetricsSnapshot {
    int fire;
    int cumulativeFireLevel;
    int fireSensitivity;

    ControlRuntimeMetricsSnapshot();
    ControlRuntimeMetricsSnapshot(int fire_, int cumulativeFireLevel_,
        int fireSensitivity_);
};

class ControlRuntimeMetrics {
public:
    virtual ~ControlRuntimeMetrics() { }

    virtual ControlRuntimeMetricsSnapshot snapshot() const = 0;
};

class AcousticControlRuntimeMetrics : public ControlRuntimeMetrics {
    const AcousticContext& acousticContext;

public:
    explicit AcousticControlRuntimeMetrics(
        const AcousticContext& acousticContext_);

    virtual ControlRuntimeMetricsSnapshot snapshot() const;
};

#endif
