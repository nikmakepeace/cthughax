/** @file
 * Unit coverage for AudioFrame metrics and AudioProcessor analysis.
 */

#include "Audio.h"
#include "AudioFftProcessor.h"
#include "AudioFrame.h"

#include <assert.h>
#include <stdarg.h>

int cth_log_enabled(int) {
    return 0;
}

int cth_log(int, const char*, ...) {
    return 0;
}

int cth_log_context(int, const char*, const char*, ...) {
    return 0;
}

int cth_log_error(const char*, ...) {
    return 0;
}

int cth_log_errno(int, const char*, ...) {
    return 0;
}

static void fillConstant(AudioFrame& frame, int left, int right) {
    for (int i = 0; i < 1024; i++) {
        frame.raw[i][0] = char(left);
        frame.raw[i][1] = char(right);
    }
}

static void fillPseudoMusic(AudioFrame& frame) {
    for (int i = 0; i < 1024; i++) {
        int left = ((i * 17 + (i * i) / 7 + ((i >> 3) * 31)) & 255) - 128;
        int right = ((i * 29 + (i * i) / 11 - ((i >> 2) * 13)) & 255) - 128;
        frame.raw[i][0] = char(left);
        frame.raw[i][1] = char(right);
    }
}

static void fillAlternating(AudioFrame& frame, int left, int right) {
    for (int i = 0; i < 1024; i++) {
        int polarity = (i & 1) == 0 ? 1 : -1;
        frame.raw[i][0] = char(left * polarity);
        frame.raw[i][1] = char(right * polarity);
    }
}

static void testAudioFrameOwnsMetrics() {
    AudioFrame frame;

    frame.metrics.amplitude = 12;
    frame.metrics.amplitudeLeft = 10;
    frame.metrics.amplitudeRight = 14;
    frame.metrics.lowPass150HzAmplitude = 8;
    frame.metrics.lowPass150HzAmplitudeLeft = 7;
    frame.metrics.lowPass150HzAmplitudeRight = 9;
    frame.metrics.noisy = 1;

    frame.clear();

    assert(frame.metrics.amplitude == 0);
    assert(frame.metrics.amplitudeLeft == 0);
    assert(frame.metrics.amplitudeRight == 0);
    assert(frame.metrics.lowPass150HzAmplitude == 0);
    assert(frame.metrics.lowPass150HzAmplitudeLeft == 0);
    assert(frame.metrics.lowPass150HzAmplitudeRight == 0);
    assert(frame.metrics.noisy == 0);
}

static void testAudioProcessorAnalyzesRawFrame() {
    AudioProcessor processor;
    AudioFrame frame;

    fillConstant(frame, 3, 4);

    AudioMetrics quietMetrics = processor.analyze(frame.raw, 5);
    assert(quietMetrics.amplitudeLeft == 3);
    assert(quietMetrics.amplitudeRight == 4);
    assert(quietMetrics.amplitude == 3);
    assert(quietMetrics.lowPass150HzAmplitudeLeft == 3);
    assert(quietMetrics.lowPass150HzAmplitudeRight == 4);
    assert(quietMetrics.lowPass150HzAmplitude == 3);
    assert(quietMetrics.noisy == 0);

    processor.analyze(frame, 4);
    assert(frame.metrics.amplitudeLeft == 3);
    assert(frame.metrics.amplitudeRight == 4);
    assert(frame.metrics.amplitude == 3);
    assert(frame.metrics.lowPass150HzAmplitudeLeft == 3);
    assert(frame.metrics.lowPass150HzAmplitudeRight == 4);
    assert(frame.metrics.lowPass150HzAmplitude == 3);
    assert(frame.metrics.noisy == 1);
}

static void testLowPassAmplitudeSuppressesFastAlternation() {
    AudioProcessor processor(44000);
    AudioFrame frame;

    fillAlternating(frame, 100, 80);

    AudioMetrics metrics = processor.analyze(frame.raw, 5);

    assert(metrics.amplitudeLeft == 100);
    assert(metrics.amplitudeRight == 80);
    assert(metrics.lowPass150HzAmplitudeLeft < metrics.amplitudeLeft);
    assert(metrics.lowPass150HzAmplitudeRight < metrics.amplitudeRight);
    assert(metrics.lowPass150HzAmplitude < metrics.amplitude);
}

