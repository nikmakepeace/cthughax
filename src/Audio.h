// New audio composition interfaces.
//
// These classes are intentionally separate from SoundDevice*.  They provide a
// target shape for the new startup path while the legacy sound backends remain
// intact during migration.

#ifndef __AUDIO_H
#define __AUDIO_H

#include "SoundDevice.h"
#include "AudioFrame.h"

#include <stdio.h>
#include <limits.h>
#include <atomic>
#include <mutex>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

class AudioBuffer;

static inline int pcmBytesForSamples(int samples, int bytesPerSample) {
    return samples * bytesPerSample;
}

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

    virtual int write(const void* buffer, int size) = 0;
    virtual int outputDelayBytes() const = 0;
    virtual int getHandle() const { return -1; }
    virtual int isOpen() const = 0;
    virtual int isRealtime() const { return 1; }
    virtual void update() { }
    virtual void configureTiming(int samplesPerSecond, int bytesPerSample, int inputChunkSamples);
    int samplesPerSecond() const { return outputSamplesPerSecond; }
    int bytesPerSample() const { return outputBytesPerSample; }
    int targetDelaySamples() const { return outputTargetDelaySamples; }
    int scratchSamples() const { return outputScratchSamples; }
    int outputDelaySamples() const;
    int queuedTargetSamples() const;
    virtual long long audibleSamplePosition(const AudioBuffer& buffer) const;
    virtual int outputDrained() const;
    int playbackComplete(const AudioBuffer& buffer, int inputFinished) const;
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
    virtual int outputDelayBytes() const;
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
    std::atomic<long long> firstSubmittedSample;
    std::atomic<long long> lastSubmittedSample;
    std::atomic<int> lastReportedUnderflows;

    void closePulse();
    int writeUnlocked(const void* buffer, int size, int waitForWritable);
    int latencyBytesUnlocked() const;
    int drainUnlocked(size_t requestedBytes);

protected:
    virtual int defaultTargetLatencyMs() const;
    virtual int timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const;

public:
    AudioPulseOutput();
    virtual ~AudioPulseOutput();

    virtual int write(const void* buffer, int size);
    virtual int outputDelayBytes() const;
    virtual int isOpen() const;
    virtual int isRealtime() const;
    virtual int outputDrained() const;
    virtual long long audibleSamplePosition(const AudioBuffer& buffer) const;
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

    void setFragment();
    void setChannels();
    void setSampleRate();
    void setFormat();
    void init();

protected:
    virtual int defaultTargetLatencyMs() const;
    virtual int timingScratchSamples(int inputChunkSamples, int targetDelaySamples) const;

public:
    AudioDSPOutput(int method);
    virtual ~AudioDSPOutput();

    virtual int write(const void* buffer, int size);
    virtual int outputDelayBytes() const;
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
    AudioBuffer(int capacitySamples, int bytesPerSample, int protectedHistorySamples = 0);
    ~AudioBuffer();

    int bytesPerSample() const { return bytesPerSampleValue; }
    int queuedForOutputSamples() const;
    int protectedWindowSamples() const;
    int writableSamples() const;
    long long decodedEndPosition() const;
    long long submittedEndPosition() const;
    void clear();

    int appendDecodedPcm(const char* src, int samples);
    int peekForOutput(char* dst, int samples) const;
    int commitOutputSamples(int samples);
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

    void build(AudioFrame& frame, const AudioBuffer& buffer, long long centerSample);
};

struct PcmFormat {
    int sampleRate;
    int channels;
    int sampleFormat;

    PcmFormat()
        : sampleRate(0)
        , channels(0)
        , sampleFormat(SF_u8) { }

    int bytesPerSample() const { return (sampleFormat < 2) ? channels : 2 * channels; }
    int bytesForSamples(int samples) const { return pcmBytesForSamples(samples, bytesPerSample()); }
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

    virtual int read(char* dst, int rawSize, int samplesRequested) = 0;
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
    AudioInput(PcmSource* source, int takeOwnership = 1);
    ~AudioInput();

    int hasError() const { return error; }
    int read(char* dst, int rawSize, int samplesRequested);
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
    Minimp3PcmSource(const char* name);
    virtual ~Minimp3PcmSource();

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
    void none(AudioFrame& frame);
    void filter1(AudioFrame& frame);
    void filter2(AudioFrame& frame);
    void fft(AudioFrame& frame);

    void none(char2* data, char2* processed);
    void filter1(char2* data, char2* processed);
    void filter2(char2* data, char2* processed);
    void fft(char2* data, char2* processed);
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

    AudioInputProcessor(AudioInput* input, int takeOwnership = 1);
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
    DspPcmSource();
    virtual ~DspPcmSource();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual int rawBufferSize(int frameRawSize, int samplesRequested) const;
    virtual void update();
    virtual int initInputControls();
};

#endif
