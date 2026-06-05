// OSS/DSP capture source.

#include "cthugha.h"
#include "Audio.h"
#include "AudioInternal.h"
#include "Mixer.h"
#include "imath.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#if WITH_DSP == 1
#ifdef HAVE_LINUX_SOUNDCARD_H
#include <linux/soundcard.h>
#else
#include <sys/soundcard.h>
#endif
#include <sys/mman.h>
#ifndef MAP_FILE
#define MAP_FILE 0
#endif
#endif

#if WITH_DSP == 1

DspPcmSource::DspPcmSource(int visualMaxDimension)
    : PcmSource()
    , handle(-1)
    , dmaBuffer(NULL)
    , dmaSize(0)
    , sampleWindow(audioSampleWindowForVisualMaxDimension(visualMaxDimension)) {
    init();
}

void DspPcmSource::setFragment() {
    int soundDSPFragment = (audioDspFragments() << 16) | audioDspFragmentSize();
    if (ioctl(handle, SNDCTL_DSP_SETFRAGMENT, &soundDSPFragment) < 0)
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SETFRAGMENT failed.");

    audioSetDspFragment(soundDSPFragment >> 16, soundDSPFragment & 0x7fff);

    if ((1 << audioDspFragmentSize()) * 2 * audioChannels() < sampleWindow)
        CTH_WARN("  sound fragment size is not set big enough.\n");
}

void DspPcmSource::setChannels() {
    int channels = audioChannels() - 1;
    if (ioctl(handle, SNDCTL_DSP_STEREO, &channels) < 0)
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_STEREO failed");
    audioSetChannels(channels + 1);
}

void DspPcmSource::setSampleRate() {
    int sampleRate = audioSampleRateHz();
    if (ioctl(handle, SNDCTL_DSP_SPEED, &sampleRate) < 0)
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SPEED failed");
    audioSetSampleRateHz(sampleRate);
}

