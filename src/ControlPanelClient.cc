/** @file
 * Client-side JSONL connection used by the wx control panel.
 */

#include "ControlPanelClient.h"

#include "ControlTransport.h"

#include <chrono>
#include <thread>

namespace {

static const size_t maxPendingClientMessages = 32;
static const size_t maxPendingClientEvents = 128;

static ControlJsonValue setMessage(int id, const std::string& target,
    const ControlJsonValue& value) {
    ControlJsonValue message = ControlJsonValue::objectValueOf();
    message.set("v", ControlJsonValue::numberValueOf(1));
    message.set("id", ControlJsonValue::numberValueOf(id));
    message.set("op", ControlJsonValue::stringValueOf("set"));
    message.set("target", ControlJsonValue::stringValueOf(target));
    message.set("value", value);
    return message;
}

static bool writeMessage(ControlStream& stream, const ControlJsonValue& message,
    std::string* error) {
    std::string line;
    if (!serializeControlJsonLine(message, &line, error))
        return false;
    return stream.writeAll(line.data(), line.size(), 25, error)
        == ControlIoReady;
}

}

ControlPanelClientEvent::ControlPanelClientEvent(Type type_)
    : type(type_)
    , message()
    , text() { }

ControlPanelClientEvent::ControlPanelClientEvent(
    Type type_, const std::string& text_)
    : type(type_)
    , message()
    , text(text_) { }

ControlPanelClientEvent::ControlPanelClientEvent(
    const ControlJsonValue& message_)
    : type(Message)
    , message(message_)
    , text() { }

ControlPanelClient::ControlPanelClient(const std::string& endpoint)
    : endpointValue(endpoint)
    , workerThread()
    , mutex()
    , outbound()
    , events()
    , stopRequested(0)
    , connectedValue(0)
    , nextId(1) { }

ControlPanelClient::~ControlPanelClient() {
    stop();
}

void ControlPanelClient::start() {
    if (workerThread.joinable())
        return;

    {
        std::lock_guard<std::mutex> lock(mutex);
        stopRequested = 0;
        connectedValue = 0;
        outbound.clear();
        events.clear();
    }

    workerThread = std::thread(&ControlPanelClient::workerMain, this);
}

void ControlPanelClient::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        stopRequested = 1;
    }

    if (workerThread.joinable())
        workerThread.join();
}

bool ControlPanelClient::connected() const {
    std::lock_guard<std::mutex> lock(mutex);
    return connectedValue != 0;
}

std::vector<ControlPanelClientEvent> ControlPanelClient::pollEvents() {
    std::deque<ControlPanelClientEvent> local;
    {
        std::lock_guard<std::mutex> lock(mutex);
        local.swap(events);
    }
    return std::vector<ControlPanelClientEvent>(local.begin(), local.end());
}

int ControlPanelClient::sendSet(
    const std::string& target, const std::string& value) {
    ControlJsonValue json = ControlJsonValue::stringValueOf(value);
    std::lock_guard<std::mutex> lock(mutex);
    int id = nextId++;
    while (outbound.size() >= maxPendingClientMessages)
        outbound.pop_front();
    outbound.push_back(setMessage(id, target, json));
    return id;
}

int ControlPanelClient::sendSetNumber(const std::string& target, int value) {
    ControlJsonValue json = ControlJsonValue::numberValueOf(value);
    std::lock_guard<std::mutex> lock(mutex);
    int id = nextId++;
    while (outbound.size() >= maxPendingClientMessages)
        outbound.pop_front();
    outbound.push_back(setMessage(id, target, json));
    return id;
}

int ControlPanelClient::sendSetNumber(
    const std::string& target, double value) {
    ControlJsonValue json = ControlJsonValue::numberValueOf(value);
    std::lock_guard<std::mutex> lock(mutex);
    int id = nextId++;
    while (outbound.size() >= maxPendingClientMessages)
        outbound.pop_front();
    outbound.push_back(setMessage(id, target, json));
    return id;
}

int ControlPanelClient::sendSetBool(const std::string& target, bool value) {
    ControlJsonValue json = ControlJsonValue::boolValueOf(value);
    std::lock_guard<std::mutex> lock(mutex);
    int id = nextId++;
    while (outbound.size() >= maxPendingClientMessages)
        outbound.pop_front();
    outbound.push_back(setMessage(id, target, json));
    return id;
}

