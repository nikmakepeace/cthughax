#ifndef __PRESENTATION_COMPOSER_H
#define __PRESENTATION_COMPOSER_H

#include "ScreenRenderContext.h"
#include "cthugha.h"

class IndexedDisplayFrame;
class IndexedFrame;
class ScreenEntry;

class PresentationFrameObserver {
public:
    virtual ~PresentationFrameObserver() { }
    virtual void indexedPixelsWillMove(unsigned char* oldPixels) = 0;
    virtual void indexedFrameGeometryChanged() = 0;
};

class PresentationScreenSelection : public ScreenSelectionController {
public:
    virtual ScreenEntry* current() = 0;
};

class PresentationComposer {
    ScreenEntry* renderedScreenValue;

public:
    PresentationComposer();

    const IndexedDisplayFrame& compose(const IndexedFrame& source,
        IndexedDisplayFrame& destination, PresentationScreenSelection& selection,
        double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond,
        PresentationFrameObserver* observer = 0);

    ScreenEntry* renderedScreen() const;
};

#endif
