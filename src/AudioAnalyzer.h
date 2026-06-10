/** @file
 * Acoustic context and audio-analysis configuration.
 */

#ifndef __AUDIO_ANALYZER_H
#define __AUDIO_ANALYZER_H

#include "AudioFrame.h"

#include <stddef.h>

class LogSink;

enum AcousticFireSource {
    AcousticFireSourceRawAmplitude,
    AcousticFireSourceLowPass150HzAmplitude
};

/** Rolling acoustic state derived from consecutive AudioFrame metrics. */
class AcousticContext {
    LogSink* log;
    double intensityValue;
    int lastAmplitudeValue;
    int attackLevelValue;
    int fireValue;
    int cumulativeFireLevelValue;
    int fireSensitivityValue;
    AcousticFireSource fireSourceValue;

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
     * Sets fire detection sensitivity.
     *
     * @param sensitivity 0..100, where 100 preserves the historical behavior
     *        and lower values suppress smaller attack bursts.
     */
    void setFireSensitivity(int sensitivity);

    /** @return Fire detection sensitivity in the range 0..100. */
    int fireSensitivity() const;

    /**
     * Sets the metric source used by fire detection.
     *
     * @param source Source enum value.
     */
    void setFireSource(AcousticFireSource source);

    /**
     * Sets the metric source used by fire detection by stable config name.
     *
     * @param sourceName Stable source name.
     * @return Nonzero when sourceName selected a known source.
     */
    int setFireSource(const char* sourceName);

    /** @return Source used by fire detection. */
    AcousticFireSource fireSource() const;

    /** @return Stable config/protocol name for fireSource(). */
    const char* fireSourceName() const;

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
