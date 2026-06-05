// Audio input, output, buffering, and frame conversion interfaces.

#ifndef __AUDIO_H
#define __AUDIO_H

#include "AudioOptions.h"
#include "AudioFrame.h"

#include <stdio.h>
#include <limits.h>
#include <atomic>
#include <mutex>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

class AudioBuffer;

class AudioOutput {
    int outputSamplesPerSecond;
    int outputBytesPerSample;
    int outputTargetDelaySamples;
    int outputScratchSamples;

protected:
    virtual int defaultTargetLatencyMs() const = 0;
    virtual int timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const;

public:
    AudioOutput();
    virtual ~AudioOutput();

    /**
     * Writes raw PCM bytes to the output backend.
     *
     * @param buffer Pointer to interleaved PCM data in the current output format.
     * @param size Number of bytes available at buffer.
     * @return Number of bytes accepted, or 0/nonpositive on backend failure.
     */
    virtual int write(const void* buffer, int size) = 0;
    virtual int getHandle() const { return -1; }
    virtual int isOpen() const = 0;
    virtual int isRealtime() const { return 1; }
    virtual void update() { }

    /**
     * Configures output pacing and scratch-buffer sizing.
     *
     * @param samplesPerSecond Output sample rate in Hertz.
     * @param bytesPerSample Bytes per interleaved output sample frame.
     * @param inputChunkSamples Number of samples produced per input service chunk.
     */
    virtual void configureTiming(int samplesPerSecond, int bytesPerSample, int inputChunkSamples);
    int samplesPerSecond() const { return outputSamplesPerSecond; }
    int bytesPerSample() const { return outputBytesPerSample; }
    int targetDelaySamples() const { return outputTargetDelaySamples; }
    int scratchSamples() const { return outputScratchSamples; }
    int queuedTargetSamples() const;
    int playbackComplete(const AudioBuffer& buffer, int inputFinished) const;

    /**
     * Moves queued decoded PCM toward the output backend.
     *
     * @param buffer Shared decoded-audio buffer to consume from.
     * @param scratch Temporary PCM buffer owned by the runtime.
     * @param scratchSamples Capacity of scratch in sample frames.
     * @param inputFinished Nonzero when the input source has no more samples.
     * @return Nonzero when at least one sample frame was submitted to output.
     */
    virtual int service(AudioBuffer& buffer, char* scratch, int scratchSamples,
        int inputFinished);
    virtual int supportsCallbackDrain() const { return 0; }
    virtual void startCallbackDrain(AudioBuffer&, const std::atomic<int>*, int) { }
    virtual void notifyCallbackDrain() { }
    virtual void stopCallbackDrain() { }
};

class AudioNullOutput : public AudioOutput {
protected:
    virtual int defaultTargetLatencyMs() const;

public:
    virtual int write(const void* buffer, int size);
    virtual int isOpen() const;
    virtual int isRealtime() const;
};

class AudioPulseOutput : public AudioOutput {
    void* mainloop;
    void* context;
    void* stream;
    void* drainEvent;
    AudioBuffer* callbackBuffer;
    const std::atomic<int>* callbackInputFinished;
    char* callbackScratch;
    int callbackScratchSamples;
    std::atomic<int> callbackDrainActive;
    int bytesPerSecondValue;
    std::atomic<int> lastReportedUnderflows;

    void closePulse();
    int writeUnlocked(const void* buffer, int size, int waitForWritable);
    int drainUnlocked(size_t requestedBytes);

protected:
    virtual int defaultTargetLatencyMs() const;
    virtual int timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const;

public:
    AudioPulseOutput();
    virtual ~AudioPulseOutput();

    virtual int write(const void* buffer, int size);
    virtual int isOpen() const;
    virtual int isRealtime() const;
    virtual void update();
    virtual int service(AudioBuffer& buffer, char* scratch, int scratchSamples,
        int inputFinished);
    virtual int supportsCallbackDrain() const;
    virtual void startCallbackDrain(AudioBuffer& buffer,
        const std::atomic<int>* inputFinished, int scratchSamples);
    virtual void notifyCallbackDrain();
    virtual void stopCallbackDrain();
    void pulseWritable(size_t requestedBytes);
    void pulseUnderflow();
};

class AudioDSPOutput : public AudioOutput {
    int handle;
    int method;
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
     * @param method DSP output method id from the sound-method option.
     * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
     *        before display zoom. Used to choose a DSP fragment/sample window.
     */
    AudioDSPOutput(int method, int visualMaxDimension);
    virtual ~AudioDSPOutput();

    virtual int write(const void* buffer, int size);
    virtual int getHandle() const;
    virtual int isOpen() const;
    virtual void update();
};

class AudioBuffer {
    char* data;
    int bytesPerSampleValue;
    int capacitySamples;
    int protectedHistorySamples;
    long long decodedEndSample;
    long long submittedEndSample;
    mutable std::mutex mutex;

