/** @file
 * Optional local control service owned by Application.
 */

#include "ControlService.h"

#include "ControlCommandMapper.h"
#include "ControlRuntimeMetrics.h"
#include "ControlSnapshot.h"
#include "ControlTransport.h"
#include "ProcessServices.h"
#include "RuntimeCommandSink.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <chrono>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

namespace {

static int controlMessageId(const ControlJsonValue& message) {
    const ControlJsonValue* id = message.member("id");
    return id != 0 ? int(id->asNumber(0)) : 0;
}

static long long steadyMilliseconds() {
    std::chrono::steady_clock::duration elapsed
        = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        elapsed).count();
}

static std::string processInstanceId() {
    char text[64];
#ifdef _WIN32
    snprintf(text, sizeof(text), "%lu", unsigned long(GetCurrentProcessId()));
#else
    snprintf(text, sizeof(text), "%ld", long(getpid()));
#endif
    return text;
}

static bool writeMessage(ControlStream& stream, const ControlJsonValue& message,
    std::string* error) {
    std::string line;
    if (!serializeControlJsonLine(message, &line, error))
        return false;
    return stream.writeAll(line.data(), line.size(), 25, error)
        == ControlIoReady;
}

static std::string directoryOf(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos)
        return "";
    if (slash == 0)
        return path.substr(0, 1);
    return path.substr(0, slash);
}

static std::string parentDirectoryOf(const std::string& path) {
    return directoryOf(directoryOf(path));
}

static void appendUnique(
    std::vector<std::string>& values, const std::string& value) {
    if (value.empty())
        return;
    for (std::vector<std::string>::const_iterator it = values.begin();
         it != values.end(); ++it) {
        if (*it == value)
            return;
    }
    values.push_back(value);
}

#ifdef _WIN32

static std::string quotedWindowsArg(const std::string& value) {
    std::string quoted = "\"";
    for (std::string::const_iterator it = value.begin(); it != value.end();
         ++it) {
        if (*it == '"')
            quoted += "\\\"";
        else
            quoted.push_back(*it);
    }
    quoted += "\"";
    return quoted;
}

#else

static int pathUsesDirectory(const std::string& path) {
    return path.find('/') != std::string::npos;
}

static std::string currentExecutablePath() {
#ifdef __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(0, &size);
    if (size == 0)
        return "";
    std::vector<char> buffer(size + 1, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0)
        return "";
    return buffer.data();
#else
    char path[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (length <= 0)
        return "";
    path[length] = '\0';
    return path;
#endif
}

static bool launchUnixPanelCandidate(const std::string& program,
    const std::string& endpoint, std::string* error, long* pidOut) {
    int execStatusPipe[2];
    if (pipe(execStatusPipe) != 0) {
        if (error != 0)
            *error = std::string("pipe failed for cthugha-panel launch: ")
                + strerror(errno);
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        int err = errno;
        close(execStatusPipe[0]);
        close(execStatusPipe[1]);
        if (error != 0)
            *error = std::string("fork failed for cthugha-panel: ")
                + strerror(err);
        return false;
    }

    if (pid == 0) {
        close(execStatusPipe[0]);
        fcntl(execStatusPipe[1], F_SETFD, FD_CLOEXEC);
        if (pathUsesDirectory(program)) {
            execl(program.c_str(), "cthugha-panel", "--control-endpoint",
                endpoint.c_str(), static_cast<char*>(0));
        } else {
            execlp(program.c_str(), "cthugha-panel", "--control-endpoint",
                endpoint.c_str(), static_cast<char*>(0));
        }
        int err = errno;
        ssize_t ignored = write(execStatusPipe[1], &err, sizeof(err));
        (void)ignored;
        _exit(127);
    }

    close(execStatusPipe[1]);
    int childErrno = 0;
    ssize_t bytes = read(execStatusPipe[0], &childErrno, sizeof(childErrno));
    close(execStatusPipe[0]);
    if (bytes == 0) {
        if (pidOut != 0)
            *pidOut = long(pid);
        return true;
    }

    int status = 0;
    waitpid(pid, &status, 0);
    if (error != 0)
        *error = std::string("exec failed for ") + program + ": "
            + strerror(childErrno);
    return false;
}

#endif

#ifndef _WIN32

static int reapUnixChild(pid_t pid) {
    int status = 0;
    pid_t waited = waitpid(pid, &status, WNOHANG);
    return waited == pid || (waited < 0 && errno == ECHILD);
}

#endif

static std::vector<std::string> controlPanelExecutableCandidates() {
    std::vector<std::string> candidates;
#ifdef _WIN32
    char modulePath[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, modulePath, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        std::string executable = modulePath;
        std::string directory = directoryOf(executable);
        appendUnique(candidates, directory + "\\cthugha-panel.exe");
        appendUnique(candidates,
            parentDirectoryOf(executable) + "\\cthugha-panel.exe");
    }
    appendUnique(candidates, "cthugha-panel.exe");
#else
    std::string executable = currentExecutablePath();
    if (!executable.empty()) {
        std::string directory = directoryOf(executable);
        appendUnique(candidates, directory + "/cthugha-panel");
        appendUnique(candidates,
            parentDirectoryOf(executable) + "/cthugha-panel");
    }
    appendUnique(candidates, "cthugha-panel");
#endif
    return candidates;
}

}

