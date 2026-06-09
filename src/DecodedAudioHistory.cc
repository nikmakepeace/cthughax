/** @file
 * Decoded PCM history, passthrough cursor, and visual-frame builder.
 */

#include "config.h"
#include "Audio.h"
#include "ProcessServices.h"
#include "imath.h"

#include <string.h>

DecodedAudioHistory::DecodedAudioHistory(int capacitySamples_, const PcmFormat& format_,
    int retainedHistorySamples_, LogSink& log_)
    : data(NULL)
    , log(log_)
    , pcmFormatValue(format_)
    , bytesPerSampleValue(pcmFormatValue.bytesPerSample())
    , capacitySamples(capacitySamples_)
    , retainedHistorySamples(retainedHistorySamples_)
    , decodedEndSample(0) {
    if (bytesPerSampleValue < 1)
        bytesPerSampleValue = 1;
    if (capacitySamples < 1)
        capacitySamples = 1;
    if (retainedHistorySamples < 0)
        retainedHistorySamples = 0;
    if (retainedHistorySamples >= capacitySamples)
        retainedHistorySamples = capacitySamples - 1;
    data = new char[pcmBytesForSamples(capacitySamples, bytesPerSampleValue)];
    log.debug("decoded audio history: created capacity-samples=%d rate=%d channels=%d format=%d bytes-per-sample=%d retained-history-samples=%d\n",
        capacitySamples, pcmFormatValue.sampleRate, pcmFormatValue.channels,
        pcmFormatValue.sampleFormat, bytesPerSampleValue, retainedHistorySamples);
}

DecodedAudioHistory::~DecodedAudioHistory() {
    delete[] data;
    data = NULL;
}

void DecodedAudioHistory::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    decodedEndSample = 0;
}

long long DecodedAudioHistory::retainedWindowStartSample() const {
    long long historyStartSample = decodedEndSample - retainedHistorySamples;
    long long capacityStartSample = decodedEndSample - capacitySamples;
    long long startSample = (historyStartSample > capacityStartSample) ? historyStartSample : capacityStartSample;

    if (startSample < 0)
        startSample = 0;
    if (startSample > decodedEndSample)
        startSample = decodedEndSample;

    return startSample;
}

int DecodedAudioHistory::retainedWindowSamples() const {
    std::lock_guard<std::mutex> lock(mutex);
    return int(decodedEndSample - retainedWindowStartSample());
}

int DecodedAudioHistory::writableSamples() const {
    std::lock_guard<std::mutex> lock(mutex);
    return capacitySamples - int(decodedEndSample - retainedWindowStartSample());
}

long long DecodedAudioHistory::oldestAvailablePosition() const {
    std::lock_guard<std::mutex> lock(mutex);
    return retainedWindowStartSample();
}

long long DecodedAudioHistory::decodedEndPosition() const {
    std::lock_guard<std::mutex> lock(mutex);
    return decodedEndSample;
}

