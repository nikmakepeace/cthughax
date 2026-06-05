/** @file
 * Unit coverage for AudioFrame metrics and AudioProcessor analysis.
 */

#include "Audio.h"
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

int audioBytesPerSample() {
    return 2;
}

static void fillConstant(AudioFrame& frame, int left, int right) {
    for (int i = 0; i < 1024; i++) {
        frame.raw[i][0] = char(left);
        frame.raw[i][1] = char(right);
    }
}

static void testAudioFrameOwnsMetrics() {
    AudioFrame frame;

    frame.metrics.amplitude = 12;
    frame.metrics.amplitudeLeft = 10;
    frame.metrics.amplitudeRight = 14;
    frame.metrics.noisy = 1;

    frame.clear();

    assert(frame.metrics.amplitude == 0);
    assert(frame.metrics.amplitudeLeft == 0);
    assert(frame.metrics.amplitudeRight == 0);
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
    assert(quietMetrics.noisy == 0);

    processor.analyze(frame, 4);
    assert(frame.metrics.amplitudeLeft == 3);
    assert(frame.metrics.amplitudeRight == 4);
    assert(frame.metrics.amplitude == 3);
    assert(frame.metrics.noisy == 1);
}

static void testAudioFrameMetricsFacadePublishesToCurrentFrame() {
    AudioFrame frame;
    AudioMetrics metrics;

    metrics.amplitude = 9;
    metrics.amplitudeLeft = 7;
    metrics.amplitudeRight = 11;
    metrics.noisy = 1;

    audioFrameSetTestOverride(&frame);
    audioFramePublishMetrics(metrics);

    assert(audioFrameMetrics().amplitude == 9);
    assert(frame.metrics.amplitude == 9);
    assert(frame.metrics.amplitudeLeft == 7);
    assert(frame.metrics.amplitudeRight == 11);
    assert(frame.metrics.noisy == 1);

    audioFrameSetTestOverride(0);
}

static void testAudioFrameMetricsFacadeKeepsLegacyFallback() {
    AudioMetrics metrics;

    metrics.amplitude = 6;
    metrics.amplitudeLeft = 5;
    metrics.amplitudeRight = 7;
    metrics.noisy = 0;

    audioFrameSetTestOverride(0);
    audioFramePublishMetrics(metrics);

    assert(audioFrameMetrics().amplitude == 6);
    assert(audioFrameMetrics().amplitudeLeft == 5);
    assert(audioFrameMetrics().amplitudeRight == 7);
    assert(audioFrameMetrics().noisy == 0);
}

int main() {
    testAudioFrameOwnsMetrics();
    testAudioProcessorAnalyzesRawFrame();
    testAudioFrameMetricsFacadePublishesToCurrentFrame();
    testAudioFrameMetricsFacadeKeepsLegacyFallback();
    return 0;
}
