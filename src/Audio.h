/** @file
 * Audio input, output, decoded-history, and frame conversion interfaces.
 */

#ifndef __AUDIO_H
#define __AUDIO_H

#include "AudioFftProcessor.h"
#include "AudioOptions.h"
#include "AudioFrame.h"
#include "AudioOutputConfig.h"
#include "AudioOutputDump.h"

#include <stdio.h>
#include <limits.h>
#include <atomic>
#include <mutex>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

class AudioPlaybackClock;
class AudioOutputStream;
class DecodedAudioHistory;
class AudioSettings;
class MiniAudioCapturePcmSourceState;
class AudioMiniAudioOutputState;
class LogSink;
class RandomSource;
class SecondsClock;

/**
 * Bounded debug reporter for PCM submitted to an output backend.
 *
 * AudioOutput owns one reporter so debug throttling is scoped to the output
 * session instead of shared across the process.
 */
class AudioSubmittedPcmDebugReporter {
    std::atomic<int> reportsValue;

public:
    /** Creates a reporter with no submitted-PCM reports emitted yet. */
    AudioSubmittedPcmDebugReporter();

    /**
     * Emits bounded debug detail for PCM submitted to an output backend.
     *
     * @param format PCM format represented by scratch.
     * @param scratch PCM bytes submitted to output.
     * @param samples Number of sample frames represented in scratch.
     * @param bytes Number of bytes represented in scratch.
     * @param written Number of bytes accepted by the backend.
     * @param queuedSamples Remaining queued sample frames after submission.
     * @param submittedEndSample Absolute sample-frame position after
     *        submission.
     * @param log Sink used to gate and emit bounded diagnostics.
     */
    void submittedPcm(const PcmFormat& format, const char* scratch,
        int samples, int bytes, int written, int queuedSamples,
        long long submittedEndSample, LogSink& log);

    /** @return Number of submitted-PCM debug reports emitted by this reporter. */
    int reportCount() const { return reportsValue.load(); }
};

class AudioOutput {
    SecondsClock& clock;
    LogSink& log;
    int outputSamplesPerSecond;
    int outputBytesPerSample;
    int outputTargetLatencyMs;
    int outputTargetDelaySamples;
    int outputScratchSamples;
    AudioOutputDump* outputDumpValue;
    AudioSubmittedPcmDebugReporter submittedPcmDebugReporterValue;

protected:
    /** @return Current process/application time in seconds from the output clock. */
    double nowSeconds() const;

    /** @return Output diagnostics sink supplied by the owner. */
    LogSink& logSink() const;

    virtual int defaultTargetLatencyMs() const;
    virtual int timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const;
    void dumpSubmittedPcm(const PcmFormat& format, const char* data, int bytes);
    void reportSubmittedPcm(const PcmFormat& format, const char* scratch,
        int samples, int bytes, int written, int queuedSamples,
        long long submittedEndSample);

public:
    /**
     * Creates an output backend base with explicit output config.
     *
     * @param targetLatencyMs Output queue target latency in milliseconds.
     * @param outputDump Optional submitted-PCM dump collaborator.
     * @param clock_ Clock used for output service trace timing. The referenced
     *        object must outlive this output.
     * @param log_ Sink for output diagnostics. The referenced object must
     *        outlive this output.
     */
    AudioOutput(int targetLatencyMs, AudioOutputDump* outputDump,
        SecondsClock& clock_, LogSink& log_);

    /** Releases output backend resources. */
    virtual ~AudioOutput();

    /**
     * Writes raw PCM bytes to the output backend.
     *
     * @param buffer Pointer to interleaved PCM data in the current output format.
     * @param size Number of bytes available at buffer.
     * @return Number of bytes accepted, or 0/nonpositive on backend failure.
     */
    virtual int write(const void* buffer, int size) = 0;
    /** @return Native file descriptor/handle when one exists, otherwise -1. */
    virtual int getHandle() const { return -1; }

    /** @return Nonzero when the backend is open and writable. */
    virtual int isOpen() const = 0;

    /** @return Nonzero when backend writes are paced by real audio hardware. */
    virtual int isRealtime() const { return 1; }

    /** Refreshes or reopens backend-local runtime state. */
    virtual void update() { }

    /**
     * Configures output pacing and scratch-buffer sizing.
     *
     * @param samplesPerSecond Output sample rate in Hertz.
     * @param bytesPerSample Bytes per interleaved output sample frame.
     * @param inputChunkSamples Number of samples produced per input service chunk.
     */
    virtual void configureTiming(int samplesPerSecond, int bytesPerSample, int inputChunkSamples);
    /** @return Configured output sample rate in Hertz. */
    int samplesPerSecond() const { return outputSamplesPerSecond; }

