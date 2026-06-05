/** @file
 * Acoustic context and audio-analysis configuration.
 */

#ifndef __AUDIO_ANALYZER_H
#define __AUDIO_ANALYZER_H

#include "cthugha.h"
#include "AudioFrame.h"
#include "Option.h"

struct AutoChangeConfig;

/** Configured noise floor used when AudioProcessor analyzes frame metrics. */
extern OptionInt sound_minnoise; /* quiet is below this */

/**
 * Applies startup auto-change audio-analysis configuration.
 *
 * @param config Startup auto-change configuration containing minNoise.
 */
void configureAudioAnalyzer(const AutoChangeConfig& config);

/** Rolling acoustic state derived from consecutive AudioFrame metrics. */
class AcousticContext {
    double intensityValue;
    int lastAmplitudeValue;
    int attackLevelValue;
    int fireValue;
    int cumulativeFireLevelValue;

public:
    AcousticContext();

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
     * @return Accumulated fire value since the last reset, used by AutoChanger.
     */
    int cumulativeFireLevel() const;

    /**
     * Clears accumulated fire after AutoChanger consumes a threshold crossing.
     */
    void resetCumulativeFireLevel();
};

#endif
