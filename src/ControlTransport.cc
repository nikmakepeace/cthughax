/** @file
 * Local byte-stream transport for the JSONL control protocol.
 */

#include "ControlTransport.h"

#include <errno.h>
#include <string.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <stdlib.h>
#endif

ControlReadResult::ControlReadResult()
    : status(ControlIoError)
    , bytes(0) { }

ControlReadResult::ControlReadResult(ControlIoStatus status_, size_t bytes_)
    : status(status_)
    , bytes(bytes_) { }

ControlEndpoint::ControlEndpoint()
    : text() { }

ControlEndpoint::ControlEndpoint(const std::string& text_)
    : text(text_) { }

ControlStream::~ControlStream() { }

ControlListener::~ControlListener() { }

namespace {

static void setError(std::string* error, const std::string& message) {
    if (error != 0)
        *error = message;
}

static std::string prefixedErrno(const char* prefix, int err) {
    std::string result = prefix;
    result += ": ";
    result += strerror(err);
    return result;
}

#ifdef _WIN32

class WindowsPipeStream : public ControlStream {
    HANDLE pipe;

public:
    explicit WindowsPipeStream(HANDLE pipe_)
        : pipe(pipe_) { }

    virtual ~WindowsPipeStream() {
        close();
    }

    virtual ControlReadResult readSome(
        char* data, size_t length, int, std::string* error) {
        DWORD readBytes = 0;
        if (!ReadFile(pipe, data, DWORD(length), &readBytes, NULL)) {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE)
                return ControlReadResult(ControlIoClosed, 0);
            setError(error, "named pipe read failed");
            return ControlReadResult(ControlIoError, 0);
        }
        if (readBytes == 0)
            return ControlReadResult(ControlIoClosed, 0);
        return ControlReadResult(ControlIoReady, size_t(readBytes));
    }

    virtual ControlIoStatus writeAll(
        const char* data, size_t length, int, std::string* error) {
        size_t written = 0;
        while (written < length) {
            DWORD chunk = DWORD(length - written);
            DWORD writtenBytes = 0;
            if (!WriteFile(pipe, data + written, chunk, &writtenBytes, NULL)) {
                DWORD err = GetLastError();
                if (err == ERROR_BROKEN_PIPE)
                    return ControlIoClosed;
                setError(error, "named pipe write failed");
                return ControlIoError;
            }
            written += size_t(writtenBytes);
        }
        return ControlIoReady;
    }

    virtual void close() {
        if (pipe != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe);
            pipe = INVALID_HANDLE_VALUE;
        }
    }
};

class WindowsPipeListener : public ControlListener {
    std::string endpointText;
    HANDLE pipe;

public:
    explicit WindowsPipeListener(const std::string& name)
        : endpointText("pipe:" + name)
        , pipe(INVALID_HANDLE_VALUE) {
        pipe = CreateNamedPipeA(name.c_str(), PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1, 65536, 65536, 0, NULL);
    }

    virtual ~WindowsPipeListener() {
        close();
    }

    bool valid() const {
        return pipe != INVALID_HANDLE_VALUE;
    }

    virtual ControlEndpoint endpoint() const {
        return ControlEndpoint(endpointText);
    }

    virtual std::unique_ptr<ControlStream> accept(int, std::string* error) {
        if (pipe == INVALID_HANDLE_VALUE) {
            setError(error, "named pipe listener is closed");
            return std::unique_ptr<ControlStream>();
        }
        BOOL connected = ConnectNamedPipe(pipe, NULL)
            ? TRUE
            : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected) {
            setError(error, "named pipe accept failed");
            return std::unique_ptr<ControlStream>();
        }
        HANDLE accepted = pipe;
        pipe = INVALID_HANDLE_VALUE;
        return std::unique_ptr<ControlStream>(new WindowsPipeStream(accepted));
    }

    virtual void close() {
        if (pipe != INVALID_HANDLE_VALUE) {
            CloseHandle(pipe);
            pipe = INVALID_HANDLE_VALUE;
        }
    }
};

static std::string pipeNameForInstance(const std::string& instanceId) {
    return "\\\\.\\pipe\\cthugha-control-" + instanceId;
}

#else

class FdControlStream : public ControlStream {
    int fdValue;

    ControlIoStatus waitFor(int readReady, int timeoutMs, std::string* error) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fdValue, &fds);

        struct timeval timeout;
        struct timeval* timeoutPtr = 0;
        if (timeoutMs >= 0) {
            timeout.tv_sec = timeoutMs / 1000;
            timeout.tv_usec = (timeoutMs % 1000) * 1000;
            timeoutPtr = &timeout;
        }

        int result;
        do {
            result = select(fdValue + 1, readReady ? &fds : 0,
                readReady ? 0 : &fds, 0, timeoutPtr);
        } while (result < 0 && errno == EINTR);

        if (result == 0)
            return ControlIoTimeout;
        if (result < 0) {
            setError(error, prefixedErrno("select failed", errno));
            return ControlIoError;
        }
        return ControlIoReady;
    }

