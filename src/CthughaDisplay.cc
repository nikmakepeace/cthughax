#include "cthugha.h"
#include "CthughaDisplay.h"
#include "display.h"
#include "DisplayDevice.h"
#include "cth_buffer.h"
#include "disp-sys.h"
#include "imath.h"
#include "Interface.h"

#include <unistd.h>

CthughaDisplay* cthughaDisplay = NULL;

CthughaDisplay::~CthughaDisplay() { }

OptionInt maxFramesPerSecond("maxFPS", 25);

OptionInt zoom("zoom", 0, 3);
xy draw_size(0, 0); /* size of the drawn image (including zoom) */

double displayStart;

double now = 0;
double deltaT = 0;

CthughaDisplay::CthughaDisplay()
    : needsClear(1)
    , fps(0) {

    frames = 0;
    displayStart = getTime();

    buffer0 = new unsigned char[4 * BUFF_SIZE];
}

/*
 * do the horizontal mirroring
 * this is done in the buffer
 */
void CthughaDisplay::mirrorHorizontally() {
    unsigned char* src = buffer;
    unsigned char* dst = buffer + (2 * BUFF_WIDTH - 1);

    for (int i = BUFF_HEIGHT; i != 0; i--) {
        for (int j = BUFF_WIDTH; j != 0; j--) {
            *dst = *src;
            src++;
            dst--;
        }
        src += bufferWidth - BUFF_WIDTH;
        dst += bufferWidth + BUFF_WIDTH;
    }
}

/*
 * do the vertical mirroring
 * this is done in the expandedBuffer
 */
void CthughaDisplay::mirrorVertically() {
    unsigned char* src = expandedBuffer;
    unsigned char* dst = expandedBuffer + (2 * BUFF_HEIGHT - 1) * expandedBufferWidth;

    for (int i = BUFF_HEIGHT; i != 0; i--) {
        memcpy(dst, src, 2 * BUFF_WIDTH * bypp);
        src += expandedBufferWidth;
        dst -= expandedBufferWidth;
    }
}

/*
 * clear the border around the image on the screen
 */
int CthughaDisplay::clearBorder() {

    if (displayDevice->textOnScreen || needsClear) {
        int bwidth = SCREEN_OFFSET_X; /* width of left/right border */
        int bheight = SCREEN_OFFSET_Y; /* height of upper/lower border */

        /* upper border */
        displayDevice->clearBox(0, 0, disp_size.x, bheight);
        /* left border */
        displayDevice->clearBox(0, bheight, bwidth, draw_size.y);
        /* right border */
        displayDevice->clearBox(disp_size.x - bwidth, bheight, bwidth, draw_size.y);
        /* lower border */
        displayDevice->clearBox(0, disp_size.y - bheight, disp_size.x, bheight);

        /* if now text is on screen, we have to clean in the next iteration,
           (maybe the text gets removed */
        needsClear = (displayDevice->textOnScreen) ? 1 : 0;

        displayDevice->needsFullCopy = 1;
    }
    return 0;
}

/*
 * copy the expandedBuffer to the screen
 */
