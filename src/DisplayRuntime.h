#ifndef __DISPLAY_RUNTIME_H
#define __DISPLAY_RUNTIME_H

#include "DisplayBackend.h"

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

extern DisplayRuntime* displayRuntime;

#endif
