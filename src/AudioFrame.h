/** @file
 * Per-frame audio samples and audio metrics.
 */

#ifndef __AUDIO_FRAME_H
#define __AUDIO_FRAME_H

#include "AudioTypes.h"

/** Per-frame audio measurements derived from AudioFrame::raw. */
struct AudioMetrics {
    /** Average RMS amplitude across left and right channels, in signed 8-bit sample units. */
    int amplitude;

    /** Left-channel RMS amplitude, in signed 8-bit sample units. */
    int amplitudeLeft;

    /** Right-channel RMS amplitude, in signed 8-bit sample units. */
    int amplitudeRight;

    /** Average RMS amplitude after a 150 Hz low-pass filter. */
    int lowPass150HzAmplitude;

    /** Left-channel RMS amplitude after a 150 Hz low-pass filter. */
    int lowPass150HzAmplitudeLeft;

    /** Right-channel RMS amplitude after a 150 Hz low-pass filter. */
    int lowPass150HzAmplitudeRight;

    /** Nonzero when either channel is at or above the configured noise floor. */
    int noisy;

    /** Creates a silent metrics value. */
    AudioMetrics();
};

class AudioFrame {
public:
    /** Absolute sample-frame position at the center of raw/processedWaveData. */
    long long centerSample;

    /** Number of valid stereo sample frames in raw and processedWaveData. */
    int samples;

    /** Signed 8-bit stereo samples used by visualization and analysis. */
    char2 raw[1024];

    /** Signed 8-bit stereo samples after the selected audio-processing mode. */
    char2 processedWaveData[1024];

    /** Metrics measured from raw for this visual frame. */
    AudioMetrics metrics;

    /** Creates an empty silent frame. */
    AudioFrame();

    /** Resets sample positions, audio data, and metrics. */
    void clear();
};

#endif