SystemControlPanelProcessLauncher::SystemControlPanelProcessLauncher()
    : panelProcessHandle(0)
    , panelProcessId(0) {
}

SystemControlPanelProcessLauncher::~SystemControlPanelProcessLauncher() {
    terminatePanel();
}

bool SystemControlPanelProcessLauncher::launchPanel(
    const std::string& endpoint, std::string* error) {
    terminatePanel();

#ifdef _WIN32
    std::vector<std::string> candidates = controlPanelExecutableCandidates();
    for (std::vector<std::string>::const_iterator it = candidates.begin();
         it != candidates.end(); ++it) {
        std::string command = quotedWindowsArg(*it)
            + " --control-endpoint " + quotedWindowsArg(endpoint);
        STARTUPINFOA startupInfo;
        PROCESS_INFORMATION processInfo;
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        ZeroMemory(&processInfo, sizeof(processInfo));
        startupInfo.cb = sizeof(startupInfo);
        std::vector<char> mutableCommand(command.begin(), command.end());
        mutableCommand.push_back('\0');
        if (!CreateProcessA(NULL, mutableCommand.data(), NULL, NULL, FALSE, 0,
                NULL, NULL, &startupInfo, &processInfo)) {
            DWORD errorCode = GetLastError();
            if (error != 0)
                *error = "CreateProcess failed for " + *it
                    + " error=" + std::to_string(unsigned long(errorCode));
            continue;
        }
        CloseHandle(processInfo.hThread);
        panelProcessHandle = processInfo.hProcess;
        panelProcessId = long(processInfo.dwProcessId);
        return true;
    }
    return false;
#else
    std::vector<std::string> candidates = controlPanelExecutableCandidates();
    std::string lastError;
    for (std::vector<std::string>::const_iterator it = candidates.begin();
         it != candidates.end(); ++it) {
        std::string candidateError;
        long pid = 0;
        if (launchUnixPanelCandidate(*it, endpoint, &candidateError, &pid)) {
            panelProcessId = pid;
            return true;
        }
        lastError = candidateError;
    }
    if (error != 0)
        *error = lastError.empty() ? "could not launch cthugha-panel"
                                   : lastError;
    return false;
#endif
}

void SystemControlPanelProcessLauncher::terminatePanel() {
#ifdef _WIN32
    if (panelProcessHandle != 0) {
        HANDLE handle = (HANDLE)panelProcessHandle;
        DWORD exitCode = 0;
        if (GetExitCodeProcess(handle, &exitCode)
            && exitCode == STILL_ACTIVE)
            TerminateProcess(handle, 0);
        CloseHandle(handle);
    }
    panelProcessHandle = 0;
    panelProcessId = 0;
#else
    if (panelProcessId > 0) {
        pid_t pid = pid_t(panelProcessId);
        if (!reapUnixChild(pid)) {
            if (kill(pid, SIGTERM) == 0 || errno != ESRCH) {
                for (int i = 0; i < 20 && !reapUnixChild(pid); i++)
                    usleep(10000);
                if (!reapUnixChild(pid)) {
                    kill(pid, SIGKILL);
                    waitpid(pid, 0, 0);
                }
            }
        }
    }
    panelProcessHandle = 0;
    panelProcessId = 0;
#endif
}

ControlService::InboundItem::InboundItem(InboundType type_)
    : type(type_)
    , message() { }

ControlService::InboundItem::InboundItem(const ControlJsonValue& message_)
    : type(InboundMessage)
    , message(message_) { }

ControlService::ControlService(RuntimeCommandSink& runtimeCommands_,
    RuntimeConfigRegistry& runtimeConfigRegistry_,
    SceneVisualSelections& sceneVisualSelections_,
    ControlDisplayCatalogs& displayCatalogs_,
    ControlRuntimeMetrics& runtimeMetrics_, LogSink& log_)
    : runtimeCommands(runtimeCommands_)
    , runtimeConfigRegistry(runtimeConfigRegistry_)
    , sceneVisualSelections(sceneVisualSelections_)
    , displayCatalogs(displayCatalogs_)
    , runtimeMetrics(runtimeMetrics_)
    , log(log_)
    , ownedProcessLauncher(new SystemControlPanelProcessLauncher())
    , processLauncher(*ownedProcessLauncher)
    , listenerValue()
    , workerThread()
    , mutex()
    , inbound()
    , outbound()
    , stopRequested(0)
    , clientConnectedValue(0)
    , launchPending(0)
    , launchedPanelValue(0)
    , dirtyValue(0)
    , revisionValue(0)
    , metricsSnapshotKnown(0)
    , lastCumulativeFireLevel(0)
    , lastFireSensitivity(0)
    , lastMetricsPublishMs(0)
    , metricsPublishIntervalMs(100)
    , maxOutboundMessages(32) { }