    /** @return Configured output queue target latency in milliseconds. */
    int targetLatencyMs() const { return outputTargetLatencyMs; }

    /** @return Bytes per interleaved output sample frame. */
    int bytesPerSample() const { return outputBytesPerSample; }

    /** @return Target queued output delay in sample frames. */
    int targetDelaySamples() const { return outputTargetDelaySamples; }

    /** @return Estimated delay between submitted samples and audible playback. */
    virtual int presentationDelaySamples() const { return outputTargetDelaySamples; }

    /** @return Recommended scratch buffer size in sample frames. */
    int scratchSamples() const { return outputScratchSamples; }

    /** @return Number of submitted-PCM debug reports emitted for this output. */
    int submittedPcmDebugReportCount() const {
        return submittedPcmDebugReporterValue.reportCount();
    }

    /** @return Desired queued output amount before playback is considered full. */
    virtual int queuedTargetSamples() const;

    /**
     * Determines whether finite passthrough has drained.
     *
     * @param stream Output stream cursor being serviced.
     * @param inputFinished Nonzero when acquisition has reached EOF.
     * @return Nonzero when output playback has completed.
     */
    int playbackComplete(const AudioOutputStream& stream, int inputFinished) const;

    /**
     * Moves queued decoded PCM toward the output backend.
     *
     * @param stream Passthrough stream cursor over decoded audio history.
     * @param scratch Temporary PCM buffer owned by the runtime.
     * @param scratchSamples Capacity of scratch in sample frames.
     * @param inputFinished Nonzero when the input source has no more samples.
     * @return Nonzero when at least one sample frame was submitted to output.
     */
    virtual int service(AudioOutputStream& stream, char* scratch, int scratchSamples,
        int inputFinished);

    /** @return Nonzero when backend can drain from an asynchronous callback. */
    virtual int supportsCallbackDrain() const { return 0; }

    /** Starts optional backend-owned callback draining. */
    virtual void startCallbackDrain(AudioOutputStream&, const std::atomic<int>*, int) { }

    /** Notifies optional backend-owned callback draining that more PCM exists. */
    virtual void notifyCallbackDrain() { }

    /** Stops optional backend-owned callback draining. */
    virtual void stopCallbackDrain() { }

    /** @return Nonzero when backend-owned callback drain is complete. */
    virtual int callbackDrainComplete(const AudioOutputStream& stream,
        int inputFinished) const;
};

class AudioNullOutput : public AudioOutput {
protected:
    virtual int defaultTargetLatencyMs() const;

public:
    /**
     * Creates a null output backend.
     *
     * @param clock_ Clock used for output service trace timing.
     * @param log_ Sink for output diagnostics.
     * @param config Output startup/session config.
     * @param outputDump Optional submitted-PCM dump collaborator.
     */
    explicit AudioNullOutput(SecondsClock& clock_, LogSink& log_,
        const AudioOutputConfig& config = AudioOutputConfig(),
        AudioOutputDump* outputDump = NULL);

    /** Drops PCM bytes without opening a real output device. */
    virtual int write(const void* buffer, int size);

    /** @return Always nonzero because the null output is always available. */
    virtual int isOpen() const;

    /** @return Zero because null output does not provide hardware pacing. */
    virtual int isRealtime() const;
};

class AudioPulseOutput : public AudioOutput {
    PcmFormat pcmFormatValue;
    AudioOutputConfig outputConfigValue;
    void* mainloop;
    void* context;
    void* stream;
    void* drainEvent;
    AudioOutputStream* callbackStream;
    const std::atomic<int>* callbackInputFinished;
    char* callbackScratch;
    int callbackScratchSamples;
    std::atomic<int> callbackDrainActive;
    int bytesPerSecondValue;
    std::atomic<int> pulsePresentationDelaySamples;
    std::atomic<int> adaptiveQueueTargetSamples;
    std::atomic<int> underflowCountValue;
    std::atomic<int> lastReportedUnderflows;

    void closePulse();
    int writeUnlocked(const void* buffer, int size, int waitForWritable);
    int drainUnlocked(size_t requestedBytes);
    int adaptiveQueueBumpSamples() const;
    int adaptiveQueueCapSamples() const;
    void growAdaptiveQueueTarget(int observedQueuedSamples, const char* reason);
    void observeCallbackQueue(int queuedSamples);

protected:
    virtual int defaultTargetLatencyMs() const;
    virtual int timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const;

public:
    /**
     * Opens a PulseAudio output stream for an explicit PCM format.
     *
     * @param format PCM format produced by the current audio session.
     * @param clock_ Clock used for output service trace timing.
     * @param log_ Sink for output diagnostics.
     * @param config Output startup/session config.
     * @param outputDump Optional submitted-PCM dump collaborator.
     * @param autoOpen Nonzero to open PulseAudio immediately.
     */
    explicit AudioPulseOutput(const PcmFormat& format,
        SecondsClock& clock_, LogSink& log_,
        const AudioOutputConfig& config = AudioOutputConfig(),
        AudioOutputDump* outputDump = NULL, int autoOpen = 1);

