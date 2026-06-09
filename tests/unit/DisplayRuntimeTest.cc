#include "DisplayRuntime.h"
#include "FramePalette.h"
#include "InputQueue.h"
#include "PixelTransfer.h"

#include <assert.h>
#include <string.h>

class FakeBackend : public DisplayBackend {
public:
    int processEventCalls;
    mutable int outputSizeCalls;
    int presentCalls;
    DisplayEventStats nextStats;
    PixelSize outputSizeValue;
    const IndexedDisplayFrame* presentedFrame;
    FramePalette* presentedPalette;
    DisplayViewport presentedViewport;
    int presentedNeedsFullCopy;
    int presentedNeedsBorderClear;
    int presentedOverlayCount;
    OverlayTextCommand presentedFirstOverlay;

    FakeBackend()
        : processEventCalls(0)
        , outputSizeCalls(0)
        , presentCalls(0)
        , nextStats()
        , outputSizeValue(12, 10)
        , presentedFrame(0)
        , presentedPalette(0)
        , presentedViewport()
        , presentedNeedsFullCopy(0)
        , presentedNeedsBorderClear(0)
        , presentedOverlayCount(0)
        , presentedFirstOverlay() {
    }

    virtual DisplayEventStats processEvents(InputEventSink&) {
        processEventCalls++;
        return nextStats;
    }

    virtual PixelSize outputSize() const {
        outputSizeCalls++;
        return outputSizeValue;
    }

    virtual void present(const DisplayPresentation& presentation) {
        presentCalls++;
        presentedFrame = &presentation.frame;
        presentedPalette = presentation.framePalette;
        presentedViewport = presentation.viewport;
        presentedNeedsFullCopy = presentation.needsFullCopy;
        presentedNeedsBorderClear = presentation.needsBorderClear;
        presentedOverlayCount = presentation.overlays.count();
        if (presentedOverlayCount > 0)
            presentedFirstOverlay = presentation.overlays.at(0);
    }
};

class IgnoringInputSink : public InputEventSink {
public:
    virtual void pushRawKey(const char*, int) { }
};

class TransferringBackend : public DisplayBackend {
public:
    unsigned char destination[16];
    int presentCalls;

    TransferringBackend()
        : presentCalls(0) {
        memset(destination, 0xaa, sizeof(destination));
    }

    virtual DisplayEventStats processEvents(InputEventSink&) {
        return DisplayEventStats();
    }

    virtual PixelSize outputSize() const {
        return PixelSize(4, 4);
    }

    virtual void present(const DisplayPresentation& presentation) {
        presentCalls++;
        PixelTransfer::indexedToNative(presentation.frame.pixels(),
            PixelSize(presentation.frame.width(), presentation.frame.height()),
            presentation.frame.pitch(), destination,
            presentation.viewport.drawSize, 4, 1, 0);
    }
};

static DisplayViewport fixedViewport() {
    DisplayViewport viewport;
    viewport.frameSize = PixelSize(4, 3);
    viewport.windowSize = PixelSize(12, 10);
    viewport.drawSize = PixelSize(4, 3);
    viewport.destination = PixelRect(4, 3, 4, 3);
    viewport.scaleMode = SCALE_MODE_FIXED_ZOOM;
    viewport.requestedZoom = 1;
    viewport.effectiveZoom = 1;
    return viewport;
}

static void testProcessEventsDelegatesToBackend() {
    FakeBackend backend;
    backend.nextStats.eventCount = 5;
    backend.nextStats.resizeEvents = 2;
    backend.nextStats.exposeEvents = 1;
    DisplayRuntime runtime(backend);
    IgnoringInputSink input;

    DisplayEventStats stats = runtime.processEvents(input);

    assert(backend.processEventCalls == 1);
    assert(stats.eventCount == 5);
    assert(stats.resizeEvents == 2);
    assert(stats.exposeEvents == 1);
}

static void testOutputSizeDelegatesToBackend() {
    FakeBackend backend;
    backend.outputSizeValue = PixelSize(20, 13);
    DisplayRuntime runtime(backend);

    PixelSize outputSize = runtime.outputSize();

    assert(backend.outputSizeCalls == 1);
    assert(outputSize == PixelSize(20, 13));
}

