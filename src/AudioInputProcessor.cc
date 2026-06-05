// Live-input rolling frame processor.

#include "cthugha.h"
#include "Audio.h"
#include "AudioInternal.h"
#include "imath.h"

AudioInputProcessor::AudioInputProcessor(AudioInput* input_, int visualMaxDimension,
    int takeOwnership)
    : input(input_)
    , inputOwned(takeOwnership)
    , tmpData(NULL)
    , tmpSize(0)
    , rawSize(0)
    , bytesPerSample(0) {
    size = audioSampleWindowForVisualMaxDimension(visualMaxDimension);
    data = new char2[1024];
    memset(data, 0, 1024 * sizeof(char2));
    memset(dataProc, 0, 1024 * sizeof(char2));
    setTmpData();
}

AudioInputProcessor::~AudioInputProcessor() {
    delete[] tmpData;
    tmpData = NULL;

    delete[] data;
    data = NULL;

    if (inputOwned)
        delete input;
    input = NULL;
}

void AudioInputProcessor::setTmpData() {
    bytesPerSample = audioBytesPerSample();
    rawSize = pcmBytesForSamples(size, bytesPerSample);
    int requestedTmpSize = input ? input->rawBufferSize(rawSize, size) : rawSize;
    if (requestedTmpSize < rawSize)
        requestedTmpSize = rawSize;

    if (tmpSize < requestedTmpSize) {
        CTH_DEBUG("audio processor: resizing raw buffer from %d to %d bytes (frame=%d)\n",
            tmpSize, requestedTmpSize, rawSize);
        delete[] tmpData;
        tmpSize = requestedTmpSize;
        tmpData = new char[tmpSize];
    }
}

void AudioInputProcessor::operator()() {
    if (input == NULL)
        return;

    bytesPerSample = audioBytesPerSample();
    rawSize = pcmBytesForSamples(size, bytesPerSample);
    setTmpData();

    int r = input->read(tmpData, rawSize, size);
    if (r < 0)
        r = 0;

    if (r >= 1024)
        convert(data, tmpData + pcmBytesForSamples(r - 1024, bytesPerSample), 1024);
    else {
        memcpy(data, data + r, sizeof(char2) * (1024 - r));
        convert(data + 1024 - r, tmpData, r);
    }
}

void AudioInputProcessor::change() {
    if (input)
        input->update();
    tmpSize = 0;
    setTmpData();
}

void AudioInputProcessor::convert(char2* dst, void* src, int n) {
    unsigned char* data_u8 = (unsigned char*)src;
    char* data_s8 = (char*)src;
    unsigned short* data_u16 = (unsigned short*)src;
    short* data_s16 = (short*)src;

    int cInc = (audioChannels() == 1) ? 0 : 1;

    switch (audioSampleFormat()) {
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