    /** Stops callback drain and closes PulseAudio resources. */
    virtual ~AudioPulseOutput();

    /** Writes PCM bytes into the PulseAudio stream. */
    virtual int write(const void* buffer, int size);

    /** @return Nonzero when PulseAudio context and stream are connected. */
    virtual int isOpen() const;

    /** @return Nonzero because PulseAudio provides presentation pacing. */
    virtual int isRealtime() const;

    /** Reopens PulseAudio stream using the session PCM format. */
    virtual void update();

    /** Services PulseAudio from the caller thread when callback drain is absent. */
    virtual int service(AudioOutputStream& stream, char* scratch, int scratchSamples,
        int inputFinished);

    /** @return Nonzero when PulseAudio callback drain has been initialized. */
    virtual int supportsCallbackDrain() const;

    /** Starts PulseAudio callback drain over the provided output stream. */
    virtual void startCallbackDrain(AudioOutputStream& stream,
        const std::atomic<int>* inputFinished, int scratchSamples);

    /** Signals the callback drain event loop. */
    virtual void notifyCallbackDrain();

    /** Stops PulseAudio callback drain and releases callback scratch state. */
    virtual void stopCallbackDrain();

    /** @return Nonzero when PulseAudio callback drain has submitted all input. */
    virtual int callbackDrainComplete(const AudioOutputStream& stream,
        int inputFinished) const;

    /** Called by the PulseAudio write callback when bytes may be drained. */
    void pulseWritable(size_t requestedBytes);

    /** Called by the PulseAudio underflow callback. */
    void pulseUnderflow();

    /** @return Number of Pulse underflows observed by this output instance. */
    int underflowCount() const { return underflowCountValue.load(); }

    /** @return Configured Pulse server name, or NULL for the default server. */
    const char* serverName() const;

    /** @return Human-readable configured Pulse server name. */
    const char* serverDisplayName() const;

    /** @return PulseAudio stream target latency in milliseconds. */
    int pulseLatencyMs() const;

    /** @return PulseAudio stream delay used by the visual presentation clock. */
    virtual int presentationDelaySamples() const;

    /** @return Adaptive decoded-audio lead target for Pulse callback drain. */
    virtual int queuedTargetSamples() const;
};

class AudioMiniAudioOutput : public AudioOutput {
    AudioMiniAudioOutputState* state;

    void open(const PcmFormat& format);
    void close();
    void drainCallback(void* output, unsigned int frameCount);
    void startCallbackDrainForStream(AudioOutputStream& stream,
        const std::atomic<int>* inputFinished, int scratchSamples);

protected:
    virtual int defaultTargetLatencyMs() const;
    virtual int timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const;

public:
    enum MiniAudioDeviceSampleFormat {
        MiniAudioDeviceSampleFormatUnknown = 0,
        MiniAudioDeviceSampleFormatU8,
        MiniAudioDeviceSampleFormatS16,
        MiniAudioDeviceSampleFormatF32
    };

    /**
     * Opens a miniaudio playback device for an explicit PCM format.
     *
     * Uses the configured miniaudio playback device name when one is supplied,
     * otherwise opens the platform default playback device.
     *
     * @param format PCM format produced by the current audio session.
     * @param clock_ Clock used for output diagnostics.
     * @param log_ Sink for output diagnostics.
     * @param config Output startup/session config.
     * @param outputDump Optional submitted-PCM dump collaborator.
     * @param autoOpen Nonzero to open miniaudio immediately.
     */
    explicit AudioMiniAudioOutput(const PcmFormat& format,
        SecondsClock& clock_, LogSink& log_,
        const AudioOutputConfig& config = AudioOutputConfig(),
        AudioOutputDump* outputDump = NULL, int autoOpen = 1);

    /** Stops callback drain and closes miniaudio resources. */
    virtual ~AudioMiniAudioOutput();

    /** Synchronous writes are not used by the miniaudio callback backend. */
    virtual int write(const void* buffer, int size);

    /** @return Nonzero when the miniaudio device is open. */
    virtual int isOpen() const;

    /** @return Nonzero because miniaudio provides hardware pacing. */
    virtual int isRealtime() const;

    /** Reopens miniaudio using the session PCM format. */
    virtual void update();

    /** @return Nonzero when miniaudio callback drain can run. */
    virtual int supportsCallbackDrain() const;

