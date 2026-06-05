#include "cthugha.h"
#include "AudioFrame.h"
#include "AudioOptions.h"

static char2 silentAudioFrameRawData[1024];
static char2 silentAudioFrameProcessedWaveData[1024];
static AudioMetrics audioFrameFallbackMetrics;
static AudioFrame* currentAudioFrame = NULL;

#ifdef CTH_AUDIO_FRAME_TEST_OVERRIDE
static AudioFrame* testAudioFrameOverride = NULL;

void audioFrameSetTestOverride(AudioFrame* frame) {
    testAudioFrameOverride = frame;
}
#endif

AudioMetrics::AudioMetrics()
    : amplitude(0)
    , amplitudeLeft(0)
    , amplitudeRight(0)
    , noisy(0) { }

AudioFrame::AudioFrame() {
    clear();
}

void AudioFrame::clear() {
    centerSample = 0;
    samples = 0;
    memset(raw, 0, sizeof(raw));
    memset(processedWaveData, 0, sizeof(processedWaveData));
    metrics = AudioMetrics();
}

void audioFrameSetCurrent(AudioFrame* frame) {
    currentAudioFrame = frame;
}

AudioFrame* audioFrameCurrent() {
#ifdef CTH_AUDIO_FRAME_TEST_OVERRIDE
    if (testAudioFrameOverride != NULL)
        return testAudioFrameOverride;
#endif
    return currentAudioFrame;
}

char2* audioFrameRawData() {
#ifdef CTH_AUDIO_FRAME_TEST_OVERRIDE
    if (testAudioFrameOverride != NULL)
        return testAudioFrameOverride->raw;
#endif
    if (currentAudioFrame != NULL)
        return currentAudioFrame->raw;

    return silentAudioFrameRawData;
}

char2* audioFrameProcessedWaveData() {
#ifdef CTH_AUDIO_FRAME_TEST_OVERRIDE
    if (testAudioFrameOverride != NULL)
        return testAudioFrameOverride->processedWaveData;
#endif
    if (currentAudioFrame != NULL)
        return currentAudioFrame->processedWaveData;

    return silentAudioFrameProcessedWaveData;
}

void audioFramePublishMetrics(const AudioMetrics& metrics) {
    AudioFrame* frame = audioFrameCurrent();
    if (frame != NULL) {
        frame->metrics = metrics;
        return;
    }

    audioFrameFallbackMetrics = metrics;
}

const AudioMetrics& audioFrameMetrics() {
    AudioFrame* frame = audioFrameCurrent();
    if (frame != NULL)
        return frame->metrics;

    return audioFrameFallbackMetrics;
}

int audioFrameBroadcastBytes() {
#ifdef CTH_AUDIO_FRAME_TEST_OVERRIDE
    if (testAudioFrameOverride != NULL) {
        int bytesPerSample = audioBytesPerSample();
        return testAudioFrameOverride->samples * bytesPerSample;
    }
#endif
    if (currentAudioFrame != NULL) {
        int bytesPerSample = audioBytesPerSample();
        return currentAudioFrame->samples * bytesPerSample;
    }

    return 0;
}
