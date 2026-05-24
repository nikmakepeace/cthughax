// New audio composition interfaces.
//
// These classes are intentionally separate from SoundDevice*.  They provide a
// target shape for the new startup path while the legacy sound backends remain
// intact during migration.

#ifndef __AUDIO_H
#define __AUDIO_H

#include "SoundDevice.h"

class AudioInput {
protected:
    int error;

public:
    AudioInput();
    virtual ~AudioInput();

    int hasError() const { return error; }

    virtual int read(char* dst, int rawSize, int samplesRequested) = 0;
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
    virtual void update() { }
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

#endif
