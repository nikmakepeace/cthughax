#include "Audio.h"
#include "AudioFrame.h"
#include "AudioProcessor.h"

#include <math.h>
#include <string.h>

namespace {

static const int kDefaultAudioProcessorSampleRateHz = 44000;
static const double kLowPass150HzCutoff = 150.0;
static const double kTwoPi = 6.28318530717958647692;

static int normalizedSampleRateHz(int sampleRateHz) {
    return sampleRateHz > 0 ? sampleRateHz
                            : kDefaultAudioProcessorSampleRateHz;
}

static double lowPassAlpha150Hz(int sampleRateHz) {
    double sampleInterval = 1.0 / double(normalizedSampleRateHz(sampleRateHz));
    double rc = 1.0 / (kTwoPi * kLowPass150HzCutoff);
    return sampleInterval / (rc + sampleInterval);
}

}

AudioProcessor::AudioProcessor()
    : defaultFftProcessorValue()
    , fftProcessorValue(&defaultFftProcessorValue)
    , sampleRateHzValue(kDefaultAudioProcessorSampleRateHz) { }

AudioProcessor::AudioProcessor(int sampleRateHz)
    : defaultFftProcessorValue()
    , fftProcessorValue(&defaultFftProcessorValue)
    , sampleRateHzValue(normalizedSampleRateHz(sampleRateHz)) { }

AudioProcessor::AudioProcessor(AudioFftProcessor& fftProcessor)
    : defaultFftProcessorValue()
    , fftProcessorValue(&fftProcessor)
    , sampleRateHzValue(kDefaultAudioProcessorSampleRateHz) { }

AudioProcessor::AudioProcessor(AudioFftProcessor& fftProcessor,
    int sampleRateHz)
    : defaultFftProcessorValue()
    , fftProcessorValue(&fftProcessor)
    , sampleRateHzValue(normalizedSampleRateHz(sampleRateHz)) { }

AudioMetrics AudioProcessor::analyze(const char2* frame, int minNoise) const {
    AudioMetrics metrics;
    int al = 0, ar = 0;
    double lowPassLeft = 0.0;
    double lowPassRight = 0.0;
    double lowPassLeftSum = 0.0;
    double lowPassRightSum = 0.0;
    double alpha = lowPassAlpha150Hz(sampleRateHzValue);

    if (frame == 0)
        return metrics;

    lowPassLeft = double(frame[0][0]);
    lowPassRight = double(frame[0][1]);

    /* Get raw and low-pass amplitudes for this frame as root mean squared. */
    for (int i = 0; i < 1024; i++) {
        int left = int(frame[i][0]);
        int right = int(frame[i][1]);
        al += left * left;
        ar += right * right;

        if (i != 0) {
            lowPassLeft += alpha * (double(left) - lowPassLeft);
            lowPassRight += alpha * (double(right) - lowPassRight);
        }
        lowPassLeftSum += lowPassLeft * lowPassLeft;
        lowPassRightSum += lowPassRight * lowPassRight;
    }

    al = int(sqrt(double(al) / 1024));
    ar = int(sqrt(double(ar) / 1024));

    metrics.amplitude = (al + ar) / 2;
    metrics.amplitudeLeft = al;
    metrics.amplitudeRight = ar;
    metrics.lowPass150HzAmplitudeLeft = int(sqrt(lowPassLeftSum / 1024.0));
    metrics.lowPass150HzAmplitudeRight = int(sqrt(lowPassRightSum / 1024.0));
    metrics.lowPass150HzAmplitude = (metrics.lowPass150HzAmplitudeLeft
        + metrics.lowPass150HzAmplitudeRight) / 2;
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
