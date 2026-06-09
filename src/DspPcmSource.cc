// OSS/DSP capture source.
#include "config.h"
#include "Audio.h"
#include "AudioInternal.h"
#include "AudioSettings.h"
#include "ProcessServices.h"
#include "imath.h"

#include <errno.h>
#include <string.h>
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

DspPcmSource::DspPcmSource(const AudioSettings& settings, int visualMaxDimension,
    LogSink& log_)
    : PcmSource(log_)
    , handle(-1)
    , dmaBuffer(NULL)
    , dmaSize(0)
    , method(settings.soundDSPMethod)
    , dspFragmentsValue(settings.dspFragments)
    , dspFragmentSizeValue(settings.dspFragmentSize)
    , dspSyncEnabledValue(settings.dspSyncEnabled)
    , sampleWindow(audioSampleWindowForVisualMaxDimension(visualMaxDimension)) {
    pcmFormat = settings.pcmFormat;
    strncpy(dspDevicePathValue, settings.dspDevicePath, PATH_MAX);
    dspDevicePathValue[PATH_MAX - 1] = '\0';
    init();
}

void DspPcmSource::setFragment() {
    int soundDSPFragment = (dspFragmentsValue << 16) | dspFragmentSizeValue;
    if (ioctl(handle, SNDCTL_DSP_SETFRAGMENT, &soundDSPFragment) < 0)
        log.errorErrno(errno, "ioctl: SNDCTL_DSP_SETFRAGMENT failed.");

    dspFragmentsValue = soundDSPFragment >> 16;
    dspFragmentSizeValue = soundDSPFragment & 0x7fff;

    if ((1 << dspFragmentSizeValue) * 2 * pcmFormat.channels < sampleWindow)
        log.warn("  sound fragment size is not set big enough.\n");
}

void DspPcmSource::setChannels() {
    int channels = pcmFormat.channels - 1;
    if (ioctl(handle, SNDCTL_DSP_STEREO, &channels) < 0)
        log.errorErrno(errno, "ioctl: SNDCTL_DSP_STEREO failed");
    pcmFormat.channels = channels + 1;
}

void DspPcmSource::setSampleRate() {
    int sampleRate = pcmFormat.sampleRate;
    if (ioctl(handle, SNDCTL_DSP_SPEED, &sampleRate) < 0)
        log.errorErrno(errno, "ioctl: SNDCTL_DSP_SPEED failed");
    pcmFormat.sampleRate = sampleRate;
}

void DspPcmSource::setFormat() {
    int sound_format;

    switch (pcmFormat.sampleFormat) {
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
        log.error("Internal error: unknown sound format.\n");
        sound_format = AFMT_U8;
    }

    int requested_sound_format = sound_format;

    if (ioctl(handle, SNDCTL_DSP_SETFMT, &sound_format) < 0) {
        log.errorErrno(errno, "ioctl: SNDCTL_DSP_SETFMT failed. Trying 8bit unsigned");

        sound_format = AFMT_U8;
        if (ioctl(handle, SNDCTL_DSP_SETFMT, &sound_format) < 0)
            log.errorErrno(errno, "ioctl: SNDCTL_DSP_SETFMT failed.");
    }

    log.debug("dsp pcm source: set sound format requested=%d returned=%d\n",
        requested_sound_format, sound_format);

    switch (sound_format) {
    case AFMT_U8:
        pcmFormat.sampleFormat = SF_u8;
        break;
    case AFMT_S8:
        pcmFormat.sampleFormat = SF_s8;
        break;
    case AFMT_U16_LE:
        pcmFormat.sampleFormat = SF_u16_le;
        break;
    case AFMT_S16_LE:
        pcmFormat.sampleFormat = SF_s16_le;
        break;
    case AFMT_U16_BE:
        pcmFormat.sampleFormat = SF_u16_be;
        break;
    case AFMT_S16_BE:
        pcmFormat.sampleFormat = SF_s16_be;
        break;
    default:
        log.error("Unknown sound format returned by SNDCTL_DSP_SETFMT %d.\n",
            sound_format);
        error = 1;
    }
}