ControlService::ControlService(RuntimeCommandSink& runtimeCommands_,
    RuntimeConfigRegistry& runtimeConfigRegistry_,
    SceneVisualSelections& sceneVisualSelections_,
    ControlDisplayCatalogs& displayCatalogs_,
    ControlRuntimeMetrics& runtimeMetrics_, LogSink& log_,
    ControlPanelProcessLauncher& processLauncher_)
    : runtimeCommands(runtimeCommands_)
    , runtimeConfigRegistry(runtimeConfigRegistry_)
    , sceneVisualSelections(sceneVisualSelections_)
    , displayCatalogs(displayCatalogs_)
    , runtimeMetrics(runtimeMetrics_)
    , log(log_)
    , ownedProcessLauncher()
    , processLauncher(processLauncher_)
    , listenerValue()
    , workerThread()
    , mutex()
    , inbound()
    , outbound()
    , stopRequested(0)
    , clientConnectedValue(0)
    , launchPending(0)
    , launchedPanelValue(0)
    , dirtyValue(0)
    , revisionValue(0)
    , metricsSnapshotKnown(0)
    , lastCumulativeFireLevel(0)
    , lastFireSensitivity(0)
    , lastMetricsPublishMs(0)
    , metricsPublishIntervalMs(100)
    , maxOutboundMessages(32) { }

ControlService::~ControlService() {
    stop();
}

bool ControlService::start(const std::string& instanceId, std::string* error) {
    if (listenerValue.get() != 0)
        return true;

    listenerValue = createControlListener(
        instanceId.empty() ? processInstanceId() : instanceId, error);
    if (listenerValue.get() == 0)
        return false;

    stopRequested = 0;
    workerThread = std::thread(&ControlService::workerMain, this);
    log.info("Control endpoint: %s\n", listenerValue->endpoint().text.c_str());
    return true;
}

void ControlService::stop() {
    int shouldTerminatePanel = 0;
    {
        std::lock_guard<std::mutex> lock(mutex);
        stopRequested = 1;
        shouldTerminatePanel = launchedPanelValue;
        launchedPanelValue = 0;
        launchPending = 0;
    }
    if (listenerValue.get() != 0)
        listenerValue->close();
    if (workerThread.joinable())
        workerThread.join();
    listenerValue.reset();
    if (shouldTerminatePanel)
        processLauncher.terminatePanel();
}

std::string ControlService::endpoint() const {
    if (listenerValue.get() == 0)
        return "";
    return listenerValue->endpoint().text;
}

bool ControlService::clientConnected() const {
    std::lock_guard<std::mutex> lock(mutex);
    return clientConnectedValue != 0;
}

void ControlService::runtimeStateChanged() {
    std::lock_guard<std::mutex> lock(mutex);
    dirtyValue = 1;
}

void ControlService::launchControlPanel() {
    std::string endpointText = endpoint();
    if (endpointText.empty())
        return;

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (clientConnectedValue || launchPending)
            return;
        launchPending = 1;
    }

    std::string error;
    if (!processLauncher.launchPanel(endpointText, &error)) {
        log.warn("Could not launch control panel: %s\n", error.c_str());
        std::lock_guard<std::mutex> lock(mutex);
        launchPending = 0;
    } else {
        std::lock_guard<std::mutex> lock(mutex);
        launchedPanelValue = 1;
    }
}

int ControlService::workerStopRequested() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stopRequested;
}

void ControlService::workerSetClientConnected(int connected) {
    std::lock_guard<std::mutex> lock(mutex);
    clientConnectedValue = connected;
    if (connected)
        launchPending = 0;
}

void ControlService::enqueueInbound(const InboundItem& item) {
    std::lock_guard<std::mutex> lock(mutex);
    inbound.push_back(item);
}

void ControlService::enqueueOutbound(const ControlJsonValue& message) {
    std::lock_guard<std::mutex> lock(mutex);
    while (outbound.size() >= maxOutboundMessages)
        outbound.pop_front();
    outbound.push_back(message);
}

void ControlService::publishCatalogs() {
    enqueueOutbound(buildControlCatalogSnapshot(
        sceneVisualSelections, displayCatalogs));
}

