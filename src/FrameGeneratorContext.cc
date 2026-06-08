// Per-frame inputs for the Frame Generator module.

#include "FrameGeneratorContext.h"

FrameGeneratorContext::FrameGeneratorContext()
    : frameRenderContextValue()
    , frameBudgetFramesPerSecondValue(60) { }

FrameGeneratorContext::FrameGeneratorContext(
    const FrameRenderContext& frameRenderContext, int frameBudgetFramesPerSecond)
    : frameRenderContextValue(frameRenderContext)
    , frameBudgetFramesPerSecondValue(
          frameBudgetFramesPerSecond > 0 ? frameBudgetFramesPerSecond : 60) { }

const FrameRenderContext& FrameGeneratorContext::frameRenderContext() const {
    return frameRenderContextValue;
}

int FrameGeneratorContext::frameBudgetFramesPerSecond() const {
    return frameBudgetFramesPerSecondValue;
}
