/** @file
 * Unit coverage for local control byte-stream transports.
 */

#include "ControlTransport.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#endif

static std::string testInstanceId(const char* suffix) {
    char text[128];
#ifdef _WIN32
    snprintf(text, sizeof(text), "test-%s", suffix);
#else
    snprintf(text, sizeof(text), "test-%ld-%s", long(getpid()), suffix);
#endif
    return text;
}

static void testCreateListenerProvidesEndpoint() {
    std::string error;
    std::unique_ptr<ControlListener> listener
        = createControlListener(testInstanceId("endpoint"), &error);

    if (listener.get() == 0)
        fprintf(stderr, "listener error: %s\n", error.c_str());
    assert(listener.get() != 0);
    assert(!listener->endpoint().text.empty());
#ifdef _WIN32
    assert(listener->endpoint().text.find("pipe:") == 0);
#else
    assert(listener->endpoint().text.find("unix:") == 0);
#endif
}

static void testAcceptTimesOutWithoutClient() {
    std::string error;
    std::unique_ptr<ControlListener> listener
        = createControlListener(testInstanceId("timeout"), &error);

    if (listener.get() == 0)
        fprintf(stderr, "listener error: %s\n", error.c_str());
    assert(listener.get() != 0);
    std::unique_ptr<ControlStream> accepted = listener->accept(1, &error);
    assert(accepted.get() == 0);
}

static void testClientServerRoundTrip() {
    std::string error;
    std::unique_ptr<ControlListener> listener
        = createControlListener(testInstanceId("roundtrip"), &error);

    if (listener.get() == 0)
        fprintf(stderr, "listener error: %s\n", error.c_str());
    assert(listener.get() != 0);
    std::unique_ptr<ControlStream> client
        = connectControlEndpoint(listener->endpoint().text, 1000, &error);
    assert(client.get() != 0);
    std::unique_ptr<ControlStream> server = listener->accept(1000, &error);
    assert(server.get() != 0);

    const char* request = "{\"v\":1,\"type\":\"hello\"}\n";
    assert(client->writeAll(request, strlen(request), 1000, &error)
        == ControlIoReady);

    char buffer[128];
    ControlReadResult read = server->readSome(
        buffer, sizeof(buffer), 1000, &error);
    assert(read.status == ControlIoReady);
    assert(read.bytes == strlen(request));
    assert(strncmp(buffer, request, read.bytes) == 0);

    const char* response = "{\"v\":1,\"type\":\"ack\",\"id\":1}\n";
    assert(server->writeAll(response, strlen(response), 1000, &error)
        == ControlIoReady);
    read = client->readSome(buffer, sizeof(buffer), 1000, &error);
    assert(read.status == ControlIoReady);
    assert(read.bytes == strlen(response));
    assert(strncmp(buffer, response, read.bytes) == 0);
}

static void testListenerCloseCleansEndpoint() {
    std::string error;
    std::unique_ptr<ControlListener> listener
        = createControlListener(testInstanceId("cleanup"), &error);

    if (listener.get() == 0)
        fprintf(stderr, "listener error: %s\n", error.c_str());
    assert(listener.get() != 0);
    std::string endpoint = listener->endpoint().text;
    listener->close();

#ifndef _WIN32
    std::string path = endpoint.substr(strlen("unix:"));
    assert(access(path.c_str(), F_OK) != 0);
#else
    (void)endpoint;
#endif
}

int main() {
#ifndef _WIN32
    setenv("XDG_RUNTIME_DIR", "/tmp", 1);
#endif
    testCreateListenerProvidesEndpoint();
    testAcceptTimesOutWithoutClient();
    testClientServerRoundTrip();
    testListenerCloseCleansEndpoint();
    return 0;
}
