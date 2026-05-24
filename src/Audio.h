// New audio composition interfaces.
//
// These classes are intentionally separate from SoundDevice*.  They provide a
// target shape for the new startup path while the legacy sound backends remain
// intact during migration.

#ifndef __AUDIO_H
#define __AUDIO_H

#include "SoundDevice.h"

#include <stdio.h>
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

class AudioInput {
protected:
    int error;

public:
    AudioInput();
    virtual ~AudioInput();

    int hasError() const { return error; }

    virtual int read(char* dst, int rawSize, int samplesRequested) = 0;
    virtual int rawBufferSize(int frameRawSize, int samplesRequested) const;
    virtual void update() { }
    virtual int initInputControls() { return 0; }
};

class AudioOutput {
public:
    virtual ~AudioOutput();

    virtual int write(const void* buffer, int size) = 0;
    virtual int outputDelayBytes() const = 0;
    virtual int getHandle() const { return -1; }
    virtual int isOpen() const = 0;
    virtual int isRealtime() const { return 1; }
    virtual void update() { }
};

class AudioNullOutput : public AudioOutput {
public:
    virtual int write(const void* buffer, int size);
    virtual int outputDelayBytes() const;
    virtual int isOpen() const;
    virtual int isRealtime() const;
};

class AudioPulseOutput : public AudioOutput {
    void* pulse;
    int bytesPerSecondValue;

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
    int readPos;
    int writePos;
    int fill;

public:
    AudioBuffer(int capacity);
    ~AudioBuffer();

    int available() const { return fill; }
    int freeSpace() const { return capacity - fill; }
    int size() const { return capacity; }
    void clear();

    int write(const char* src, int bytes);
    int read(char* dst, int bytes);
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

class PcmSource {
protected:
    int error;
    PcmFormat pcmFormat;

public:
    PcmSource();
    virtual ~PcmSource();

    int hasError() const { return error; }
    const PcmFormat& format() const { return pcmFormat; }

    virtual int read(char* dst, int bytes) = 0;
    virtual void rewind() = 0;
};

class AudioPcmInput : public AudioInput {
    PcmSource* source;
    int sourceOwned;

    void applyFormat();

public:
    AudioPcmInput(PcmSource* source, int takeOwnership = 1);
    virtual ~AudioPcmInput();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual void update();
};

class WavAudioSource : public PcmSource {
    char name[PATH_MAX];
    FILE* file;
    long dataStart;
    long dataLength;
    long dataRead;

    int open();
    int readChunkHeader(char id[4], unsigned int& size);
    int parseHeader();

public:
    WavAudioSource(const char* name);
    virtual ~WavAudioSource();

    virtual int read(char* dst, int bytes);
    virtual void rewind();
};

class AudioProcessor {
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

    AudioProcessor(AudioInput* input, int takeOwnership = 1);
    ~AudioProcessor();

    AudioInput* audioInput() { return input; }

    void operator()();
    void change();
    int frameRawSize() const { return rawSize; }
};

class AudioRandomInput : public AudioInput {
    int v1;
    int v2;
    int maxdv;

public:
    AudioRandomInput();

    virtual int read(char* dst, int rawSize, int samplesRequested);
    virtual void update();
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
