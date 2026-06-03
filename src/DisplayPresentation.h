#ifndef __DISPLAY_PRESENTATION_H
#define __DISPLAY_PRESENTATION_H

#include "DisplayGeometry.h"
#include "IndexedDisplayFrame.h"
#include "OverlaySource.h"

class FramePalette;

class DisplayPresentation {
public:
    const IndexedDisplayFrame& frame;
    FramePalette* framePalette;
    DisplayViewport viewport;
    int needsFullCopy;
    int needsBorderClear;
    OverlayCommands overlays;

    DisplayPresentation(const IndexedDisplayFrame& frame_,
        const DisplayViewport& viewport_, int needsFullCopy_,
        int needsBorderClear_)
        : frame(frame_)
        , framePalette(frame_.framePalette())
        , viewport(viewport_)
        , needsFullCopy(needsFullCopy_)
        , needsBorderClear(needsBorderClear_)
        , overlays() {
    }

    DisplayPresentation(const IndexedDisplayFrame& frame_,
        const DisplayViewport& viewport_, int needsFullCopy_,
        int needsBorderClear_, const OverlayCommands& overlays_)
        : frame(frame_)
        , framePalette(frame_.framePalette())
        , viewport(viewport_)
        , needsFullCopy(needsFullCopy_)
        , needsBorderClear(needsBorderClear_)
        , overlays(overlays_) {
    }
};

#endif
