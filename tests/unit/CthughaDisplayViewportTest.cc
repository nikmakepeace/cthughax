#include "CthughaDisplay.h"
#include "DisplayDevice.h"
#include "DisplayPresentationOptions.h"
#include "DisplayRuntime.h"
#include "InputQueue.h"
#include "ProcessServices.h"

#include <assert.h>
#include <stdarg.h>
#include <vector>

int bypp = 2;
int bytes_per_line = 40;
enum draw_mode_t draw_mode = DM_mapped2;

DisplayDevice::DisplayDevice()
    : textOnScreen(0)
    , darkenPalette(0)
    , needsFullCopy(0)
    , framePalette(0) { }

DisplayDevice::~DisplayDevice() { }

int DisplayDevice::setGlobalPalette() {
    return 0;
}

void DisplayDevice::setFramePalette(FramePalette* framePalette_) {
    framePalette = framePalette_;
}

void DisplayDevice::prePrint() { }

void DisplayDevice::postPrint() { }

void DisplayDevice::printString(
    int, int, const char*, int, int, int) { }

double DisplayDevice::print(const char*, double y, int, int, int) {
    return y;
}

int screen_up(ScreenRenderContext&) { return 0; }
int screen_down(ScreenRenderContext&) { return 0; }
int screen_2hor(ScreenRenderContext&) { return 0; }
int screen_r2hor(ScreenRenderContext&) { return 0; }
int screen_4hor(ScreenRenderContext&) { return 0; }
int screen_2verd(ScreenRenderContext&) { return 0; }
int screen_r2verd(ScreenRenderContext&) { return 0; }
int screen_4kal(ScreenRenderContext&) { return 0; }
int screen_hfield(ScreenRenderContext&) { return 0; }
int screen_roll(ScreenRenderContext&) { return 0; }
int screen_zick(ScreenRenderContext&) { return 0; }
int screen_bent(ScreenRenderContext&) { return 0; }
int screen_plate(ScreenRenderContext&) { return 0; }
int screen_vscale_hmirror(ScreenRenderContext&) { return 0; }
int screen_hscale_vmirror(ScreenRenderContext&) { return 0; }
int screen_source(ScreenRenderContext&) { return 0; }

int cth_log_enabled(int /*lvl*/) {
    return 0;
}

int cth_log(int /*lvl*/, const char* /*fmt*/, ...) {
    return 0;
}

int cth_log_context(int /*lvl*/, const char* /*context*/, const char* /*fmt*/,
    ...) {
    return 0;
}

int cth_log_error(const char* /*fmt*/, ...) {
    return 0;
}

int cth_log_errno(int /*errnum*/, const char* /*fmt*/, ...) {
    return 0;
}

class FakeSecondsClock : public SecondsClock {
public:
    double value;

    explicit FakeSecondsClock(double value_)
        : value(value_) { }

    virtual double nowSeconds() const { return value; }
};

static FakeSecondsClock displayClock(100.0);

class ViewportDisplayHarness : public CthughaDisplay {
public:
    ViewportDisplayHarness(DisplayDevice& device, DisplayRuntime& runtime,
        DisplayPresentationSettings& settings)
        : CthughaDisplay(device, runtime, displayClock, settings) {
    }

    void setDisplayFrameSize(int width, int height) {
        indexedDisplayFrameValue.resize(width, height);
    }

    void setViewport(const DisplayViewport& viewport) {
        displayViewportValue = viewport;
    }

    void updateViewport() {
        checkZoom();
    }

    int clearViewportBorder() {
        return clearBorder();
    }

};

class RecordingDisplayDevice : public DisplayDevice {
public:
    std::vector<PixelRect> clearedRects;

    virtual void clearBox(int x, int y, int width, int height) {
        clearedRects.push_back(PixelRect(x, y, width, height));
    }
};

class ResizableOutputBackend : public DisplayBackend {
public:
    PixelSize outputSizeValue;

    ResizableOutputBackend()
        : outputSizeValue(0, 0) {
    }

    virtual DisplayEventStats processEvents(InputEventSink&) {
        return DisplayEventStats();
    }

    virtual PixelSize outputSize() const {
        return outputSizeValue;
    }

    virtual void present(const DisplayPresentation&) {
    }
};

class DisplayStageFixture {
public:
    RecordingDisplayDevice device;
    ResizableOutputBackend backend;
    DisplayRuntime runtime;
    DisplayPresentationSettings settings;
    ViewportDisplayHarness display;