void DspPcmSource::setFormat() {
    int sound_format;

    switch (audioSampleFormat()) {
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

    int requested_sound_format = sound_format;

    if (ioctl(handle, SNDCTL_DSP_SETFMT, &sound_format) < 0) {
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SETFMT failed. Trying 8bit unsigned");

        sound_format = AFMT_U8;
        if (ioctl(handle, SNDCTL_DSP_SETFMT, &sound_format) < 0)
            CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SETFMT failed.");
    }

    CTH_DEBUG("dsp pcm source: set sound format requested=%d returned=%d\n",
        requested_sound_format, sound_format);

    switch (sound_format) {
    case AFMT_U8:
        audioSetSampleFormat(SF_u8);
        break;
    case AFMT_S8:
        audioSetSampleFormat(SF_s8);
        break;
    case AFMT_U16_LE:
        audioSetSampleFormat(SF_u16_le);
        break;
    case AFMT_S16_LE:
        audioSetSampleFormat(SF_s16_le);
        break;
    case AFMT_U16_BE:
        audioSetSampleFormat(SF_u16_be);
        break;
    case AFMT_S16_BE:
        audioSetSampleFormat(SF_s16_be);
        break;
    default:
        CTH_ERROR("Unknown sound format returned by SNDCTL_DSP_SETFMT %d.\n", sound_format);
        error = 1;
    }
}

void DspPcmSource::init() {
    CTH_DEBUG("  setting %s for reading...\n", audioDspDevicePath());
    CTH_DEBUG("dsp pcm source: init method=%d sample-window=%d\n",
        audioDspMethod(), sampleWindow);

    if (handle >= 0)
        close(handle);
    handle = -1;

    if ((handle = open(audioDspDevicePath(), O_RDONLY)) < 0) {
        CTH_ERRNO(errno, "Can't open `%s' for reading.", audioDspDevicePath());
        error = 1;
        return;
    }

    switch (audioDspMethod()) {
    case 0:
        CTH_INFO("   Using sound method 0 - optimal fragment size\n");
        audioSetDspFragmentSize(ilog2(sampleWindow) - 1);
        setFragment();
        setChannels();
        setSampleRate();
        setFormat();
        break;

    case 1:
        CTH_INFO("   Using sound method 1 - small fragment size\n");
        audioSetDspFragment(2, 4);
        setFragment();
        setChannels();
        setSampleRate();
        setFormat();
        break;

    case 2: {
        CTH_INFO("   Using sound method 2 - old version\n");
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

    case 3:
        CTH_INFO("   Using sound method 3 - primitiv version\n");
        setFormat();
        setChannels();
        setSampleRate();
        break;

    case 4: {
        CTH_INFO("   Using sound method 4 - directly using DMA buffer\n");
        setFormat();
        setChannels();
        setSampleRate();

        audioSetDspFragment(2, 10);
        setFragment();

        struct audio_buf_info info;
        if (ioctl(handle, SNDCTL_DSP_GETISPACE, &info) == -1) {
            CTH_ERROR("ioctl: SNDCTL_DSP_GETISPACE failed.");
            error = 1;
            break;
        }

        dmaSize = info.fragstotal * info.fragsize;
        if (dmaSize < 2048)
            CTH_ERROR("Fragment size changed. New value (%d) is too small.\n"
                    "Please use a different sound method.\n",
                dmaSize);

        dmaBuffer = (char*)mmap(NULL, dmaSize, PROT_READ, MAP_FILE | MAP_SHARED, handle, 0);
        if (dmaBuffer == (char*)-1) {
            CTH_ERROR("mmap failed.\n");
            dmaBuffer = NULL;
            dmaSize = 0;
            error = 1;
            break;
        }

        int tmp = 0;
        ioctl(handle, SNDCTL_DSP_SETTRIGGER, &tmp);

        tmp = PCM_ENABLE_INPUT;
        ioctl(handle, SNDCTL_DSP_SETTRIGGER, &tmp);
        break;
    }

    default:
        CTH_ERROR("Unknown sound method %d.", audioDspMethod());
        CTH_ERROR("   available sound methods:\n"
                "   0: sophisticated 1 (optimal fragment size)\n"
                "   1: sophisticated 2 (small fragments)\n"
                "   2: simple (small DMA buffer)\n"
                "   3: primitiv\n"
                "   4: directly use DMA buffer\n");
        error = 1;
        return;
    }

    pcmFormat = audioPcmFormat();
}

int DspPcmSource::read(char* dst, int rawSize, int samplesRequested) {
    int r = 0;
    int bytesPerSample = audioBytesPerSample();

    switch (audioDspMethod()) {
    case 0: {
        audio_buf_info bi;
        const int nr_read = audioChannels() * samplesRequested
            / (1 << audioDspFragmentSize());

        if (ioctl(handle, SNDCTL_DSP_GETISPACE, &bi) < 0)
            CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_GETISPACE failed.");

        if (bi.fragments > nr_read) {
            for (int i = 0; i < bi.fragments - nr_read; i++)
                ::read(handle, dst, (1 << audioDspFragmentSize()));
            r = (1 << audioDspFragmentSize()) * nr_read;
        } else
            r = (1 << audioDspFragmentSize()) * bi.fragments;

        if (r == 0)
            r = (1 << audioDspFragmentSize());

        if (::read(handle, dst, r) < 0)
            CTH_ERRNO(errno, "reading sound failed.");
        break;
    }

    case 1: {
        unsigned char* sbuff;
        int nr_read;
        for (nr_read = 0, sbuff = (unsigned char*)dst; nr_read < rawSize; nr_read += 32) {
            if (::read(handle, sbuff, 16) < 0)
                CTH_ERRNO(errno, "dsp read < 0");
            sbuff += 16;
            if (::read(handle, sbuff, 16) < 0)
                CTH_ERRNO(errno, "dsp read < 0");
            sbuff += 16;
        }
        r = nr_read;
        break;
    }

    case 2:
    case 3:
        r = ::read(handle, dst, rawSize);
        if (r < 0)
            CTH_ERRNO(errno, "get_sound: read < 0.");
        break;

    case 4:
        if (dmaBuffer != NULL) {
            memcpy(dst, dmaBuffer, 2048);
            r = 2048;
        }
        break;
    }

    if (audioDspSyncEnabled())
        ioctl(handle, SNDCTL_DSP_RESET);

    return r / bytesPerSample;
}

int DspPcmSource::rawBufferSize(int frameRawSize, int samplesRequested) const {
    switch (audioDspMethod()) {
    case 0:
        return max(frameRawSize, (1 << audioDspFragmentSize()) * 4);
    case 1:
        return max(frameRawSize, samplesRequested * 4 + 32);
    case 4:
        return max(frameRawSize, 2048);
    default:
        return frameRawSize;
    }
}

void DspPcmSource::update() {
    if (dmaBuffer != NULL) {
        munmap(dmaBuffer, dmaSize);
        dmaBuffer = NULL;
        dmaSize = 0;
    }
    init();
}

int DspPcmSource::initInputControls() {
    CTH_INFO("Initializing OSS mixer device...\n");
    return init_mixer();
}

DspPcmSource::~DspPcmSource() {
    if (dmaBuffer != NULL)
        munmap(dmaBuffer, dmaSize);
    dmaBuffer = NULL;
    dmaSize = 0;

    if (handle >= 0)
        close(handle);
}

#else

DspPcmSource::DspPcmSource(int visualMaxDimension)
    : PcmSource()
    , handle(-1)
    , dmaBuffer(NULL)
    , dmaSize(0)
    , sampleWindow(audioSampleWindowForVisualMaxDimension(visualMaxDimension)) {
    CTH_ERROR("DSP device was disabled at compile time.\n");
    error = 1;
}

DspPcmSource::~DspPcmSource() { }
void DspPcmSource::setFragment() { }
void DspPcmSource::setChannels() { }
void DspPcmSource::setSampleRate() { }
void DspPcmSource::setFormat() { }
void DspPcmSource::init() { }
int DspPcmSource::read(char*, int, int) { return 0; }
int DspPcmSource::rawBufferSize(int frameRawSize, int) const { return frameRawSize; }
void DspPcmSource::update() { }
int DspPcmSource::initInputControls() { return 0; }

#endif
