#ifndef __VIEWPORT_POLICY_H
#define __VIEWPORT_POLICY_H

#include "DisplayGeometry.h"

class ViewportPolicy {
public:
    DisplayViewport viewportFor(PixelSize frameSize, PixelSize windowSize,
        int zoomMode) const;
};

#endif
