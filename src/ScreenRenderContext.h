#ifndef __SCREEN_RENDER_CONTEXT_H
#define __SCREEN_RENDER_CONTEXT_H

#include "IndexedDisplayFrame.h"
#include "IndexedFrame.h"

class ScreenSelectionController {
public:
    virtual ~ScreenSelectionController() { }
    virtual void change(int by, int doSave) = 0;
};

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
    ScreenSelectionController* selectionControllerValue;

public:
    ScreenRenderContext(const IndexedFrame& source, IndexedDisplayFrame& destination,
        double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond,
        ScreenSelectionController* selectionController = 0);

    ScreenRenderContext(const IndexedFrame& source, IndexedDisplayFrame& destination,
        unsigned char* destinationPixels, int destinationWidth, int destinationHeight,
        int destinationPitch, double frameTimeSeconds, double deltaTimeSeconds,
        double framesPerSecond, ScreenSelectionController* selectionController = 0);

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

    int requestScreenChange(int by, int doSave);
};

ScreenRenderContext* currentScreenRenderContext();
void setCurrentScreenRenderContext(ScreenRenderContext* context);

class ScopedScreenRenderContext {
    ScreenRenderContext* previousContext;

public:
    explicit ScopedScreenRenderContext(ScreenRenderContext& context);
    ~ScopedScreenRenderContext();
};

#endif
