#include "DisplayRuntime.h"

#include <utility>

DisplayRuntimeOwnership::DisplayRuntimeOwnership(
    std::unique_ptr<DisplayDevice> device_,
    std::unique_ptr<DisplayBackend> backend_,
    std::unique_ptr<DisplayRuntime> runtime_)
    : deviceValue(std::move(device_))
    , backendValue(std::move(backend_))
    , runtimeValue(std::move(runtime_)) {
}

DisplayRuntimeOwnership::~DisplayRuntimeOwnership() {
    shutdown();
}

DisplayDevice& DisplayRuntimeOwnership::device() {
    return *deviceValue;
}

DisplayBackend& DisplayRuntimeOwnership::backend() {
    return *backendValue;
}

DisplayRuntime& DisplayRuntimeOwnership::runtime() {
    return *runtimeValue;
}

void DisplayRuntimeOwnership::publishAliases() {
    displayDevice = deviceValue.get();
    displayBackend = backendValue.get();
    displayRuntime = runtimeValue.get();
}

void DisplayRuntimeOwnership::shutdown() {
    if (displayRuntime == runtimeValue.get())
        displayRuntime = 0;
    runtimeValue.reset();

    if (displayBackend == backendValue.get())
        displayBackend = 0;
    backendValue.reset();

    if (displayDevice == deviceValue.get())
        displayDevice = 0;
    deviceValue.reset();
}
