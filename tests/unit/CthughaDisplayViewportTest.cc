#include "CthughaDisplay.h"
#include "DisplayDevice.h"

#include <assert.h>
#include <stdarg.h>
#include <vector>

DisplayDevice* displayDevice = 0;
int bypp = 2;
int bytes_per_line = 40;
xy disp_size(0, 0);
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

double getTime() {
    return 100.0;
}

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

class ViewportDisplayHarness : public CthughaDisplay {
public:
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

    void copyZoomed(unsigned char* destination, int pitch) {
        zoom2Screen(destination, pitch);
    }
};

class RecordingDisplayDevice : public DisplayDevice {
public:
    std::vector<PixelRect> clearedRects;

    virtual void clearBox(int x, int y, int width, int height) {
        clearedRects.push_back(PixelRect(x, y, width, height));
    }
};

static void prepareHarness(ViewportDisplayHarness& display, int frameWidth,
    int frameHeight, int windowWidth, int windowHeight, int zoomValue) {
    cthughaDisplay = &display;
    display.setDisplayFrameSize(frameWidth, frameHeight);
    disp_size = xy(windowWidth, windowHeight);
    draw_size = xy(-1, -1);
    zoom.setValue(zoomValue);
    display.needsClear = 0;
}

static void testCheckZoomPublishesFitViewportAndLegacyDrawSize() {
    ViewportDisplayHarness display;
    prepareHarness(display, 4, 3, 10, 9, 0);

    display.updateViewport();

    const DisplayViewport& viewport = display.displayViewport();
    assert(viewport.valid());
    assert(viewport.scaleMode == SCALE_MODE_FIT_WINDOW);
    assert(viewport.drawSize == PixelSize(10, 9));
    assert(viewport.destination == PixelRect(0, 0, 10, 9));
    assert(draw_size.x == 10);
    assert(draw_size.y == 9);
    assert(int(zoom) == 0);
}

static void testCheckZoomPublishesFixedViewportAndOffsets() {
    ViewportDisplayHarness display;
    prepareHarness(display, 4, 3, 10, 9, 1);

    display.updateViewport();

    const DisplayViewport& viewport = display.displayViewport();
    assert(viewport.valid());
    assert(viewport.scaleMode == SCALE_MODE_FIXED_ZOOM);
    assert(viewport.drawSize == PixelSize(4, 3));
    assert(viewport.destination == PixelRect(3, 3, 4, 3));
    assert(viewport.screenOffsetBytes(bytes_per_line, bypp) == 126);
    assert(draw_size.x == 4);
    assert(draw_size.y == 3);
    assert(int(zoom) == 1);
}

static void testCheckZoomReducesOversizedFixedZoom() {
    ViewportDisplayHarness display;
    prepareHarness(display, 8, 4, 10, 5, 2);

    display.updateViewport();

    const DisplayViewport& viewport = display.displayViewport();
    assert(viewport.zoomWasReduced());
    assert(viewport.requestedZoom == 2);
    assert(viewport.effectiveZoom == 1);
    assert(draw_size.x == 8);
    assert(draw_size.y == 4);
    assert(int(zoom) == 1);
}

static void testCheckZoomMarksBorderClearWhenResizeMovesViewport() {
    ViewportDisplayHarness display;
    prepareHarness(display, 4, 3, 10, 9, 1);
    display.updateViewport();
    display.needsClear = 0;

    disp_size = xy(12, 9);
    display.updateViewport();

    const DisplayViewport& viewport = display.displayViewport();
    assert(viewport.destination == PixelRect(4, 3, 4, 3));
    assert(display.needsClear == 1);
}

static void testClearBorderUsesViewportGeometryInsteadOfLegacyGlobals() {
    ViewportDisplayHarness display;
    RecordingDisplayDevice device;
    DisplayViewport viewport;
    viewport.frameSize = PixelSize(4, 3);
    viewport.windowSize = PixelSize(20, 13);
    viewport.drawSize = PixelSize(8, 6);
    viewport.destination = PixelRect(6, 3, 8, 6);
    viewport.scaleMode = SCALE_MODE_FIXED_ZOOM;
    viewport.requestedZoom = 2;
    viewport.effectiveZoom = 2;

    displayDevice = &device;
    cthughaDisplay = &display;
    display.setViewport(viewport);
    display.needsClear = 1;

    disp_size = xy(99, 88);
    draw_size = xy(1, 1);

    display.clearViewportBorder();

    assert(device.clearedRects.size() == 4);
    assert(device.clearedRects[0] == PixelRect(0, 0, 20, 3));
    assert(device.clearedRects[1] == PixelRect(0, 3, 6, 6));
    assert(device.clearedRects[2] == PixelRect(14, 3, 6, 6));
    assert(device.clearedRects[3] == PixelRect(0, 9, 20, 4));
}

static void testFitZoomUsesViewportDrawSizeInsteadOfLegacyDisplaySize() {
    ViewportDisplayHarness display;
    unsigned char expanded[] = {
        10, 20,
        30, 40
    };
    unsigned char output[] = {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };
    DisplayViewport viewport;
    viewport.frameSize = PixelSize(2, 2);
    viewport.windowSize = PixelSize(4, 4);
    viewport.drawSize = PixelSize(4, 4);
    viewport.destination = PixelRect(0, 0, 4, 4);
    viewport.scaleMode = SCALE_MODE_FIT_WINDOW;
    viewport.requestedZoom = 0;
    viewport.effectiveZoom = 0;

    cthughaDisplay = &display;
    display.setDisplayFrameSize(2, 2);
    display.setViewport(viewport);
    display.expandedBuffer = expanded;
    display.expandedBufferWidth = 2;
    bypp = 1;
    zoom.setValue(0);

    disp_size = xy(2, 2);
    draw_size = xy(2, 2);

    display.copyZoomed(output, 4);

    assert(output[0] == 10);
    assert(output[1] == 10);
    assert(output[2] == 20);
    assert(output[3] == 20);
    assert(output[4] == 10);
    assert(output[5] == 10);
    assert(output[6] == 20);
    assert(output[7] == 20);
    assert(output[8] == 30);
    assert(output[9] == 30);
    assert(output[10] == 40);
    assert(output[11] == 40);
    assert(output[12] == 30);
    assert(output[13] == 30);
    assert(output[14] == 40);
    assert(output[15] == 40);
}

int main() {
    testCheckZoomPublishesFitViewportAndLegacyDrawSize();
    testCheckZoomPublishesFixedViewportAndOffsets();
    testCheckZoomReducesOversizedFixedZoom();
    testCheckZoomMarksBorderClearWhenResizeMovesViewport();
    testClearBorderUsesViewportGeometryInsteadOfLegacyGlobals();
    testFitZoomUsesViewportDrawSizeInsteadOfLegacyDisplaySize();
    return 0;
}
