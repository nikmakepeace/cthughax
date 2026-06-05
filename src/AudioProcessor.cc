#include "cthugha.h"
#include "Audio.h"
#include "AudioFftProcessor.h"
#include "AudioFrame.h"
#include "AudioProcessor.h"

#include <math.h>

AudioProcessor::AudioProcessor()
    : fftProcessorValue(&defaultAudioFftProcessor()) { }

AudioProcessor::AudioProcessor(AudioFftProcessor& fftProcessor)
    : fftProcessorValue(&fftProcessor) { }

AudioMetrics AudioProcessor::analyze(const char2* frame, int minNoise) const {
    AudioMetrics metrics;
    int al = 0, ar = 0;

    if (frame == 0)
        return metrics;

    /* Get the amplitude of this sound frame as root mean squared. */
    const char* d = (const char*)frame;
    for (int i = 1024; i != 0; i--) {
        al += *d * *d;
        d++;
        ar += *d * *d;
        d++;
    }

    al = int(sqrt(double(al) / 1024));
    ar = int(sqrt(double(ar) / 1024));

    metrics.amplitude = (al + ar) / 2;
    metrics.amplitudeLeft = al;
    metrics.amplitudeRight = ar;
    metrics.noisy = ((metrics.amplitudeLeft >= minNoise)
        || (metrics.amplitudeRight >= minNoise));
    return metrics;
}

void AudioProcessor::analyze(AudioFrame& frame, int minNoise) const {
    frame.metrics = analyze(frame.raw, minNoise);
}

void AudioProcessor::none(AudioFrame& frame) {
    none(frame.raw, frame.processedWaveData);
}

void AudioProcessor::filter1(AudioFrame& frame) {
    filter1(frame.raw, frame.processedWaveData);
}

void AudioProcessor::filter2(AudioFrame& frame) {
    filter2(frame.raw, frame.processedWaveData);
}

void AudioProcessor::fft(AudioFrame& frame) {
    fft(frame.raw, frame.processedWaveData);
}

void AudioProcessor::none(char2* raw, char2* processedWaveData) {
    memcpy(processedWaveData, raw, 1024 * sizeof(char2));
}

void AudioProcessor::filter1(char2* raw, char2* processedWaveData) {
    memcpy(processedWaveData, raw, 1024 * sizeof(char2));

    int temp = processedWaveData[0][1];
    int temp2 = processedWaveData[0][0];
    for (int x = 1; x < 1024; x++) {
        if ((processedWaveData[x][1] - temp) > 10)
            processedWaveData[x][1] = temp + 10;
        else if ((processedWaveData[x][1] - temp) < -10)
            processedWaveData[x][1] = temp - 10;

        if ((processedWaveData[x][0] - temp2) > 10)
            processedWaveData[x][0] = temp2 + 10;
        else if ((processedWaveData[x][0] - temp2) < -10)
            processedWaveData[x][0] = temp2 - 10;

        temp = processedWaveData[x][1];
        temp2 = processedWaveData[x][0];
    }
}

void AudioProcessor::filter2(char2* raw, char2* processedWaveData) {
    int temp = raw[0][1];
    int temp2 = raw[0][0];
    for (int x = 1; x < 1024; x++) {
        processedWaveData[x][1] = processedWaveData[x - 1][1] + (raw[x][1] - temp) / 16;
        processedWaveData[x][0] = processedWaveData[x - 1][0] + (raw[x][0] - temp2) / 16;
        temp2 = processedWaveData[x][0];
        temp = processedWaveData[x][1];
    }
}

void AudioProcessor::fft(char2* raw, char2* processedWaveData) {
    fftProcessorValue->transform(raw, processedWaveData);
}
