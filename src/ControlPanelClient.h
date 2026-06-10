/** @file
 * Client-side JSONL connection used by the wx control panel.
 */

#ifndef CTHUGHA_CONTROL_PANEL_CLIENT_H
#define CTHUGHA_CONTROL_PANEL_CLIENT_H

#include "ControlProtocol.h"

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class ControlStream;

struct ControlPanelClientEvent {
    enum Type {
        Connected,
        Disconnected,
        Message,
        Error
    };

    Type type;
    ControlJsonValue message;
    std::string text;

    explicit ControlPanelClientEvent(Type type_);
    ControlPanelClientEvent(Type type_, const std::string& text_);
    explicit ControlPanelClientEvent(const ControlJsonValue& message_);
};

class ControlPanelClient {
    std::string endpointValue;
    std::thread workerThread;
    mutable std::mutex mutex;
    std::deque<ControlJsonValue> outbound;
    std::deque<ControlPanelClientEvent> events;
    int stopRequested;
    int connectedValue;
    int nextId;

    void workerMain();
    int shouldStop() const;
    void setConnected(int connected);
    void pushEvent(const ControlPanelClientEvent& event);

public:
    explicit ControlPanelClient(const std::string& endpoint);
    ~ControlPanelClient();

    void start();
    void stop();
    bool connected() const;
    std::vector<ControlPanelClientEvent> pollEvents();

    int sendSet(const std::string& target, const std::string& value);
    int sendSetNumber(const std::string& target, int value);
    int sendSetBool(const std::string& target, bool value);
};

#endif
