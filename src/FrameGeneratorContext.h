// Per-frame inputs for the Frame Generator module.

#ifndef CTHUGHA_FRAME_GENERATOR_CONTEXT_H
#define CTHUGHA_FRAME_GENERATOR_CONTEXT_H

#include "FrameRenderContext.h"

/**
 * Borrowed inputs and frame-budget policy for one frame generation pass.
 *
 * Audio, scene, and timing data are represented by FrameRenderContext for
 * compatibility with legacy filters. The frame budget is explicit so generator
 * policy never reads display globals to estimate durations.
 */
class FrameGeneratorContext {
    FrameRenderContext frameRenderContextValue;
    int frameBudgetFramesPerSecondValue;

public:
    /** Creates an empty context with a 60 FPS duration budget. */
    FrameGeneratorContext();

    /**
     * Creates a context from legacy filter inputs and an explicit frame budget.
     *
     * @param frameRenderContext Per-frame audio, scene, and timing inputs.
     * @param frameBudgetFramesPerSecond Frames per second used for transitions.
     */
    FrameGeneratorContext(const FrameRenderContext& frameRenderContext,
        int frameBudgetFramesPerSecond);

    /** @return Legacy filter context borrowed for this render call. */
    const FrameRenderContext& frameRenderContext() const;

    /** @return Explicit frame-rate budget for transition durations. */
    int frameBudgetFramesPerSecond() const;
};

#endif
