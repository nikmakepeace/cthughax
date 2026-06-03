#include "cthugha.h"
#include "CthughaDisplay.h"
#include "display.h"
#include "DisplayDevice.h"
#include "cth_buffer.h"
#include "disp-sys.h"
#include "imath.h"
#include "Interface.h"
#include "IndexedFrame.h"
#include "Screen.h"
#include "ViewportPresentation.h"

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

class GlobalPresentationScreenSelection : public PresentationScreenSelection {
public:
    virtual ScreenEntry* current() {
        return (ScreenEntry*)screen.current();
    }

    virtual void change(int by, int doSave) {
        screen.change(by, doSave);
    }
};

void newCthughaDisplay() { cthughaDisplay = new CthughaDisplayX11(); }

CthughaDisplayX11::CthughaDisplayX11()
    : CthughaDisplay()
    , expandedBuffer0(NULL)
    , expandedBufferByteCount(0)
    , expandedBufferBypp(0) {
}

CthughaDisplayX11::~CthughaDisplayX11() {
    if (expandedBuffer0 && expandedBuffer0 != buffer0)
        delete[] expandedBuffer0;
}

void CthughaDisplayX11::indexedBufferWillChange(unsigned char* oldBuffer) {
    if (expandedBuffer0 == oldBuffer) {
        expandedBuffer0 = NULL;
        expandedBufferByteCount = 0;
        expandedBufferBypp = 0;
    }
}

void CthughaDisplayX11::prepareExpandedBuffer() {
    // bypp is not known until DisplayDeviceX11::allocImage() has queried the
    // X server. Allocate this lazily so true-color visuals do not overwrite
    // the 8-bit indexed scratch buffer.
    if (bypp == 1) {
        if (expandedBuffer0 && (expandedBuffer0 != buffer0))
            delete[] expandedBuffer0;
        expandedBuffer0 = buffer0;
        expandedBufferByteCount = indexedDisplayFrameValue.capacityByteCount();
        expandedBufferBypp = bypp;
        return;
    }

    int byteCount = cthughaDisplay->indexedDisplayFrame().byteCount() * bypp;
    if ((expandedBuffer0 == NULL) || (expandedBuffer0 == buffer0) || (expandedBufferBypp != bypp)
        || (expandedBufferByteCount != byteCount)) {
        if (expandedBuffer0 && (expandedBuffer0 != buffer0))
            delete[] expandedBuffer0;
        expandedBuffer0 = new unsigned char[byteCount];
        expandedBufferByteCount = byteCount;
        expandedBufferBypp = bypp;
    }
}

/*
 * expand the palette
 */
void CthughaDisplayX11::expandPalette(int height) {
    if (height <= 0)
        return;

    if (height > visualBuffer().displayHeight())
        height = visualBuffer().displayHeight();

    unsigned char* dst = expandedBuffer;

    switch (draw_mode) {
    case DM_mapped1:
    case DM_tmp_mapped:
        for (int i = height; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = visualBuffer().displayWidth() / 4; j != 0; j--) {
                uint32_t b = *buff;
                buff++;
                uint32_t a;
                a = bitmap_colors0[b & 0xff];
                b >>= 8;
                a |= bitmap_colors1[b & 0xff];
                b >>= 8;
                a |= bitmap_colors2[b & 0xff];
                b >>= 8;
                a |= bitmap_colors3[b];
                *scrn = a;
                scrn++;
            }
            dst += expandedBufferWidth;
            buffer += cthughaDisplay->bufferWidth;
        }
        break;
    case DM_mapped2:
        for (int i = height; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = visualBuffer().displayWidth() / 4; j != 0; j--) {
                uint32_t b = *buff;
                buff++;
                uint32_t a;
                a = bitmap_colors0[b & 0xff];
                b >>= 8;
                a |= bitmap_colors1[b & 0xff];
                b >>= 8;
                *scrn = a;
                scrn++;
                a = bitmap_colors0[b & 0xff];
                b >>= 8;
                a |= bitmap_colors1[b];
                *scrn = a;
                scrn++;
            }
            dst += expandedBufferWidth;
            buffer += cthughaDisplay->bufferWidth;
        }
        break;
    case DM_mapped3:
        for (int i = height; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = visualBuffer().displayWidth() / 4; j != 0; j--) {
                uint32_t a, b;
                uint32_t c = *buff;
                buff++;
                a = bitmap_colors0[c & 0xff];
                c >>= 8;
                b = bitmap_colors0[c & 0xff];
                c >>= 8;
                *scrn = a | ((b & 0xff) << 24);
                scrn++;
                a = b >> 8;

                b = bitmap_colors0[c & 0xff];
                c >>= 8;
                *scrn = a | ((b & 0xffff) << 16);
                scrn++;
                a = b >> 16;

                b = bitmap_colors0[c];
                *scrn = a | (b << 8);
                scrn++;
            }
            dst += expandedBufferWidth;
            buffer += cthughaDisplay->bufferWidth;
        }
        break;
    case DM_mapped4:
        for (int i = height; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = visualBuffer().displayWidth() / 4; j != 0; j--) {
                uint32_t b = *buff;
                buff++;
                *scrn = bitmap_colors0[b & 0xff];
                b >>= 8;
                scrn++;
                *scrn = bitmap_colors0[b & 0xff];
                b >>= 8;
                scrn++;
                *scrn = bitmap_colors0[b & 0xff];
                b >>= 8;
                scrn++;
                *scrn = bitmap_colors0[b];
                scrn++;
            }
            dst += expandedBufferWidth;
            buffer += cthughaDisplay->bufferWidth;
        }
        break;
    case DM_direct:;
    }
}

