#include "cthugha.h"
#include "CthughaDisplay.h"
#include "display.h"
#include "DisplayDevice.h"
#include "cth_buffer.h"
#include "imath.h"
#include "Interface.h"
#include "IndexedFrame.h"
#include "DisplayRuntime.h"
#include "FpsOverlay.h"
#include "OverlaySource.h"

#include <stdint.h>

class RecordingOverlayDisplayDevice : public DisplayDevice {
    OverlaySink& sink;

public:
    explicit RecordingOverlayDisplayDevice(OverlaySink& sink_)
        : DisplayDevice()
        , sink(sink_) {
    }

    virtual double print(const char* text, double y, int justification,
        int color, int noDarken = 0) {
        return sink.printText(text, y, justification, color, noDarken);
    }
};

class ScopedOverlayDisplayDevice {
    DisplayDevice* previous;
    RecordingOverlayDisplayDevice recorder;

public:
    explicit ScopedOverlayDisplayDevice(OverlaySink& sink)
        : previous(displayDevice)
        , recorder(sink) {
        displayDevice = &recorder;
    }

    ~ScopedOverlayDisplayDevice() {
        displayDevice = previous;
    }
};

class CurrentInterfaceOverlayProducer : public OverlayProducer {
public:
    virtual void produceOverlay(OverlaySink& sink) {
        Interface* currentInterface = Interface::current;
        if (currentInterface == 0)
            return;

        ScopedOverlayDisplayDevice scope(sink);
        currentInterface->display();
    }
};

class ErrorMessagesOverlayProducer : public OverlayProducer {
    ErrorMessages& errorMessages;

public:
    explicit ErrorMessagesOverlayProducer(ErrorMessages& errorMessages_)
        : errorMessages(errorMessages_) {
    }

    virtual void produceOverlay(OverlaySink& sink) {
        ScopedOverlayDisplayDevice scope(sink);
        errorMessages.display();
    }
};

static OverlayCommands collectDisplayOverlays(double framesPerSecond) {
    CurrentInterfaceOverlayProducer interfaceProducer;
    ErrorMessagesOverlayProducer errorProducer(errors);
    OverlaySource source(&interfaceProducer, &errorProducer);
    OverlayCommands overlays = source.collect();
    FpsOverlay::append(overlays, framesPerSecond, int(showFPS));
    return overlays;
}

std::unique_ptr<CthughaDisplay> newCthughaDisplay(
    DisplayDevice& device, DisplayRuntime& runtime) {
    return std::unique_ptr<CthughaDisplay>(new CthughaDisplayX11(device, runtime));
}

CthughaDisplayX11::CthughaDisplayX11(DisplayDevice& device, DisplayRuntime& runtime)
    : CthughaDisplay(device, runtime) {
}

CthughaDisplayX11::~CthughaDisplayX11() {
}

void CthughaDisplayX11::operator()() {
    DisplayDevice& target = device();
    DisplayRuntime& stage = runtime();
    int traceDisplayTiming = CTH_LOG_ENABLED(CTH_LOG_TRACE);
    double displayTiming[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (traceDisplayTiming)
        displayTiming[0] = getTime();

    /*
     * Sync the palette before indexed pixels are expanded or copied. Waiting
     * until postDraw() leaves the first frame after a palette change rendered
     * through stale bitmap colors.
     */
    target.setGlobalPalette();
    if (traceDisplayTiming)
        displayTiming[1] = getTime();

    composePresentationFrame();
    if (traceDisplayTiming)
        displayTiming[2] = getTime();

    /*
     * prepare the display device
     */
    target.preDraw();
    if (traceDisplayTiming)
        displayTiming[3] = getTime();

    /*
     * The composer has already produced completed indexed pixels.  From this
     * point on X11 only converts, scales, decorates, and presents them.
     */
    buffer = buffer0;
    bufferWidth = indexedDisplayFrameValue.pitch();

    checkZoom();
    if (traceDisplayTiming)
        displayTiming[4] = getTime();

    /*
     * clear the border around the image
     */
    int borderClearRequested = target.textOnScreen || needsClear;
    clearBorder();

    OverlayCommands overlays = collectDisplayOverlays(fps);
    if (traceDisplayTiming)
        displayTiming[5] = getTime();

    /*
     * Transfer indexed pixels to backend-native display memory.
     */
    stage.present(indexedDisplayFrameValue, displayViewport(),
        target.needsFullCopy, borderClearRequested, overlays);
    if (traceDisplayTiming)
        displayTiming[6] = getTime();

    /*
     * make sure everything is really copied to the screen
     */
    target.postDraw();
    if (traceDisplayTiming) {
        displayTiming[7] = getTime();
        const DisplayViewport& viewport = displayViewport();
        CTH_TRACE("x11 frame-ms=%.3f palette-ms=%.3f compose-ms=%.3f prepare-ms=%.3f viewport-ms=%.3f overlay-ms=%.3f transfer-clear-ms=%.3f post-ms=%.3f size=%dx%d draw=%dx%d\n",
            "display timing",
            (displayTiming[7] - displayTiming[0]) * 1000.0,
            (displayTiming[1] - displayTiming[0]) * 1000.0,
            (displayTiming[2] - displayTiming[1]) * 1000.0,
            (displayTiming[3] - displayTiming[2]) * 1000.0,
            (displayTiming[4] - displayTiming[3]) * 1000.0,
            (displayTiming[5] - displayTiming[4]) * 1000.0,
            (displayTiming[6] - displayTiming[5]) * 1000.0,
            (displayTiming[7] - displayTiming[6]) * 1000.0,
            viewport.windowSize.width, viewport.windowSize.height,
            viewport.drawSize.width, viewport.drawSize.height);
    }
}
