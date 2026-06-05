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

int main() {
    testAudioFrameOwnsMetrics();
    testAudioProcessorAnalyzesRawFrame();
    return 0;
}