void CthughaDisplay::zoom2Screen(unsigned char* scrn, int bytesPerLine) {
    unsigned char* b1 = expandedBuffer;
    unsigned short* b2 = (unsigned short*)(expandedBuffer);
    unsigned long* b4 = (unsigned long*)(expandedBuffer);

    unsigned char* s1 = (unsigned char*)(scrn);
    unsigned short* s2 = (unsigned short*)(scrn);
    unsigned long* s4 = (unsigned long*)(scrn);

    int bpl2 = bytesPerLine / 2;
    int bpl4 = bytesPerLine / 4;

    if ((zoom != 1) && (disp_size.x == 2 * BUFF_WIDTH) && (disp_size.y == 2 * BUFF_HEIGHT)) {
        CTH_INFO("Display size matches buffer size. setting zoom to 1\n");
        zoom.setValue(1);
    }

    if ((zoom != 1) && (bypp != 1) && (bypp != 2) && (bypp != 4)) {
        CTH_ERROR("Sorry zooming is only implements for 1,2 and 4 bytes/pixel displays.");
        zoom.setValue(1);
    }

    switch (zoom) {
    case 0: {
        //
        // zoom 0: zoom, so that the image fits the screen
        //
        int dx = int(65536.0 * double(2 * BUFF_WIDTH) / double(disp_size.x));
        double dy = double(2 * BUFF_HEIGHT) / double(disp_size.y);

        switch (bypp) {
        case 1:
            for (int i = 0; i < disp_size.y; i++) {
                b1 = (unsigned char*)(expandedBuffer + int(dy * double(i)) * expandedBufferWidth);
                int x = 0;

                for (int j = disp_size.x; j != 0; j--) {
                    *s1 = *(b1 + (x >> 16));
                    s1++;
                    x += dx;
                }
                s1 = s1 + bytesPerLine - disp_size.x;
            }
            break;
        case 2:
            for (int i = 0; i < disp_size.y; i++) {
                b2 = (unsigned short*)(expandedBuffer + int(dy * double(i)) * expandedBufferWidth);
                int x = 0;

                for (int j = disp_size.x; j != 0; j--) {
                    *s2 = *(b2 + (x >> 16));
                    s2++;
                    x += dx;
                }
                s2 = s2 + bpl2 - disp_size.x;
            }
            break;
        case 4:
            for (int i = 0; i < disp_size.y; i++) {
                b4 = (unsigned long*)(expandedBuffer + int(dy * double(i)) * expandedBufferWidth);
                int x = 0;

                for (int j = disp_size.x; j != 0; j--) {
                    *s4 = *(b4 + (x >> 16));
                    s4++;
                    x += dx;
                }
                s4 = s4 + bpl4 - disp_size.x;
            }
            break;
        }
        break;
    }
    case 1:
        //
        // no zooming, just copy
        // this works for all bypp
        //
        for (int i = 2 * BUFF_HEIGHT; i != 0; i--) {
            memcpy(scrn, b1, 2 * BUFF_WIDTH * bypp);
            scrn += bytesPerLine;
            b1 += expandedBufferWidth;
        }
        break;
    case 2:
        //
        // zoom by a factor of 2
        //
        switch (bypp) {
        case 1:
            for (int i = 2 * BUFF_HEIGHT; i != 0; i--) {
                for (int j = 2 * BUFF_WIDTH; j != 0; j--) {
                    unsigned short s = (*b1) | ((unsigned short)(*b1) << 8);
                    *s2 = s;
                    *(s2 + bpl2) = s;
                    s2++;
                    b1++;
                }
                s2 += bpl2 + bpl2 - 2 * BUFF_WIDTH;
                b1 += expandedBufferWidth - 2 * BUFF_WIDTH;
            }
            break;
        case 2:
            for (int i = 2 * BUFF_HEIGHT; i != 0; i--) {
                for (int j = 2 * BUFF_WIDTH; j != 0; j--) {
                    unsigned long s = (*b2) | ((*b2) << 16);
                    *s4 = s;
                    *(s4 + bpl4) = s;
                    s4++;
                    b2++;
                }
                s4 += bpl4 + bpl4 - 2 * BUFF_WIDTH;
                b2 += expandedBufferWidth / 2 - 2 * BUFF_WIDTH;
            }
            break;
        case 4:
            for (int i = 2 * BUFF_HEIGHT; i != 0; i--) {
                for (int j = 2 * BUFF_WIDTH; j != 0; j--) {
                    unsigned long s = *b4;
                    *s4 = s;
                    *(s4 + 1) = s;
                    *(s4 + bpl4) = s;
                    *(s4 + bpl4 + 1) = s;
                    s4 += 2;
                    b4++;
                }
                s4 += bpl4 + bpl4 - 4 * BUFF_WIDTH;
                b4 += expandedBufferWidth / 4 - 2 * BUFF_WIDTH;
            }
            break;
        }
        break;
    }
}

void CthughaDisplay::checkZoom() {
    /*
     * compute the size of the drawing area, and check for sane zoom value
     */
    if (zoom == 0) { /* use optimal size */
        draw_size = disp_size;
    } else { /* use a specified scaling factor */
        static int oldZoom = -1;
        if (zoom != oldZoom) {
            oldZoom = zoom;
            needsClear = 1;
        }

        draw_size.x = zoom * 2 * BUFF_WIDTH;
        draw_size.y = zoom * 2 * BUFF_HEIGHT;
        while ((draw_size.x > disp_size.x) || (draw_size.y > disp_size.y)) {
            CTH_ERROR("Zoom factor is set too high for current display size. reducing.\n");
            zoom.setValue(zoom / 2);
            draw_size.x = zoom * 2 * BUFF_WIDTH;
            draw_size.y = zoom * 2 * BUFF_HEIGHT;
        }
    }
}

void CthughaDisplay::checkFPS() {
    /*
     * compute frames/second, of at least 3 frames or 0.1 second
     */
    double i = now - displayStart;
    if ((i > 0.1) && (frames > 2))
        fps = double(frames) / i;
    frames++;

    // check for maximum frames/sec
    if (maxFramesPerSecond) {
        double delta = (1.0 / maxFramesPerSecond) - deltaT;
        if (delta > 0) {
            usleep(int(delta * 1e6));
        }
    }
}

void CthughaDisplay::resetFPS() {
    displayStart = getTime(); // now;
    frames = 0;
}

//
// start a new frame
//
// get time for 'now'
// check for FPS limit
//
void CthughaDisplay::nextFrame() {

    // new position in time
    double nower = getTime();
    deltaT = nower - now;
    now = nower;

    checkFPS();
}

const char* CthughaDisplay::status() {
    static char txt[512];

    sprintf(txt, "fps: %5.2f ", fps);
    return txt;
}
