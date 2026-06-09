/** @file
 * Unit coverage for FrameGeneratorContext audio input ownership.
 */

#include "FrameGeneratorContext.h"
#include "AudioFrame.h"

#include <assert.h>

static void testContextBorrowsAudioBuffersAndCopiesAnalysis() {
    AudioFrame frame;
    frame.samples = 128;
    frame.raw[0][0] = 3;
    frame.raw[0][1] = 4;
    frame.processedWaveData[0][0] = 5;
    frame.processedWaveData[0][1] = 6;
    frame.metrics.amplitude = 7;
    frame.metrics.amplitudeLeft = 8;
    frame.metrics.amplitudeRight = 9;
    frame.metrics.noisy = 1;

    AudioAnalysisSnapshot analysis(frame.metrics, 0.25, 11, 13);
    FrameGeneratorContext context(&frame, frame.raw, frame.processedWaveData,
        analysis, 0, 42.0, 0.125, 0);

    assert(context.audioFrame() == &frame);
    assert(context.rawAudioData() == frame.raw);
    assert(context.processedWaveData() == frame.processedWaveData);
    assert(context.now() == 42.0);
    assert(context.deltaT() == 0.125);
    assert(context.frameBudgetFramesPerSecond() == 60);

    frame.metrics.amplitude = 99;
    assert(context.audioAnalysis().amplitude() == 7);
    assert(context.audioAnalysis().amplitudeLeft() == 8);
    assert(context.audioAnalysis().amplitudeRight() == 9);
    assert(context.audioAnalysis().noisy() == 1);
    assert(context.audioAnalysis().intensity() == 0.25);
    assert(context.audioAnalysis().fire() == 11);
    assert(context.audioAnalysis().cumulativeFireLevel() == 13);
}

int main() {
    testContextBorrowsAudioBuffersAndCopiesAnalysis();
    return 0;
}
