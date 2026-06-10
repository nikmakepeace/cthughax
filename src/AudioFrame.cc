#include "AudioFrame.h"

#include <string.h>

AudioMetrics::AudioMetrics()
    : amplitude(0)
    , amplitudeLeft(0)
    , amplitudeRight(0)
    , lowPass150HzAmplitude(0)
    , lowPass150HzAmplitudeLeft(0)
    , lowPass150HzAmplitudeRight(0)
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
