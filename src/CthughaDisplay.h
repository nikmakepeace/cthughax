// -*- c++ -*-
#ifndef __CTHUGHA_DISPLAY_H
#define __CTHUGHA_DISPLAY_H

#include "cthugha.h"
#include "DisplayGeometry.h"
#include "EffectControl.h"
#include "IndexedDisplayFrame.h"
#include "PresentationComposer.h"

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
extern double deltaT; // elapsed time between the last two frames, in seconds

class IndexedFrame;

class CthughaDisplay : public PresentationFrameObserver {
protected:
    const IndexedFrame* sourceFrame;
    IndexedDisplayFrame indexedDisplayFrameValue;
    PresentationComposer presentationComposer;
    DisplayViewport displayViewportValue;

    // Non-owning alias for indexedDisplayFrameValue.pixels(), retained while
    // classic screen functions still write through cthughaDisplay->buffer.
    unsigned char* buffer0;

    // FPS accounting starts at displayStart and counts completed frames.
    double displayStart;
    int frames;
    double visualLatencyEstimate;

    // Keep stale text/old image data out of the letterboxed area.
    int clearBorder();

    // Copy expandedBuffer to the device memory, applying the configured zoom.
    void zoom2Screen(unsigned char*, int);

    // Backends override this when indexed rows must be converted to the
    // device's native pixel format before completion/zooming run.
    virtual void expandPalette(int) { }

    /**
     * Gives a backend a chance to drop aliases before indexed pixels move.
     *
     * @param oldPixels Previous indexed display pixels. Backends must not free
     *        this pointer; CthughaDisplay still owns it.
     */
    virtual void indexedBufferWillChange(unsigned char*) { }

    virtual void indexedPixelsWillMove(unsigned char*);
    virtual void indexedFrameGeometryChanged();

    const IndexedDisplayFrame& composePresentationFrame(
        PresentationScreenSelection& screenSelection);

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

    /**
     * Starts a new visual frame and publishes now/deltaT.
     *
     * Application calls this before audio analysis and visual filters so every
     * frame subsystem observes the same visual clock.
     */
    void nextFrame();

    /**
     * Presents a filterchain-published indexed frame.
     *
     * @param frame Indexed pixels, dimensions, pitch, and palette to hand to
     *        the display backend.
     */
    void present(const IndexedFrame& frame);

    /** Legacy display path used when no IndexedFrame is available. */
    virtual void operator()() { }

    const unsigned char* sourcePixels() const;
    int sourceWidth() const;
    int sourceHeight() const;
    int sourcePitch() const;
    int sourceSize() const;
    const IndexedDisplayFrame& indexedDisplayFrame() const;
    const DisplayViewport& displayViewport() const;
    int displayFrameWidth() const;
    int displayFrameHeight() const;

    void resetFPS();

    /**
     * Adds a display presentation latency sample.
     *
     * @param seconds Time spent presenting the most recent visual frame.
     */
    void observeVisualLatency(double seconds);

    /** @return Smoothed display presentation latency in seconds. */
    double visualLatencySeconds() const;

    double fps; // most recently measured frames per second

    /** @return Human-readable FPS/status text for the interface overlay. */
    const char* status();

    friend int save_display();

    virtual ~CthughaDisplay() = 0;
};

//
// a special CthughaDisplay for X11
//
class CthughaDisplayX11 : public CthughaDisplay {
    unsigned char* expandedBuffer0;
    int expandedBufferByteCount;
    int expandedBufferBypp;
    virtual void expandPalette(int);
    virtual void expandPaletteMirrorHV();
    virtual void indexedBufferWillChange(unsigned char*);
    void prepareExpandedBuffer();

public:
    CthughaDisplayX11();
    virtual ~CthughaDisplayX11();
    virtual void operator()();
};

extern CthughaDisplay* cthughaDisplay;

/** Allocates the frontend-specific global CthughaDisplay instance. */
void newCthughaDisplay();

#endif