int ControlPanelClient::sendSetStringArray(
    const std::string& target, const std::vector<std::string>& value) {
    ControlJsonValue json = ControlJsonValue::arrayValueOf();
    for (std::vector<std::string>::const_iterator it = value.begin();
         it != value.end(); ++it)
        json.append(ControlJsonValue::stringValueOf(*it));

    std::lock_guard<std::mutex> lock(mutex);
    int id = nextId++;
    while (outbound.size() >= maxPendingClientMessages)
        outbound.pop_front();
    outbound.push_back(setMessage(id, target, json));
    return id;
}

int ControlPanelClient::sendSetFilterchainStages(const std::string& target,
    const std::vector<std::string>& stages,
    const std::vector<int>& enabled) {
    ControlJsonValue json = ControlJsonValue::arrayValueOf();
    for (std::size_t i = 0; i < stages.size(); i++) {
        ControlJsonValue item = ControlJsonValue::objectValueOf();
        item.set("stage", ControlJsonValue::stringValueOf(stages[i]));
        item.set("enabled", ControlJsonValue::boolValueOf(
            i >= enabled.size() || enabled[i] != 0));
        json.append(item);
    }

    std::lock_guard<std::mutex> lock(mutex);
    int id = nextId++;
    while (outbound.size() >= maxPendingClientMessages)
        outbound.pop_front();
    outbound.push_back(setMessage(id, target, json));
    return id;
}

int ControlPanelClient::shouldStop() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stopRequested;
}

void ControlPanelClient::setConnected(int connected) {
    std::lock_guard<std::mutex> lock(mutex);
    connectedValue = connected;
}

void ControlPanelClient::pushEvent(const ControlPanelClientEvent& event) {
    std::lock_guard<std::mutex> lock(mutex);
    while (events.size() >= maxPendingClientEvents)
        events.pop_front();
    events.push_back(event);
}

void ControlPanelClient::workerMain() {
    std::string error;
    std::unique_ptr<ControlStream> stream = connectControlEndpoint(
        endpointValue, 1000, &error);
    if (stream.get() == 0) {
        pushEvent(ControlPanelClientEvent(
            ControlPanelClientEvent::Error, error));
        return;
    }

    ControlJsonLineReader reader;
    setConnected(1);
    pushEvent(ControlPanelClientEvent(ControlPanelClientEvent::Connected));

    while (!shouldStop()) {
        char buffer[4096];
        ControlReadResult read = stream->readSome(
            buffer, sizeof(buffer), 10, &error);
        if (read.status == ControlIoReady) {
            std::vector<ControlJsonValue> messages;
            if (!reader.feed(buffer, read.bytes, &messages, &error)) {
                pushEvent(ControlPanelClientEvent(
                    ControlPanelClientEvent::Error, error));
                break;
            }
            for (std::vector<ControlJsonValue>::const_iterator it
                     = messages.begin();
                 it != messages.end(); ++it)
                pushEvent(ControlPanelClientEvent(*it));
        } else if (read.status == ControlIoClosed) {
            break;
        } else if (read.status == ControlIoError) {
            pushEvent(ControlPanelClientEvent(
                ControlPanelClientEvent::Error, error));
            break;
        }

        std::deque<ControlJsonValue> localOutbound;
        {
            std::lock_guard<std::mutex> lock(mutex);
            localOutbound.swap(outbound);
        }

        for (std::deque<ControlJsonValue>::const_iterator it
                 = localOutbound.begin();
             it != localOutbound.end(); ++it) {
            if (!writeMessage(*stream, *it, &error)) {
                pushEvent(ControlPanelClientEvent(
                    ControlPanelClientEvent::Error, error));
                stream->close();
                setConnected(0);
                pushEvent(ControlPanelClientEvent(
                    ControlPanelClientEvent::Disconnected));
                return;
            }
        }

        if (read.status == ControlIoTimeout && localOutbound.empty())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    stream->close();
    setConnected(0);
    pushEvent(ControlPanelClientEvent(ControlPanelClientEvent::Disconnected));
}
