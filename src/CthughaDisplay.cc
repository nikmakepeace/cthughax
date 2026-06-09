#include "cthugha.h"
#include "CthughaDisplay.h"
#include "display.h"
#include "DisplayDevice.h"
#include "DisplayPresentationOptions.h"
#include "DisplayRuntime.h"
#include "Configuration.h"
#include "imath.h"
#include "Interface.h"
#include "IndexedFrame.h"
#include "Screen.h"
#include "FrameRenderContext.h"
#include "ViewportPolicy.h"
#include "ViewportPresentation.h"

#include <stdint.h>

CthughaDisplay::~CthughaDisplay() { }

class GlobalPresentationScreenSelection : public PresentationScreenSelection {
public:
    virtual ScreenEntry* current() {
        return (ScreenEntry*)screen.current();
    }
};

CthughaDisplay::CthughaDisplay(DisplayDevice& device, DisplayRuntime& runtime,
    SecondsClock& clock, DisplayPresentationSettings& settings)
    : deviceValue(device)
    , runtimeValue(runtime)
    , presentationSettingsValue(settings)
    , sourceFrame(0)
    , presentationContextValue(0)
    , indexedDisplayFrameValue()
    , presentationComposer()
    , displayViewportValue()
    , frameClockValue(clock)
    , frameNowValue(0.0)
    , frameDeltaTValue(0.0)
    , buffer0(0)
    , visualLatencyEstimate(0)
    , buffer(0)
    , bufferWidth(0)
    , needsClear(1)
    , fps(0)
    , rollingFps(0) {
}

void CthughaDisplay::present(const IndexedFrame& frame) {
    if (!frame.valid()) {
        sourceFrame = NULL;
        return;
    }

    sourceFrame = &frame;
    if (frame.framePalette != NULL)
        device().setFramePalette(frame.framePalette);
    presentSourceWithContext(0);
}

void CthughaDisplay::present(const IndexedFrame& frame,
    const FrameRenderContext& context) {
    if (!frame.valid()) {
        sourceFrame = NULL;
        return;
    }

    sourceFrame = &frame;
    if (frame.framePalette != NULL)
        device().setFramePalette(frame.framePalette);
    presentSourceWithContext(&context);
}

void CthughaDisplay::presentSourceWithContext(const FrameRenderContext* context) {
    presentationContextValue = context;
    (*this)();
    presentationContextValue = 0;
}

const unsigned char* CthughaDisplay::sourcePixels() const {
    if (sourceFrame != NULL && sourceFrame->valid())
        return sourceFrame->pixels;

    return NULL;
}

int CthughaDisplay::sourceWidth() const {
    if (sourceFrame != NULL && sourceFrame->valid())
        return sourceFrame->width;

    return 0;
}

int CthughaDisplay::sourceHeight() const {
    if (sourceFrame != NULL && sourceFrame->valid())
        return sourceFrame->height;

    return 0;
}

int CthughaDisplay::sourcePitch() const {
    if (sourceFrame != NULL && sourceFrame->valid())
        return sourceFrame->pitch;

    return 0;
}

int CthughaDisplay::sourceSize() const {
    return sourceWidth() * sourceHeight();
}

const IndexedDisplayFrame& CthughaDisplay::indexedDisplayFrame() const {
    return indexedDisplayFrameValue;
}

const DisplayViewport& CthughaDisplay::displayViewport() const {
    return displayViewportValue;
}

int CthughaDisplay::displayFrameWidth() const {
    if (indexedDisplayFrameValue.valid())
        return indexedDisplayFrameValue.width();

    return 2 * sourceWidth();
}

int CthughaDisplay::displayFrameHeight() const {
    if (indexedDisplayFrameValue.valid())
        return indexedDisplayFrameValue.height();

    return 2 * sourceHeight();
}

void CthughaDisplay::indexedPixelsWillMove(unsigned char*) {
}

void CthughaDisplay::indexedFrameGeometryChanged() {
    needsClear = 1;
}

const IndexedDisplayFrame& CthughaDisplay::composePresentationFrame(
    PresentationScreenSelection& screenSelection) {
    IndexedFrame screenSource(sourcePixels(), sourceWidth(), sourceHeight(), sourcePitch(),
        sourceFrame != NULL ? sourceFrame->framePalette : NULL);
    const IndexedDisplayFrame& frame = presentationComposer.compose(screenSource,
        indexedDisplayFrameValue, screenSelection, frameNowValue,
        frameDeltaTValue, fps, this, presentationContextValue);
    buffer0 = indexedDisplayFrameValue.pixels();
    return frame;
}