/*
 * expand the palette
 */
void CthughaDisplayX11::expandPaletteMirrorHV() {
    unsigned char* dst = expandedBuffer;

    switch (draw_mode) {
    case DM_mapped1:
    case DM_tmp_mapped:
        for (int i = visualBuffer().height(); i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;

            uint32_t* buff = (uint32_t*)buffer;
            for (int j = visualBuffer().displayWidth() / 4; j != 0; j--) {
                uint32_t b = *buff;
                buff++;
                uint32_t a;
                a = bitmap_colors0[b & 0xff];
                b >>= 8;
                a |= bitmap_colors1[b & 0xff];
                b >>= 8;
                a |= bitmap_colors2[b & 0xff];
                b >>= 8;
                a |= bitmap_colors3[b];

                *scrn = a;
                scrn++;
            }
            dst += expandedBufferWidth;
            buffer += cthughaDisplay->bufferWidth;
        }
        break;
    case DM_mapped2:
        for (int i = visualBuffer().height(); i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = visualBuffer().displayWidth() / 4; j != 0; j--) {
                uint32_t b = *buff;
                buff++;
                uint32_t a;
                a = bitmap_colors0[b & 0xff];
                b >>= 8;
                a |= bitmap_colors1[b & 0xff];
                b >>= 8;
                *scrn = a;
                scrn++;
                a = bitmap_colors0[b & 0xff];
                b >>= 8;
                a |= bitmap_colors1[b];
                *scrn = a;
                scrn++;
            }
            dst += expandedBufferWidth;
            buffer += cthughaDisplay->bufferWidth;
        }
        break;
    case DM_mapped3:
        for (int i = visualBuffer().height(); i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = visualBuffer().displayWidth() / 4; j != 0; j--) {
                uint32_t a, b;
                uint32_t c = *buff;
                buff++;
                a = bitmap_colors0[c & 0xff];
                c >>= 8;
                b = bitmap_colors0[c & 0xff];
                c >>= 8;
                *scrn = a | ((b & 0xff) << 24);
                scrn++;
                a = b >> 8;

                b = bitmap_colors0[c & 0xff];
                c >>= 8;
                *scrn = a | ((b & 0xffff) << 16);
                scrn++;
                a = b >> 16;

                b = bitmap_colors0[c];
                *scrn = a | (b << 8);
                scrn++;
            }
            dst += expandedBufferWidth;
            buffer += cthughaDisplay->bufferWidth;
        }
        break;
    case DM_mapped4: {
        for (int i = 0; i < visualBuffer().height(); i++) {
            uint32_t* scrn1 = (uint32_t*)(expandedBuffer + i * expandedBufferWidth);
            uint32_t* scrn2 = scrn1 + visualBuffer().displayWidth() - 1;
            uint32_t* scrn3
                = (uint32_t*)(expandedBuffer + (visualBuffer().displayHeight() - i) * expandedBufferWidth);

            uint32_t* scrn4 = scrn3 + visualBuffer().displayWidth() - 1;

            uint32_t* buff = (uint32_t*)(buffer + i * cthughaDisplay->bufferWidth);

            for (int j = visualBuffer().displayWidth() / 4; j != 0; j--) {
                uint32_t b = *buff;
                buff++;

                uint32_t a = bitmap_colors0[b & 0xff];
                b >>= 8;
                *scrn1 = a;
                scrn1++;
                *scrn2 = a;
                scrn2--;
                *scrn3 = a;
                scrn3++;
                *scrn4 = a;
                scrn4--;

                a = bitmap_colors0[b & 0xff];
                b >>= 8;
                *scrn1 = a;
                scrn1++;
                *scrn2 = a;
                scrn2--;
                *scrn3 = a;
                scrn3++;
                *scrn4 = a;
                scrn4--;

                a = bitmap_colors0[b & 0xff];
                b >>= 8;
                *scrn1 = a;
                scrn1++;
                *scrn2 = a;
                scrn2--;
                *scrn3 = a;
                scrn3++;
                *scrn4 = a;
                scrn4--;

                a = bitmap_colors0[b];
                *scrn1 = a;
                scrn1++;
                *scrn2 = a;
                scrn2--;
                *scrn3 = a;
                scrn3++;
                *scrn4 = a;
                scrn4--;
            }
        }
    } break;
    case DM_direct:;
    }
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

    GlobalPresentationScreenSelection screenSelection;
    composePresentationFrame(screenSelection);
    if (traceDisplayTiming)
        displayTiming[2] = getTime();

    /*
     * prepare the display device
     */
    unsigned char* display_base = displayDevice->preDraw();
    prepareExpandedBuffer();
    if (traceDisplayTiming)
        displayTiming[3] = getTime();

    /*
     * The composer has already produced completed indexed pixels.  From this
     * point on X11 only converts, scales, decorates, and presents them.
     */
    buffer = buffer0;
    bufferWidth = indexedDisplayFrameValue.pitch();

    /*
     * Pick the buffer that post-processing reads from.  On 8-bit displays the
     * indexed buffer is already display-ready; otherwise expandPalette()
     * writes native pixels into expandedBuffer0.
     */
    if (bypp == 1) {
        expandedBuffer = buffer;
        expandedBufferWidth = bufferWidth;
    } else {
        expandedBuffer = expandedBuffer0;
        expandedBufferWidth = indexedDisplayFrameValue.pitch() * bypp;
    }

    checkZoom();

    /*
     * expand the palette (right now only the palette of buffer 0,
     *  but maybe later a different palette for each quadrant)
     */
    expandPalette(indexedDisplayFrameValue.height());
    if (traceDisplayTiming)
        displayTiming[4] = getTime();

#if 0
    expandPaletteMirrorHV();
#endif

    /*
     * clear the border around the image
     */
    clearBorder();

    /*
     * copy (with zoom) the image to the display
     */
    zoom2Screen(display_base
            + ViewportPresentation::drawOffsetBytes(displayViewport(), bytes_per_line, bypp),
        bytes_per_line, displayViewport());
    if (traceDisplayTiming)
        displayTiming[5] = getTime();

    /*
     * bring text to screen
     */
    displayDevice->prePrint();
    Interface::current->display(); // print the text of the current interface
    errors.display(); // and the error messages
    displayDevice->postPrint();
    if (traceDisplayTiming)
        displayTiming[6] = getTime();

    /*
     * make sure everything is really copied to the screen
     */
    displayDevice->postDraw();
    if (traceDisplayTiming) {
        displayTiming[7] = getTime();
        CTH_TRACE("x11 frame-ms=%.3f palette-ms=%.3f compose-ms=%.3f prepare-ms=%.3f expand-ms=%.3f zoom-clear-ms=%.3f text-ms=%.3f post-ms=%.3f draw-mode=%d bypp=%d size=%dx%d draw=%dx%d\n",
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
