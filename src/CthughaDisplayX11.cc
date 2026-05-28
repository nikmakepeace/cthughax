#include "cthugha.h"
#include "CthughaDisplay.h"
#include "CthughaBuffer.h"
#include "display.h"
#include "DisplayDevice.h"
#include "cth_buffer.h"
#include "disp-sys.h"
#include "imath.h"
#include "Interface.h"

#include <stdint.h>

void newCthughaDisplay() { cthughaDisplay = new CthughaDisplayX11(); }

CthughaDisplayX11::CthughaDisplayX11()
    : CthughaDisplay()
    , expandedBuffer0(NULL)
    , expandedBufferBypp(0) {
}

void CthughaDisplayX11::prepareExpandedBuffer() {
    // bypp is not known until DisplayDeviceX11::allocImage() has queried the
    // X server. Allocate this lazily so true-color visuals do not overwrite
    // the 8-bit indexed scratch buffer.
    if (bypp == 1) {
        if (expandedBuffer0 && (expandedBuffer0 != buffer0))
            delete[] expandedBuffer0;
        expandedBuffer0 = buffer0;
        expandedBufferBypp = bypp;
        return;
    }

    if ((expandedBuffer0 == NULL) || (expandedBuffer0 == buffer0) || (expandedBufferBypp != bypp)) {
        if (expandedBuffer0 && (expandedBuffer0 != buffer0))
            delete[] expandedBuffer0;
        expandedBuffer0 = new unsigned char[4 * BUFF_SIZE * bypp];
        expandedBufferBypp = bypp;
    }
}

/*
 * expand the palette
 */
