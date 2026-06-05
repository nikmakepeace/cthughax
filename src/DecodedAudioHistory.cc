// Decoded PCM ring buffer and visual-frame builder.

#include "cthugha.h"
#include "Audio.h"
#include "imath.h"

AudioBuffer::AudioBuffer(int capacitySamples_, int bytesPerSample_, int protectedHistorySamples_)
    : data(NULL)
    , bytesPerSampleValue(bytesPerSample_)
    , capacitySamples(capacitySamples_)
    , protectedHistorySamples(protectedHistorySamples_)
    , decodedEndSample(0)
    , submittedEndSample(0) {
    if (bytesPerSampleValue < 1)
        bytesPerSampleValue = 1;
    if (capacitySamples < 1)
        capacitySamples = 1;
    if (protectedHistorySamples < 0)
        protectedHistorySamples = 0;
    if (protectedHistorySamples > capacitySamples)
        protectedHistorySamples = capacitySamples;
    data = new char[pcmBytesForSamples(capacitySamples, bytesPerSampleValue)];
    CTH_DEBUG("audio buffer: created capacity-samples=%d bytes-per-sample=%d protected-history-samples=%d\n",
        capacitySamples, bytesPerSampleValue, protectedHistorySamples);
}

AudioBuffer::~AudioBuffer() {
    delete[] data;
    data = NULL;
}

void AudioBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    decodedEndSample = 0;
    submittedEndSample = 0;
}

long long AudioBuffer::protectedWindowStartSample() const {
    // Protected span:
    //   [protectedWindowStartSample(), submittedEndSample) = recent submitted
    //       history retained for visual-frame reads.
    //   [submittedEndSample, decodedEndSample) = decoded PCM queued ahead of output.
    // Future driver writes may overwrite only data outside this span.
    long long historyStartSample = submittedEndSample - protectedHistorySamples;
    long long capacityStartSample = decodedEndSample - capacitySamples;
    long long startSample = (historyStartSample > capacityStartSample) ? historyStartSample : capacityStartSample;

    if (startSample < 0)
        startSample = 0;
    if (startSample > submittedEndSample)
        startSample = submittedEndSample;

    return startSample;
}

int AudioBuffer::queuedForOutputSamples() const {
    std::lock_guard<std::mutex> lock(mutex);
    return int(decodedEndSample - submittedEndSample);
}

int AudioBuffer::protectedWindowSamples() const {
    std::lock_guard<std::mutex> lock(mutex);
    return int(decodedEndSample - protectedWindowStartSample());
}

int AudioBuffer::writableSamples() const {
    std::lock_guard<std::mutex> lock(mutex);
    return capacitySamples - int(decodedEndSample - protectedWindowStartSample());
}

long long AudioBuffer::oldestProtectedPosition() const {
    std::lock_guard<std::mutex> lock(mutex);
    return protectedWindowStartSample();
}

long long AudioBuffer::decodedEndPosition() const {
    std::lock_guard<std::mutex> lock(mutex);
    return decodedEndSample;
}

long long AudioBuffer::submittedEndPosition() const {
    std::lock_guard<std::mutex> lock(mutex);
    return submittedEndSample;
}

int AudioBuffer::copyAt(long long samplePosition, char* dst, int samples) const {
    int copiedSamples = 0;
    long long startSample = protectedWindowStartSample();

    if ((samples <= 0) || (samplePosition < startSample) || (samplePosition >= decodedEndSample))
        return 0;

    long long availableSamples = decodedEndSample - samplePosition;
    int wantedSamples = (samples < availableSamples) ? samples : int(availableSamples);

    while (copiedSamples < wantedSamples) {
        int posSample = int((samplePosition + copiedSamples) % capacitySamples);
        int chunkSamples = min(wantedSamples - copiedSamples, capacitySamples - posSample);
        memcpy(dst + pcmBytesForSamples(copiedSamples, bytesPerSampleValue),
            data + pcmBytesForSamples(posSample, bytesPerSampleValue),
            pcmBytesForSamples(chunkSamples, bytesPerSampleValue));
        copiedSamples += chunkSamples;
    }

    return copiedSamples;
}