const IndexedDisplayFrame& CthughaDisplay::composePresentationFrame() {
    GlobalPresentationScreenSelection screenSelection;
    return composePresentationFrame(screenSelection);
}

/*
 * clear the border around the image on the screen
 */
int CthughaDisplay::clearBorder() {

    DisplayDevice& target = device();
    if (target.textOnScreen || needsClear) {
        PixelRect window = ViewportPresentation::fullCopyRect(displayViewportValue);
        PixelRect draw = ViewportPresentation::drawCopyRect(displayViewportValue);
        int bwidth = draw.x; /* width of left border */
        int bheight = draw.y; /* height of upper border */
        int rightWidth = window.width - draw.right();
        int lowerHeight = window.height - draw.bottom();
        if (rightWidth < 0)
            rightWidth = 0;
        if (lowerHeight < 0)
            lowerHeight = 0;

        /* upper border */
        target.clearBox(0, 0, window.width, bheight);
        /* left border */
        target.clearBox(0, bheight, bwidth, draw.height);
        /* right border */
        target.clearBox(draw.right(), bheight, rightWidth, draw.height);
        /* lower border */
        target.clearBox(0, draw.bottom(), window.width, lowerHeight);

        /* if text is on screen, we have to clean in the next iteration,
           because the text may be removed then */
        needsClear = (target.textOnScreen) ? 1 : 0;

        // Border clearing touches pixels outside the normal image rectangle,
        // so partial-copy display devices must refresh the full frame.
        target.needsFullCopy = 1;
    }
    return 0;
}

void CthughaDisplay::checkZoom() {
    ViewportPolicy policy;
    DisplayViewport previous = displayViewportValue;
    displayViewportValue = policy.viewportFor(
        PixelSize(displayFrameWidth(), displayFrameHeight()),
        runtime().outputSize(), int(settings().zoom));

    for (int i = 0; i < displayViewportValue.reductionCount; ++i)
        CTH_ERROR("Zoom factor is set too high for current display size. reducing.\n");

    if (displayViewportValue.effectiveZoom != int(settings().zoom))
        settings().zoom.setValue(displayViewportValue.effectiveZoom);

    if (displayViewportValue.requiresBorderClearFrom(previous))
        needsClear = 1;
}

DisplayDevice& CthughaDisplay::device() {
    return deviceValue;
}

DisplayRuntime& CthughaDisplay::runtime() {
    return runtimeValue;
}

DisplayPresentationSettings& CthughaDisplay::settings() {
    return presentationSettingsValue;
}

void CthughaDisplay::updateFPS() {
    fps = frameClockValue.framesPerSecond();
    rollingFps = frameClockValue.rollingFramesPerSecond();

    CTH_TRACE("updateFPS deltaT-ms=%.3f fps=%.3f rolling-fps=%.3f\n",
        "frame pacing", frameDeltaTValue * 1000.0, fps, rollingFps);
}

void CthughaDisplay::observeVisualLatency(double seconds) {
    double previous = visualLatencyEstimate;
    double alpha = 0.1;

    if (seconds < 0)
        seconds = 0;

    if (visualLatencyEstimate <= 0)
        visualLatencyEstimate = seconds;
    else
        visualLatencyEstimate = visualLatencyEstimate * (1.0 - alpha) + seconds * alpha;

    CTH_TRACE("visual-latency observed-ms=%.3f previous-ms=%.3f alpha=%.3f estimate-ms=%.3f\n",
        "display timing", seconds * 1000.0, previous * 1000.0, alpha,
        visualLatencyEstimate * 1000.0);
}

double CthughaDisplay::visualLatencySeconds() const {
    return visualLatencyEstimate;
}

double CthughaDisplay::currentFrameTimeSeconds() const {
    return frameNowValue;
}

double CthughaDisplay::currentFrameDeltaSeconds() const {
    return frameDeltaTValue;
}

// Start a new frame by publishing a stable timestamp for all modules that run
// during this frame, then update FPS accounting.
void CthughaDisplay::nextFrame() {

    double previousNow = frameNowValue;
    frameClockValue.beginFrame();
    frameNowValue = frameClockValue.now();
    frameDeltaTValue = frameClockValue.deltaT();
    CTH_TRACE("nextFrame previous-now=%.6f sampled-now=%.6f raw-delta-ms=%.3f\n",
        "frame pacing", previousNow, frameNowValue, frameDeltaTValue * 1000.0);

    updateFPS();
}

const char* CthughaDisplay::status() {
    static char txt[512];

    snprintf(txt, sizeof(txt), "fps: %5.2f ", fps);
    return txt;
}
