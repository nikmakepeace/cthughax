/** @file
 * Optional local control service owned by Application.
 */

#ifndef CTHUGHA_CONTROL_SERVICE_H
#define CTHUGHA_CONTROL_SERVICE_H

#include "ControlPanelLauncher.h"
#include "ControlProtocol.h"
#include "RuntimeStateObserver.h"

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class ControlListener;
class ControlDisplayCatalogs;
class ControlRuntimeMetrics;
class ControlStream;
class LogSink;
class RuntimeCommandSink;
class RuntimeConfigRegistry;
class SceneVisualSelections;

class ControlPanelProcessLauncher {
public:
    virtual ~ControlPanelProcessLauncher() { }

    virtual bool launchPanel(
        const std::string& endpoint, std::string* error) = 0;
    virtual void terminatePanel() { }
};

class SystemControlPanelProcessLauncher : public ControlPanelProcessLauncher {
    void* panelProcessHandle;
    long panelProcessId;

public:
    SystemControlPanelProcessLauncher();
    virtual ~SystemControlPanelProcessLauncher();

    virtual bool launchPanel(
        const std::string& endpoint, std::string* error);
    virtual void terminatePanel();
};

class ControlService : public ControlPanelLauncher,
                       public RuntimeStateObserver {
    enum InboundType {
        InboundConnected,
        InboundDisconnected,
        InboundMessage
    };

    struct InboundItem {
        InboundType type;
        ControlJsonValue message;

        explicit InboundItem(InboundType type_);
        explicit InboundItem(const ControlJsonValue& message_);
    };

    RuntimeCommandSink& runtimeCommands;
    RuntimeConfigRegistry& runtimeConfigRegistry;
    SceneVisualSelections& sceneVisualSelections;
    ControlDisplayCatalogs& displayCatalogs;
    ControlRuntimeMetrics& runtimeMetrics;
    LogSink& log;
    std::unique_ptr<ControlPanelProcessLauncher> ownedProcessLauncher;
    ControlPanelProcessLauncher& processLauncher;

    std::unique_ptr<ControlListener> listenerValue;
    std::thread workerThread;
    mutable std::mutex mutex;
    std::deque<InboundItem> inbound;
    std::deque<ControlJsonValue> outbound;
    int stopRequested;
    int clientConnectedValue;
    int launchPending;
    int launchedPanelValue;
    int dirtyValue;
    int revisionValue;
    int metricsSnapshotKnown;
    int lastCumulativeFireLevel;
    int lastFireSensitivity;
    long long lastMetricsPublishMs;
    int metricsPublishIntervalMs;
    size_t maxOutboundMessages;

    void workerMain();
    void workerSetClientConnected(int connected);
    int workerStopRequested() const;
    void enqueueInbound(const InboundItem& item);
    void enqueueOutbound(const ControlJsonValue& message);
    void publishCatalogs();
    void publishState();
    int liveMetricsShouldPublish(long long nowMs);
    void processMessage(const ControlJsonValue& message);

public:
    ControlService(RuntimeCommandSink& runtimeCommands_,
        RuntimeConfigRegistry& runtimeConfigRegistry_,
        SceneVisualSelections& sceneVisualSelections_,
        ControlDisplayCatalogs& displayCatalogs_,
        ControlRuntimeMetrics& runtimeMetrics_, LogSink& log_);
    ControlService(RuntimeCommandSink& runtimeCommands_,
        RuntimeConfigRegistry& runtimeConfigRegistry_,
        SceneVisualSelections& sceneVisualSelections_,
        ControlDisplayCatalogs& displayCatalogs_,
        ControlRuntimeMetrics& runtimeMetrics_, LogSink& log_,
        ControlPanelProcessLauncher& processLauncher_);
    ~ControlService();

    bool start(const std::string& instanceId, std::string* error);
    void stop();
    std::string endpoint() const;

    void serviceFrame(int maxCommands);
    bool clientConnected() const;

    virtual void launchControlPanel();
    virtual void runtimeStateChanged();
};

#endif
