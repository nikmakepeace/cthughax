/** @file
 * X11 display coordinator.
 */

#include "cthugha.h"
#include "CthughaDisplay.h"
#include "display.h"
#include "DisplayDevice.h"
#include "DisplayOverlay.h"
#include "imath.h"
#include "Interface.h"
#include "InterfaceRuntime.h"
#include "IndexedFrame.h"
#include "DisplayPresentationOptions.h"
#include "DisplayRuntime.h"
#include "OverlaySource.h"
#include "ProcessServices.h"

#include <stdint.h>

std::unique_ptr<CthughaDisplay> newCthughaDisplay(
    DisplayDevice& device, DisplayRuntime& runtime, SecondsClock& clock,
    DisplayPresentationSettings& settings, InterfaceRuntime& interfaceRuntime,
    ErrorMessages& errorMessages) {
    return std::unique_ptr<CthughaDisplay>(
        new CthughaDisplayX11(device, runtime, clock, settings,
            interfaceRuntime, errorMessages));
}

CthughaDisplayX11::CthughaDisplayX11(DisplayDevice& device,
    DisplayRuntime& runtime, SecondsClock& clock_,
    DisplayPresentationSettings& settings_, InterfaceRuntime& interfaceRuntime_,
    ErrorMessages& errorMessages_)
    : CthughaDisplay(device, runtime, clock_, settings_)
    , clock(clock_)
    , interfaceRuntime(interfaceRuntime_)
    , errorMessages(errorMessages_) {
}

CthughaDisplayX11::~CthughaDisplayX11() {
}

void CthughaDisplayX11::operator()() {
    DisplayDevice& target = device();
    DisplayRuntime& stage = runtime();
    int traceDisplayTiming = CTH_LOG_ENABLED(CTH_LOG_TRACE);
    double displayTiming[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    if (traceDisplayTiming)
        displayTiming[0] = clock.nowSeconds();

    /*
     * Sync the palette before indexed pixels are expanded or copied. Waiting
     * until postDraw() leaves the first frame after a palette change rendered
     * through stale bitmap colors.
     */
    target.setGlobalPalette();
    if (traceDisplayTiming)
        displayTiming[1] = clock.nowSeconds();

    composePresentationFrame();
    if (traceDisplayTiming)
        displayTiming[2] = clock.nowSeconds();

    /*
     * prepare the display device
     */
    target.preDraw();
    if (traceDisplayTiming)
        displayTiming[3] = clock.nowSeconds();

    /*
     * The composer has already produced completed indexed pixels.  From this
     * point on X11 only converts, scales, decorates, and presents them.
     */
    buffer = buffer0;
    bufferWidth = indexedDisplayFrameValue.pitch();

    checkZoom();
    if (traceDisplayTiming)
        displayTiming[4] = clock.nowSeconds();

    /*
     * clear the border around the image
     */
    int borderClearRequested = target.textOnScreen || needsClear;
    clearBorder();

    OverlayLayout overlayLayout(text_size.x, text_size.y, fontSize.x,
        fontSize.y);
    DisplayStatusSnapshot overlayStatus(status(), currentFrameDeltaSeconds());
    OverlayCommands overlays = collectDisplayOverlayCommands(fps,
        interfaceRuntime, errorMessages, overlayLayout, overlayStatus,
        settings());
    if (traceDisplayTiming)
        displayTiming[5] = clock.nowSeconds();

    /*
     * Transfer indexed pixels to backend-native display memory.
     */
    stage.present(indexedDisplayFrameValue, displayViewport(),
        target.needsFullCopy, borderClearRequested, overlays);
    if (traceDisplayTiming)
        displayTiming[6] = clock.nowSeconds();

    /*
     * make sure everything is really copied to the screen
     */
    target.postDraw();
    if (traceDisplayTiming) {
        displayTiming[7] = clock.nowSeconds();
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