void CthughaDisplayX11::expandPalette(int narrow) {
    int height = narrow ? BUFF_HEIGHT : 2 * BUFF_HEIGHT;
    unsigned char* dst = expandedBuffer;

    switch (draw_mode) {
    case DM_mapped1:
    case DM_tmp_mapped:
        for (int i = height; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = 2 * BUFF_WIDTH / 4; j != 0; j--) {
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
            buffer += 2 * BUFF_WIDTH;
        }
        break;
    case DM_mapped2:
        for (int i = height; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = 2 * BUFF_WIDTH / 4; j != 0; j--) {
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
            buffer += 2 * BUFF_WIDTH;
        }
        break;
    case DM_mapped3:
        for (int i = height; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = 2 * BUFF_WIDTH / 4; j != 0; j--) {
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
            buffer += 2 * BUFF_WIDTH;
        }
        break;
    case DM_mapped4:
        for (int i = height; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = 2 * BUFF_WIDTH / 4; j != 0; j--) {
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
            buffer += 2 * BUFF_WIDTH;
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
        for (int i = BUFF_HEIGHT; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;

            uint32_t* buff = (uint32_t*)buffer;
            for (int j = 2 * BUFF_WIDTH / 4; j != 0; j--) {
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
            buffer += 2 * BUFF_WIDTH;
        }
        break;
    case DM_mapped2:
        for (int i = BUFF_HEIGHT; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = 2 * BUFF_WIDTH / 4; j != 0; j--) {
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
            buffer += 2 * BUFF_WIDTH;
        }
        break;
    case DM_mapped3:
        for (int i = BUFF_HEIGHT; i != 0; i--) {
            uint32_t* scrn = (uint32_t*)dst;
            uint32_t* buff = (uint32_t*)buffer;
            for (int j = 2 * BUFF_WIDTH / 4; j != 0; j--) {
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
            buffer += 2 * BUFF_WIDTH;
        }
        break;
    case DM_mapped4: {
        for (int i = 0; i < BUFF_HEIGHT; i++) {
            uint32_t* scrn1 = (uint32_t*)(expandedBuffer + i * expandedBufferWidth);
            uint32_t* scrn2 = scrn1 + 2 * BUFF_WIDTH - 1;
            uint32_t* scrn3
                = (uint32_t*)(expandedBuffer + (2 * BUFF_HEIGHT - i) * expandedBufferWidth);

            uint32_t* scrn4 = scrn3 + 2 * BUFF_WIDTH - 1;

            uint32_t* buff = (uint32_t*)(buffer + i * 2 * BUFF_WIDTH);

            for (int j = 2 * BUFF_WIDTH / 4; j != 0; j--) {
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

    /*
     * prepare the display device
     */
    unsigned char* display_base = displayDevice->preDraw();
    prepareExpandedBuffer();
    if (traceDisplayTiming)
        displayTiming[2] = getTime();

    /*
     * Choose the buffer that screen() should draw into.  Direct mode draws
     * straight to device memory; mapped modes render indexed pixels into a
     * scratch buffer and expand them afterwards.
     */
    if (draw_mode == DM_direct) {
        buffer = display_base;
        bufferWidth = bytes_per_line;
    } else {
        buffer = buffer0;
        bufferWidth = 2 * BUFF_WIDTH;
    }

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
        expandedBufferWidth = 2 * BUFF_WIDTH * bypp;
    }

    checkZoom();

    /*
     * Run the selected Cthugha screen function until it accepts the current
     * geometry.  Some display functions ask to retry when the aspect ratio is
     * too awkward for their mapping.
     */
    while (screen())
        ; /* draw the buffer */
    if (traceDisplayTiming)
        displayTiming[3] = getTime();

    const xy& s = ((ScreenEntry*)screen.current())->size;

    /*
     * If the selected screen function drew only half or a quadrant, mirror the
     * missing pieces before the image is copied to the display device.
     */
    if (s.x == 1)
        mirrorHorizontally(s.y * BUFF_HEIGHT);

    /*
     * expand the palette (right now only the palette of buffer 0,
     *  but maybe later a different palette for each quadrant)
     */
    expandPalette((s.y == 1) ? 1 : 0);
    if (traceDisplayTiming)
        displayTiming[4] = getTime();

    /*
     * do veritical mirroring, if necessary
     */
    if (s.y == 1)
        mirrorVertically();
    if (traceDisplayTiming)
        displayTiming[5] = getTime();

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
    zoom2Screen(display_base + SCREEN_OFFSET, bytes_per_line);
    if (traceDisplayTiming)
        displayTiming[6] = getTime();

    /*
     * bring text to screen
     */
    displayDevice->prePrint();
    Interface::current->display(); // print the text of the current interface
    errors.display(); // and the error messages
    displayDevice->postPrint();
    if (traceDisplayTiming)
        displayTiming[7] = getTime();

    /*
     * make sure everything is really copied to the screen
     */
    displayDevice->postDraw();
    if (traceDisplayTiming) {
        displayTiming[8] = getTime();
        CTH_TRACE("x11 frame-ms=%.3f palette-ms=%.3f prepare-ms=%.3f screen-ms=%.3f expand-ms=%.3f mirror-v-ms=%.3f zoom-clear-ms=%.3f text-ms=%.3f post-ms=%.3f draw-mode=%d bypp=%d size=%dx%d draw=%dx%d\n",
            "display timing",
            (displayTiming[8] - displayTiming[0]) * 1000.0,
            (displayTiming[1] - displayTiming[0]) * 1000.0,
            (displayTiming[2] - displayTiming[1]) * 1000.0,
            (displayTiming[3] - displayTiming[2]) * 1000.0,
            (displayTiming[4] - displayTiming[3]) * 1000.0,
            (displayTiming[5] - displayTiming[4]) * 1000.0,
            (displayTiming[6] - displayTiming[5]) * 1000.0,
            (displayTiming[7] - displayTiming[6]) * 1000.0,
            (displayTiming[8] - displayTiming[7]) * 1000.0,
            draw_mode, bypp, disp_size.x, disp_size.y, draw_size.x, draw_size.y);
    }
}
