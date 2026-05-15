#include "cthugha.h"

#include "SoundDevice.h"
#include "imath.h"
#include "cth_buffer.h"

int bytesPerSample = 0;

//
// SoundDevice
//

SoundDevice::SoundDevice() {

    error = 0;

    size = 1 << ilog2(max(BUFF_WIDTH, BUFF_HEIGHT));
    data = new char2[1024];

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
    rawSize = bytesPerSample * size;

    int r = this->read(); // returns nr. of samples read

    if (r >= 1024) // got more than enough data
        convert(data, (char*)tmpData + (r - 1024) * bytesPerSample, 1024);
    else {
        memcpy(data, data + r, sizeof(char2) * (1024 - r)); // keep old data
        convert(data + 1024 - r, tmpData, r); // and convert new samples
    }
}

int SoundDevice::read() {
    CTH_ERROR("internal error. SoundDevice::read() called.\n");
    exit(0);
}

void SoundDevice::change() {

    tmpSize = 0;

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
// #elif
// #error unknown endianess
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

    // create the right device
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

    // check for errors
    if ((soundDevice == NULL) || (soundDevice->error != 0)) {
        delete soundDevice;

        CTH_ERROR("Can not use requested sound device. Using random noise.\n");
        soundDevice = ::new SoundDeviceRandom;
    }

    soundDevice->setTmpData(); // make sure the temporary array is big enough
}
