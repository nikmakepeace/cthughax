#include "cthugha.h"
#include "Audio.h"
#include "imath.h"
#include "cth_buffer.h"

AudioInput::AudioInput()
    : error(0) { }

AudioInput::~AudioInput() { }

AudioOutput::~AudioOutput() { }

AudioProcessor::AudioProcessor(AudioInput* input_, int takeOwnership)
    : input(input_)
    , inputOwned(takeOwnership)
    , tmpData(NULL)
    , tmpSize(0)
    , rawSize(0)
    , bytesPerSample(0) {
    size = 1 << ilog2(max(BUFF_WIDTH, BUFF_HEIGHT));
    data = new char2[1024];
    memset(data, 0, 1024 * sizeof(char2));
    memset(dataProc, 0, 1024 * sizeof(char2));
    setTmpData();
}

AudioProcessor::~AudioProcessor() {
    delete[] tmpData;
    tmpData = NULL;

    delete[] data;
    data = NULL;

    if (inputOwned)
        delete input;
    input = NULL;
}

void AudioProcessor::setTmpData() {
    bytesPerSample = (soundFormat < 2) ? soundChannels : 2 * soundChannels;
    rawSize = bytesPerSample * size;

    if (tmpSize < rawSize) {
        delete[] tmpData;
        tmpSize = rawSize;
        tmpData = new char[tmpSize];
    }
}

void AudioProcessor::operator()() {
    if (input == NULL)
        return;

    bytesPerSample = (soundFormat < 2) ? soundChannels : 2 * soundChannels;
    rawSize = bytesPerSample * size;
    setTmpData();

    int r = input->read(tmpData, rawSize, size);
    if (r < 0)
        r = 0;

    if (r >= 1024)
        convert(data, tmpData + (r - 1024) * bytesPerSample, 1024);
    else {
        memcpy(data, data + r, sizeof(char2) * (1024 - r));
        convert(data + 1024 - r, tmpData, r);
    }
}

void AudioProcessor::change() {
    if (input)
        input->update();
    tmpSize = 0;
    setTmpData();
}

void AudioProcessor::convert(char2* dst, void* src, int n) {
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

AudioRandomInput::AudioRandomInput()
    : AudioInput()
    , v1(0)
    , v2(0)
    , maxdv(2) {
    update();
}

int AudioRandomInput::read(char* dst, int rawSize, int samplesRequested) {
    int samples = min(samplesRequested, rawSize / int(sizeof(char2)));
    samples = min(samples, 256);

    if (samples <= 0)
        return 0;

    char2* sound_data = (char2*)dst;

    sound_data[0][0] = 144;
    sound_data[0][1] = 112;
    for (int x = 1; x < samples; x++) {
        if (rand() % 256 > sound_data[x - 1][0])
            v1 += rand() % maxdv;
        else
            v1 -= rand() % maxdv;

        if (rand() % 256 > sound_data[x - 1][1])
            v2 += rand() % maxdv;
        else
            v2 -= rand() % maxdv;

        sound_data[x][0] = sound_data[x - 1][0] + v1;
        sound_data[x][1] = sound_data[x - 1][1] + v2;
    }

    return samples;
}

void AudioRandomInput::update() {
    soundFormat.setValue(SF_u8);
    soundChannels.setValue(2);
}
