#include "DisplayRuntime.h"

DisplayBackend* displayBackend = 0;
DisplayRuntime* displayRuntime = 0;

DisplayRuntime::DisplayRuntime(DisplayBackend& backend_)
    : backend(backend_) {
}

DisplayEventStats DisplayRuntime::processEvents() {
    return backend.processEvents();
}

PixelSize DisplayRuntime::outputSize() const {
    return backend.outputSize();
}

void DisplayRuntime::present(const IndexedDisplayFrame& frame,
    const DisplayViewport& viewport, int needsFullCopy,
    int needsBorderClear) {
    DisplayPresentation presentation(frame, viewport, needsFullCopy,
        needsBorderClear);
    backend.present(presentation);
}

void DisplayRuntime::present(const IndexedDisplayFrame& frame,
    const DisplayViewport& viewport, int needsFullCopy,
    int needsBorderClear, const OverlayCommands& overlays) {
    DisplayPresentation presentation(frame, viewport, needsFullCopy,
        needsBorderClear, overlays);
    backend.present(presentation);
}