int AudioBuffer::appendDecodedPcm(const char* src, int samples) {
    std::lock_guard<std::mutex> lock(mutex);
    int writtenSamples = 0;
    int protectedSamples = int(decodedEndSample - protectedWindowStartSample());
    int wantedSamples = min(samples, capacitySamples - protectedSamples);

    while (writtenSamples < wantedSamples) {
        int posSample = int((decodedEndSample + writtenSamples) % capacitySamples);
        int chunkSamples = min(wantedSamples - writtenSamples, capacitySamples - posSample);
        memcpy(data + pcmBytesForSamples(posSample, bytesPerSampleValue),
            src + pcmBytesForSamples(writtenSamples, bytesPerSampleValue),
            pcmBytesForSamples(chunkSamples, bytesPerSampleValue));
        writtenSamples += chunkSamples;
    }

    decodedEndSample += writtenSamples;

    return writtenSamples;
}

int AudioBuffer::peekForOutput(char* dst, int samples) const {
    std::lock_guard<std::mutex> lock(mutex);
    return copyAt(submittedEndSample, dst, samples);
}

int AudioBuffer::commitOutputSamples(int samples) {
    std::lock_guard<std::mutex> lock(mutex);
    int queuedSamples = int(decodedEndSample - submittedEndSample);
    int committedSamples = samples;

    if (committedSamples <= 0)
        return 0;
    if (committedSamples > queuedSamples)
        committedSamples = queuedSamples;

    submittedEndSample += committedSamples;

    return committedSamples;
}

int AudioBuffer::readProtectedPcmAt(long long samplePosition, char* dst, int samples) const {
    std::lock_guard<std::mutex> lock(mutex);
    return copyAt(samplePosition, dst, samples);
}

AudioFrameBuilder::AudioFrameBuilder()
    : rawData(NULL)
    , rawCapacity(0) { }

AudioFrameBuilder::~AudioFrameBuilder() {
    delete[] rawData;
    rawData = NULL;
    rawCapacity = 0;
}

void AudioFrameBuilder::setRawCapacity(int rawBytes) {
    if (rawCapacity >= rawBytes)
        return;

    delete[] rawData;
    rawCapacity = rawBytes;
    rawData = new char[rawCapacity];
    CTH_DEBUG("audio frame builder: resized raw buffer to %d bytes\n", rawCapacity);
}

void AudioFrameBuilder::build(AudioFrame& frame, const AudioBuffer& buffer, long long centerSample) {
    int bytesPerSample = buffer.bytesPerSample();
    int rawBytes;
    int halfFrameSamples;
    long long startSample;
    int sampleOffset;
    int samplesRead;

    frame.clear();
    frame.centerSample = centerSample;

    if (bytesPerSample <= 0)
        return;

    rawBytes = pcmBytesForSamples(1024, bytesPerSample);
    halfFrameSamples = 1024 / 2;
    setRawCapacity(rawBytes);
    memset(rawData, 0, rawBytes);

    startSample = centerSample - halfFrameSamples;
    sampleOffset = 0;
    if (startSample < 0) {
        sampleOffset = int(-startSample);
        startSample = 0;
    }

    long long oldestSample = buffer.oldestProtectedPosition();
    if (startSample < oldestSample) {
        long long skippedSamples = oldestSample - startSample;
        if (skippedSamples < 1024)
            sampleOffset += int(skippedSamples);
        else
            sampleOffset = 0;
        startSample = oldestSample;
    }
    if (sampleOffset >= 1024) {
        CTH_TRACE("no overlap center-sample=%lld oldest-sample=%lld decoded-end-sample=%lld\n",
            "audio frame builder", centerSample, oldestSample, buffer.decodedEndPosition());
        return;
    }

    samplesRead = buffer.readProtectedPcmAt(startSample,
        rawData + pcmBytesForSamples(sampleOffset, bytesPerSample),
        1024 - sampleOffset);
    if (samplesRead <= 0) {
        CTH_TRACE("no samples center-sample=%lld start-sample=%lld\n", "audio frame builder",
            centerSample, startSample);
        return;
    }

    convert(frame.raw + sampleOffset,
        rawData + pcmBytesForSamples(sampleOffset, bytesPerSample), samplesRead);
    memcpy(frame.processedWaveData, frame.raw, sizeof(frame.processedWaveData));
    frame.samples = sampleOffset + samplesRead;

    CTH_TRACE("built frame center-sample=%lld start-sample=%lld samples=%d raw-bytes=%d\n", "audio frame builder",
        frame.centerSample, startSample, frame.samples,
        pcmBytesForSamples(frame.samples, bytesPerSample));
}

void AudioFrameBuilder::convert(char2* dst, void* src, int n) {
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