void DspPcmSource::init() {
    log.debug("  setting %s for reading...\n", dspDevicePathValue);
    log.debug("dsp pcm source: init method=%d sample-window=%d\n",
        method, sampleWindow);

    if (handle >= 0)
        close(handle);
    handle = -1;

    if ((handle = open(dspDevicePathValue, O_RDONLY)) < 0) {
        log.errorErrno(errno, "Can't open `%s' for reading.", dspDevicePathValue);
        error = 1;
        return;
    }

    switch (method) {
    case 0:
        log.info("   Using sound method 0 - optimal fragment size\n");
        dspFragmentSizeValue = ilog2(sampleWindow) - 1;
        setFragment();
        setChannels();
        setSampleRate();
        setFormat();
        break;

    case 1:
        log.info("   Using sound method 1 - small fragment size\n");
        dspFragmentsValue = 2;
        dspFragmentSizeValue = 4;
        setFragment();
        setChannels();
        setSampleRate();
        setFormat();
        break;

    case 2: {
        log.info("   Using sound method 2 - old version\n");
        int sound_div = 4;
        if (ioctl(handle, SNDCTL_DSP_SUBDIVIDE, &sound_div) < 0)
            log.errorErrno(errno, "ioctl: SNDCTL_DSP_SUBDIVIDE failed.");

        int dummy = 0;
        if (ioctl(handle, SNDCTL_DSP_STEREO, &dummy) < 0)
            log.errorErrno(errno, "ioctl: SNDCTL_DSP_STEREO failed.");

        dummy = 0;
        if (ioctl(handle, SNDCTL_DSP_SPEED, &dummy) < 0)
            log.errorErrno(errno, "ioctl: SNDCTL_DSP_SPEED failed.");

        int sound_blkSize;
        if (ioctl(handle, SNDCTL_DSP_GETBLKSIZE, &sound_blkSize) < 0)
            log.errorErrno(errno, "ioctl: SNDCTL_DSP_GETBLKSIZE failed.");

        setChannels();
        setSampleRate();
        setFormat();

        if (ioctl(handle, SNDCTL_DSP_GETBLKSIZE, &sound_blkSize) < 0)
            log.errorErrno(errno, "ioctl: SNDCTL_DSP_GETBLKSIZE");
        break;
    }

    case 3:
        log.info("   Using sound method 3 - primitiv version\n");
        setFormat();
        setChannels();
        setSampleRate();
        break;

    case 4: {
        log.info("   Using sound method 4 - directly using DMA buffer\n");
        setFormat();
        setChannels();
        setSampleRate();

        dspFragmentsValue = 2;
        dspFragmentSizeValue = 10;
        setFragment();

        struct audio_buf_info info;
        if (ioctl(handle, SNDCTL_DSP_GETISPACE, &info) == -1) {
            log.error("ioctl: SNDCTL_DSP_GETISPACE failed.");
            error = 1;
            break;
        }

        dmaSize = info.fragstotal * info.fragsize;
        if (dmaSize < 2048)
            log.error("Fragment size changed. New value (%d) is too small.\n"
                    "Please use a different sound method.\n",
                dmaSize);

        dmaBuffer = (char*)mmap(NULL, dmaSize, PROT_READ, MAP_FILE | MAP_SHARED, handle, 0);
        if (dmaBuffer == (char*)-1) {
            log.error("mmap failed.\n");
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
        log.error("Unknown sound method %d.", method);
        log.error("   available sound methods:\n"
                "   0: sophisticated 1 (optimal fragment size)\n"
                "   1: sophisticated 2 (small fragments)\n"
                "   2: simple (small DMA buffer)\n"
                "   3: primitiv\n"
                "   4: directly use DMA buffer\n");
        error = 1;
        return;
    }
}

int DspPcmSource::read(char* dst, int rawSize, int samplesRequested) {
    int r = 0;
    int bytesPerSample = pcmFormat.bytesPerSample();

    switch (method) {
    case 0: {
        audio_buf_info bi;
        const int nr_read = pcmFormat.channels * samplesRequested
            / (1 << dspFragmentSizeValue);

        if (ioctl(handle, SNDCTL_DSP_GETISPACE, &bi) < 0)
            log.errorErrno(errno, "ioctl: SNDCTL_DSP_GETISPACE failed.");

        if (bi.fragments > nr_read) {
            for (int i = 0; i < bi.fragments - nr_read; i++)
                ::read(handle, dst, (1 << dspFragmentSizeValue));
            r = (1 << dspFragmentSizeValue) * nr_read;
        } else
            r = (1 << dspFragmentSizeValue) * bi.fragments;

        if (r == 0)
            r = (1 << dspFragmentSizeValue);

        if (::read(handle, dst, r) < 0)
            log.errorErrno(errno, "reading sound failed.");
        break;
    }

    case 1: {
        unsigned char* sbuff;
        int nr_read;
        for (nr_read = 0, sbuff = (unsigned char*)dst; nr_read < rawSize; nr_read += 32) {
            if (::read(handle, sbuff, 16) < 0)
                log.errorErrno(errno, "dsp read < 0");
            sbuff += 16;
            if (::read(handle, sbuff, 16) < 0)
                log.errorErrno(errno, "dsp read < 0");
            sbuff += 16;
        }
        r = nr_read;
        break;
    }

    case 2:
    case 3:
        r = ::read(handle, dst, rawSize);
        if (r < 0)
            log.errorErrno(errno, "get_sound: read < 0.");
        break;

    case 4:
        if (dmaBuffer != NULL) {
            memcpy(dst, dmaBuffer, 2048);
            r = 2048;
        }
        break;
    }

    if (dspSyncEnabledValue)
        ioctl(handle, SNDCTL_DSP_RESET);

    return r / bytesPerSample;
}

int DspPcmSource::rawBufferSize(int frameRawSize, int samplesRequested) const {
    switch (method) {
    case 0:
        return max(frameRawSize, (1 << dspFragmentSizeValue) * 4);
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

DspPcmSource::~DspPcmSource() {
    if (dmaBuffer != NULL)
        munmap(dmaBuffer, dmaSize);
    dmaBuffer = NULL;
    dmaSize = 0;

    if (handle >= 0)
        close(handle);
}

#else

DspPcmSource::DspPcmSource(const AudioSettings& settings, int visualMaxDimension,
    LogSink& log_)
    : PcmSource(log_)
    , handle(-1)
    , dmaBuffer(NULL)
    , dmaSize(0)
    , method(settings.soundDSPMethod)
    , dspFragmentsValue(settings.dspFragments)
    , dspFragmentSizeValue(settings.dspFragmentSize)
    , dspSyncEnabledValue(settings.dspSyncEnabled)
    , sampleWindow(audioSampleWindowForVisualMaxDimension(visualMaxDimension)) {
    pcmFormat = settings.pcmFormat;
    strncpy(dspDevicePathValue, settings.dspDevicePath, PATH_MAX);
    dspDevicePathValue[PATH_MAX - 1] = '\0';
    log.error("DSP device was disabled at compile time.\n");
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
#endif
