#ifndef __DISPLAY_RUNTIME_H
#define __DISPLAY_RUNTIME_H

#include "DisplayBackend.h"

#include <memory>

class DisplayRuntime {
    DisplayBackend& backend;

public:
    explicit DisplayRuntime(DisplayBackend& backend_);

    DisplayEventStats processEvents();
    PixelSize outputSize() const;
    void present(const IndexedDisplayFrame& frame,
        const DisplayViewport& viewport, int needsFullCopy,
        int needsBorderClear);
    void present(const IndexedDisplayFrame& frame,
        const DisplayViewport& viewport, int needsFullCopy,
        int needsBorderClear, const OverlayCommands& overlays);
};

class DisplayRuntimeOwnership {
    std::unique_ptr<DisplayDevice> deviceValue;
    std::unique_ptr<DisplayBackend> backendValue;
    std::unique_ptr<DisplayRuntime> runtimeValue;

public:
    DisplayRuntimeOwnership(std::unique_ptr<DisplayDevice> device_,
        std::unique_ptr<DisplayBackend> backend_,
        std::unique_ptr<DisplayRuntime> runtime_);
    ~DisplayRuntimeOwnership();

    DisplayDevice& device();
    DisplayBackend& backend();
    DisplayRuntime& runtime();

    void publishAliases();
    void shutdown();
};

extern DisplayRuntime* displayRuntime;

#endif
