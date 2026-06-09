/** @file
 * Application-owned audio acquisition and visual-frame production.
 */

#ifndef __AUDIO_INGEST_H
#define __AUDIO_INGEST_H

#include "Audio.h"
#include "AudioPassthrough.h"
#include "Configuration.h"

#include <atomic>
#include <memory>
#include <thread>

class SecondsClock;
class LogSink;
class RandomSource;

/**
 * Owns audio acquisition and publishes the current visual AudioFrame.
 *
 * AudioIngest owns the input source, decoded PCM history, visual frame builder,
 * and optional passthrough output. Passthrough may provide a presentation clock
 * estimate, but acquisition history and current-frame ownership remain here.
 */
class AudioIngest {
    AudioConfig configValue;
    int hasConfig;
    int visualMaxDimensionValue;
    int startWorkerThreadsValue;
    int autoCloseOnInputFinishedValue;
    RandomSource* randomSource;
    SecondsClock* clock;
    LogSink* log;
    std::unique_ptr<AudioInput> inputValue;
    std::unique_ptr<AudioOutput> injectedOutputValue;
    std::unique_ptr<AudioOutputDump> outputDumpValue;
    std::unique_ptr<DecodedAudioHistory> historyValue;
    std::unique_ptr<AudioPassthrough> passthroughValue;
    AudioFrameBuilder frameBuilder;
    std::unique_ptr<char[]> inputChunk;
    AudioFrame frameValue;
    PcmFormat pcmFormatValue;
    int initializedValue;
    int inputChunkSamplesValue;
    int decodeAheadSamplesValue;
    int samplesPerSecondValue;
    int bytesPerSampleValue;
    int visualClockStarted;
    double visualClockStartSeconds;
    std::atomic<int> inputFinished;
    std::atomic<int> stopRequested;
    std::thread inputThread;
    int inputThreadStarted;

    int buildFromConfig();
    int buildFromInjectedRuntime();
    int initializeRuntimeObjects(AudioOutput* output, int silentPassthrough);
    int fillInput(int maxSamples);
    void pumpInputToTarget();
    void inputThreadMain();
    long long visualClockSamplePosition();
    long long frameCenterSamplePosition();
    long long decodeTargetSamplePosition();

public:
    /**
     * Creates production audio ingest from startup configuration.
     *
     * @param config Audio startup configuration.
     * @param visualMaxDimension Maximum logical visual-buffer dimension, in
     *        pixels before display zoom.
     * @param randomSource_ Random source used by synthetic audio input.
     * @param clock_ Clock used for visual/presentation pacing.
     * @param log_ Sink for startup/runtime-selection diagnostics.
     * @param startWorkerThreads Nonzero to run input/output asynchronously.
     */
    AudioIngest(const AudioConfig& config, int visualMaxDimension,
        RandomSource& randomSource_, SecondsClock& clock_, LogSink& log_,
        int startWorkerThreads = 1);

    /**
     * Creates test/injected audio ingest.
     *
     * @param input Newly allocated input wrapper. Ownership is transferred.
     * @param output Optional newly allocated output backend. Ownership is
     *        transferred when non-NULL.
     * @param visualMaxDimension Maximum logical visual-buffer dimension.
     * @param clock_ Clock used for deterministic frame pacing.
     * @param log_ Sink for injected-runtime diagnostics.
     * @param autoCloseOnInputFinished Nonzero to report complete at input EOF.
     * @param startWorkerThreads Nonzero to run input/output asynchronously.
     */
    AudioIngest(AudioInput* input, AudioOutput* output, int visualMaxDimension,
        SecondsClock& clock_, LogSink& log_, int autoCloseOnInputFinished = 1,
        int startWorkerThreads = 0);

    /** Stops ingest and releases owned runtime resources. */
    ~AudioIngest();

    /**
     * Starts acquisition and optional passthrough.
     *
     * @return 0 on success, nonzero when requested startup audio cannot open.
     */
    int start();

    /** Stops worker threads/callbacks and releases input/output/history. */
    void stop();

    /** Advances acquisition and rebuilds the current visual AudioFrame. */
    void tick();

    /** @return Mutable current audio frame. */
    AudioFrame& currentFrame();

    /** @return Immutable current audio frame. */
    const AudioFrame& currentFrame() const;

    /** @return Nonzero after start() has successfully initialized objects. */
    int initialized() const { return initializedValue; }

    /** @return Negotiated PCM format owned by the active ingest session. */
    const PcmFormat& format() const;

    /** @return Nonzero when finite audio input has reached configured EOF. */
    int complete() const;

    /** @return Absolute decoded sample-frame position. */
    long long decodedSamplePosition() const;

    /** @return Estimated presentation sample-frame position. */
    long long presentationSamplePosition() const;

    /** @return Optional passthrough output, or NULL when disabled. */
    AudioPassthrough* passthrough();

    /** @return Decoded history owned by ingest, or NULL before start(). */
    DecodedAudioHistory* history();
};

#endif
