// -*- c++ -*-
#ifndef __CTHUGHA_DISPLAY_H
#define __CTHUGHA_DISPLAY_H

#include "cthugha.h"
#include "DisplayGeometry.h"
#include "FrameClock.h"
#include "IndexedDisplayFrame.h"
#include "PresentationComposer.h"

#include <memory>

// The CthughaDisplay layer sits between the effect buffers and the selected
// DisplayDevice backend.  It owns the per-frame timing, temporary image
// buffers, mirroring/zooming, and the final handoff to the device.
//
// It is initialized with the owned display stage so viewport and palette work
// do not need to read transitional globals.

class IndexedFrame;
class DisplayDevice;
class DisplayRuntime;
class ErrorMessages;
class InterfaceRuntime;
class SecondsClock;
class FrameRenderContext;
struct DisplayConfig;

class CthughaDisplay : public PresentationFrameObserver {
protected:
    DisplayDevice& deviceValue;
    DisplayRuntime& runtimeValue;
    const IndexedFrame* sourceFrame;
    const FrameRenderContext* presentationContextValue;
    IndexedDisplayFrame indexedDisplayFrameValue;
    PresentationComposer presentationComposer;
    DisplayViewport displayViewportValue;
    FrameClock frameClockValue;
    double frameNowValue;
    double frameDeltaTValue;

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
    void presentSourceWithContext(const FrameRenderContext* context);

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

    CthughaDisplay(DisplayDevice& device, DisplayRuntime& runtime,
        SecondsClock& clock);

    /**
     * Starts a new visual frame and updates owned frame timing.
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
    void present(const IndexedFrame& frame, const FrameRenderContext& context);

    /** Frontend-specific presentation path for the current explicit source frame. */
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

    /** @return Current visual-frame timestamp in seconds. */
    double currentFrameTimeSeconds() const;

    /** @return Seconds elapsed since the previous visual frame. */
    double currentFrameDeltaSeconds() const;

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
    SecondsClock& clock;
    InterfaceRuntime& interfaceRuntime;
    ErrorMessages& errorMessages;

public:
    CthughaDisplayX11(DisplayDevice& device, DisplayRuntime& runtime,
        SecondsClock& clock_, InterfaceRuntime& interfaceRuntime_,
        ErrorMessages& errorMessages_);
    virtual ~CthughaDisplayX11();
    virtual void operator()();
};

/** Allocates the frontend-specific display coordinator. */
std::unique_ptr<CthughaDisplay> newCthughaDisplay(
    DisplayDevice& device, DisplayRuntime& runtime, SecondsClock& clock,
    InterfaceRuntime& interfaceRuntime, ErrorMessages& errorMessages);

void configureCthughaDisplay(const DisplayConfig& config);

#endif