    /** Starts miniaudio callback drain over the provided output stream. */
    virtual void startCallbackDrain(AudioOutputStream& stream,
        const std::atomic<int>* inputFinished, int scratchSamples);

    /** Miniaudio callbacks are device-driven; this is a no-op wake hint. */
    virtual void notifyCallbackDrain();

    /** Stops miniaudio callback drain without closing the device. */
    virtual void stopCallbackDrain();

    /** @return Nonzero when callback drain has submitted all finite input. */
    virtual int callbackDrainComplete(const AudioOutputStream& stream,
        int inputFinished) const;

    /** @return Current miniaudio presentation-delay estimate. */
    virtual int presentationDelaySamples() const;

    /** @return Decoded-audio lead target for miniaudio callback drain. */
    virtual int queuedTargetSamples() const;

    /** @return Number of miniaudio callback underruns observed by this instance. */
    int underflowCount() const;

    /** @return Human-readable miniaudio backend name selected for this device. */
    const char* backendName() const;

#ifdef AUDIO_OUTPUT_TEST_HOOKS
    static MiniAudioDeviceSampleFormat testDeviceSampleFormatFor(
        const PcmFormat& format, int* directCopy);
    static int testBackendNameIsNull(const char* backendName);
    static int testPresentationDelaySamples(int sampleRate,
        int targetLatencyMs, int internalPeriodFrames, int internalPeriods);
    void testStartCallbackDrainWithoutDevice(AudioOutputStream& stream,
        const std::atomic<int>* inputFinished, int scratchSamples);
    void testStartCallbackDrainWithoutDeviceFormat(AudioOutputStream& stream,
        const std::atomic<int>* inputFinished, int scratchSamples,
        MiniAudioDeviceSampleFormat callbackFormat, int callbackChannels);
    void testDrainCallback(void* output, unsigned int frameCount);
    void testLogConnectedDiagnostics(const char* backendName,
        const char* deviceName, int sampleRate, int channels,
        MiniAudioDeviceSampleFormat callbackFormat, int directCopy,
        int requestedPeriodMilliseconds, int requestedPeriods,
        int internalPeriodFrames, int internalPeriods,
        int presentationDelaySamples);
#endif
};

class AudioDSPOutput : public AudioOutput {
    int handle;
    int method;
    PcmFormat pcmFormatValue;
    int dspFragmentsValue;
    int dspFragmentSizeValue;
    char dspDevicePathValue[PATH_MAX];
    int sampleWindow;

    void setFragment();
    void setChannels();
    void setSampleRate();
    void setFormat();
    void init();

protected:
    virtual int defaultTargetLatencyMs() const;
    virtual int timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const;

public:
    /**
     * Opens the OSS/DSP output backend.
     *
     * @param settings Audio startup/session settings to negotiate locally.
     * @param config Output startup/session config.
     * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
     *        before display zoom. Used to choose a DSP fragment/sample window.
     * @param clock_ Clock used for output service trace timing.
     * @param log_ Sink for output diagnostics.
     * @param outputDump Optional submitted-PCM dump collaborator.
     */
    AudioDSPOutput(const AudioSettings& settings,
        const AudioOutputConfig& config, int visualMaxDimension,
        SecondsClock& clock_, LogSink& log_, AudioOutputDump* outputDump = NULL);

    /** Closes the OSS/DSP output backend. */
    virtual ~AudioDSPOutput();

    /** Writes PCM bytes to the OSS/DSP output device. */
    virtual int write(const void* buffer, int size);

    /** @return Open OSS/DSP file descriptor, or -1. */
    virtual int getHandle() const;

    /** @return Nonzero when the OSS/DSP output device is open. */
    virtual int isOpen() const;

    /** Reopens and renegotiates the OSS/DSP output device. */
    virtual void update();
};

/**
 * Thread-safe decoded PCM history owned by audio acquisition.
 *
 * The history keeps a recent window of decoded samples for visual-frame
 * reconstruction. It deliberately has no output/submitted cursor; passthrough
 * playback owns that cursor through AudioOutputStream.
 */
class DecodedAudioHistory {
    char* data;
    LogSink& log;
    PcmFormat pcmFormatValue;
    int bytesPerSampleValue;
    int capacitySamples;
    int retainedHistorySamples;
    long long decodedEndSample;
    mutable std::mutex mutex;

    long long retainedWindowStartSample() const;
    int copyAt(long long samplePosition, char* dst, int samples) const;

public:
    /**
     * Allocates a decoded PCM ring buffer.
     *
     * @param capacitySamples Writable capacity in sample frames.
     * @param format PCM format stored in the history.
     * @param retainedHistorySamples Recent decoded sample frames retained for
     *        visual-frame reconstruction.
     * @param log_ Sink for history diagnostics. The referenced object must
     *        outlive this history.
     */
    DecodedAudioHistory(int capacitySamples, const PcmFormat& format,
        int retainedHistorySamples, LogSink& log_);

