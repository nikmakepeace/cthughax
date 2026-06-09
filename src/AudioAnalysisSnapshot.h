/** @file
 * Immutable per-frame audio analysis values for visual consumers.
 */

#ifndef CTHUGHA_AUDIO_ANALYSIS_SNAPSHOT_H
#define CTHUGHA_AUDIO_ANALYSIS_SNAPSHOT_H

#include "AudioFrame.h"

/**
 * Immutable audio analysis consumed by Frame Generator renderers.
 *
 * This value captures the frame-local metrics and rolling acoustic values that
 * sound-reactive visual code reads during one render call. It deliberately
 * does not expose Audio's rolling analysis object, so the generator cannot
 * mutate or retain Audio-owned analysis state.
 */
class AudioAnalysisSnapshot {
    AudioMetrics metricsValue;
    double intensityValue;
    int fireValue;
    int cumulativeFireLevelValue;

public:
    /** Creates a silent analysis snapshot. */
    AudioAnalysisSnapshot();

    /**
     * Creates an analysis snapshot from explicit audio values.
     *
     * @param metrics Frame-local RMS/noise metrics.
     * @param intensity Rolling acoustic intensity.
     * @param fire Frame-local attack/fire energy.
     * @param cumulativeFireLevel Rolling fire accumulation.
     */
    AudioAnalysisSnapshot(const AudioMetrics& metrics, double intensity,
        int fire, int cumulativeFireLevel);

    /** @return Frame-local RMS/noise metrics. */
    const AudioMetrics& metrics() const;

    /** @return Average RMS amplitude across channels. */
    int amplitude() const;

    /** @return Left-channel RMS amplitude. */
    int amplitudeLeft() const;

    /** @return Right-channel RMS amplitude. */
    int amplitudeRight() const;

    /** @return Nonzero when the source frame is above the noise floor. */
    int noisy() const;

    /** @return Rolling acoustic intensity. */
    double intensity() const;

    /** @return Frame-local attack/fire energy. */
    int fire() const;

    /** @return Rolling fire accumulation. */
    int cumulativeFireLevel() const;
};

#endif
