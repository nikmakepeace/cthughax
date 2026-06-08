#include "FrameRenderContext.h"

FrameRenderContext::FrameRenderContext()
    : audioFrame(0)
    , rawAudioData(0)
    , processedWaveData(0)
    , audioMetrics(0)
    , acousticContext(0)
    , sceneSnapshot(0)
    , now(0.0)
    , deltaT(0.0) { }
