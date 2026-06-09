/** @file
 * Visual frame clock implementation.
 */

#include "FrameClock.h"
#include "ProcessServices.h"

FrameClock::FrameClock(SecondsClock& clock_)
    : clock(clock_)
    , nowValue(0.0)
    , deltaValue(0.0)
    , hasFrameValue(0)
    , frameRateMeter() {
}

void FrameClock::beginFrame() {
    double previous = nowValue;
    nowValue = clock.nowSeconds();
    if (hasFrameValue) {
        deltaValue = nowValue - previous;
        frameRateMeter.observeFrameDuration(deltaValue);
    } else {
        deltaValue = 0.0;
        hasFrameValue = 1;
    }
}

double FrameClock::sample() {
    return clock.nowSeconds();
}

double FrameClock::now() const {
    return nowValue;
}

double FrameClock::deltaT() const {
    return deltaValue;
}

double FrameClock::framesPerSecond() const {
    return frameRateMeter.framesPerSecond();
}

double FrameClock::rollingFramesPerSecond() const {
    return frameRateMeter.rollingFramesPerSecond();
}
