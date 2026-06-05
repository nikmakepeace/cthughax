#include "ScreenRenderContext.h"
#include "VideoFilterchain.h"

ScreenRenderContext::ScreenRenderContext(const IndexedFrame& source,
    IndexedDisplayFrame& destination, double frameTimeSeconds, double deltaTimeSeconds,
    double framesPerSecond)
    : sourceValue(&source)
    , destinationValue(&destination)
    , destinationPixelsValue(destination.pixels())
    , destinationWidthValue(destination.width())
    , destinationHeightValue(destination.height())
    , destinationPitchValue(destination.pitch())
    , audioFrameValue(0)
    , rawAudioDataValue(0)
    , processedWaveDataValue(0)
    , audioMetricsValue(0)
    , acousticContextValue(0)
    , frameTimeSecondsValue(frameTimeSeconds)
    , deltaTimeSecondsValue(deltaTimeSeconds)
    , framesPerSecondValue(framesPerSecond) {
}

ScreenRenderContext::ScreenRenderContext(const IndexedFrame& source,
    IndexedDisplayFrame& destination, double frameTimeSeconds,
    double deltaTimeSeconds, double framesPerSecond,
    const VideoFrameContext& frameContext)
    : sourceValue(&source)
    , destinationValue(&destination)
    , destinationPixelsValue(destination.pixels())
    , destinationWidthValue(destination.width())
    , destinationHeightValue(destination.height())
    , destinationPitchValue(destination.pitch())
    , audioFrameValue(frameContext.audioFrame)
    , rawAudioDataValue(frameContext.rawAudioData)
    , processedWaveDataValue(frameContext.processedWaveData)
    , audioMetricsValue(frameContext.audioMetrics)
    , acousticContextValue(frameContext.acousticContext)
    , frameTimeSecondsValue(frameTimeSeconds)
    , deltaTimeSecondsValue(deltaTimeSeconds)
    , framesPerSecondValue(framesPerSecond) {
}

ScreenRenderContext::ScreenRenderContext(const IndexedFrame& source,
    IndexedDisplayFrame& destination, unsigned char* destinationPixels,
    int destinationWidth, int destinationHeight, int destinationPitch,
    double frameTimeSeconds, double deltaTimeSeconds, double framesPerSecond)
    : sourceValue(&source)
    , destinationValue(&destination)
    , destinationPixelsValue(destinationPixels)
    , destinationWidthValue(destinationWidth)
    , destinationHeightValue(destinationHeight)
    , destinationPitchValue(destinationPitch)
    , audioFrameValue(0)
    , rawAudioDataValue(0)
    , processedWaveDataValue(0)
    , audioMetricsValue(0)
    , acousticContextValue(0)
    , frameTimeSecondsValue(frameTimeSeconds)
    , deltaTimeSecondsValue(deltaTimeSeconds)
    , framesPerSecondValue(framesPerSecond) {
}

const IndexedFrame& ScreenRenderContext::source() const {
    return *sourceValue;
}

IndexedDisplayFrame& ScreenRenderContext::destination() const {
    return *destinationValue;
}

const unsigned char* ScreenRenderContext::sourcePixels() const {
    return sourceValue->pixels;
}

const unsigned char* ScreenRenderContext::sourceLine(int y) const {
    return sourceValue->pixels + y * sourceValue->pitch;
}

int ScreenRenderContext::sourceWidth() const {
    return sourceValue->width;
}

int ScreenRenderContext::sourceHeight() const {
    return sourceValue->height;
}

int ScreenRenderContext::sourcePitch() const {
    return sourceValue->pitch;
}

unsigned char* ScreenRenderContext::destinationPixels() {
    return destinationPixelsValue;
}

unsigned char* ScreenRenderContext::destinationLine(int y) {
    return destinationPixelsValue + y * destinationPitchValue;
}

int ScreenRenderContext::destinationWidth() const {
    return destinationWidthValue;
}

int ScreenRenderContext::destinationHeight() const {
    return destinationHeightValue;
}

int ScreenRenderContext::destinationPitch() const {
    return destinationPitchValue;
}

const AudioFrame* ScreenRenderContext::audioFrame() const {
    return audioFrameValue;
}

const char2* ScreenRenderContext::rawAudioData() const {
    return rawAudioDataValue;
}

const char2* ScreenRenderContext::processedWaveData() const {
    return processedWaveDataValue;
}

const AudioMetrics* ScreenRenderContext::audioMetrics() const {
    return audioMetricsValue;
}

const AcousticContext* ScreenRenderContext::acousticContext() const {
    return acousticContextValue;
}

double ScreenRenderContext::frameTimeSeconds() const {
    return frameTimeSecondsValue;
}

double ScreenRenderContext::deltaTimeSeconds() const {
    return deltaTimeSecondsValue;
}

double ScreenRenderContext::framesPerSecond() const {
    return framesPerSecondValue;
}
