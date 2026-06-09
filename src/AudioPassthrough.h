/** @file
 * Optional decoded-audio passthrough output.
 */

#ifndef __AUDIO_PASSTHROUGH_H
#define __AUDIO_PASSTHROUGH_H

#include "Audio.h"

#include <atomic>
#include <memory>
#include <thread>

class LogSink;

/**
 * Owns audio output and its playback cursor over decoded history.
 *
 * AudioPassthrough consumes DecodedAudioHistory through AudioOutputStream. It
 * never owns acquisition state and never advances the visual-frame clock by
 * mutating decoded history.
 */
class AudioPassthrough {
    std::unique_ptr<AudioOutput> outputValue;
    AudioPlaybackClock playbackClockValue;
    AudioOutputStream streamValue;
    const std::atomic<int>& inputFinished;
    LogSink& log;
    char* scratch;
    int scratchSamplesValue;
    std::atomic<int> stopRequested;
    std::atomic<int> completeValue;
    std::atomic<int> callbackDrainStarted;
    std::thread outputThread;
    int outputThreadStarted;

    void outputThreadMain();

public:
    /**
     * Creates passthrough output.
     *
     * @param output Newly allocated output backend. Ownership is transferred.
     * @param history Decoded history to consume. The referenced object must
     *        outlive this passthrough.
     * @param inputFinished_ Acquisition completion flag observed for drain
     *        completion.
     * @param log_ Sink for passthrough lifecycle diagnostics.
     */
    AudioPassthrough(AudioOutput* output, DecodedAudioHistory& history,
        const std::atomic<int>& inputFinished_, LogSink& log_);

    /** Stops output work and releases output resources. */
    ~AudioPassthrough();

    /**
     * Starts output timing and optional worker/callback drain.
     *
     * @param samplesPerSecond PCM sample rate in Hertz.
     * @param bytesPerSample Bytes per interleaved PCM sample frame.
     * @param inputChunkSamples Acquisition chunk size in sample frames.
     * @param startWorkerThread Nonzero to drain output asynchronously.
     * @return 0 on success, nonzero when the output backend is unusable.
     */
    int start(int samplesPerSecond, int bytesPerSample, int inputChunkSamples,
        int startWorkerThread = 1);

    /** Stops worker/callback drain activity. Safe to call repeatedly. */
    void stop();

    /** Services output once on the caller's thread. */
    int serviceOnce();

    /** Wakes callback-based output after new decoded PCM arrives. */
    void notifyDecodedPcm();

    /** @return Nonzero when an output backend is present and open. */
    int enabled() const;

    /** @return Nonzero when input is finished and passthrough has drained. */
    int complete() const;

    /** @return Nonzero when this output is a real-time presentation clock. */
    int providesPresentationClock() const;

    /** @return Estimated audible sample-frame position for visual sync. */
    long long presentationSamplePosition() const;

    /** @return Absolute sample-frame position after last output submission. */
    long long submittedSamplePosition() const;

    /** @return Number of sample frames queued for this passthrough cursor. */
    int queuedSamples() const;

    /** @return Desired queued output lead in sample frames. */
    int targetDelaySamples() const;

    /** @return Cursor used by output backends. */
    AudioOutputStream& stream() { return streamValue; }
};

#endif
