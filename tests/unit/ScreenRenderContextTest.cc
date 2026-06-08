#include "AudioFrame.h"
#include "IndexedFrameTestFixtures.h"
#include "ScreenRenderContext.h"
#include "FrameRenderContext.h"

#include <assert.h>

static void testRowAccessorsHonorSourceAndDestinationPitch() {
    IndexedFrameFixture source(5, 3, 8);
    IndexedDisplayFrame destination;
    preparePaddedDestination(destination, 4, 2, 7, 0x5a);

    ScreenRenderContext context(source.frame(), destination, 12.5, 0.125, 59.5);

    assert(&context.source() == &source.frame());
    assert(&context.destination() == &destination);

    assert(context.sourcePixels() == source.frame().pixels);
    assert(context.sourceLine(2) == source.line(2));
    assert(context.sourceWidth() == 5);
    assert(context.sourceHeight() == 3);
    assert(context.sourcePitch() == 8);

    assert(context.destinationPixels() == destination.pixels());
    assert(context.destinationLine(1) == destination.line(1));
    assert(context.destinationWidth() == 4);
    assert(context.destinationHeight() == 2);
    assert(context.destinationPitch() == 7);
}

static void testFrameTimingValuesAreCopied() {
    IndexedFrameFixture source(5, 3, 8);
    IndexedDisplayFrame destination;
    preparePaddedDestination(destination, 4, 2, 7, 0x5a);

    ScreenRenderContext context(source.frame(), destination, 12.5, 0.125, 59.5);

    assert(context.frameTimeSeconds() == 12.5);
    assert(context.deltaTimeSeconds() == 0.125);
    assert(context.framesPerSecond() == 59.5);
}

static void testAudioInputsDefaultToAbsent() {
    IndexedFrameFixture source(5, 3, 8);
    IndexedDisplayFrame destination;
    preparePaddedDestination(destination, 4, 2, 7, 0x5a);

    ScreenRenderContext context(source.frame(), destination, 12.5, 0.125, 59.5);

    assert(context.audioFrame() == 0);
    assert(context.rawAudioData() == 0);
    assert(context.processedWaveData() == 0);
    assert(context.audioMetrics() == 0);
    assert(context.acousticContext() == 0);
}

static void testAudioInputsAreBorrowedFromFrameRenderContext() {
    IndexedFrameFixture source(5, 3, 8);
    IndexedDisplayFrame destination;
    preparePaddedDestination(destination, 4, 2, 7, 0x5a);
    AudioFrame frame;
    frame.raw[0][0] = 12;
    frame.processedWaveData[0][0] = 34;
    frame.metrics.amplitude = 56;

    FrameRenderContext frameContext;
    frameContext.audioFrame = &frame;
    frameContext.rawAudioData = frame.raw;
    frameContext.processedWaveData = frame.processedWaveData;
    frameContext.audioMetrics = &frame.metrics;
    frameContext.acousticContext = (const AcousticContext*)0x1234;

    ScreenRenderContext context(source.frame(), destination, 12.5, 0.125, 59.5,
        frameContext);

    assert(context.audioFrame() == &frame);
    assert(context.rawAudioData() == frame.raw);
    assert(context.processedWaveData() == frame.processedWaveData);
    assert(context.audioMetrics() == &frame.metrics);
    assert(context.rawAudioData()[0][0] == 12);
    assert(context.processedWaveData()[0][0] == 34);
    assert(context.audioMetrics()->amplitude == 56);
    assert(context.acousticContext() == (const AcousticContext*)0x1234);
}

int main() {
    testRowAccessorsHonorSourceAndDestinationPitch();
    testFrameTimingValuesAreCopied();
    testAudioInputsDefaultToAbsent();
    testAudioInputsAreBorrowedFromFrameRenderContext();
    return 0;
}
