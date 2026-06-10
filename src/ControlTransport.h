/** @file
 * Local byte-stream transport for the JSONL control protocol.
 */

#ifndef CTHUGHA_CONTROL_TRANSPORT_H
#define CTHUGHA_CONTROL_TRANSPORT_H

#include <stddef.h>

#include <memory>
#include <string>

enum ControlIoStatus {
    ControlIoReady,
    ControlIoTimeout,
    ControlIoClosed,
    ControlIoError
};

struct ControlReadResult {
    ControlIoStatus status;
    size_t bytes;

    ControlReadResult();
    ControlReadResult(ControlIoStatus status_, size_t bytes_);
};

struct ControlEndpoint {
    std::string text;

    ControlEndpoint();
    explicit ControlEndpoint(const std::string& text_);
};

class ControlStream {
public:
    virtual ~ControlStream();

    virtual ControlReadResult readSome(
        char* data, size_t length, int timeoutMs, std::string* error) = 0;
    virtual ControlIoStatus writeAll(
        const char* data, size_t length, int timeoutMs, std::string* error) = 0;
    virtual void close() = 0;
};

class ControlListener {
public:
    virtual ~ControlListener();

    virtual ControlEndpoint endpoint() const = 0;
    virtual std::unique_ptr<ControlStream> accept(
        int timeoutMs, std::string* error) = 0;
    virtual void close() = 0;
};

std::string controlDefaultRuntimeDirectory();

std::unique_ptr<ControlListener> createControlListener(
    const std::string& instanceId, std::string* error);

std::unique_ptr<ControlStream> connectControlEndpoint(
    const std::string& endpoint, int timeoutMs, std::string* error);

#endif
