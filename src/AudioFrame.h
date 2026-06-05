/** @file
 * Current audio-frame facade and per-frame audio metrics.
 *
 * This keeps visual/audio consumers on the frame facade instead of runtime
 * implementation details.
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

    /** Signed 8-bit stereo samples after the selected AudioProcessingOption. */
    char2 processedWaveData[1024];

    /** Metrics measured from raw for this visual frame. */
    AudioMetrics metrics;

    /** Creates an empty silent frame. */
    AudioFrame();

    /** Resets sample positions, audio data, and metrics. */
    void clear();
};

/**
 * Advances the published audio frame to the current visual-frame time.
 *
 * Services AudioRuntime and updates the AudioFrame returned by audioFrameCurrent().
 * Called once per visual frame before AudioVisualBridge::runFrame().
 */
void audioFrameTick();

/**
 * Reconfigures audio frame processing after audio options change.
 *
 * Used by option setters and sound reset actions to rebuild processor buffers
 * without tearing down the whole application.
 */
void audioFrameChange();

/**
 * @return Current audio frame, or NULL when the runtime has not published one.
 */
AudioFrame* audioFrameCurrent();

/**
 * @return Current raw signed 8-bit stereo frame data. Falls back to legacy
 *         processor storage when no AudioFrame facade is available.
 */
char2* audioFrameRawData();

/**
 * @return Current processed signed 8-bit stereo frame data. Falls back to legacy
 *         processor storage when no AudioFrame facade is available.
 */
char2* audioFrameProcessedWaveData();

/**
 * Publishes metrics for the current audio frame.
 *
 * If an AudioFrame facade exists, this updates AudioFrame::metrics. Legacy
 * processor-only paths keep the latest metrics in a facade-owned fallback slot.
 *
 * @param metrics Metrics measured for this visual frame.
 */
void audioFramePublishMetrics(const AudioMetrics& metrics);

/**
 * @return Metrics for the current audio frame, or silent metrics when no audio
 *         frame or legacy metrics have been published.
 */
const AudioMetrics& audioFrameMetrics();

/**
 * @return Number of bytes in the current broadcast audio frame.
 */
int audioFrameBroadcastBytes();

#ifdef CTH_AUDIO_FRAME_TEST_OVERRIDE
/**
 * Installs a test-only current-frame override.
 *
 * @param frame Frame pointer to publish during tests, or NULL to clear.
 */
void audioFrameSetTestOverride(AudioFrame* frame);
#endif

#endif