    long long protectedWindowStartSample() const;
    int copyAt(long long samplePosition, char* dst, int samples) const;

public:
    /**
     * Allocates a decoded PCM ring buffer.
     *
     * @param capacitySamples Writable capacity in sample frames.
     * @param bytesPerSample Bytes per interleaved PCM sample frame.
     * @param protectedHistorySamples Number of already-submitted samples to keep
     *        available for visual-frame reconstruction.
     */
    AudioBuffer(int capacitySamples, int bytesPerSample, int protectedHistorySamples = 0);
    ~AudioBuffer();

    int bytesPerSample() const { return bytesPerSampleValue; }
    int queuedForOutputSamples() const;
    int protectedWindowSamples() const;
    int writableSamples() const;
    long long oldestProtectedPosition() const;
    long long decodedEndPosition() const;
    long long submittedEndPosition() const;
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
     * Copies queued PCM for output without advancing the submitted position.
     *
     * @param dst Destination buffer for PCM bytes.
     * @param samples Maximum number of sample frames to copy.
     * @return Number of sample frames copied.
     */
    int peekForOutput(char* dst, int samples) const;

    /**
     * Marks queued PCM as submitted to the output backend.
     *
     * @param samples Number of sample frames accepted by output.
     * @return Number of sample frames committed.
     */
    int commitOutputSamples(int samples);

    /**
     * Reads PCM from the protected history window.
     *
     * @param samplePosition Absolute sample-frame position to read from.
     * @param dst Destination buffer for PCM bytes.
     * @param samples Maximum number of sample frames to copy.
     * @return Number of sample frames copied from protected history.
     */
    int readProtectedPcmAt(long long samplePosition, char* dst, int samples) const;
};

class AudioFrameBuilder {
    char* rawData;
    int rawCapacity;

    void setRawCapacity(int rawBytes);
    void convert(char2* dst, void* src, int n);

public:
    AudioFrameBuilder();
    ~AudioFrameBuilder();

    /**
     * Builds Cthugha's signed 8-bit stereo visual-audio frame.
     *
     * @param frame Frame object to populate.
     * @param buffer Decoded PCM buffer to sample from.
     * @param centerSample Absolute sample-frame position centered in frame.
     */
    void build(AudioFrame& frame, const AudioBuffer& buffer, long long centerSample);
};

class PcmSource {
protected:
    int error;
    PcmFormat pcmFormat;

public:
    PcmSource();
    virtual ~PcmSource();

    int hasError() const { return error; }
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
    virtual int canFinish() const { return 0; }
    virtual void rewind() { }
    virtual void update() { }
    virtual int initInputControls() { return 0; }
};

class AudioInput {
    int error;
    PcmSource* source;
    int sourceOwned;
    int finished;

    void applyFormat();

public:
    /**
     * Wraps a PCM source for runtime reads.
     *
     * @param source PCM source to read from.
     * @param takeOwnership Nonzero to delete source in the AudioInput destructor.
     */
    AudioInput(PcmSource* source, int takeOwnership = 1);
    ~AudioInput();

    int hasError() const { return error; }

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
    int isFinished() const;
    void update();
    int initInputControls();
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
     */
    WavPcmSource(const char* name);
    virtual ~WavPcmSource();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual int canFinish() const;
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
     */
    Minimp3PcmSource(const char* name);
    virtual ~Minimp3PcmSource();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual int canFinish() const;
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
     */
    RawPcmSource(const char* name);
    virtual ~RawPcmSource();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual int canFinish() const;
    virtual void rewind();
};

class RandomNoisePcmSource : public PcmSource {
    int v1;
    int v2;
    int maxdv;

public:
    RandomNoisePcmSource();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual void rewind();
};

class AudioProcessor {
public:
    /**
     * Measures one signed 8-bit stereo audio frame.
     *
     * @param frame Pointer to 1024 stereo samples in Cthugha's char2 format.
     * @param minNoise Noise-floor threshold used to set AudioMetrics::noisy.
     * @return Frame-local RMS amplitudes and noisy/quiet flag.
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

class AudioInputProcessor {
    AudioInput* input;
    int inputOwned;

    char* tmpData;
    int tmpSize;
    int rawSize;
    int bytesPerSample;

    void setTmpData();
    void convert(char2* dst, void* src, int n);

public:
    int size;
    char2* data;
    char2 dataProc[1024];

    /**
     * Creates the live-input processor and sample window.
     *
     * @param input Audio input wrapper to read from.
     * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
     *        before display zoom. Used to choose the processor sample window.
     * @param takeOwnership Nonzero to delete input in the processor destructor.
     */
    AudioInputProcessor(AudioInput* input, int visualMaxDimension, int takeOwnership = 1);
    ~AudioInputProcessor();

    AudioInput* audioInput() { return input; }

    void operator()();
    void change();
    int frameRawSize() const { return rawSize; }
};

class DspPcmSource : public PcmSource {
    int handle;
    char* dmaBuffer;
    int dmaSize;
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
     * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
     *        before display zoom. Used to size the DSP input sample window.
     */
    DspPcmSource(int visualMaxDimension);
    virtual ~DspPcmSource();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual int rawBufferSize(int frameRawSize, int samplesRequested) const;
    virtual void update();
    virtual int initInputControls();
};

#endif
