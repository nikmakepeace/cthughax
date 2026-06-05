// OSS/DSP output backend.

#include "cthugha.h"
#include "Audio.h"
#include "AudioInternal.h"
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
#endif

#if WITH_DSP == 1

AudioDSPOutput::AudioDSPOutput(int method_, int visualMaxDimension)
    : handle(-1)
    , method(method_)
    , sampleWindow(audioSampleWindowForVisualMaxDimension(visualMaxDimension)) {
    init();
}

int AudioDSPOutput::defaultTargetLatencyMs() const {
    // OSS latency behaviour varies with the selected snd-method and driver.
    // Keep the conservative passthrough target here, owned by the OSS output
    // path rather than by AudioIngest.
    switch (method) {
    case 0:
    case 1:
    case 2:
    case 3:
    default:
        return audio_dsp_target_latency_msec;
    }
}

int AudioDSPOutput::timingScratchSamples(int, int targetDelaySamples) const {
    return targetDelaySamples;
}

void AudioDSPOutput::setFragment() {
    int soundDSPFragment = (audioDspFragments() << 16) | audioDspFragmentSize();
    if (ioctl(handle, SNDCTL_DSP_SETFRAGMENT, &soundDSPFragment) < 0)
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SETFRAGMENT failed.");

    audioSetDspFragment(soundDSPFragment >> 16, soundDSPFragment & 0x7fff);
}

void AudioDSPOutput::setChannels() {
    int channels = audioChannels() - 1;
    if (ioctl(handle, SNDCTL_DSP_STEREO, &channels) < 0)
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_STEREO failed");
    audioSetChannels(channels + 1);
}

void AudioDSPOutput::setSampleRate() {
    int sampleRate = audioSampleRateHz();
    if (ioctl(handle, SNDCTL_DSP_SPEED, &sampleRate) < 0)
        CTH_ERRNO(errno, "ioctl: SNDCTL_DSP_SPEED failed");
    audioSetSampleRateHz(sampleRate);
}

void AudioDSPOutput::setFormat() {
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
    CTH_DEBUG("audio dsp output: set sound format requested=%d returned=%d\n",
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
    }
}

void AudioDSPOutput::init() {
    CTH_DEBUG("  setting %s for writing...\n", audioDspDevicePath());
    CTH_DEBUG("audio dsp output: init device=`%s' method=%d\n", audioDspDevicePath(),
        method);

    if (handle >= 0)
        close(handle);
    handle = -1;

    if ((handle = open(audioDspDevicePath(), O_WRONLY)) < 0) {
        CTH_ERRNO(errno, "Can't open `%s' for writing.", audioDspDevicePath());
        return;
    }

    switch (method) {
    case 0: {
        CTH_INFO("   Using sound method 0 - optimal fragment size\n");
        audioSetDspFragmentSize(ilog2(sampleWindow) - 1);
        setFragment();
        setChannels();
        setSampleRate();
        setFormat();
        break;
    }

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

    case 4:
        CTH_ERROR("Sound method 4 is only available for reading sound data.\n"
                "Please use a different sound method.\n");
        close(handle);
        handle = -1;
        break;

    default:
        CTH_ERROR("Unknown sound method %d.", method);
        CTH_ERROR("   available sound methods:\n"
                "   0: sophisticated 1 (optimal fragment size)\n"
                "   1: sophisticated 2 (small fragments)\n"
                "   2: simple (small DMA buffer)\n"
                "   3: primitiv\n"
                "   4: directly use DMA buffer\n");
        close(handle);
        handle = -1;
        break;
    }
}

int AudioDSPOutput::write(const void* buffer, int size) {
    if (handle < 0)
        return 0;
    return ::write(handle, buffer, size);
}

int AudioDSPOutput::getHandle() const { return handle; }
int AudioDSPOutput::isOpen() const { return handle >= 0; }
void AudioDSPOutput::update() { init(); }

AudioDSPOutput::~AudioDSPOutput() {
    if (handle >= 0)
        close(handle);
    handle = -1;
}

#else

AudioDSPOutput::AudioDSPOutput(int method_, int visualMaxDimension)
    : handle(-1)
    , method(method_)
    , sampleWindow(audioSampleWindowForVisualMaxDimension(visualMaxDimension)) {
    CTH_DEBUG("    audio output strategy: OSS DSP unavailable because support is not compiled in\n");
    CTH_DEBUG("audio dsp output: unavailable method=%d\n", method);
}

AudioDSPOutput::~AudioDSPOutput() { }
int AudioDSPOutput::defaultTargetLatencyMs() const { return audio_dsp_target_latency_msec; }
int AudioDSPOutput::timingScratchSamples(int, int targetDelaySamples) const { return targetDelaySamples; }
void AudioDSPOutput::setFragment() { }
void AudioDSPOutput::setChannels() { }
void AudioDSPOutput::setSampleRate() { }
void AudioDSPOutput::setFormat() { }
void AudioDSPOutput::init() { }
int AudioDSPOutput::write(const void*, int) { return 0; }
int AudioDSPOutput::getHandle() const { return -1; }
int AudioDSPOutput::isOpen() const { return 0; }
void AudioDSPOutput::update() { }

#endif
