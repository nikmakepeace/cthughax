// Per-frame inputs for the Frame Generator module.

#include "FrameGeneratorContext.h"

FrameGeneratorContext::FrameGeneratorContext()
    : audioFrameValue(0)
    , rawAudioDataValue(0)
    , processedWaveDataValue(0)
    , audioAnalysisValue()
    , sceneSnapshotValue(0)
    , nowValue(0.0)
    , deltaTValue(0.0)
    , frameBudgetFramesPerSecondValue(60) { }

FrameGeneratorContext::FrameGeneratorContext(const AudioFrame* audioFrame,
    const char2* rawAudioData, const char2* processedWaveData,
    const AudioAnalysisSnapshot& audioAnalysis,
    const SceneSnapshot* sceneSnapshot, double now, double deltaT,
    int frameBudgetFramesPerSecond)
    : audioFrameValue(audioFrame)
    , rawAudioDataValue(rawAudioData)
    , processedWaveDataValue(processedWaveData)
    , audioAnalysisValue(audioAnalysis)
    , sceneSnapshotValue(sceneSnapshot)
    , nowValue(now)
    , deltaTValue(deltaT)
    , frameBudgetFramesPerSecondValue(
          frameBudgetFramesPerSecond > 0 ? frameBudgetFramesPerSecond : 60) { }

const AudioFrame* FrameGeneratorContext::audioFrame() const {
    return audioFrameValue;
}

const char2* FrameGeneratorContext::rawAudioData() const {
    return rawAudioDataValue;
}

const char2* FrameGeneratorContext::processedWaveData() const {
    return processedWaveDataValue;
}

const AudioAnalysisSnapshot& FrameGeneratorContext::audioAnalysis() const {
    return audioAnalysisValue;
}

const SceneSnapshot* FrameGeneratorContext::sceneSnapshot() const {
    return sceneSnapshotValue;
}

double FrameGeneratorContext::now() const {
    return nowValue;
}

double FrameGeneratorContext::deltaT() const {
    return deltaTValue;
}

int FrameGeneratorContext::frameBudgetFramesPerSecond() const {
    return frameBudgetFramesPerSecondValue;
}