void ControlService::publishState() {
    ControlRuntimeMetricsSnapshot metrics = runtimeMetrics.snapshot();
    revisionValue++;
    enqueueOutbound(buildControlStateSnapshot(
        runtimeConfigRegistry, metrics, revisionValue));
    metricsSnapshotKnown = 1;
    lastCumulativeFireLevel = metrics.cumulativeFireLevel;
    lastFireSensitivity = metrics.fireSensitivity;
    lastMetricsPublishMs = steadyMilliseconds();
}

int ControlService::liveMetricsShouldPublish(long long nowMs) {
    if (!clientConnected())
        return 0;
    if (metricsPublishIntervalMs <= 0)
        return 0;
    if (lastMetricsPublishMs != 0
        && (nowMs - lastMetricsPublishMs) < metricsPublishIntervalMs)
        return 0;

    ControlRuntimeMetricsSnapshot metrics = runtimeMetrics.snapshot();
    if (!metricsSnapshotKnown
        || metrics.cumulativeFireLevel != lastCumulativeFireLevel
        || metrics.fireSensitivity != lastFireSensitivity)
        return 1;

    lastMetricsPublishMs = nowMs;
    return 0;
}

void ControlService::processMessage(const ControlJsonValue& message) {
    int id = controlMessageId(message);
    ControlMappedCommand mapped;
    std::string errorCode;
    std::string errorMessage;
    if (!controlCommandFromJson(message, &mapped, &errorCode, &errorMessage)) {
        enqueueOutbound(controlErrorMessage(id, errorCode, errorMessage));
        return;
    }

    RuntimeChangeSet changes = runtimeCommands.apply(mapped.command);
    if (changes.any())
        runtimeStateChanged();
    enqueueOutbound(controlAckMessage(id));
}

void ControlService::serviceFrame(int maxCommands) {
    std::deque<InboundItem> localInbound;
    int publishDirty = 0;
    {
        std::lock_guard<std::mutex> lock(mutex);
        localInbound.swap(inbound);
        publishDirty = dirtyValue;
        dirtyValue = 0;
    }

    int commandsApplied = 0;
    for (std::deque<InboundItem>::const_iterator it = localInbound.begin();
         it != localInbound.end(); ++it) {
        if (it->type == InboundConnected) {
            metricsSnapshotKnown = 0;
            publishCatalogs();
            publishState();
            continue;
        }
        if (it->type == InboundDisconnected) {
            std::lock_guard<std::mutex> lock(mutex);
            launchPending = 0;
            continue;
        }
        if (maxCommands >= 0 && commandsApplied >= maxCommands) {
            enqueueInbound(*it);
            continue;
        }
        processMessage(it->message);
        commandsApplied++;
        publishDirty = 1;
    }

    if (!publishDirty && liveMetricsShouldPublish(steadyMilliseconds()))
        publishDirty = 1;

    if (publishDirty && clientConnected())
        publishState();
}

void ControlService::workerMain() {
    std::unique_ptr<ControlStream> client;
    ControlJsonLineReader reader;
    while (!workerStopRequested()) {
        std::string error;
        if (client.get() == 0) {
            if (listenerValue.get() == 0)
                break;
            client = listenerValue->accept(50, &error);
            if (client.get() != 0) {
                reader.clear();
                workerSetClientConnected(1);
                enqueueInbound(InboundItem(InboundConnected));
            }
            continue;
        }

        char buffer[4096];
        ControlReadResult read = client->readSome(
            buffer, sizeof(buffer), 10, &error);
        if (read.status == ControlIoReady) {
            std::vector<ControlJsonValue> messages;
            if (!reader.feed(buffer, read.bytes, &messages, &error)) {
                client->close();
                client.reset();
                workerSetClientConnected(0);
                enqueueInbound(InboundItem(InboundDisconnected));
                continue;
            }
            for (std::vector<ControlJsonValue>::const_iterator it
                     = messages.begin();
                 it != messages.end(); ++it)
                enqueueInbound(InboundItem(*it));
        } else if (read.status == ControlIoClosed
            || read.status == ControlIoError) {
            client->close();
            client.reset();
            workerSetClientConnected(0);
            enqueueInbound(InboundItem(InboundDisconnected));
            continue;
        }

        std::deque<ControlJsonValue> localOutbound;
        {
            std::lock_guard<std::mutex> lock(mutex);
            localOutbound.swap(outbound);
        }
        for (std::deque<ControlJsonValue>::const_iterator it
                 = localOutbound.begin();
             it != localOutbound.end(); ++it) {
            if (!writeMessage(*client, *it, &error)) {
                client->close();
                client.reset();
                workerSetClientConnected(0);
                enqueueInbound(InboundItem(InboundDisconnected));
                break;
            }
        }
    }

    if (client.get() != 0)
        client->close();
    workerSetClientConnected(0);
}