    DisplayStageFixture()
        : device()
        , backend()
        , runtime(backend)
        , settings()
        , display(device, runtime, settings) {
    }
};

static void prepareHarness(DisplayStageFixture& fixture, int frameWidth,
    int frameHeight, int windowWidth, int windowHeight, int zoomValue) {
    ViewportDisplayHarness& display = fixture.display;
    display.setDisplayFrameSize(frameWidth, frameHeight);
    fixture.backend.outputSizeValue = PixelSize(windowWidth, windowHeight);
    fixture.settings.zoom.setValue(zoomValue);
    display.needsClear = 0;
}

static void testCheckZoomPublishesFitViewportFromRuntimeOutputSize() {
    DisplayStageFixture fixture;
    ViewportDisplayHarness& display = fixture.display;
    prepareHarness(fixture, 4, 3, 10, 9, 0);

    display.updateViewport();

    const DisplayViewport& viewport = display.displayViewport();
    assert(viewport.valid());
    assert(viewport.scaleMode == SCALE_MODE_FIT_WINDOW);
    assert(viewport.drawSize == PixelSize(10, 9));
    assert(viewport.destination == PixelRect(0, 0, 10, 9));
    assert(int(fixture.settings.zoom) == 0);
}

static void testCheckZoomPublishesFixedViewportAndOffsets() {
    DisplayStageFixture fixture;
    ViewportDisplayHarness& display = fixture.display;
    prepareHarness(fixture, 4, 3, 10, 9, 1);

    display.updateViewport();

    const DisplayViewport& viewport = display.displayViewport();
    assert(viewport.valid());
    assert(viewport.scaleMode == SCALE_MODE_FIXED_ZOOM);
    assert(viewport.drawSize == PixelSize(4, 3));
    assert(viewport.destination == PixelRect(3, 3, 4, 3));
    assert(viewport.screenOffsetBytes(bytes_per_line, bypp) == 126);
    assert(int(fixture.settings.zoom) == 1);
}

static void testCheckZoomReducesOversizedFixedZoom() {
    DisplayStageFixture fixture;
    ViewportDisplayHarness& display = fixture.display;
    prepareHarness(fixture, 8, 4, 10, 5, 2);

    display.updateViewport();

    const DisplayViewport& viewport = display.displayViewport();
    assert(viewport.zoomWasReduced());
    assert(viewport.requestedZoom == 2);
    assert(viewport.effectiveZoom == 1);
    assert(int(fixture.settings.zoom) == 1);
}

static void testCheckZoomMarksBorderClearWhenResizeMovesViewport() {
    DisplayStageFixture fixture;
    ViewportDisplayHarness& display = fixture.display;
    prepareHarness(fixture, 4, 3, 10, 9, 1);
    display.updateViewport();
    display.needsClear = 0;

    fixture.backend.outputSizeValue = PixelSize(12, 9);
    display.updateViewport();

    const DisplayViewport& viewport = display.displayViewport();
    assert(viewport.destination == PixelRect(4, 3, 4, 3));
    assert(display.needsClear == 1);
}

static void testClearBorderUsesViewportGeometryInsteadOfLegacyGlobals() {
    DisplayStageFixture fixture;
    ViewportDisplayHarness& display = fixture.display;
    RecordingDisplayDevice& device = fixture.device;
    DisplayViewport viewport;
    viewport.frameSize = PixelSize(4, 3);
    viewport.windowSize = PixelSize(20, 13);
    viewport.drawSize = PixelSize(8, 6);
    viewport.destination = PixelRect(6, 3, 8, 6);
    viewport.scaleMode = SCALE_MODE_FIXED_ZOOM;
    viewport.requestedZoom = 2;
    viewport.effectiveZoom = 2;

    display.setViewport(viewport);
    display.needsClear = 1;

    display.clearViewportBorder();

    assert(device.clearedRects.size() == 4);
    assert(device.clearedRects[0] == PixelRect(0, 0, 20, 3));
    assert(device.clearedRects[1] == PixelRect(0, 3, 6, 6));
    assert(device.clearedRects[2] == PixelRect(14, 3, 6, 6));
    assert(device.clearedRects[3] == PixelRect(0, 9, 20, 4));
}

int main() {
    testCheckZoomPublishesFitViewportFromRuntimeOutputSize();
    testCheckZoomPublishesFixedViewportAndOffsets();
    testCheckZoomReducesOversizedFixedZoom();
    testCheckZoomMarksBorderClearWhenResizeMovesViewport();
    testClearBorderUsesViewportGeometryInsteadOfLegacyGlobals();
    return 0;
}