class RecordingFftProcessor : public AudioFftProcessor {
public:
    mutable int calls;

    RecordingFftProcessor()
        : calls(0) { }

    virtual void transform(
        const char2* raw, char2* processedWaveData) const {
        calls++;
        processedWaveData[0][0] = raw[0][0] + 1;
        processedWaveData[0][1] = raw[0][1] + 2;
    }
};

static void testAudioProcessorDelegatesFftToInjectedProcessor() {
    RecordingFftProcessor fftProcessor;
    AudioProcessor processor(fftProcessor);
    AudioFrame frame;

    fillConstant(frame, 5, 7);

    processor.fft(frame);

    assert(fftProcessor.calls == 1);
    assert(frame.processedWaveData[0][0] == 6);
    assert(frame.processedWaveData[0][1] == 9);
}

static void testDefaultAudioProcessorFftProducesSilentOutputForSilentInput() {
    AudioProcessor processor;
    AudioFrame frame;

    processor.fft(frame);

    for (int i = 0; i < 1024; i++) {
        assert(frame.processedWaveData[i][0] == 0);
        assert(frame.processedWaveData[i][1] == 0);
    }
}

static int absoluteValue(int value) {
    return value < 0 ? -value : value;
}

static void testFixedPointFftProducesCharacterizedOutput() {
    struct ExpectedBin {
        int index;
        int left;
        int right;
    };

    static const ExpectedBin expectedBins[] = {
        { 0, 5, -78 },
        { 1, -100, -90 },
        { 2, -17, 115 },
        { 3, -70, 76 },
        { 4, -39, -92 },
        { 5, -85, 120 },
        { 6, -116, -19 },
        { 7, -37, 127 },
        { 8, 12, 27 },
        { 9, 100, 95 },
        { 10, -6, -86 },
        { 11, 66, 81 },
        { 12, -106, 66 },
        { 13, -119, 23 },
        { 14, -68, -126 },
        { 15, -47, -101 },
        { 31, 126, -87 },
        { 63, -44, -45 },
        { 127, 65, -101 },
        { 255, -19, 110 },
        { 511, 29, 4 },
        { 767, -28, -82 },
        { 1023, 67, 5 },
    };

    FixedPointAudioFftProcessor fixedPoint;
    AudioFrame frame;
    char2 fixedOutput[1024];
    long long checksum = 0;
    long absTotal = 0;
    int maxAbs = 0;

    fillPseudoMusic(frame);
    fixedPoint.transform(frame.raw, fixedOutput);

    for (int i = 0; i < 1024; i++) {
        int left = int(fixedOutput[i][0]);
        int right = int(fixedOutput[i][1]);
        int leftAbs = absoluteValue(left);
        int rightAbs = absoluteValue(right);
        checksum += (long long)(i + 1) * (left + 257 * right);
        absTotal += leftAbs + rightAbs;
        if (leftAbs > maxAbs)
            maxAbs = leftAbs;
        if (rightAbs > maxAbs)
            maxAbs = rightAbs;
    }

    for (unsigned int i = 0; i < sizeof(expectedBins) / sizeof(expectedBins[0]); i++) {
        const ExpectedBin& expected = expectedBins[i];
        assert(int(fixedOutput[expected.index][0]) == expected.left);
        assert(int(fixedOutput[expected.index][1]) == expected.right);
    }

    assert(checksum == 157069866LL);
    assert(absTotal == 130031);
    assert(maxAbs == 128);
}

int main() {
    testAudioFrameOwnsMetrics();
    testAudioProcessorAnalyzesRawFrame();
    testLowPassAmplitudeSuppressesFastAlternation();
    testAudioProcessorDelegatesFftToInjectedProcessor();
    testDefaultAudioProcessorFftProducesSilentOutputForSilentInput();
    testFixedPointFftProducesCharacterizedOutput();
    return 0;
}