    /** Releases decoded PCM storage. */
    ~DecodedAudioHistory();

    /** @return PCM format stored in this history. */
    const PcmFormat& format() const { return pcmFormatValue; }

    /** @return Bytes in one interleaved sample frame. */
    int bytesPerSample() const { return bytesPerSampleValue; }

    /** @return Number of recent decoded sample frames retained for readers. */
    int retainedWindowSamples() const;

    /** @return Remaining writable sample-frame capacity before overwrite. */
    int writableSamples() const;

    /** @return Oldest absolute sample-frame position still retained. */
    long long oldestAvailablePosition() const;

    /** @return Absolute sample-frame position after the decoded tail. */
    long long decodedEndPosition() const;

    /** Clears decoded history and resets positions to zero. */
    void clear();

    /**
     * Appends decoded PCM to the writable tail.
     *
     * @param src Pointer to decoded PCM bytes in this buffer's format.
     * @param samples Number of sample frames available at src.
     * @return Number of sample frames appended.
     */
    int appendDecodedPcm(const char* src, int samples);

    /**
     * Copies retained PCM at an absolute sample-frame position.
     *
     * @param samplePosition Absolute sample-frame position to read.
     * @param dst Destination buffer for PCM bytes.
     * @param samples Maximum number of sample frames to copy.
     * @return Number of sample frames copied, or zero if unavailable.
     */
    int readPcmAt(long long samplePosition, char* dst, int samples) const;
};

/**
 * Atomic playback position shared by output callbacks and visual timing.
 *
 * Values are absolute PCM sample-frame positions, never ring-buffer offsets.
 */
class AudioPlaybackClock {
    std::atomic<long long> submittedEndSample;
    std::atomic<int> presentationDelaySampleCount;

public:
    /** Creates a clock at sample zero with no presentation delay. */
    AudioPlaybackClock();

    /** Publishes the absolute sample frame after the latest submitted output. */
    void publishSubmittedEndSample(long long sample);

    /** Publishes the estimated device/output delay in sample frames. */
    void publishPresentationDelaySamples(int samples);

    /** @return Absolute sample frame after the latest submitted output. */
    long long submittedEndPosition() const;

    /** @return Current presentation delay in sample frames. */
    int presentationDelaySamples() const;

    /** @return Submitted sample position minus delay, clamped to zero. */
    long long presentationCenterSample() const;
};

/**
 * Output-side cursor over decoded PCM history.
 *
 * Audio passthrough owns this object. It advances independently of acquisition
 * and resynchronizes if it falls behind the history retained by
 * DecodedAudioHistory.
 */
class AudioOutputStream {
    const DecodedAudioHistory& history;
    AudioPlaybackClock* playbackClock;
    long long submittedEndSample;
    mutable std::mutex mutex;

public:
    /**
     * Creates an output cursor over decoded history.
     *
     * @param history_ Decoded PCM history to read from. The referenced object
     *        must outlive this stream.
     * @param playbackClock_ Optional clock to publish committed output
     *        positions to. The referenced object must outlive this stream.
     */
    AudioOutputStream(const DecodedAudioHistory& history_,
        AudioPlaybackClock* playbackClock_ = NULL);

    /** Resets the submitted cursor to the given absolute sample-frame position. */
    void reset(long long samplePosition = 0);

    /** @return Bytes per interleaved PCM sample frame in the backing history. */
    int bytesPerSample() const;

    /** @return PCM format in the backing history. */
    const PcmFormat& format() const;

    /** @return Absolute decoded sample-frame position available through history. */
    long long decodedEndPosition() const;

    /** @return Absolute sample-frame position after the last output submission. */
    long long submittedEndPosition() const;

    /**
     * @return Sample frames currently available to submit from this cursor.
     */
    int queuedForOutputSamples() const;

    /**
     * Copies queued PCM without advancing the submitted cursor.
     *
     * @param dst Destination buffer for PCM bytes.
     * @param samples Maximum number of sample frames to copy.
     * @return Number of sample frames copied.
     */
    int peekForOutput(char* dst, int samples) const;

    /**
     * Marks PCM as submitted to the output backend.
     *
     * @param samples Number of sample frames accepted by output.
     * @return Number of sample frames committed.
     */
    int commitOutputSamples(int samples);

    /**
     * Moves the cursor to the oldest retained sample when it has fallen behind.
     *
     * @return Number of skipped sample frames.
     */
    int resyncIfBehind();
};

class AudioFrameBuilder {
    char* rawData;
    int rawCapacity;
    LogSink& log;

