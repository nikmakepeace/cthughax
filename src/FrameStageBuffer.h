#ifndef CTHUGHA_FRAME_STAGE_BUFFER_H
#define CTHUGHA_FRAME_STAGE_BUFFER_H

#include "FrameRenderTarget.h"

/**
 * Explicit per-stage indexed frame IO.
 *
 * This adapter intentionally presents the current stage as immutable source
 * pixels and mutable destination pixels.
 */
class FrameStageBuffer {
    FrameRenderTarget* target;

public:
    explicit FrameStageBuffer(FrameRenderTarget& target_)
        : target(&target_) { }

    int width() const { return target->width(); }
    int height() const { return target->height(); }
    int pitch() const { return target->pitch(); }
    int size() const { return target->size(); }
    int visibleStorageByteCount() const {
        return target->visibleStorageByteCount();
    }
    int bottom() const { return target->bottom(); }
    int maxDimension() const { return target->maxDimension(); }
    int hiddenBorderRows() const { return target->hiddenBorderRows(); }
    int hiddenBorderByteCount() const {
        return target->hiddenBorderByteCount();
    }

    const unsigned char* sourcePixels() const {
        return static_cast<const FrameRenderTarget*>(target)->sourcePixels();
    }
    const unsigned char* sourceRow(int y) const {
        return static_cast<const FrameRenderTarget*>(target)->sourceRow(y);
    }
    unsigned char* destinationPixels() {
        return target->destinationPixels();
    }
    unsigned char* destinationRow(int y) {
        return target->destinationRow(y);
    }
    unsigned char* destinationTopHiddenRows() {
        return target->destinationTopHiddenRows();
    }
    unsigned char* destinationBottomHiddenRows() {
        return target->destinationBottomHiddenRows();
    }

    int visibleRowsArePacked() const {
        return target->visibleRowsArePacked();
    }
    int visibleOffset(int x, int y) const {
        return target->visibleOffset(x, y);
    }
    int visibleLinearOffset(int linearOffset) const {
        return target->visibleLinearOffset(linearOffset);
    }
};

#endif
