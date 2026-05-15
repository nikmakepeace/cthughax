#include "cthugha.h"
#include "Sound.h"
#include "imath.h"
#include "Interface.h"

#if WITH_DSP == 1

// include the right soundcard.h
#ifdef HAVE_LINUX_SOUNDCARD_H
#include <linux/soundcard.h>
#else
#include <sys/soundcard.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

#ifndef MAP_FILE
#define MAP_FILE 0
#endif

char SoundDeviceDSP::dev_dsp[PATH_MAX] = DEV_DSP;

SoundDeviceDSP::SoundDeviceDSP(int mode)
    : SoundDevice()
    , handle(0)
    , DMAbuffer(NULL) {
    init(mode);
}

void SoundDeviceDSP::setFragment() {

    int soundDSPFragment = (int(soundDSPFragments) << 16) | int(soundDSPFragmentSize);
    if (ioctl(handle, SNDCTL_DSP_SETFRAGMENT, &soundDSPFragment) < 0)
        printfee("ioctl: SNDCTL_DSP_SETFRAGMENT failed.");

    soundDSPFragments.setValue(soundDSPFragment >> 16);
    soundDSPFragmentSize.setValue(soundDSPFragment & 0x7fff);

    if ((1 << soundDSPFragmentSize) * 2 * int(soundChannels) < size) {
        CTH_WARN("  sound fragment size is not set big enough.\n");
    }
}

void SoundDeviceDSP::setChannels() {
    /* set stereo or mono */
    int channels = int(soundChannels) - 1;
    if (ioctl(handle, SNDCTL_DSP_STEREO, &channels) < 0)
        printfee("ioctl: SNDCTL_DSP_STEREO failed");
    soundChannels.setValue(channels + 1);
}

void SoundDeviceDSP::setSampleRate() {

    /* set sample rate */
    if (ioctl(handle, SNDCTL_DSP_SPEED, &(soundSampleRate.value)) < 0)
        printfee("ioctl: SNDCTL_DSP_SPEED failed");
}

void SoundDeviceDSP::setFormat() {

    /* set the sound format based on the sound_sampleSize */
    int sound_format;

    switch (soundFormat) {
    case SF_u8:
        sound_format = AFMT_U8;
        break;
    case SF_s8:
        sound_format = AFMT_S8;
        break;
    case SF_u16_le:
        sound_format = AFMT_U16_LE;
        break;
    case SF_s16_le:
        sound_format = AFMT_S16_LE;
        break;
    case SF_u16_be:
        sound_format = AFMT_U16_BE;
        break;
    case SF_s16_be:
        sound_format = AFMT_S16_BE;
        break;
    default:
        CTH_ERROR("Internal error: unknown sound format.\n");
        sound_format = AFMT_U8;
    }

    CTH_TRACE("setting sound format: %d   ", sound_format);

    if (ioctl(handle, SNDCTL_DSP_SETFMT, &sound_format) < 0) {
        printfee("ioctl: SNDCTL_DSP_SETFMT failed. Trying 8bit unsigned");

        sound_format = AFMT_U8;
        if (ioctl(handle, SNDCTL_DSP_SETFMT, &sound_format) < 0) {
            printfee("ioctl: SNDCTL_DSP_SETFMT failed.");
        }
    }

    CTH_TRACE("returned: %d\n", sound_format);

    switch (sound_format) {
    case AFMT_U8:
        soundFormat.setValue(SF_u8);
        break;
    case AFMT_S8:
        soundFormat.setValue(SF_s8);
        break;
    case AFMT_U16_LE:
        soundFormat.setValue(SF_u16_le);
        break;
    case AFMT_S16_LE:
        soundFormat.setValue(SF_s16_le);
        break;
    case AFMT_U16_BE:
        soundFormat.setValue(SF_u16_be);
        break;
    case AFMT_S16_BE:
        soundFormat.setValue(SF_s16_be);
        break;
    default:
        CTH_ERROR("Unknown sound format returned by SNDCTL_DSP_SETFMT %d.\n", sound_format);
        error = 1;
    }
}

