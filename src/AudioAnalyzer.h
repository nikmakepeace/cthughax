/** @file
 * Acoustic context and audio-analysis configuration.
 */

#ifndef __AUDIO_ANALYZER_H
#define __AUDIO_ANALYZER_H

#include "AudioFrame.h"

#include <stddef.h>

class LogSink;

/** Rolling acoustic state derived from consecutive AudioFrame metrics. */
class AcousticContext {
    LogSink* log;
    double intensityValue;
    int lastAmplitudeValue;
    int attackLevelValue;
    int fireValue;
    int cumulativeFireLevelValue;

public:
    /**
     * Creates rolling acoustic state.
     *
     * @param log_ Optional sink for bounded acoustic diagnostics. When NULL,
     *        diagnostics are suppressed. The referenced object must outlive
     *        this context when supplied.
     */
    explicit AcousticContext(LogSink* log_ = NULL);

    /**
     * Updates rolling acoustic state from one analyzed frame.
     *
     * @param metrics Frame-local audio metrics from AudioProcessor::analyze().
     *        Amplitudes are signed 8-bit RMS units.
     */
    void update(const AudioMetrics& metrics);

    /**
     * @return Smoothed audio intensity, roughly normalized against 8-bit samples.
     */
    double intensity() const;

    /**
     * @return Attack energy released on this frame, in accumulated amplitude units.
     */
    int fire() const;

    /**
     * @return Accumulated fire value since the last reset, used by SceneChangeScheduler.
     */
    int cumulativeFireLevel() const;

    /**
     * Clears accumulated fire after SceneChangeScheduler consumes a threshold crossing.
     */
    void resetCumulativeFireLevel();
};

#endif
