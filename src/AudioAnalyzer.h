// -*- c++ -*-

#ifndef __AUDIO_ANALYZER_H
#define __AUDIO_ANALYZER_H

#include "cthugha.h"
#include "AudioTypes.h"

#include "EffectControl.h"

struct AutoChangeConfig;

extern OptionInt sound_minnoise; /* quiet is below this */
void configureAudioAnalyzer(const AutoChangeConfig& config);

struct AudioMetrics {
    /** Average RMS amplitude across left and right channels, in signed 8-bit sample units. */
    int amplitude;

    /** Left-channel RMS amplitude, in signed 8-bit sample units. */
    int amplitudeLeft;

    /** Right-channel RMS amplitude, in signed 8-bit sample units. */
    int amplitudeRight;

    /** Nonzero when either channel is at or above sound_minnoise. */
    int noisy;

    AudioMetrics();
};

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
     * @param metrics Frame-local audio metrics from AudioAnalyzer::analyze().
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

class AudioAnalyzer {
public:
    AudioAnalyzer();

    /**
     * Measures one signed 8-bit stereo audio frame.
     *
     * @param frame Pointer to 1024 stereo samples in Cthugha's char2 format.
     * @return Frame-local RMS amplitudes and noisy/quiet flag.
     */
    AudioMetrics analyze(const char2* frame);

    /**
     * Publishes global audio metrics for the current frame.
     *
     * Reads audioFrameRawData(), stores audioMetrics, and updates
     * acousticContext. Called once per visual frame by AudioVisualBridge.
     */
    void operator()();
};

extern AudioAnalyzer audioAnalyzer;
extern AudioMetrics audioMetrics;
extern AcousticContext acousticContext;

#endif
