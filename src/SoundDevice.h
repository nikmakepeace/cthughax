// Sound backend interface and concrete backend declarations.
// The global soundDevice pointer is the current runtime strategy: startup and
// resume create one concrete subclass, the main loop calls operator() each
// frame, and shutdown/suspend deletes it.

#ifndef __SOUND_DEVICE_H
#define __SOUND_DEVICE_H

#include "Option.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern Option& soundDeviceNr;

extern Option& soundFormat;
extern Option& soundChannels;
extern Option& soundSampleRate;

extern Option& soundDSPMethod;
extern Option& soundDSPFragments;
extern Option& soundDSPFragmentSize;
extern Option& soundDSPSync;

extern Option& soundBuffer;
extern Option& soundSilent;

extern int soundPlayLoop;

// Derived from soundFormat and soundChannels for the active backend.
extern int bytesPerSample;

enum soundFormat_t { SF_u8 = 0, SF_s8, SF_u16_le, SF_s16_le, SF_u16_be, SF_s16_be };

// One stereo sample in Cthugha's internal signed 8-bit format.
typedef char char2[2];

class SoundDevice {
protected:
    int error;

    char* tmpData; // Raw backend-format samples read by read().
    int tmpDataOwned; // False when SoundDeviceFork lends shared memory.
    int rawSize; // Bytes requested from the backend for one frame.
    int tmpSize; // Allocated bytes in tmpData.
    void setTmpData();
    void borrowTmpData(char* data);

    void convert(char2* dst, void* src, int n);
public:
    int size; // Samples requested from read().
    char2* data; // Last 1024 converted stereo samples.
    char2 dataProc[1024]; // Post-processing workspace used by waves.

    SoundDevice();
    virtual ~SoundDevice();

    static void newSD();

    // Per-frame tick: read backend data, then convert it into data.
    void operator()();

    void change();

    virtual int read();
    virtual void update() { };

    friend class SoundDeviceFork;
    friend class SoundServer;
};

extern SoundDevice* soundDevice;

enum SoundDeviceNr { SDN_DSPIn, SDN_Net, SDN_Random, SDN_File, SDN_Max };

// OSS /dev/dsp backend shared by reader and writer devices.
class SoundDeviceDSP : public SoundDevice {
protected:
    int handle;
    int mode;
    char* DMAbuffer;

    void setFragment();
    void setChannels();
    void setSampleRate();
    void setFormat();

    SoundDeviceDSP() { }
    void init(int mode);

public:
    static char dev_dsp[];

    SoundDeviceDSP(int mode);
    virtual ~SoundDeviceDSP();

    virtual void update();
};

// Reader side of OSS /dev/dsp, used as the normal live input backend.
class SoundDeviceDSPIn : public SoundDeviceDSP {
public:
    SoundDeviceDSPIn()
        : SoundDeviceDSP(O_RDONLY) { }
    virtual int read();
};

// Writer side of OSS /dev/dsp, used when file playback also feeds the soundcard.
class SoundDeviceDSPOut : public SoundDeviceDSP {
public:
    SoundDeviceDSPOut()
        : SoundDeviceDSP(O_WRONLY) { }

    int write(const void* buffer, int size);
    int outputDelayBytes() const;

    int getHandle() const { return handle; }
};

// Receives sound samples from another Cthugha instance over the network.
class SoundDeviceNet : public SoundDevice {
protected:
    int handle;
    void net_request(int);

public:
    static char sound_hostname[256];

    SoundDeviceNet();
    virtual ~SoundDeviceNet();

    virtual int read();
    virtual void update();
};

// Synthesizes input when no real sound backend is available.
class SoundDeviceRandom : public SoundDevice {
public:
    SoundDeviceRandom();
    virtual int read();
    virtual void update();
};

// Reads sound from files or external decoder programs.
class SoundDeviceFile : public SoundDevice {
protected:
    FILE* file;
    SoundDeviceDSPOut* dsp;
    int bufferPid;
    int childPid;

    unsigned char* buffer;
    int bufferSize;
    int bufferChunkSize;
    int bufferPos;
    int bufferFill;

    unsigned char* playbackHistory;
    int playbackHistorySize;
    long long playbackHistoryWritten;

    int playNext();

    int open();

    int openFile();
    int openProg(char*);
    int close();
    int wavHeader();
    void rememberPlayback(const unsigned char* data, int n);
    int copyPlaybackAtOutputTime(int n);
    void copyPlaybackHistory(long long pos, unsigned char* dst, int n);
    void fillRawSilence(unsigned char* dst, int n);

    SoundDeviceFile(int /*dummy*/)
        : SoundDevice()
        , dsp(NULL)
        , bufferPid(-1)
        , playbackHistory(NULL)
        , playbackHistorySize(0)
        , playbackHistoryWritten(0) { }

public:
    static char name[];
    static char fifo[];
    static char fifoDir[];

    SoundDeviceFile();
    virtual ~SoundDeviceFile();

    virtual int read();
    virtual void update();
};

// Parent-side proxy for forked file playback. The child owns the real file
// backend and writes raw samples through shared memory.
class SoundDeviceFork : public SoundDevice {
    int sound_child;
    int sound_shm_key;
    void* sound_shared;

public:
    SoundDeviceFork();
    virtual ~SoundDeviceFork();

    virtual int read();
    virtual void update();
};

#endif
