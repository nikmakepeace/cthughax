#include "cthugha.h"
#include "DisplayDevice.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "imath.h"

#if HAVE_NCURSES == 1
#if HAVE_NCURSES_H
#include <ncurses.h>
#else
#if HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#else
#if HAVE_CURSES_H
#include <curses.h>
#else
#if HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#endif
#endif
#endif
#endif
#endif

DisplayDevice* displayDevice = NULL;

int display_mode = 0; /* predefined graphic mode to use */

int bypp = 1; /* bytes per pixel */
int bytes_per_line = 0;
xy disp_size(0, 0); /* size of drawing area */
enum draw_mode_t draw_mode = DM_direct; /* how drawing is done */

//
// prepare to set the global palette
//
int DisplayDevice::setGlobalPalette() {

    static int old_tos = 0;

    if ((textOnScreen != old_tos)) { // text came on screen, or text left screen
        old_tos = textOnScreen;
        return 1;
    }

    return CthughaBuffer::buffers[0].palChanged;
}

DisplayDevice::DisplayDevice()
    : textOnScreen(0)
    , darkenPalette(0)
    , needsFullCopy(1) { }
DisplayDevice::~DisplayDevice() { }

//
// text
//
/*
 * some important parts written by:
 *  Mark Vojkovich (mvojkovi@ucsd.edu)
 */

int DisplayDevice::textColorRGB[][3] = {
    { 192, 192, 192 }, /* normal */
    { 255, 0, 0 }, /* error */
    { 255, 255, 255 } /* highlight */
};
int DisplayDevice::textColor[3];
int DisplayDevice::textColors = 3;

int DisplayDevice::text_on_term = 0;

xy text_size(0, 0);

xy fontSize(8, 8);

void DisplayDevice::prePrint() {

    textOnScreen = 0;
#if HAVE_NCURSES == 1
    if (text_on_term)
        erase();
#endif
}

void DisplayDevice::postPrint() {

#if HAVE_NCURSES == 1
    if (text_on_term)
        refresh();
#endif
}

void DisplayDevice::printString(
    int /*x*/, int /*y*/, const char* /*tex*/, int /*color*/, int /*len*/, int /*noDarken */) { }

double DisplayDevice::print(const char* text, double y, int justify, int color, int noDarken) {

    if (text_size.x == 0) {
        if (text_on_term) {
#if HAVE_NCURSES == 1
            text_size.x = COLS;
            text_size.y = LINES;
#else
            text_size.x = disp_size.x / fontSize.x;
            text_size.y = disp_size.y / fontSize.y;
#endif
        } else {
            text_size.x = disp_size.x / fontSize.x;
            text_size.y = disp_size.y / fontSize.y;
        }
    }

    const char* lineStart = text;
    const char* lineEnd;
    do {
        lineEnd = strchr(lineStart, '\n');
        int len = lineEnd ? lineEnd - lineStart : strlen(lineStart);
        int x;

        if (len == 0)
            return y;

        switch (justify) {
        case 'l':
            x = 0;
            break;
        case 'c':
            x = max(fontSize.x * (text_size.x - len) / 2, 0);
            break;
        case 'r':
            x = max(fontSize.x * (text_size.x - len), 0);
            break;
        default:
            CTH_ERROR("unknown justification (internal error).\n");
            x = 0;
        }

        if (text_on_term) {
#if HAVE_NCURSES == 1
            attrset(color ? A_BOLD : A_NORMAL);
            mvaddnstr(
                (y >= 0) ? int(y) : int(text_size.y + y), x / fontSize.x, (char*)lineStart, len);
#else
            printString(x, (y >= 0) ? int(y * fontSize.y) : int(fontSize.y * (text_size.y + y)),
                lineStart, color, len, noDarken);
#endif
        } else {
            printString(x, (y >= 0) ? int(y * fontSize.y) : int(fontSize.y * (text_size.y + y)),
                lineStart, color, len, noDarken);
        }

        lineStart = lineEnd + 1; // go to next line
        y += 1;
    } while (lineEnd);

    return y;
}