void SoundDeviceDSP::init(int mode) {
    CTH_DEBUG("  setting %s for %s...\n", dev_dsp,
        (mode == O_RDONLY)       ? "reading"
            : (mode == O_WRONLY) ? "writing"
                                 : "");

    this->mode = mode;

    /* Get sound-device */
    if ((handle = open(dev_dsp, mode)) < 0) {
        printfee("Can't open `%s' for %s.", dev_dsp,
            (mode == O_RDONLY)       ? "reading"
                : (mode == O_WRONLY) ? "writing"
                                     : "");
        error = 1;
        return;
    }

    switch (int(soundDSPMethod)) {

    case 0: { // set sound fragment size and number of fragments
        CTH_INFO("   Using sound method 0 - optimal fragment size\n");

        soundDSPFragmentSize.setValue(ilog2(size) - 1);

        setFragment();
        setChannels();
        setSampleRate();
        setFormat();

        tmpSize = (1 << soundDSPFragmentSize) * 4;
        setTmpData();

        break;
    }
    case 1: { // fragment size of 16 bytes (2^4), two fragments (2)
        CTH_INFO("   Using sound method 1 - small fragment size\n");

        soundDSPFragments.setValue(2);
        soundDSPFragmentSize.setValue(4);

        setFragment();
        setChannels();
        setSampleRate();
        setFormat();

        tmpSize = size * 4 + 32;
        setTmpData();

        break;
    }
    case 2: { // from version 0.3
        CTH_INFO("   Using sound method 2 - old version\n");

        /* create a DMA-Buffer as small as possible.
           1. create the buffer for lowerest possible sample rate (4000) and
           mono, with maximul SUBDIVE (4). (The buffersize is calculated
           from the sample rate and number of channels and is not changed
           after it is set for the first time by a call to GETBLKSIZE)
           On my machine I get a BLKSIZE of 1024 bytes
           2. set the sample rate and nr. of channels as specified
        */
        int sound_div = 4; /* blocks as small as possible */
        if (ioctl(handle, SNDCTL_DSP_SUBDIVIDE, &sound_div) < 0)
            printfee("ioctl: SNDCTL_DSP_SUBDIVIDE failed.");

        int dummy = 0;
        if (ioctl(handle, SNDCTL_DSP_STEREO, &dummy) < 0)
            printfee("ioctl: SNDCTL_DSP_STEREO failed.");

        dummy = 0;
        if (ioctl(handle, SNDCTL_DSP_SPEED, &dummy) < 0)
            printfee("ioctl: SNDCTL_DSP_SPEED failed.");

        int sound_blkSize;
        if (ioctl(handle, SNDCTL_DSP_GETBLKSIZE, &sound_blkSize) < 0)
            printfee("ioctl: SNDCTL_DSP_GETBLKSIZE failed.");

        setChannels();
        setSampleRate();
        setFormat();

        if (ioctl(handle, SNDCTL_DSP_GETBLKSIZE, &sound_blkSize) < 0)
            printfee("ioctl: SNDCTL_DSP_GETBLKSIZE");
        break;
    }
    case 3: { // primitiv. only set format, channels and speed
        CTH_INFO("   Using sound method 3 - primitiv version\n");

        setFormat();
        setChannels();
        setSampleRate();
        break;
    }
    case 4: { // use DMA
        if (mode == O_WRONLY) {
            CTH_ERROR("Sound method 4 is only available for reading sound data.\n"
                    "Please use a different sound method.\n");
            error = 1;
            break;
        }
        CTH_INFO("   Using sound method 4 - directly using DMA buffer\n");

        setFormat();
        setChannels();
        setSampleRate();

        soundDSPFragments.setValue(2); // 2 fragments
        soundDSPFragmentSize.setValue(10); // 2^10 = 1024 bytes/fragment
        setFragment();

        struct audio_buf_info info;
        if (ioctl(handle, SNDCTL_DSP_GETISPACE, &info) == -1) {
            CTH_ERROR("ioctl: SNDCTL_DSP_GETISPACE failed.");
            error = 1;
            break;
        }
        int sz = info.fragstotal * info.fragsize;
        if (sz < 2048) {
            CTH_ERROR("Fragment size changed. New value (%d) is too small.\n"
                    "Please use a different sound method.\n",
                sz);
        }
        if ((DMAbuffer = (char*)mmap(NULL, sz, PROT_READ, MAP_FILE | MAP_SHARED, handle, 0))
            == (char*)-1) {
            CTH_ERROR("mmap failed.\n");
            error = 1;
            break;
        }
        tmpSize = 2048;

        int tmp = 0;
        ioctl(handle, SNDCTL_DSP_SETTRIGGER, &tmp);

        tmp = PCM_ENABLE_INPUT;
        ioctl(handle, SNDCTL_DSP_SETTRIGGER, &tmp);

        break;
    }
    default:
        CTH_ERROR("Unknown sound method %d.", int(soundDSPMethod));
        CTH_ERROR("   available sound methods:\n"
                "   0: sophisticated 1 (optimal fragment size)\n"
                "   1: sophisticated 2 (small fragments)\n"
                "   2: simple (small DMA buffer)\n"
                "   3: pimitiv\n"
                "   4: directly use DMA buffer\n");
        error = 1;
        return;
    }
}