public:
    explicit FdControlStream(int fd_)
        : fdValue(fd_) { }

    virtual ~FdControlStream() {
        close();
    }

    virtual ControlReadResult readSome(
        char* data, size_t length, int timeoutMs, std::string* error) {
        if (fdValue < 0)
            return ControlReadResult(ControlIoClosed, 0);

        ControlIoStatus wait = waitFor(1, timeoutMs, error);
        if (wait != ControlIoReady)
            return ControlReadResult(wait, 0);

        ssize_t n;
        do {
            n = recv(fdValue, data, length, 0);
        } while (n < 0 && errno == EINTR);

        if (n == 0)
            return ControlReadResult(ControlIoClosed, 0);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return ControlReadResult(ControlIoTimeout, 0);
            setError(error, prefixedErrno("socket read failed", errno));
            return ControlReadResult(ControlIoError, 0);
        }
        return ControlReadResult(ControlIoReady, size_t(n));
    }

    virtual ControlIoStatus writeAll(
        const char* data, size_t length, int timeoutMs, std::string* error) {
        size_t written = 0;
        while (written < length) {
            ControlIoStatus wait = waitFor(0, timeoutMs, error);
            if (wait != ControlIoReady)
                return wait;

            ssize_t n;
            do {
                int flags = 0;
#ifdef MSG_NOSIGNAL
                flags = MSG_NOSIGNAL;
#endif
                n = send(fdValue, data + written, length - written, flags);
            } while (n < 0 && errno == EINTR);
            if (n == 0)
                return ControlIoClosed;
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return ControlIoTimeout;
                setError(error, prefixedErrno("socket write failed", errno));
                return ControlIoError;
            }
            written += size_t(n);
        }
        return ControlIoReady;
    }

    virtual void close() {
        if (fdValue >= 0) {
            ::close(fdValue);
            fdValue = -1;
        }
    }
};

class UnixControlListener : public ControlListener {
    int fdValue;
    std::string pathValue;
    ControlEndpoint endpointValue;

    ControlIoStatus waitForAccept(int timeoutMs, std::string* error) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fdValue, &fds);

        struct timeval timeout;
        struct timeval* timeoutPtr = 0;
        if (timeoutMs >= 0) {
            timeout.tv_sec = timeoutMs / 1000;
            timeout.tv_usec = (timeoutMs % 1000) * 1000;
            timeoutPtr = &timeout;
        }

        int result;
        do {
            result = select(fdValue + 1, &fds, 0, 0, timeoutPtr);
        } while (result < 0 && errno == EINTR);

        if (result == 0)
            return ControlIoTimeout;
        if (result < 0) {
            setError(error, prefixedErrno("select failed", errno));
            return ControlIoError;
        }
        return ControlIoReady;
    }

public:
    UnixControlListener(int fd_, const std::string& path_)
        : fdValue(fd_)
        , pathValue(path_)
        , endpointValue("unix:" + path_) { }

    virtual ~UnixControlListener() {
        close();
    }

    virtual ControlEndpoint endpoint() const {
        return endpointValue;
    }

    virtual std::unique_ptr<ControlStream> accept(
        int timeoutMs, std::string* error) {
        if (fdValue < 0) {
            setError(error, "unix listener is closed");
            return std::unique_ptr<ControlStream>();
        }
        ControlIoStatus wait = waitForAccept(timeoutMs, error);
        if (wait != ControlIoReady)
            return std::unique_ptr<ControlStream>();

        int accepted;
        do {
            accepted = ::accept(fdValue, 0, 0);
        } while (accepted < 0 && errno == EINTR);

        if (accepted < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                setError(error, prefixedErrno("accept failed", errno));
            return std::unique_ptr<ControlStream>();
        }

        return std::unique_ptr<ControlStream>(new FdControlStream(accepted));
    }

    virtual void close() {
        if (fdValue >= 0) {
            ::close(fdValue);
            fdValue = -1;
        }
        if (!pathValue.empty()) {
            unlink(pathValue.c_str());
            pathValue.clear();
        }
    }
};

static int makeDirectory0700(const std::string& path, std::string* error) {
    if (mkdir(path.c_str(), 0700) == 0)
        return 1;
    if (errno == EEXIST) {
        chmod(path.c_str(), 0700);
        return 1;
    }
    setError(error, prefixedErrno("mkdir failed", errno));
    return 0;
}

static int setNonBlocking(int fd, std::string* error) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        setError(error, prefixedErrno("fcntl get failed", errno));
        return 0;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
        setError(error, prefixedErrno("fcntl set failed", errno));
        return 0;
    }
    return 1;
}

static ControlIoStatus waitForFd(
    int fd, int readReady, int timeoutMs, std::string* error) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval timeout;
    struct timeval* timeoutPtr = 0;
    if (timeoutMs >= 0) {
        timeout.tv_sec = timeoutMs / 1000;
        timeout.tv_usec = (timeoutMs % 1000) * 1000;
        timeoutPtr = &timeout;
    }

    int result;
    do {
        result = select(fd + 1, readReady ? &fds : 0, readReady ? 0 : &fds,
            0, timeoutPtr);
    } while (result < 0 && errno == EINTR);

    if (result == 0)
        return ControlIoTimeout;
    if (result < 0) {
        setError(error, prefixedErrno("select failed", errno));
        return ControlIoError;
    }
    return ControlIoReady;
}

