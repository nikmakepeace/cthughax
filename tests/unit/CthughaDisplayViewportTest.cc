#include "CthughaDisplay.h"
#include "DisplayDevice.h"

#include <assert.h>
#include <stdarg.h>

DisplayDevice* displayDevice = 0;
int bypp = 2;
int bytes_per_line = 40;
xy disp_size(0, 0);
enum draw_mode_t draw_mode = DM_mapped2;

void DisplayDevice::setFramePalette(FramePalette* framePalette_) {
    framePalette = framePalette_;
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

    void updateViewport() {
        checkZoom();
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

int main() {
    testCheckZoomPublishesFitViewportAndLegacyDrawSize();
    testCheckZoomPublishesFixedViewportAndOffsets();
    testCheckZoomReducesOversizedFixedZoom();
    testCheckZoomMarksBorderClearWhenResizeMovesViewport();
    return 0;
}
