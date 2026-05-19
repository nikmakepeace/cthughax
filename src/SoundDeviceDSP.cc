// OSS /dev/dsp sound backend.
// This code configures legacy OSS devices with ioctl calls, then reads either
// from normal device reads or, for method 4, a mapped DMA buffer.

#include "cthugha.h"
#include "Sound.h"
#include "imath.h"
#include "Interface.h"

#if WITH_DSP == 1

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

    // OSS packs fragment count in the high word and log2(fragment size) in
    // the low word. The driver may adjust both values.
    int soundDSPFragment = (int(soundDSPFragments) << 16) | int(soundDSPFragmentSize);
    if (ioctl(handle, SNDCTL_DSP_SETFRAGMENT, &soundDSPFragment) < 0)
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SETFRAGMENT failed.");

    soundDSPFragments.setValue(soundDSPFragment >> 16);
    soundDSPFragmentSize.setValue(soundDSPFragment & 0x7fff);

    if ((1 << soundDSPFragmentSize) * 2 * int(soundChannels) < size) {
        CTH_WARN("  sound fragment size is not set big enough.\n");
    }
}

void SoundDeviceDSP::setChannels() {
    int channels = int(soundChannels) - 1;
    if (ioctl(handle, SNDCTL_DSP_STEREO, &channels) < 0)
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_STEREO failed");
    soundChannels.setValue(channels + 1);
}

void SoundDeviceDSP::setSampleRate() {

    if (ioctl(handle, SNDCTL_DSP_SPEED, &(soundSampleRate.value)) < 0)
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SPEED failed");
}

void SoundDeviceDSP::setFormat() {

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
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SETFMT failed. Trying 8bit unsigned");

        sound_format = AFMT_U8;
        if (ioctl(handle, SNDCTL_DSP_SETFMT, &sound_format) < 0) {
            CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SETFMT failed.");
        }
    }

    CTH_TRACE("returned: %d\n", sound_format);

    // Keep the public option value in sync with what the driver accepted.
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

    if ((handle = open(dev_dsp, mode)) < 0) {
        CTH_ERRNO(errno, "Can't open `%s' for %s.", dev_dsp,
            (mode == O_RDONLY)       ? "reading"
                : (mode == O_WRONLY) ? "writing"
                                     : "");
        error = 1;
        return;
    }

    // These methods are compatibility profiles for different OSS driver
    // behaviors: tune fragment geometry, use conservative reads, or map DMA.
    switch (int(soundDSPMethod)) {

    case 0: {
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
    case 1: {
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
    case 2: {
        CTH_INFO("   Using sound method 2 - old version\n");

        // Some OSS drivers lock buffer geometry on the first GETBLKSIZE call.
        // This method first asks for a tiny mono/low-rate setup, then applies
        // the desired channels/rate/format after the driver has committed to a
        // small block size.
        int sound_div = 4;
        if (ioctl(handle, SNDCTL_DSP_SUBDIVIDE, &sound_div) < 0)
            CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SUBDIVIDE failed.");

        int dummy = 0;
        if (ioctl(handle, SNDCTL_DSP_STEREO, &dummy) < 0)
            CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_STEREO failed.");

        dummy = 0;
        if (ioctl(handle, SNDCTL_DSP_SPEED, &dummy) < 0)
            CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SPEED failed.");

        int sound_blkSize;
        if (ioctl(handle, SNDCTL_DSP_GETBLKSIZE, &sound_blkSize) < 0)
            CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_GETBLKSIZE failed.");

        setChannels();
        setSampleRate();
        setFormat();

        if (ioctl(handle, SNDCTL_DSP_GETBLKSIZE, &sound_blkSize) < 0)
            CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_GETBLKSIZE");
        break;
    }
    case 3: {
        CTH_INFO("   Using sound method 3 - primitiv version\n");

        setFormat();
        setChannels();
        setSampleRate();
        break;
    }
    case 4: {
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

        soundDSPFragments.setValue(2);
        soundDSPFragmentSize.setValue(10);
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
                "   3: primitiv\n"
                "   4: directly use DMA buffer\n");
        error = 1;
        return;
    }
}

void SoundDeviceDSP::update() {
    close(handle);
    init(mode);
}

int SoundDeviceDSPIn::read() {

    int r = 0;

    switch (int(soundDSPMethod)) {
    case 0: {
        audio_buf_info bi;
        const int nr_read = int(soundChannels) * size / (1 << soundDSPFragmentSize);

        if (ioctl(handle, SNDCTL_DSP_GETISPACE, &bi) < 0) {
            CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_GETISPACE failed.");
        }

        // If the device has fallen behind, drain older fragments and keep the
        // newest audio. The visuals should react to "now", not stale input.
        if (bi.fragments > nr_read) {
            for (int i = 0; i < bi.fragments - nr_read; i++)
                ::read(handle, tmpData, (1 << soundDSPFragmentSize));
            r = (1 << soundDSPFragmentSize) * nr_read;
        } else
            r = (1 << soundDSPFragmentSize) * bi.fragments;

        if (r == 0)
            r = (1 << soundDSPFragmentSize);

        if (::read(handle, tmpData, r) < 0) {
            CTH_ERRNO(errno, "reading sound failed.");
        }

        break;
    }
    case 1: {
        unsigned char* sbuff;
        int nr_read;
        // Reads are split into small chunks because some OSS devices behave
        // better when drained at fragment boundaries.
        for (nr_read = 0, sbuff = (unsigned char*)tmpData; nr_read < rawSize; nr_read += 32) {
            if (::read(handle, sbuff, 16) < 0)
                CTH_ERRNO(errno, "sound_read < 0");
            sbuff += 16;
            if (::read(handle, sbuff, 16) < 0)
                CTH_ERRNO(errno, "sound_read < 0");
            sbuff += 16;
        }

        r = nr_read;

        break;
    }
    case 2:
    case 3: {
        r = ::read(handle, tmpData, rawSize);

        if (r < 0) {
            CTH_ERRNO(errno, "get_sound: read < 0.");
        }

        break;
    }
    case 4:
        memcpy(tmpData, DMAbuffer, 2048);
        r = 2048;
    }

    if (int(soundDSPSync))
        ioctl(handle, SNDCTL_DSP_RESET);

    return r / bytesPerSample;
}

int SoundDeviceDSPOut::write(const void* data, int size) { return ::write(handle, data, size); }

int SoundDeviceDSPOut::outputDelayBytes() const {
    int delay = 0;

#ifdef SNDCTL_DSP_GETODELAY
    if (ioctl(handle, SNDCTL_DSP_GETODELAY, &delay) == 0)
        return max(0, delay);
#endif

#ifdef SNDCTL_DSP_GETOSPACE
    {
        audio_buf_info bi;

        if (ioctl(handle, SNDCTL_DSP_GETOSPACE, &bi) == 0)
            return max(0, bi.fragstotal * bi.fragsize - bi.bytes);
    }
#endif

    return 0;
}

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
int SoundDeviceDSPOut::outputDelayBytes() const { return 0; }
SoundDeviceDSP::~SoundDeviceDSP() { }

#endif
