// -*- c++ -*-

#ifndef __DISPLAY_DEVICE_H
#define __DISPLAY_DEVICE_H

#include "display.h"

//
//  Stuff about text-display
//
#define TEXT_COLOR_NORMAL 0
#define TEXT_COLOR_ERROR 1
#define TEXT_COLOR_HIGHLIGHT 2

extern int display_mode; // predefined graphics mode to use

extern int display_text_time; // how long text is kept on the screen

extern xy text_size;
extern xy fontSize;

extern int bypp; /* bytes per pixel */
extern int bytes_per_line;
extern xy disp_size; /* size of drawing area (window) */

extern xy screenSizes[]; // predefined sizes of the screen or window
extern int nScreenSizes;

extern xy bufferSizes[]; // and corresponding buffer sizes
extern int nBufferSizes;

enum draw_mode_t { DM_direct, DM_tmp_mapped, DM_mapped1, DM_mapped2, DM_mapped3, DM_mapped4 };
extern enum draw_mode_t draw_mode; /* how drawing is done */

class DisplayDevice {
protected:
    static int textColorRGB[][3];
    static int textColor[];
    static int textColors;

public:
    static int text_on_term; // text is drawn on terminal (with ncurses)

    int textOnScreen;
    int darkenPalette; // palette should be darkend
    int needsFullCopy; // Complete image with border must be copied

    DisplayDevice();
    virtual ~DisplayDevice();

    virtual void mainLoop() { }

    virtual int printScreen() { return 0; }

    virtual int setGlobalPalette();

    virtual unsigned char* preDraw() { return NULL; } // return buffer to display memory
    virtual void copyBox(int, int, int, int, int, int) { }
    virtual void clearBox(int, int, int, int) { }
    virtual void postDraw() { }

    virtual void prePrint();
    virtual double print(const char* text, /* what text to display */
        double y, /* line number */
        int justifcation, /* 'l', 'c', 'r' */
        int color, /* colorindex to use 255, 254, .. */
        int noDarken = 0); // do not darken the palette (e.g. fps display)
    virtual void printString(int, int, const char*, int, int, int);
    virtual void postPrint();
};

extern DisplayDevice* displayDevice;

extern void newDisplayDevice();

#endif
