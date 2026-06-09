// Per-frame inputs for the Frame Generator module.

#ifndef CTHUGHA_FRAME_GENERATOR_CONTEXT_H
#define CTHUGHA_FRAME_GENERATOR_CONTEXT_H

#include "AudioAnalysisSnapshot.h"
#include "AudioTypes.h"

class AudioFrame;
class SceneSnapshot;

/**
 * Borrowed inputs and frame-budget policy for one frame generation pass.
 *
 * The generator borrows audio buffers, scene snapshot, and timing for the
 * duration of one render call. Audio analysis is supplied as an immutable
 * snapshot so generator code does not depend on Audio's rolling analysis
 * state.
 */
class FrameGeneratorContext {
    const AudioFrame* audioFrameValue;
    const char2* rawAudioDataValue;
    const char2* processedWaveDataValue;
    AudioAnalysisSnapshot audioAnalysisValue;
    const SceneSnapshot* sceneSnapshotValue;
    double nowValue;
    double deltaTValue;
    int frameBudgetFramesPerSecondValue;

public:
    /** Creates an empty context with a 60 FPS duration budget. */
    FrameGeneratorContext();

    /**
     * Creates a context from explicit per-frame generator inputs.
     *
     * @param audioFrame Audio frame facade for this visual frame, or NULL.
     * @param rawAudioData Raw signed 8-bit stereo samples, or NULL.
     * @param processedWaveData Processed signed 8-bit stereo samples, or NULL.
     * @param audioAnalysis Immutable audio analysis for this visual frame.
     * @param sceneSnapshot Immutable scene state for this visual frame, or NULL.
     * @param now Current visual-frame timestamp, in seconds.
     * @param deltaT Seconds elapsed since the previous visual frame.
     * @param frameBudgetFramesPerSecond Frames per second used for transitions.
     */
    FrameGeneratorContext(const AudioFrame* audioFrame,
        const char2* rawAudioData, const char2* processedWaveData,
        const AudioAnalysisSnapshot& audioAnalysis,
        const SceneSnapshot* sceneSnapshot, double now, double deltaT,
        int frameBudgetFramesPerSecond);

    /** @return Audio frame facade for this visual frame, or NULL. */
    const AudioFrame* audioFrame() const;

    /** @return Raw signed 8-bit stereo samples, or NULL. */
    const char2* rawAudioData() const;

    /** @return Processed signed 8-bit stereo samples, or NULL. */
    const char2* processedWaveData() const;

    /** @return Immutable audio analysis for this visual frame. */
    const AudioAnalysisSnapshot& audioAnalysis() const;

    /** @return Immutable scene state for this visual frame, or NULL. */
    const SceneSnapshot* sceneSnapshot() const;

    /** @return Current visual-frame timestamp, in seconds. */
    double now() const;

    /** @return Seconds elapsed since the previous visual frame. */
    double deltaT() const;

    /** @return Explicit frame-rate budget for transition durations. */
    int frameBudgetFramesPerSecond() const;
};

#endif
