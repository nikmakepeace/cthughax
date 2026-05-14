// -*-c++-*-
#ifndef __SOUND_DEVICE_H
#define __SOUND_DEVICE_H

#include "Option.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//
// the sound related options
//
extern Option & soundDeviceNr;			// which sound device to use

extern Option & soundFormat;
extern Option & soundChannels;
extern Option & soundSampleRate;

extern Option & soundDSPMethod;			// DSP Options
extern Option & soundDSPFragments;
extern Option & soundDSPFragmentSize;
extern Option & soundDSPSync;

extern Option & soundBuffer;			// only for File
extern Option & soundSilent;

extern int soundPlayLoop;			// play file(s) over and over again

extern int bytesPerSample;

enum soundFormat_t {
    SF_u8 = 0,
    SF_s8,
    SF_u16_le,
    SF_s16_le,
    SF_u16_be,
    SF_s16_be
};

//
// class SoundDevice
//
// gets the sound from soundcard, file, random noise, ...
// plays sound to soundcard if necessary
// convets sound to cthugha format
//

typedef char char2[2];

class SoundDevice {
protected:
    int error;

    char * tmpData;			// sound data in format read 
    int tmpDataOwned;			// tmpData was allocated with new[]
    int rawSize;			// number of bytes to get
    int tmpSize;			// size of temorary buffer (>= rawSize)
    void setTmpData();
    void borrowTmpData(char * data);

    void convert(char2 * dst, void * src, int n);	// convert tmpData to stereo,signed,8bit
public:
    int size;				// number of samples to read
    char2 * data;			// sound data in stereo, signed, 8 bit
    char2 dataProc[1024];		// sound data after processing

    SoundDevice();
    virtual ~SoundDevice();

    // NOTE: using operator new instead of this newSD is not possible,
    // the virtual functions are not called correctly then.
    // I don't know why that is so.
    static void newSD();		// create a new sound device, create the
					// right device depending on the sound option

    void operator()();

    void change();			// update device after chaning one of the
					// sound device options (sample size, ...)

    virtual int read();			// get sound data (device dependend)
    virtual void update() {};		// update sound device after chainging
					// sample size, ...

    friend class SoundDeviceFork;
    friend class SoundServer;
};

extern SoundDevice * soundDevice;	// current sound device

enum SoundDeviceNr {
    SDN_DSPIn, 
    SDN_Net, 
    SDN_Random, 
    SDN_File, 
    SDN_Max
};




// interface to /dev/dsp
class SoundDeviceDSP : public SoundDevice {
protected:
    int handle;
    int mode;
    char * DMAbuffer;
    
    void setFragment();
    void setChannels();
    void setSampleRate();
    void setFormat();

    SoundDeviceDSP() {}		// only use one of the subclasses
    void init(int mode);
public:
    static char dev_dsp[];

    SoundDeviceDSP(int mode);
    virtual ~SoundDeviceDSP();

    virtual void update();
};

// /dev/dsp only for reading
class SoundDeviceDSPIn : public SoundDeviceDSP {
public:
    SoundDeviceDSPIn() : SoundDeviceDSP(O_RDONLY) {}
    virtual int read();
};

// /dev/dsp only for writing
class SoundDeviceDSPOut : public SoundDeviceDSP {
public:
    SoundDeviceDSPOut() : SoundDeviceDSP(O_WRONLY) {}
    
    int write(const void * buffer, int size);

    int getHandle() const { return handle; }
};

// sound via network
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

// only random noise
class SoundDeviceRandom : public SoundDevice {
public:
    SoundDeviceRandom();
    virtual int read();
    virtual void update();
};

// get sound from a file
class SoundDeviceFile : public SoundDevice {
protected:
    FILE * file;
    SoundDeviceDSPOut * dsp;
    int bufferPid;
    int childPid;
 
    unsigned char * buffer;
    int bufferSize;
    int bufferChunkSize;
    int bufferPos;
    int bufferFill;

    int playNext();

    int open();

    int openFile();
    int openProg(char *);
    int close();
    int wavHeader();

    SoundDeviceFile(int /*dummy*/) : SoundDevice(), dsp(NULL), bufferPid(-1) {}
public:
    static char name[];
    static char fifo[];
    static char fifoDir[];

    SoundDeviceFile();
    virtual ~SoundDeviceFile();

    virtual int read();
    virtual void update();
};


// get sound data from child process
class SoundDeviceFork : public SoundDevice {
    int sound_child;
    int sound_shm_key;
    void * sound_shared;
public:
    SoundDeviceFork();
    virtual ~SoundDeviceFork();

    virtual int read();
    virtual void update();
};

#endif

