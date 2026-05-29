// -*- c++ -*-
#ifndef __CTHUGHA_DISPLAY_H
#define __CTHUGHA_DISPLAY_H

#include "cthugha.h"
#include "CoreOption.h"

// The CthughaDisplay layer sits between the effect buffers and the selected
// DisplayDevice backend.  It owns the per-frame timing, temporary image
// buffers, mirroring/zooming, and the final handoff to the device.
//
// It must be initialized after displayDevice because it sizes and formats its
// buffers around the active device's pixel layout.

extern xy draw_size;
extern OptionInt zoom;
extern OptionInt maxFramesPerSecond;

extern double now; // timestamp used by all modules while drawing this frame
extern double deltaT; // elapsed time between the last two frames

class CthughaDisplay {
protected:
    // Scratch storage for indexed 8-bit Cthugha output before it is palette
    // expanded, mirrored, zoomed, or copied to the device buffer.
    unsigned char* buffer0;

    // FPS accounting starts at displayStart and counts completed frames.
    double displayStart;
    int frames;
    double visualLatencyEstimate;

    // Complete the logical 2x2 Cthugha image when a screen() function only
    // produced the left half, top half, or top-left quadrant.
    void mirrorHorizontally(int height);
    void mirrorVertically();

    // Keep stale text/old image data out of the letterboxed area.
    int clearBorder();

    // Copy expandedBuffer to the device memory, applying the configured zoom.
    void zoom2Screen(unsigned char*, int);

    // Backends override this when indexed pixels must be converted to the
    // device's native pixel format before zoom2Screen() runs.
    virtual void expandPalette(int) { }

    void checkFPS();
    void checkZoom();

public:
    // buffer points at either buffer0 or directly at the display memory in
    // DM_direct mode. bufferWidth is measured in bytes.
    unsigned char* buffer;
    int bufferWidth;

    // expandedBuffer is the palette-expanded image that zoom2Screen() reads.
    // On 8-bit backends it may be the same memory as buffer.
    unsigned char* expandedBuffer;
    int expandedBufferWidth;

    int needsClear; // border must be cleared before the next frame is shown

    CthughaDisplay();

    void nextFrame(); // start the next frame and publish now/deltaT
    virtual void operator()() { }

    void resetFPS();
    void observeVisualLatency(double seconds);
    double visualLatencySeconds() const;

    double fps; // most recently measured frames per second

    const char* status();

    friend int save_display();

    virtual ~CthughaDisplay() = 0;
};

//
// a special CthughaDisplay for X11
//
class CthughaDisplayX11 : public CthughaDisplay {
    unsigned char* expandedBuffer0;
    int expandedBufferBypp;
    virtual void expandPalette(int);
    virtual void expandPaletteMirrorHV();
    void prepareExpandedBuffer();

public:
    CthughaDisplayX11();
    virtual void operator()();
};

class ScreenEntry : public CoreOptionEntry {
public:
    int (*screen)();

    // Number of logical halves filled by screen(): {1,1} means the screen
    // routine draws only the upper-left quadrant and the display layer mirrors
    // it; {2,2} means it filled the whole Cthugha image.
    xy size;

    ScreenEntry(int (*f)(), const char* name, const char* desc, xy s, int inUse = 1)
        : CoreOptionEntry(name, desc, inUse)
        , screen(f)
        , size(s) { }

    int operator()() { return (*screen)(); }
};

extern CthughaDisplay* cthughaDisplay;

void newCthughaDisplay();

#endif