    void setRawCapacity(int rawBytes);
    void convert(char2* dst, void* src, int n, const PcmFormat& format);

public:
    /**
     * Creates a reusable frame builder.
     *
     * @param log_ Sink for frame-builder diagnostics. The referenced object
     *        must outlive this builder.
     */
    explicit AudioFrameBuilder(LogSink& log_);

    /** Releases temporary conversion storage. */
    ~AudioFrameBuilder();

    /**
     * Builds Cthugha's signed 8-bit stereo visual-audio frame.
     *
     * @param frame Frame object to populate.
     * @param history Decoded PCM history to sample from.
     * @param centerSample Absolute sample-frame position centered in frame.
     */
    void build(AudioFrame& frame, const DecodedAudioHistory& history,
        long long centerSample);
};

class PcmSource {
protected:
    LogSink& log;
    int error;
    PcmFormat pcmFormat;

public:
    /**
     * Creates a source with no negotiated format and no error.
     *
     * @param log_ Sink for source diagnostics. The referenced object must
     *        outlive the source.
     */
    explicit PcmSource(LogSink& log_);

    /** Releases source resources. */
    virtual ~PcmSource();

    /** @return Nonzero when source construction or negotiation failed. */
    int hasError() const { return error; }

    /** @return Source-negotiated PCM format. */
    const PcmFormat& format() const { return pcmFormat; }

    /**
     * Reads PCM from the source into the caller's raw buffer.
     *
     * @param dst Destination buffer for raw PCM bytes.
     * @param rawSize Capacity of dst in bytes.
     * @param samplesRequested Desired number of sample frames.
     * @return Number of sample frames read into dst.
     */
    virtual int read(char* dst, int rawSize, int samplesRequested) = 0;

    /**
     * Computes a raw byte-buffer size for a requested visual frame.
     *
     * @param frameRawSize Current visual frame raw size in bytes.
     * @param samplesRequested Desired number of sample frames.
     * @return Required byte capacity for a read() call.
     */
    virtual int rawBufferSize(int frameRawSize, int samplesRequested) const;

    /** @return Nonzero when a zero-sample read means finite EOF. */
    virtual int canFinish() const { return 0; }

    /** Rewinds finite input to the beginning when supported. */
    virtual void rewind() { }

    /** Refreshes or reopens source-local state after runtime reset. */
    virtual void update() { }
};

class AudioInput {
    int error;
    LogSink& log;
    PcmSource* source;
    int sourceOwned;
    int loopEnabledValue;
    int finished;

public:
    /**
     * Wraps a PCM source for runtime reads.
     *
     * @param source PCM source to read from.
     * @param log Sink for input wrapper diagnostics.
     * @param takeOwnership Nonzero to delete source in the AudioInput destructor.
     * @param loopEnabled Nonzero to rewind finite sources at end-of-input.
     */
    AudioInput(PcmSource* source, LogSink& log, int takeOwnership = 1,
        int loopEnabled = 0);

    /** Releases the wrapped source when ownership was transferred. */
    ~AudioInput();

    /** @return Nonzero when the wrapped source could not be used. */
    int hasError() const { return error; }
    /** @return Negotiated PCM format from the wrapped source. */
    const PcmFormat& format() const;

    /**
     * Reads from the wrapped source.
     *
     * @param dst Destination buffer for raw PCM bytes.
     * @param rawSize Capacity of dst in bytes.
     * @param samplesRequested Desired number of sample frames.
     * @return Number of sample frames read into dst.
     */
    int read(char* dst, int rawSize, int samplesRequested);

    /**
     * @param frameRawSize Current visual frame raw size in bytes.
     * @param samplesRequested Desired number of sample frames.
     * @return Required byte capacity for a read() call.
     */
    int rawBufferSize(int frameRawSize, int samplesRequested) const;

    /** @return Nonzero once finite input has reached EOF. */
    int isFinished() const;

    /** Refreshes the wrapped source and clears EOF state. */
    void update();
};

class WavPcmSource : public PcmSource {
    char name[PATH_MAX];
    FILE* file;
    long dataStart;
    long dataLength;
    long dataRead;

    int open();
    int readChunkHeader(char id[4], unsigned int& size);
    int parseHeader();

public:
    /**
     * Opens a WAV PCM file source.
     *
     * @param name Filesystem path to a WAV file.
     * @param log Sink for source diagnostics.
     */
    WavPcmSource(const char* name, LogSink& log);

    /** Closes the WAV file handle. */
    virtual ~WavPcmSource();

    /** Reads interleaved PCM samples from the WAV data chunk. */
    virtual int read(char* dst, int rawSize, int samplesRequested);

    /** @return Nonzero because WAV input is finite. */
    virtual int canFinish() const;

    /** Rewinds to the start of the WAV data chunk. */
    virtual void rewind();
};

