// Per-frame render inputs shared by Frame Generator and presentation.

#ifndef CTHUGHA_FRAME_RENDER_CONTEXT_H
#define CTHUGHA_FRAME_RENDER_CONTEXT_H

#include "AudioFrame.h"
#include "AudioAnalyzer.h"

class SceneSnapshot;

/**
 * Borrowed inputs shared by one indexed-frame render/presentation pass.
 *
 * The pointers are borrowed for one frame. Time values are in seconds and match
 * the visual frame clock, not the audio sample clock.
 */
class FrameRenderContext {
public:
    /** Audio frame facade for this visual frame, or NULL when audio is absent. */
    const AudioFrame* audioFrame;

    /** Raw signed 8-bit stereo audio samples, usually audioFrame->raw. */
    const char2* rawAudioData;

    /** Processed signed 8-bit stereo samples after the selected audio processor. */
    const char2* processedWaveData;

    /** Analysis metrics derived from the current audio frame, or NULL. */
    const AudioMetrics* audioMetrics;

    /** Higher-level acoustic state used by sound-reactive filters, or NULL. */
    const AcousticContext* acousticContext;

    /** Immutable scene state for this visual frame, or NULL when unavailable. */
    const SceneSnapshot* sceneSnapshot;

    /** Current visual-frame timestamp, in seconds. */
    double now;

    /** Seconds elapsed since the previous visual frame. */
    double deltaT;

    /** Creates an empty frame render context. */
    FrameRenderContext();
};

#endif