void SoundDeviceDSP::update() {
    close(handle);
    init(mode);
}

/*
 * Get sound from sound-card
 *
 * Jan Kujawa <kujawa@kallisti.montana.com>
 *   helped a lot to do this better.
 */
int SoundDeviceDSPIn::read() {

    int r = 0; // nr of bytes really read

    switch (int(soundDSPMethod)) {
    case 0: {
        audio_buf_info bi;
        const int nr_read = int(soundChannels) * size / (1 << soundDSPFragmentSize);

        // get number of available fragments (-> bi.framents)
        if (ioctl(handle, SNDCTL_DSP_GETISPACE, &bi) < 0) {
            printfee("ioctl: SNDCTL_DSP_GETISPACE failed.");
        }

        // if there is more data available than needed: read any extra sound data
        if (bi.fragments > nr_read) {
            for (int i = 0; i < bi.fragments - nr_read; i++)
                ::read(handle, tmpData, (1 << soundDSPFragmentSize));
            r = (1 << soundDSPFragmentSize) * nr_read;
        } else
            r = (1 << soundDSPFragmentSize) * bi.fragments;

        if (r == 0) // read at least one
            r = (1 << soundDSPFragmentSize);

        /* read the sound data, that will be display */
        if (::read(handle, tmpData, r) < 0) {
            printfee("reading sound failed.");
        }

        break;
    }
    case 1: {
        unsigned char* sbuff;
        int nr_read;
        /* Important information from Jan Kujawa <kujawa@kallisti.montana.com>
           how to do this correctly */
        for (nr_read = 0, sbuff = (unsigned char*)tmpData; nr_read < rawSize; nr_read += 32) {
            if (::read(handle, sbuff, 16) < 0)
                printfee("sound_read < 0");
            sbuff += 16;
            if (::read(handle, sbuff, 16) < 0)
                printfee("sound_read < 0");
            sbuff += 16;
        }

        r = nr_read;

        break;
    }
    case 2:
    case 3: {
        r = ::read(handle, tmpData, rawSize);

        if (r < 0) {
            printfee("get_sound: read < 0.");
        }

        break;
    }
    case 4:
        memcpy(tmpData, DMAbuffer, 2048);
        r = 2048;
    }

    /* this should no not be necessary - it only causes problems */
    if (int(soundDSPSync))
        ioctl(handle, SNDCTL_DSP_RESET);

    return r / bytesPerSample;
}

int SoundDeviceDSPOut::write(const void* data, int size) { return ::write(handle, data, size); }

SoundDeviceDSP::~SoundDeviceDSP() { close(handle); }

#else

char SoundDeviceDSP::dev_dsp[PATH_MAX] = "";

SoundDeviceDSP::SoundDeviceDSP(int mode)
    : handle(0) {
    init(mode);
}

void SoundDeviceDSP::init(int) {
    CTH_ERROR("DSP device was disabled at compile time.\n");
    error = 1;
}

void SoundDeviceDSP::setFragment() { }
void SoundDeviceDSP::setChannels() { }
void SoundDeviceDSP::setSampleRate() { }
void SoundDeviceDSP::setFormat() { }
void SoundDeviceDSP::update() { }
int SoundDeviceDSPIn::read() { return 0; }
int SoundDeviceDSPOut::write(const void*, int) { return 0; }
SoundDeviceDSP::~SoundDeviceDSP() { }

#endif
