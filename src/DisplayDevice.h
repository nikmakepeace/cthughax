// -*- c++ -*-

#ifndef __DISPLAY_DEVICE_H
#define __DISPLAY_DEVICE_H

#include "display.h"
#include "OverlaySource.h"

#include <memory>

class FramePalette;
class ImageOption;
class InputEventSink;
class RuntimeConfigRegistry;
class RuntimeCommandSink;
class RuntimeCommandTargetRouter;
class Scene;
class SceneVisualSelections;
class SecondsClock;
struct DisplayConfig;

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

enum draw_mode_t { DM_mapped1, DM_mapped2, DM_mapped3, DM_mapped4 };
extern enum draw_mode_t draw_mode; /* how drawing is done */

inline int mappedDrawModeForBytesPerPixel(int bytesPerPixel, draw_mode_t& mode) {
    switch (bytesPerPixel) {
    case 1:
        mode = DM_mapped1;
        return 1;
    case 2:
        mode = DM_mapped2;
        return 1;
    case 3:
        mode = DM_mapped3;
        return 1;
    case 4:
        mode = DM_mapped4;
        return 1;
    default:
        return 0;
    }
}

struct DisplayEventStats {
    /** Number of platform/window events processed in one loop iteration. */
    int eventCount;

    /** Number of resize/configure events processed. */
    int resizeEvents;

    /** Number of expose/redraw events processed. */
    int exposeEvents;

    DisplayEventStats()
        : eventCount(0)
        , resizeEvents(0)
        , exposeEvents(0) { }
};

class DisplayDevice {
protected:
    static int textColorRGB[][3];
    static int textColor[];
    static int textColors;

public:
    int textOnScreen;
    int darkenPalette; // palette should be darkend
    int needsFullCopy; // Complete image with border must be copied
    FramePalette* framePalette;

    DisplayDevice();
    virtual ~DisplayDevice();

    /**
     * Polls and handles platform/window events.
     *
     * Application calls this once per main-loop iteration before interface and
     * frame work so input, resize, and expose state are current.
     *
     * @return Counts used by trace logging.
     */
    virtual DisplayEventStats processEvents(InputEventSink&) { return DisplayEventStats(); }

    /**
     * Supplies the palette published by the active frame filterchain.
     *
     * @param framePalette_ Borrowed palette pointer, or NULL.
     */
    void setFramePalette(FramePalette* framePalette_);

    /** Uploads the current global/frame palette to the backend when needed. */
    virtual int setGlobalPalette();

    /** @return Writable display-memory buffer for direct/mapped draw modes. */
    virtual unsigned char* preDraw() { return NULL; } // return buffer to display memory
    virtual void copyBox(int, int, int, int, int, int) { }
    virtual void clearBox(int, int, int, int) { }
    /** Completes backend-specific drawing/presentation work for the frame. */
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

#endif
