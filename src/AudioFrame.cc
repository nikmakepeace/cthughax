#include "cthugha.h"
#include "AudioFrame.h"
#include "AudioOptions.h"
#ifndef CTH_AUDIO_FRAME_NO_RUNTIME
#include "AudioRuntime.h"
#endif

static char2 silentAudioFrameRawData[1024];
static char2 silentAudioFrameProcessedWaveData[1024];
static AudioMetrics audioFrameFallbackMetrics;

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

void audioFrameTick() {
#ifndef CTH_AUDIO_FRAME_NO_RUNTIME
    double tickStart = getTime();
    audioRuntimeTick();
    CTH_TRACE("audioFrameTick-ms=%.3f\n", "audio timing", (getTime() - tickStart) * 1000.0);
#endif
}

void audioFrameChange() {
#ifndef CTH_AUDIO_FRAME_NO_RUNTIME
    if (audioRuntimeProcessor()) {
        audioRuntimeProcessor()->change();
        return;
    }
#endif
}

AudioFrame* audioFrameCurrent() {
#ifdef CTH_AUDIO_FRAME_TEST_OVERRIDE
    if (testAudioFrameOverride != NULL)
        return testAudioFrameOverride;
#endif
#ifdef CTH_AUDIO_FRAME_NO_RUNTIME
    return NULL;
#else
    return audioRuntimeCurrentFrame();
#endif
}

char2* audioFrameRawData() {
#ifdef CTH_AUDIO_FRAME_TEST_OVERRIDE
    if (testAudioFrameOverride != NULL)
        return testAudioFrameOverride->raw;
#endif
#ifndef CTH_AUDIO_FRAME_NO_RUNTIME
    if (audioRuntimeCurrentFrame())
        return audioRuntimeCurrentFrame()->raw;

    if (audioRuntimeProcessor())
        return audioRuntimeProcessor()->data;
#endif

    return silentAudioFrameRawData;
}

char2* audioFrameProcessedWaveData() {
#ifdef CTH_AUDIO_FRAME_TEST_OVERRIDE
    if (testAudioFrameOverride != NULL)
        return testAudioFrameOverride->processedWaveData;
#endif
#ifndef CTH_AUDIO_FRAME_NO_RUNTIME
    if (audioRuntimeCurrentFrame())
        return audioRuntimeCurrentFrame()->processedWaveData;

    if (audioRuntimeProcessor())
        return audioRuntimeProcessor()->dataProc;
#endif

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
#ifndef CTH_AUDIO_FRAME_NO_RUNTIME
    if (audioRuntimeCurrentFrame()) {
        int bytesPerSample = audioBytesPerSample();
        return audioRuntimeCurrentFrame()->samples * bytesPerSample;
    }

    if (audioRuntimeProcessor())
        return audioRuntimeProcessor()->frameRawSize();
#endif

    return 0;
}
