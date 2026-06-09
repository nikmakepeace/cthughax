/** @file
 * Visual frame clock and FPS accounting.
 */

#ifndef __FRAME_CLOCK_H
#define __FRAME_CLOCK_H

#include "FrameRateMeter.h"

class SecondsClock;

/**
 * Samples frame time from an explicit process clock and derives frame deltas.
 */
class FrameClock {
    SecondsClock& clock;
    double nowValue;
    double deltaValue;
    int hasFrameValue;
    FrameRateMeter frameRateMeter;

public:
    /**
     * Creates a frame clock using an explicit seconds clock.
     *
     * @param clock_ Clock sampled at frame boundaries. The referenced object
     *        must outlive this FrameClock.
     */
    explicit FrameClock(SecondsClock& clock_);

    /** Starts a new frame and updates now/delta/fps accounting. */
    void beginFrame();

    /** @return Current clock sample without advancing frame state. */
    double sample();

    /** @return Time sampled for the current visual frame. */
    double now() const;

    /** @return Seconds elapsed between the two most recent frames. */
    double deltaT() const;

    /** @return Instantaneous frames-per-second from the latest frame delta. */
    double framesPerSecond() const;

    /** @return Rolling frames-per-second over the recent observation window. */
    double rollingFramesPerSecond() const;

};

#endif
