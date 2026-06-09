#include "cthugha.h"
#include "DisplayDevice.h"
#include "CthughaDisplay.h"
#include "Configuration.h"
#include "FramePalette.h"
#include "imath.h"

int bypp = 1; /* bytes per pixel */
int bytes_per_line = 0;
xy disp_size(0, 0); /* size of drawing area */
enum draw_mode_t draw_mode = DM_mapped1; /* how drawing is done */

//
// prepare to set the global palette
//
int DisplayDevice::setGlobalPalette() {

    static int old_tos = 0;

    if ((textOnScreen != old_tos)) { // text came on screen, or text left screen
        old_tos = textOnScreen;
        return 1;
    }

    return (framePalette != 0) ? framePalette->paletteDirty() : 1;
}

DisplayDevice::DisplayDevice()
    : textOnScreen(0)
    , darkenPalette(0)
    , needsFullCopy(1)
    , framePalette(0) { }
DisplayDevice::~DisplayDevice() { }

void DisplayDevice::setFramePalette(FramePalette* framePalette_) {
    framePalette = framePalette_;
}

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

xy text_size(0, 0);

xy fontSize(8, 8);

void DisplayDevice::prePrint() {

    textOnScreen = 0;
}

void DisplayDevice::postPrint() { }

void DisplayDevice::printString(
    int /*x*/, int /*y*/, const char* /*tex*/, int /*color*/, int /*len*/, int /*noDarken */) { }

double DisplayDevice::print(const char* text, double y, int justify, int color, int noDarken) {

    if (text_size.x == 0) {
        text_size.x = disp_size.x / fontSize.x;
        text_size.y = disp_size.y / fontSize.y;
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

        printString(x, (y >= 0) ? int(y * fontSize.y) : int(fontSize.y * (text_size.y + y)),
            lineStart, color, len, noDarken);

        lineStart = lineEnd + 1; // go to next line
        y += 1;
    } while (lineEnd);

    return y;
}