int DecodedAudioHistory::copyAt(long long samplePosition, char* dst, int samples) const {
    int copiedSamples = 0;
    long long startSample = retainedWindowStartSample();

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

int DecodedAudioHistory::appendDecodedPcm(const char* src, int samples) {
    std::lock_guard<std::mutex> lock(mutex);
    int writtenSamples = 0;
    int retainedSamples = int(decodedEndSample - retainedWindowStartSample());
    int wantedSamples = min(samples, capacitySamples - retainedSamples);

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

int DecodedAudioHistory::readPcmAt(long long samplePosition, char* dst, int samples) const {
    std::lock_guard<std::mutex> lock(mutex);
    return copyAt(samplePosition, dst, samples);
}

AudioPlaybackClock::AudioPlaybackClock()
    : submittedEndSample(0)
    , presentationDelaySampleCount(0) { }

void AudioPlaybackClock::publishSubmittedEndSample(long long sample) {
    if (sample < 0)
        sample = 0;
    submittedEndSample.store(sample);
}

void AudioPlaybackClock::publishPresentationDelaySamples(int samples) {
    if (samples < 0)
        samples = 0;
    presentationDelaySampleCount.store(samples);
}

long long AudioPlaybackClock::submittedEndPosition() const {
    return submittedEndSample.load();
}

int AudioPlaybackClock::presentationDelaySamples() const {
    return presentationDelaySampleCount.load();
}

long long AudioPlaybackClock::presentationCenterSample() const {
    long long sample = submittedEndSample.load()
        - presentationDelaySampleCount.load();
    return sample > 0 ? sample : 0;
}

AudioOutputStream::AudioOutputStream(const DecodedAudioHistory& history_,
    AudioPlaybackClock* playbackClock_)
    : history(history_)
    , playbackClock(playbackClock_)
    , submittedEndSample(0) { }

void AudioOutputStream::reset(long long samplePosition) {
    std::lock_guard<std::mutex> lock(mutex);
    submittedEndSample = samplePosition;
    if (submittedEndSample < 0)
        submittedEndSample = 0;
    if (playbackClock != NULL)
        playbackClock->publishSubmittedEndSample(submittedEndSample);
}

int AudioOutputStream::bytesPerSample() const {
    return history.bytesPerSample();
}

const PcmFormat& AudioOutputStream::format() const {
    return history.format();
}

long long AudioOutputStream::decodedEndPosition() const {
    return history.decodedEndPosition();
}

long long AudioOutputStream::submittedEndPosition() const {
    std::lock_guard<std::mutex> lock(mutex);
    return submittedEndSample;
}

int AudioOutputStream::queuedForOutputSamples() const {
    std::lock_guard<std::mutex> lock(mutex);
    long long decodedEndSample = history.decodedEndPosition();
    long long oldestSample = history.oldestAvailablePosition();
    long long cursor = submittedEndSample;

    if (cursor < oldestSample)
        cursor = oldestSample;
    if (cursor > decodedEndSample)
        return 0;

    return int(decodedEndSample - cursor);
}

int AudioOutputStream::peekForOutput(char* dst, int samples) const {
    std::lock_guard<std::mutex> lock(mutex);
    long long oldestSample = history.oldestAvailablePosition();
    long long cursor = submittedEndSample;

    if (cursor < oldestSample)
        cursor = oldestSample;

    return history.readPcmAt(cursor, dst, samples);
}

int AudioOutputStream::commitOutputSamples(int samples) {
    std::lock_guard<std::mutex> lock(mutex);
    long long decodedEndSample = history.decodedEndPosition();
    long long oldestSample = history.oldestAvailablePosition();
    int committedSamples = samples;

    if (submittedEndSample < oldestSample)
        submittedEndSample = oldestSample;
    if (committedSamples <= 0)
        return 0;

    long long queuedSamples = decodedEndSample - submittedEndSample;
    if (queuedSamples <= 0)
        return 0;
    if (committedSamples > queuedSamples)
        committedSamples = int(queuedSamples);

    submittedEndSample += committedSamples;
    if (playbackClock != NULL)
        playbackClock->publishSubmittedEndSample(submittedEndSample);

    return committedSamples;
}

int AudioOutputStream::resyncIfBehind() {
    std::lock_guard<std::mutex> lock(mutex);
    long long oldestSample = history.oldestAvailablePosition();

    if (submittedEndSample >= oldestSample)
        return 0;

    int skippedSamples = int(oldestSample - submittedEndSample);
    submittedEndSample = oldestSample;
    if (playbackClock != NULL)
        playbackClock->publishSubmittedEndSample(submittedEndSample);
    return skippedSamples;
}

AudioFrameBuilder::AudioFrameBuilder(LogSink& log_)
    : rawData(NULL)
    , rawCapacity(0)
    , log(log_) { }

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
    log.debug("audio frame builder: resized raw buffer to %d bytes\n", rawCapacity);
}

void AudioFrameBuilder::build(AudioFrame& frame, const DecodedAudioHistory& history,
    long long centerSample) {
    int bytesPerSample = history.bytesPerSample();
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

    long long oldestSample = history.oldestAvailablePosition();
    if (startSample < oldestSample) {
        long long skippedSamples = oldestSample - startSample;
        if (skippedSamples < 1024)
            sampleOffset += int(skippedSamples);
        else
            sampleOffset = 0;
        startSample = oldestSample;
    }
    if (sampleOffset >= 1024) {
        log.trace("audio frame builder",
            "no overlap center-sample=%lld oldest-sample=%lld decoded-end-sample=%lld\n",
            centerSample, oldestSample, history.decodedEndPosition());
        return;
    }

    samplesRead = history.readPcmAt(startSample,
        rawData + pcmBytesForSamples(sampleOffset, bytesPerSample),
        1024 - sampleOffset);
    if (samplesRead <= 0) {
        log.trace("audio frame builder",
            "no samples center-sample=%lld start-sample=%lld\n",
            centerSample, startSample);
        return;
    }

    convert(frame.raw + sampleOffset,
        rawData + pcmBytesForSamples(sampleOffset, bytesPerSample), samplesRead,
        history.format());
    memcpy(frame.processedWaveData, frame.raw, sizeof(frame.processedWaveData));
    frame.samples = sampleOffset + samplesRead;

    log.trace("audio frame builder",
        "built frame center-sample=%lld start-sample=%lld samples=%d raw-bytes=%d\n",
        frame.centerSample, startSample, frame.samples,
        pcmBytesForSamples(frame.samples, bytesPerSample));
}

void AudioFrameBuilder::convert(char2* dst, void* src, int n,
    const PcmFormat& format) {
    AudioByte* data_u8 = (AudioByte*)src;
    AudioByte* data_s8 = (AudioByte*)src;
    unsigned short* data_u16 = (unsigned short*)src;

    int cInc = (format.channels == 1) ? 0 : 1;

    switch (format.sampleFormat) {
    case SF_u8:
        for (int i = 0; i < n; i++) {
            dst[i][1] = audioSampleFromUnsigned8Byte(*data_u8);
            data_u8 += cInc;
            dst[i][0] = audioSampleFromUnsigned8Byte(*data_u8);
            data_u8++;
        }
        break;

    case SF_s8:
        for (int i = 0; i < n; i++) {
            dst[i][1] = audioSampleFromSigned8Byte(*data_s8);
            data_s8 += cInc;
            dst[i][0] = audioSampleFromSigned8Byte(*data_s8);
            data_s8++;
        }
        break;

#ifdef WORDS_BIGENDIAN
    case SF_s16_be:
#else
    case SF_s16_le:
#endif
        for (int i = 0; i < n; i++) {
            dst[i][1] = audioSampleFromSigned8Byte(AudioByte(int(*data_u16) >> 8));
            data_u16 += cInc;
            dst[i][0] = audioSampleFromSigned8Byte(AudioByte(int(*data_u16) >> 8));
            data_u16++;
        }
        break;

#ifdef WORDS_BIGENDIAN
    case SF_s16_le:
#else
    case SF_s16_be:
#endif
        for (int i = 0; i < n; i++) {
            dst[i][1] = audioSampleFromSigned8Byte(AudioByte(int(*data_u16) & 255));
            data_u16 += cInc;
            dst[i][0] = audioSampleFromSigned8Byte(AudioByte(int(*data_u16) & 255));
            data_u16++;
        }
        break;

#ifdef WORDS_BIGENDIAN
    case SF_u16_be:
#else
    case SF_u16_le:
#endif
        for (int i = 0; i < n; i++) {
            dst[i][1] = audioSampleFromUnsigned8Byte(AudioByte(int(*data_u16) >> 8));
            data_u16 += cInc;
            dst[i][0] = audioSampleFromUnsigned8Byte(AudioByte(int(*data_u16) >> 8));
            data_u16++;
        }
        break;

#ifdef WORDS_BIGENDIAN
    case SF_u16_le:
#else
    case SF_u16_be:
#endif
        for (int i = 0; i < n; i++) {
            dst[i][1] = audioSampleFromUnsigned8Byte(AudioByte(int(*data_u16) & 255));
            data_u16 += cInc;
            dst[i][0] = audioSampleFromUnsigned8Byte(AudioByte(int(*data_u16) & 255));
            data_u16++;
        }
        break;

    default:
        log.error("internal error: wrong sound format.\n");
    }
}
