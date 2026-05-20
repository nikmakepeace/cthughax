// Base sound-device implementation.
// Concrete backends supply read(); this layer owns the per-frame conversion
// from backend-specific raw samples into Cthugha's 1024-sample stereo buffer.

#include "cthugha.h"

#include "SoundDevice.h"
#include "imath.h"
#include "cth_buffer.h"

#include <errno.h>
#include <limits.h>
#include <sys/stat.h>

int bytesPerSample = 0;

static void dump_audio_frame(const char2* data, int samplesRead, int requestedSamples) {
    static int initialized = 0;
    static int enabled = 0;
    static int frame = 0;
    static int dumped = 0;
    static int limit = 1;
    static int every = 1;
    static char directory[PATH_MAX];

    const char* env;

    if (!initialized) {
        initialized = 1;
        env = getenv("CTHUGHA_DUMP_AUDIO_FRAMES");
        if (env && env[0]) {
            strncpy(directory, env, PATH_MAX - 1);
            directory[PATH_MAX - 1] = '\0';
            enabled = 1;

            env = getenv("CTHUGHA_DUMP_AUDIO_FRAME_LIMIT");
            if (env && env[0])
                limit = max(1, atoi(env));

            env = getenv("CTHUGHA_DUMP_AUDIO_FRAME_EVERY");
            if (env && env[0])
                every = max(1, atoi(env));

            if ((mkdir(directory, 0777) != 0) && (errno != EEXIST)) {
                CTH_ERRNO(errno, "Can not create audio frame dump directory");
                enabled = 0;
            }
        }
    }

    if (!enabled || (data == NULL))
        return;

    frame++;
    if ((frame % every) != 0)
        return;
    if (dumped >= limit)
        return;

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/audio-%06d.csv", directory, frame);

    FILE* out = fopen(path, "w");
    if (out == NULL) {
        CTH_ERRNO(errno, "Can not create audio frame dump");
        enabled = 0;
        return;
    }

    fprintf(out, "# frame,%d\n", frame);
    fprintf(out, "# samples_read,%d\n", samplesRead);
    fprintf(out, "# requested_samples,%d\n", requestedSamples);
    fprintf(out, "# bytes_per_sample,%d\n", bytesPerSample);
    fprintf(out, "sample,left,right\n");

    for (int i = 0; i < 1024; i++)
        fprintf(out, "%d,%d,%d\n", i, (int)data[i][0], (int)data[i][1]);

    fclose(out);
    dumped++;
}

SoundDevice::SoundDevice() {

    error = 0;

    size = 1 << ilog2(max(BUFF_WIDTH, BUFF_HEIGHT));
    data = new char2[1024];
    memset(data, 0, 1024 * sizeof(char2));
    memset(dataProc, 0, 1024 * sizeof(char2));

    tmpData = NULL;
    tmpDataOwned = 0;
    tmpSize = 0;
    setTmpData();
}

SoundDevice::~SoundDevice() {
    if (tmpDataOwned)
        delete[] tmpData;
    tmpData = NULL;
    tmpDataOwned = 0;
    delete[] data;
    data = NULL;
}

void SoundDevice::operator()() {
    bytesPerSample = (soundFormat < 2) ? soundChannels : 2 * soundChannels;

    // rawSize is the amount of audio Cthugha wants for one visual update.  It is
    // derived from the visual buffer geometry, not from the sound device's
    // preferred playback/capture block size.
    rawSize = bytesPerSample * size;

    int r = this->read();

    // Keep a sliding 1024-sample window. If the backend returns more than one
    // window, display the newest samples; if it returns less, preserve the
    // older tail and append the new samples.
    if (r >= 1024)
        convert(data, (char*)tmpData + (r - 1024) * bytesPerSample, 1024);
    else {
        memcpy(data, data + r, sizeof(char2) * (1024 - r));
        convert(data + 1024 - r, tmpData, r);
    }

    dump_audio_frame(data, r, size);
}

int SoundDevice::read() {
    CTH_ERROR("internal error. SoundDevice::read() called.\n");
    exit(0);
}

void SoundDevice::change() {

    tmpSize = 0;

    // Let the concrete backend apply the new option values before this layer
    // resizes its raw input buffer.
    this->update();

    bytesPerSample = (soundFormat < 2) ? soundChannels : 2 * soundChannels;

    rawSize = bytesPerSample * size;

    setTmpData();
}

void SoundDevice::setTmpData() {
    bytesPerSample = (soundFormat < 2) ? soundChannels : 2 * soundChannels;
    rawSize = bytesPerSample * size;

    if (tmpDataOwned)
        delete[] tmpData;
    if (tmpSize < rawSize)
        tmpSize = rawSize;
    tmpData = new char[tmpSize];
    tmpDataOwned = 1;
}

