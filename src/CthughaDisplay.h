// -*- c++ -*-
#ifndef __CTHUGHA_DISPLAY_H
#define __CTHUGHA_DISPLAY_H

#include "cthugha.h"
#include "DisplayGeometry.h"
#include "EffectControl.h"
#include "IndexedDisplayFrame.h"
#include "PresentationComposer.h"

#include <memory>

// The CthughaDisplay layer sits between the effect buffers and the selected
// DisplayDevice backend.  It owns the per-frame timing, temporary image
// buffers, mirroring/zooming, and the final handoff to the device.
//
// It is initialized with the owned display stage so viewport and palette work
// do not need to read transitional globals.

extern OptionInt zoom;
extern OptionInt maxFramesPerSecond;
extern OptionOnOff showFPS;

extern double now; // timestamp used by all modules while drawing this frame
extern double deltaT; // elapsed time between the last two frames, in seconds

class IndexedFrame;
class DisplayDevice;
class DisplayRuntime;
class VideoFrameContext;
struct DisplayConfig;

class CthughaDisplay : public PresentationFrameObserver {
protected:
    DisplayDevice& deviceValue;
    DisplayRuntime& runtimeValue;
    const IndexedFrame* sourceFrame;
    const VideoFrameContext* presentationContextValue;
    IndexedDisplayFrame indexedDisplayFrameValue;
    PresentationComposer presentationComposer;
    DisplayViewport displayViewportValue;

    // Non-owning alias for indexedDisplayFrameValue.pixels(), retained for
    // subclasses that publish the composed frame to backend presentation.
    unsigned char* buffer0;

    double visualLatencyEstimate;

    // Keep stale text/old image data out of the letterboxed area.
    int clearBorder();

    virtual void indexedPixelsWillMove(unsigned char*);
    virtual void indexedFrameGeometryChanged();

    const IndexedDisplayFrame& composePresentationFrame(
        PresentationScreenSelection& screenSelection);
    const IndexedDisplayFrame& composePresentationFrame();
    void presentCurrentWithContext(const VideoFrameContext* context);

    void updateFPS();
    void checkZoom();
    DisplayDevice& device();
    DisplayRuntime& runtime();

public:
    // buffer points at the indexed presentation buffer. bufferWidth is measured
    // in bytes.
    unsigned char* buffer;
    int bufferWidth;

    int needsClear; // border must be cleared before the next frame is shown

    CthughaDisplay(DisplayDevice& device, DisplayRuntime& runtime);

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

    /**
     * Presents a filterchain-published indexed frame with audio context.
     *
     * @param frame Indexed pixels, dimensions, pitch, and palette to hand to
     *        the display backend.
     * @param context Borrowed per-frame audio/timing context.
     */
    void present(const IndexedFrame& frame, const VideoFrameContext& context);

    /**
     * Presents the current source frame or legacy buffer with audio context.
     *
     * This keeps the no-argument virtual display entry point while allowing the
     * application frame loop to supply explicit audio state.
     *
     * @param context Borrowed per-frame audio/timing context.
     */
    void presentCurrent(const VideoFrameContext& context);

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

    /**
     * Adds a display presentation latency sample.
     *
     * @param seconds Time spent presenting the most recent visual frame.
     */
    void observeVisualLatency(double seconds);

    /** @return Smoothed display presentation latency in seconds. */
    double visualLatencySeconds() const;

    double fps; // instantaneous frames per second from the last completed frame
    double rollingFps; // rolling average frames per second for frame budgets

    /** @return Human-readable FPS/status text for the interface overlay. */
    const char* status();

    friend int save_display();

    virtual ~CthughaDisplay() = 0;
};

//
// a special CthughaDisplay for X11
//
class CthughaDisplayX11 : public CthughaDisplay {
public:
    CthughaDisplayX11(DisplayDevice& device, DisplayRuntime& runtime);
    virtual ~CthughaDisplayX11();
    virtual void operator()();
};

extern CthughaDisplay* cthughaDisplay;

/** Allocates the frontend-specific display coordinator. */
std::unique_ptr<CthughaDisplay> newCthughaDisplay(
    DisplayDevice& device, DisplayRuntime& runtime);

void configureCthughaDisplay(const DisplayConfig& config);

#endif