class Minimp3PcmSource : public PcmSource {
    char name[PATH_MAX];
    void* decoder;

    int open();
    int applyFormat();

public:
    /**
     * Opens an MP3 file source using minimp3.
     *
     * @param name Filesystem path to an MP3 file.
     * @param log Sink for source diagnostics.
     */
    Minimp3PcmSource(const char* name, LogSink& log);

    /** Closes the minimp3 decoder. */
    virtual ~Minimp3PcmSource();

    /** Reads decoded interleaved PCM samples from the MP3 stream. */
    virtual int read(char* dst, int rawSize, int samplesRequested);

    /** @return Nonzero because MP3 file input is finite. */
    virtual int canFinish() const;

    /** Rewinds the MP3 decoder to the beginning. */
    virtual void rewind();
};

class RawPcmSource : public PcmSource {
    char name[PATH_MAX];
    FILE* file;

    int open();
    int applyFormat();

public:
    /**
     * Opens a raw PCM file source.
     *
     * @param name Filesystem path to raw PCM data in the current sound-format.
     * @param format Headerless PCM format to assign to the source.
     * @param log Sink for source diagnostics.
     */
    RawPcmSource(const char* name, const PcmFormat& format, LogSink& log);

    /** Closes the raw PCM file handle. */
    virtual ~RawPcmSource();

    /** Reads interleaved PCM samples using the explicit source format. */
    virtual int read(char* dst, int rawSize, int samplesRequested);

    /** @return Nonzero because raw file input is finite. */
    virtual int canFinish() const;

    /** Rewinds to the beginning of the raw PCM file. */
    virtual void rewind();
};

class RandomNoisePcmSource : public PcmSource {
    RandomSource& randomSource;
    int v1;
    int v2;
    int maxdv;

public:
    /**
     * Creates synthetic noise input for an explicit session format.
     *
     * @param requestedFormat Startup/session format; sampleRate is retained,
     *        while noise itself is generated as unsigned 8-bit stereo.
     * @param randomSource_ Random source used to generate the synthetic walk.
     *        The referenced object must outlive this source.
     * @param log Sink for source diagnostics.
     */
    RandomNoisePcmSource(const PcmFormat& requestedFormat,
        RandomSource& randomSource_, LogSink& log);

    /** Generates unsigned 8-bit stereo noise samples. */
    virtual int read(char* dst, int rawSize, int samplesRequested);

    /** Resets the synthetic noise walk state. */
    virtual void rewind();
};

class AudioProcessor {
    FixedPointAudioFftProcessor defaultFftProcessorValue;
    AudioFftProcessor* fftProcessorValue;
    int sampleRateHzValue;

public:
    /** Creates an audio processor using the default FFT implementation. */
    AudioProcessor();

    /**
     * Creates an audio processor using the default FFT implementation.
     *
     * @param sampleRateHz PCM sample rate used for frequency-domain metrics.
     */
    explicit AudioProcessor(int sampleRateHz);

    /**
     * Creates an audio processor with an injected FFT implementation.
     *
     * @param fftProcessor FFT processor used by fft() calls. The referenced
     *        object must outlive this AudioProcessor.
     */
    explicit AudioProcessor(AudioFftProcessor& fftProcessor);

    /**
     * Creates an audio processor with an injected FFT implementation.
     *
     * @param fftProcessor FFT processor used by fft() calls. The referenced
     *        object must outlive this AudioProcessor.
     * @param sampleRateHz PCM sample rate used for frequency-domain metrics.
     */
    AudioProcessor(AudioFftProcessor& fftProcessor, int sampleRateHz);

    /**
     * Measures one signed 8-bit stereo audio frame.
     *
     * @param frame Pointer to 1024 stereo samples in Cthugha's char2 format.
     * @param minNoise Noise-floor threshold used to set AudioMetrics::noisy.
     * @return Frame-local RMS amplitudes, low-pass RMS amplitudes, and
     *         noisy/quiet flag.
     */
    AudioMetrics analyze(const char2* frame, int minNoise) const;

    /**
     * Measures AudioFrame::raw and writes AudioFrame::metrics.
     *
     * @param frame Frame whose raw samples should be analyzed.
     * @param minNoise Noise-floor threshold used to set AudioMetrics::noisy.
     */
    void analyze(AudioFrame& frame, int minNoise) const;

    /**
     * Copies raw samples into processedWaveData without filtering.
     *
     * @param frame Frame whose raw data should become processed data.
     */
    void none(AudioFrame& frame);

    /**
     * Applies the first smoothing filter to frame raw samples.
     *
     * @param frame Frame whose processed data should be populated.
     */
    void filter1(AudioFrame& frame);

