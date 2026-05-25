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
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

class AudioBuffer;

class AudioInput {
protected:
    int error;

public:
    AudioInput();
    virtual ~AudioInput();

    int hasError() const { return error; }

    virtual int read(char* dst, int rawSize, int samplesRequested) = 0;
    virtual int rawBufferSize(int frameRawSize, int samplesRequested) const;
    virtual int isFinished() const { return 0; }
    virtual void update() { }
    virtual int initInputControls() { return 0; }
};

class AudioOutput {
    int outputBytesPerSecond;
    int outputTargetDelayBytes;
    int outputScratchBytes;

protected:
    virtual int defaultTargetLatencyMs() const = 0;
    virtual int timingScratchBytes(int inputChunkBytes, int targetDelayBytes) const;

public:
    AudioOutput();
    virtual ~AudioOutput();

    virtual int write(const void* buffer, int size) = 0;
    virtual int outputDelayBytes() const = 0;
    virtual int getHandle() const { return -1; }
    virtual int isOpen() const = 0;
    virtual int isRealtime() const { return 1; }
    virtual void update() { }
    virtual void configureTiming(int bytesPerSecond, int inputChunkBytes);
    int targetDelayBytes() const { return outputTargetDelayBytes; }
    int scratchBytes() const { return outputScratchBytes; }
    int queuedTargetBytes() const;
    long long audibleBytePosition(const AudioBuffer& buffer) const;
    int playbackComplete(const AudioBuffer& buffer, int inputFinished) const;
    virtual int service(AudioBuffer& buffer, char* scratch, int scratchSize,
        int inputFinished);
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
    void* pulse;
    int bytesPerSecondValue;

protected:
    virtual int defaultTargetLatencyMs() const;
    virtual int timingScratchBytes(int inputChunkBytes, int targetDelayBytes) const;

public:
    AudioPulseOutput();
    virtual ~AudioPulseOutput();

    virtual int write(const void* buffer, int size);
    virtual int outputDelayBytes() const;
    virtual int isOpen() const;
    virtual int isRealtime() const;
    virtual void update();
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
    virtual int timingScratchBytes(int inputChunkBytes, int targetDelayBytes) const;

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
    int capacity;
    int protectedHistoryBytes;
    long long decodedEndByte;
    long long submittedEndByte;

    long long protectedWindowStartByte() const;
    int copyAt(long long bytePosition, char* dst, int bytes) const;

public:
    AudioBuffer(int capacity, int protectedHistoryBytes = 0);
    ~AudioBuffer();

    int queuedForOutputBytes() const { return int(decodedEndByte - submittedEndByte); }
    int protectedWindowBytes() const { return int(decodedEndByte - protectedWindowStartByte()); }
    int writableBytes() const { return capacity - protectedWindowBytes(); }
    int size() const { return capacity; }
    long long protectedWindowStartPosition() const { return protectedWindowStartByte(); }
    long long decodedEndPosition() const { return decodedEndByte; }
    long long submittedEndPosition() const { return submittedEndByte; }
    void clear();

    int appendDecodedPcm(const char* src, int bytes);
    int readForOutput(char* dst, int bytes);
    int readProtectedPcmAt(long long bytePosition, char* dst, int bytes) const;
};

class AudioFrameBuilder {
    char* rawData;
    int rawCapacity;

    void setRawCapacity(int rawBytes);
    void convert(char2* dst, void* src, int n);

public:
    AudioFrameBuilder();
    ~AudioFrameBuilder();

    void build(AudioFrame& frame, const AudioBuffer& buffer, long long centerByte);
};

struct PcmFormat {
    int sampleRate;
    int channels;
    int sampleFormat;

    PcmFormat()
        : sampleRate(0)
        , channels(0)
        , sampleFormat(SF_u8) { }
};

class PcmDriver {
protected:
    int error;
    PcmFormat pcmFormat;

public:
    PcmDriver();
    virtual ~PcmDriver();

    int hasError() const { return error; }
    const PcmFormat& format() const { return pcmFormat; }

    virtual int read(char* dst, int bytes) = 0;
    virtual void rewind() = 0;
};

class PcmSource {
    PcmDriver* driver;
    int driverOwned;

public:
    PcmSource(PcmDriver* driver, int takeOwnership = 1);
    ~PcmSource();

    int hasError() const;
    const PcmFormat& format() const;
    int read(char* dst, int bytes);
    void rewind();
};

class AudioPcmInput : public AudioInput {
    PcmSource* source;
    int sourceOwned;
    int finished;

    void applyFormat();

public:
    AudioPcmInput(PcmSource* source, int takeOwnership = 1);
    virtual ~AudioPcmInput();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual int isFinished() const;
    virtual void update();
};

class WavPcmDriver : public PcmDriver {
    char name[PATH_MAX];
    FILE* file;
    long dataStart;
    long dataLength;
    long dataRead;

    int open();
    int readChunkHeader(char id[4], unsigned int& size);
    int parseHeader();

public:
    WavPcmDriver(const char* name);
    virtual ~WavPcmDriver();

    virtual int read(char* dst, int bytes);
    virtual void rewind();
};

class Minimp3PcmDriver : public PcmDriver {
    char name[PATH_MAX];
    void* decoder;

    int open();
    int applyFormat();

public:
    Minimp3PcmDriver(const char* name);
    virtual ~Minimp3PcmDriver();

    virtual int read(char* dst, int bytes);
    virtual void rewind();
};

class RandomNoisePcmDriver : public PcmDriver {
    int v1;
    int v2;
    int maxdv;

public:
    RandomNoisePcmDriver();

    virtual int read(char* dst, int bytes);
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

class AudioNetInput : public AudioInput {
    int handle;

    void netRequest(int request);

public:
    AudioNetInput();
    virtual ~AudioNetInput();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual int rawBufferSize(int frameRawSize, int samplesRequested) const;
    virtual void update();
};

class AudioDSPInput : public AudioInput {
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
    AudioDSPInput();
    virtual ~AudioDSPInput();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual int rawBufferSize(int frameRawSize, int samplesRequested) const;
    virtual void update();
    virtual int initInputControls();
};

#endif
