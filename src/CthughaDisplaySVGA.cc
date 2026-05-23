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
#include <unistd.h>

void newCthughaDisplay() { cthughaDisplay = new CthughaDisplaySVGA(); }

CthughaDisplaySVGA::CthughaDisplaySVGA()
    : CthughaDisplay() { }

/*
 * expand the palette
 */
void CthughaDisplaySVGA::expandPalette(int narrow) {
    int height = narrow ? BUFF_HEIGHT : 2 * BUFF_HEIGHT;
    unsigned char* dst = expandedBuffer;

    if (draw_mode != DM_direct) {
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
    }
}

void CthughaDisplaySVGA::operator()() {

    /*
     * Sync the palette before indexed pixels are expanded or copied. Waiting
     * until postDraw() leaves the first frame after a palette change rendered
     * through stale bitmap colors.
     */
    displayDevice->setGlobalPalette();

    /*
     * prepare the display device
     */
    unsigned char* display_base = displayDevice->preDraw();

    /*
     * use right expandedBuffer
     */
    if (draw_mode == DM_direct) {
        buffer = display_base;
        bufferWidth = bytes_per_line;
    } else {
        buffer = buffer0;
        bufferWidth = 2 * BUFF_WIDTH;
    }

    expandedBuffer = buffer;
    expandedBufferWidth = bufferWidth;

    checkZoom();

    /*
     * bring Cthugha-Buffer(s) to Display-Buffer
     * - some screen() functions may fail because ratio of width to height
     *   is too strange
     */
    while (screen())
        ; /* draw the buffer */

    const xy& s = ((ScreenEntry*)screen.current())->size;

    /*
     * do horizontal mirroring, if necessary
     */
    if (s.x == 1)
        mirrorHorizontally(s.y * BUFF_HEIGHT);

    /*
     * expand the palette (right now only the palette of buffer 0,
     *  but maybe later a different palette for each quadrant)
     */
    expandPalette((s.y == 1) ? 1 : 0);

    /*
     * do veritical mirroring, if necessary
     */
    if (s.y == 1)
        mirrorVertically();

    /*
     * clear the border around the image
     */
    clearBorder();

    /*
     * copy (with zoom) the image to the display
     */
    zoom2Screen(display_base + SCREEN_OFFSET, bytes_per_line);

    /*
     * bring text to screen
     */
    displayDevice->prePrint();
    Interface::current->display(); // print the text of the current interface
    errors.display(); // and the error messages
    displayDevice->postPrint();

    /*
     * make sure everything is really copied to the screen
     */
    displayDevice->postDraw();
}