void SoundDevice::borrowTmpData(char* data) {
    if (tmpDataOwned)
        delete[] tmpData;
    tmpData = data;
    tmpDataOwned = 0;
}

void SoundDevice::convert(char2* dst, void* src, int n) {
    unsigned char* data_u8 = (unsigned char*)src;
    char* data_s8 = (char*)src;
    unsigned short* data_u16 = (unsigned short*)src;
    short* data_s16 = (short*)src;

    int cInc = (soundChannels == 1) ? 0 : 1;

    switch (soundFormat) {
    case SF_u8:
        for (int i = 0; i < n; i++) {
            dst[i][1] = int(*data_u8) - 128;
            data_u8 += cInc;
            dst[i][0] = int(*data_u8) - 128;
            data_u8++;
        }
        break;

    case SF_s8:
        for (int i = 0; i < n; i++) {
            dst[i][1] = int(*data_s8);
            data_s8 += cInc;
            dst[i][0] = int(*data_s8);
            data_s8++;
        }
        break;

#if (__BYTE_ORDER == __BIG_ENDIAN)
    case SF_s16_be:
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
    case SF_s16_le:
#endif
        for (int i = 0; i < n; i++) {
            dst[i][1] = int(*data_u16) >> 8;
            data_u16 += cInc;
            dst[i][0] = int(*data_u16) >> 8;
            data_u16++;
        }
        break;

#if (__BYTE_ORDER == __BIG_ENDIAN)
    case SF_s16_le:
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
    case SF_s16_be:
#endif
        for (int i = 0; i < n; i++) {
            dst[i][1] = int(*data_u16) & 255;
            data_u16 += cInc;
            dst[i][0] = int(*data_u16) & 255;
            data_u16++;
        }
        break;

#if (__BYTE_ORDER == __BIG_ENDIAN)
    case SF_u16_be:
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
    case SF_u16_le:
#endif
        for (int i = 0; i < n; i++) {
            dst[i][1] = (int(*data_s16) >> 8) - 128;
            data_s16 += cInc;
            dst[i][0] = (int(*data_s16) >> 8) - 128;
            data_s16++;
        }
        break;

#if (__BYTE_ORDER == __BIG_ENDIAN)
    case SF_u16_le:
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
    case SF_u16_be:
#endif
        for (int i = 0; i < n; i++) {
            dst[i][1] = (int(*data_s16) & 255) - 128;
            data_s16 += cInc;
            dst[i][0] = (int(*data_s16) & 255) - 128;
            data_s16++;
        }
        break;

    default:
        CTH_ERROR("internal error: wrong sound format.\n");
    }
}

void SoundDevice::newSD() {
    static int forked = 0;

    // soundDevice is the global strategy slot. File playback normally goes
    // through SoundDeviceFork once so the child can keep blocking reads away
    // from the visual frame loop; the child then creates SoundDeviceFile.
    switch (soundDeviceNr) {
    case SDN_DSPIn:
        soundDevice = ::new SoundDeviceDSPIn();
        break;
    case SDN_Net:
        soundDevice = ::new SoundDeviceNet();
        break;
    case SDN_Random:
        soundDevice = ::new SoundDeviceRandom();
        break;
    case SDN_File:
        if (!soundSilent && !forked) {
            forked++;
            soundDevice = ::new SoundDeviceFork();
        } else
            soundDevice = ::new SoundDeviceFile();
        break;
    default:
        CTH_ERROR("Illegal SoundDeviceNr %d.\n", int(soundDeviceNr));
        exit(0);
    }

    CTH_DEBUG("    channels           : %s\n", soundChannels.text());
    CTH_DEBUG("    sound format       : %s\n", soundFormat.text());
    CTH_DEBUG("    sample rate        : %d Hz\n", int(soundSampleRate));
    CTH_DEBUG("    DSP method         : %d\n", int(soundDSPMethod));
    CTH_DEBUG("    DSP fragments      : %d\n", int(soundDSPFragments));
    CTH_DEBUG("    DSP fragment size  : %d\n", int(soundDSPFragmentSize));
    CTH_DEBUG("    DSP sync.          : %d\n", int(soundDSPSync));

    if ((soundDevice == NULL) || (soundDevice->error != 0)) {
        delete soundDevice;

        CTH_ERROR("Can not use requested sound device. Using random noise.\n");
        soundDevice = ::new SoundDeviceRandom;
    }

    soundDevice->setTmpData();
}