    /**
     * Applies the second smoothing filter to frame raw samples.
     *
     * @param frame Frame whose processed data should be populated.
     */
    void filter2(AudioFrame& frame);

    /**
     * Applies FFT visualization processing to frame raw samples.
     *
     * @param frame Frame whose processed data should be populated.
     */
    void fft(AudioFrame& frame);

    /**
     * Copies a raw 1024-sample stereo frame without filtering.
     *
     * @param raw Source samples in signed 8-bit stereo Cthugha format.
     * @param processedWaveData Destination buffer for 1024 processed samples.
     */
    void none(char2* raw, char2* processedWaveData);
    /**
     * Applies the first smoothing filter to raw samples.
     *
     * @param raw Source samples in signed 8-bit stereo Cthugha format.
     * @param processedWaveData Destination buffer for 1024 processed samples.
     */
    void filter1(char2* raw, char2* processedWaveData);
    /**
     * Applies the second smoothing filter to raw samples.
     *
     * @param raw Source samples in signed 8-bit stereo Cthugha format.
     * @param processedWaveData Destination buffer for 1024 processed samples.
     */
    void filter2(char2* raw, char2* processedWaveData);
    /**
     * Applies FFT visualization processing to raw samples.
     *
     * @param raw Source samples in signed 8-bit stereo Cthugha format.
     * @param processedWaveData Destination buffer for 1024 processed samples.
     */
    void fft(char2* raw, char2* processedWaveData);
};

class MiniAudioCapturePcmSource : public PcmSource {
    MiniAudioCapturePcmSourceState* state;

    int open();
    void close();

public:
    /**
     * Opens a miniaudio capture device.
     *
     * Uses the configured miniaudio capture device name when one is supplied,
     * otherwise opens the platform default capture device.
     *
     * @param settings Desired live-input PCM format.
     * @param log_ Sink for capture diagnostics.
     * @param autoOpen Nonzero to open miniaudio immediately.
     */
    explicit MiniAudioCapturePcmSource(const AudioSettings& settings,
        LogSink& log_, int autoOpen = 1);

    /** Stops capture and releases miniaudio resources. */
    virtual ~MiniAudioCapturePcmSource();

    /** Reads captured PCM frames from the miniaudio callback ring buffer. */
    virtual int read(char* dst, int rawSize, int samplesRequested);

    /** @return Byte capacity required for a capture read. */
    virtual int rawBufferSize(int frameRawSize, int samplesRequested) const;

    /** Reopens the miniaudio capture device using the negotiated format. */
    virtual void update();

    /** Writes capture callback frames into this source's PCM ring buffer. */
    void writeCapturedFrames(const void* input, unsigned int frames);

    /** @return Nonzero when the capture device is open. */
    int isOpen() const;

    /** @return Number of capture callback overruns observed by this instance. */
    int overrunCount() const;

    /** @return Human-readable miniaudio backend name selected for capture. */
    const char* backendName() const;

#ifdef AUDIO_CAPTURE_TEST_HOOKS
    static int testBackendNameIsNull(const char* backendName);
    void testInitializeRingWithoutDevice(int ringFrames);
    void testInitializeRingWithoutDeviceFormat(int sampleRate, int channels,
        int sampleFormat, int ringFrames);
    void testWriteCapturedFrames(const void* input, unsigned int frames);
    void testLogConnectedDiagnostics(const char* backendName,
        const char* deviceName, int sampleRate, int channels,
        int sampleFormat, int ringFrames);
    int testAvailableReadFrames() const;
    void testSetError(int value);
#endif
};

class DspPcmSource : public PcmSource {
    int handle;
    char* dmaBuffer;
    int dmaSize;
    int method;
    int dspFragmentsValue;
    int dspFragmentSizeValue;
    int dspSyncEnabledValue;
    char dspDevicePathValue[PATH_MAX];
    int sampleWindow;

    void setFragment();
    void setChannels();
    void setSampleRate();
    void setFormat();
    void init();

public:
    /**
     * Opens the OSS/DSP input source.
     *
     * @param settings Audio startup/session settings to negotiate locally.
     * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
     *        before display zoom. Used to size the DSP input sample window.
     * @param log Sink for source diagnostics.
     */
    DspPcmSource(const AudioSettings& settings, int visualMaxDimension,
        LogSink& log);

    /** Closes OSS/DSP input resources. */
    virtual ~DspPcmSource();

    /** Reads PCM samples from the OSS/DSP capture device. */
    virtual int read(char* dst, int rawSize, int samplesRequested);

    /** Computes the raw input buffer size needed by the selected DSP method. */
    virtual int rawBufferSize(int frameRawSize, int samplesRequested) const;

    /** Reopens and renegotiates the OSS/DSP capture device. */
    virtual void update();
};

#endif