static void testPresentationRequestCarriesFrameViewportAndFlags() {
    FakeBackend backend;
    DisplayRuntime runtime(backend);
    IndexedDisplayFrame frame;
    FramePalette palette;
    DisplayViewport viewport = fixedViewport();
    frame.resize(4, 3, 6);
    frame.setFramePalette(&palette);

    runtime.present(frame, viewport, 1, 0);

    assert(backend.presentCalls == 1);
    assert(backend.presentedFrame == &frame);
    assert(backend.presentedPalette == &palette);
    assert(backend.presentedViewport.frameSize == viewport.frameSize);
    assert(backend.presentedViewport.windowSize == viewport.windowSize);
    assert(backend.presentedViewport.drawSize == viewport.drawSize);
    assert(backend.presentedViewport.destination == viewport.destination);
    assert(backend.presentedViewport.scaleMode == viewport.scaleMode);
    assert(backend.presentedViewport.requestedZoom == viewport.requestedZoom);
    assert(backend.presentedViewport.effectiveZoom == viewport.effectiveZoom);
    assert(backend.presentedNeedsFullCopy == 1);
    assert(backend.presentedNeedsBorderClear == 0);
}

static void testPresentationRequestCarriesOverlayCommandsAndInvalidationFlags() {
    FakeBackend backend;
    DisplayRuntime runtime(backend);
    IndexedDisplayFrame frame;
    DisplayViewport viewport = fixedViewport();
    OverlayCommands overlays;
    frame.resize(4, 3, 6);
    overlays.addText("help", 2.0, 'c', 2, 1);

    runtime.present(frame, viewport, 0, 1, overlays);

    assert(backend.presentCalls == 1);
    assert(backend.presentedNeedsFullCopy == 0);
    assert(backend.presentedNeedsBorderClear == 1);
    assert(backend.presentedOverlayCount == 1);
    assert(strcmp(backend.presentedFirstOverlay.text.c_str(), "help") == 0);
    assert(backend.presentedFirstOverlay.y == 2.0);
    assert(backend.presentedFirstOverlay.justification == 'c');
    assert(backend.presentedFirstOverlay.color == 2);
    assert(backend.presentedFirstOverlay.noDarken == 1);
}

static void testBackendCanTransferPresentationFrameFromRuntime() {
    TransferringBackend backend;
    DisplayRuntime runtime(backend);
    IndexedDisplayFrame frame;
    DisplayViewport viewport;
    frame.resize(2, 2);
    frame.line(0)[0] = 10;
    frame.line(0)[1] = 20;
    frame.line(1)[0] = 30;
    frame.line(1)[1] = 40;
    viewport.frameSize = PixelSize(2, 2);
    viewport.windowSize = PixelSize(4, 4);
    viewport.drawSize = PixelSize(4, 4);
    viewport.destination = PixelRect(0, 0, 4, 4);

    runtime.present(frame, viewport, 0, 0);

    assert(backend.presentCalls == 1);
    assert(backend.destination[0] == 10);
    assert(backend.destination[1] == 10);
    assert(backend.destination[2] == 20);
    assert(backend.destination[3] == 20);
    assert(backend.destination[4] == 10);
    assert(backend.destination[5] == 10);
    assert(backend.destination[6] == 20);
    assert(backend.destination[7] == 20);
    assert(backend.destination[8] == 30);
    assert(backend.destination[9] == 30);
    assert(backend.destination[10] == 40);
    assert(backend.destination[11] == 40);
    assert(backend.destination[12] == 30);
    assert(backend.destination[13] == 30);
    assert(backend.destination[14] == 40);
    assert(backend.destination[15] == 40);
}

int main() {
    testProcessEventsDelegatesToBackend();
    testOutputSizeDelegatesToBackend();
    testPresentationRequestCarriesFrameViewportAndFlags();
    testPresentationRequestCarriesOverlayCommandsAndInvalidationFlags();
    testBackendCanTransferPresentationFrameFromRuntime();
    return 0;
}