static std::string endpointPath(const std::string& endpoint) {
    const std::string prefix = "unix:";
    if (endpoint.compare(0, prefix.size(), prefix) == 0)
        return endpoint.substr(prefix.size());
    return endpoint;
}

#endif

}

std::string controlDefaultRuntimeDirectory() {
#ifdef _WIN32
    return "";
#else
    const char* runtimeDir = getenv("XDG_RUNTIME_DIR");
    std::string base = (runtimeDir != 0 && runtimeDir[0] != '\0')
        ? runtimeDir
        : "/tmp";
    if (!base.empty() && base[base.size() - 1] == '/')
        return base + "cthugha";
    return base + "/cthugha";
#endif
}

std::unique_ptr<ControlListener> createControlListener(
    const std::string& instanceId, std::string* error) {
#ifdef _WIN32
    std::string name = pipeNameForInstance(instanceId);
    std::unique_ptr<WindowsPipeListener> listener(new WindowsPipeListener(name));
    if (!listener->valid()) {
        setError(error, "CreateNamedPipe failed");
        return std::unique_ptr<ControlListener>();
    }
    return std::unique_ptr<ControlListener>(listener.release());
#else
    std::string directory = controlDefaultRuntimeDirectory();
    if (!makeDirectory0700(directory, error))
        return std::unique_ptr<ControlListener>();

    std::string path = directory + "/cthugha-control-" + instanceId + ".sock";
    struct sockaddr_un sizeProbe;
    if (path.size() >= sizeof(sizeProbe.sun_path)) {
        setError(error, "unix socket path is too long");
        return std::unique_ptr<ControlListener>();
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        setError(error, prefixedErrno("socket failed", errno));
        return std::unique_ptr<ControlListener>();
    }

    if (!setNonBlocking(fd, error)) {
        ::close(fd);
        return std::unique_ptr<ControlListener>();
    }

    unlink(path.c_str());

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);

    if (bind(fd, reinterpret_cast<struct sockaddr*>(&address),
            sizeof(address))
        != 0) {
        setError(error, prefixedErrno("bind failed", errno));
        ::close(fd);
        return std::unique_ptr<ControlListener>();
    }

    if (listen(fd, 1) != 0) {
        setError(error, prefixedErrno("listen failed", errno));
        ::close(fd);
        unlink(path.c_str());
        return std::unique_ptr<ControlListener>();
    }

    return std::unique_ptr<ControlListener>(new UnixControlListener(fd, path));
#endif
}

std::unique_ptr<ControlStream> connectControlEndpoint(
    const std::string& endpoint, int timeoutMs, std::string* error) {
#ifdef _WIN32
    std::string name = endpoint;
    const std::string prefix = "pipe:";
    if (name.compare(0, prefix.size(), prefix) == 0)
        name = name.substr(prefix.size());
    HANDLE pipe = CreateFileA(name.c_str(), GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL);
    if (pipe == INVALID_HANDLE_VALUE) {
        setError(error, "CreateFile named pipe failed");
        return std::unique_ptr<ControlStream>();
    }
    (void)timeoutMs;
    return std::unique_ptr<ControlStream>(new WindowsPipeStream(pipe));
#else
    std::string path = endpointPath(endpoint);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        setError(error, prefixedErrno("socket failed", errno));
        return std::unique_ptr<ControlStream>();
    }

    if (!setNonBlocking(fd, error)) {
        ::close(fd);
        return std::unique_ptr<ControlStream>();
    }

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    if (path.size() >= sizeof(address.sun_path)) {
        setError(error, "unix socket path is too long");
        ::close(fd);
        return std::unique_ptr<ControlStream>();
    }
    strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);

    int result = ::connect(fd, reinterpret_cast<struct sockaddr*>(&address),
        sizeof(address));
    if (result != 0 && errno != EINPROGRESS) {
        setError(error, prefixedErrno("connect failed", errno));
        ::close(fd);
        return std::unique_ptr<ControlStream>();
    }

    ControlIoStatus wait = waitForFd(fd, 0, timeoutMs, error);
    if (wait != ControlIoReady) {
        ::close(fd);
        return std::unique_ptr<ControlStream>();
    }

    int socketError = 0;
    socklen_t socketErrorSize = sizeof(socketError);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError, &socketErrorSize)
        != 0) {
        setError(error, prefixedErrno("getsockopt failed", errno));
        ::close(fd);
        return std::unique_ptr<ControlStream>();
    }
    if (socketError != 0) {
        setError(error, prefixedErrno("connect failed", socketError));
        ::close(fd);
        return std::unique_ptr<ControlStream>();
    }

    return std::unique_ptr<ControlStream>(new FdControlStream(fd));
#endif
}
