/** @file
 * Unit coverage for explicit AudioVisualBridge frame input.
 */

#include "AudioVisualBridge.h"
#include "AudioAnalyzer.h"
#include "AudioFrame.h"
#include "AudioProcessor.h"

#include <assert.h>
#include <stdarg.h>

int cth_log_enabled(int) { return 0; }
int cth_log(int, const char*, ...) { return 0; }
int cth_log_context(int, const char*, const char*, ...) { return 0; }
int cth_log_error(const char*, ...) { return 0; }
int cth_log_errno(int, const char*, ...) { return 0; }
double getTime() { return 0.0; }
int audioBytesPerSample() { return 2; }

static void fillConstant(AudioFrame& frame, int left, int right) {
    frame.samples = 1024;
    for (int i = 0; i < 1024; i++) {
        frame.raw[i][0] = char(left);
        frame.raw[i][1] = char(right);
    }
}

static void testBridgeProcessesSuppliedFrame() {
    AudioFrame frame;
    AudioVisualBridge bridge;

    sound_minnoise.setValue(4);
    audioProcessing.change("none");
    fillConstant(frame, 3, 4);

    bridge.runFrame(frame);

    assert(frame.metrics.amplitudeLeft == 3);
    assert(frame.metrics.amplitudeRight == 4);
    assert(frame.metrics.amplitude == 3);
    assert(frame.metrics.noisy == 1);
    assert(frame.processedWaveData[0][0] == 3);
    assert(frame.processedWaveData[0][1] == 4);
}

int main() {
    testBridgeProcessesSuppliedFrame();
    return 0;
}
