#ifndef __SCREEN_RENDER_CONTEXT_H
#define __SCREEN_RENDER_CONTEXT_H

#include "IndexedDisplayFrame.h"
#include "IndexedFrame.h"

class ScreenRenderContext {
    const IndexedFrame* sourceValue;
    IndexedDisplayFrame* destinationValue;
    unsigned char* destinationPixelsValue;
    int destinationWidthValue;
    int destinationHeightValue;
    int destinationPitchValue;
    double frameTimeSecondsValue;
    double deltaTimeSecondsValue;
    double framesPerSecondValue;

public:
    ScreenRenderContext(const IndexedFrame& source, IndexedDisplayFrame& destination,
        double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond);

    ScreenRenderContext(const IndexedFrame& source, IndexedDisplayFrame& destination,
        unsigned char* destinationPixels, int destinationWidth, int destinationHeight,
        int destinationPitch, double frameTimeSeconds, double deltaTimeSeconds,
        double framesPerSecond);

    const IndexedFrame& source() const;
    IndexedDisplayFrame& destination() const;

    const unsigned char* sourcePixels() const;
    const unsigned char* sourceLine(int y) const;
    int sourceWidth() const;
    int sourceHeight() const;
    int sourcePitch() const;

    unsigned char* destinationPixels();
    unsigned char* destinationLine(int y);
    int destinationWidth() const;
    int destinationHeight() const;
    int destinationPitch() const;

    double frameTimeSeconds() const;
    double deltaTimeSeconds() const;
    double framesPerSecond() const;
};

#endif
