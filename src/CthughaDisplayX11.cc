#include "cthugha.h"
#include "CthughaDisplay.h"
#include "display.h"
#include "DisplayDevice.h"
#include "cth_buffer.h"
#include "disp-sys.h"
#include "imath.h"
#include "Interface.h"
#include "IndexedFrame.h"
#include "DisplayRuntime.h"
#include "OverlaySource.h"

#include <stdint.h>

class VisualFrameView {
public:
    int width() const {
        return cthughaDisplay->sourceWidth();
    }

    int height() const {
        return cthughaDisplay->sourceHeight();
    }

    int size() const {
        return width() * height();
    }

    int displayWidth() const {
        return cthughaDisplay->displayFrameWidth();
    }

    int displayHeight() const {
        return cthughaDisplay->displayFrameHeight();
    }
};

static VisualFrameView visualBuffer() {
    return VisualFrameView();
}

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

static OverlayCommands collectDisplayOverlays() {
    CurrentInterfaceOverlayProducer interfaceProducer;
    ErrorMessagesOverlayProducer errorProducer(errors);
    OverlaySource source(&interfaceProducer, &errorProducer);
    return source.collect();
}

void newCthughaDisplay() { cthughaDisplay = new CthughaDisplayX11(); }

CthughaDisplayX11::CthughaDisplayX11()
    : CthughaDisplay() {
}

CthughaDisplayX11::~CthughaDisplayX11() {
}

void CthughaDisplayX11::operator()() {
    int traceDisplayTiming = CTH_LOG_ENABLED(CTH_LOG_TRACE);
    double displayTiming[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (traceDisplayTiming)
        displayTiming[0] = getTime();

    /*
     * Sync the palette before indexed pixels are expanded or copied. Waiting
     * until postDraw() leaves the first frame after a palette change rendered
     * through stale bitmap colors.
     */
    displayDevice->setGlobalPalette();
    if (traceDisplayTiming)
        displayTiming[1] = getTime();

    composePresentationFrame();
    if (traceDisplayTiming)
        displayTiming[2] = getTime();

    /*
     * prepare the display device
     */
    displayDevice->preDraw();
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
    int borderClearRequested = displayDevice->textOnScreen || needsClear;
    clearBorder();

    OverlayCommands overlays = collectDisplayOverlays();
    if (traceDisplayTiming)
        displayTiming[5] = getTime();

    /*
     * Transfer indexed pixels to backend-native display memory.
     */
    if (displayRuntime != NULL)
        displayRuntime->present(indexedDisplayFrameValue, displayViewport(),
            displayDevice->needsFullCopy, borderClearRequested, overlays);
    if (traceDisplayTiming)
        displayTiming[6] = getTime();

    /*
     * make sure everything is really copied to the screen
     */
    displayDevice->postDraw();
    if (traceDisplayTiming) {
        displayTiming[7] = getTime();
        CTH_TRACE("x11 frame-ms=%.3f palette-ms=%.3f compose-ms=%.3f prepare-ms=%.3f viewport-ms=%.3f overlay-ms=%.3f transfer-clear-ms=%.3f post-ms=%.3f draw-mode=%d bypp=%d size=%dx%d draw=%dx%d\n",
            "display timing",
            (displayTiming[7] - displayTiming[0]) * 1000.0,
            (displayTiming[1] - displayTiming[0]) * 1000.0,
            (displayTiming[2] - displayTiming[1]) * 1000.0,
            (displayTiming[3] - displayTiming[2]) * 1000.0,
            (displayTiming[4] - displayTiming[3]) * 1000.0,
            (displayTiming[5] - displayTiming[4]) * 1000.0,
            (displayTiming[6] - displayTiming[5]) * 1000.0,
            (displayTiming[7] - displayTiming[6]) * 1000.0,
            draw_mode, bypp, disp_size.x, disp_size.y, draw_size.x, draw_size.y);
    }
}
