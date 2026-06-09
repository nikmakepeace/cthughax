// Frame Generator module root.

#include "FrameGeneratorRuntime.h"

#include "FrameFilterchain.h"
#include "IndexedFrame.h"
#include "ProcessServices.h"
#include "SceneGeometry.h"

FrameGeneratorRuntimeConfig::FrameGeneratorRuntimeConfig()
    : frameSize(160, 100)
    , paletteSmoothingChance(0.0)
    , paletteSmoothSeconds(1)
    , quietMessageDurationMs(0)
    , silenceMessages() { }

FrameGeneratorRuntime::FrameGeneratorRuntime(RandomSource& randomSource,
    CountdownTimerFactory& timerFactory, LogSink& log)
    : randomSourceValue(randomSource)
    , logValue(log)
    , geometryValue()
    , frameStoreValue()
    , transitionControllerValue()
    , sceneBindingValue(geometryValue, transitionControllerValue,
          randomSourceValue, timerFactory, logValue)
    , pipelineValue(logValue) { }

FrameGeneratorRuntime::~FrameGeneratorRuntime() {
    unbindScene();
}

void FrameGeneratorRuntime::configure(
    const FrameGeneratorRuntimeConfig& config) {
    geometryValue = FrameGeometry(config.frameSize);
    frameStoreValue.resize(geometryValue);
    transitionControllerValue.configureTransitions(
        config.paletteSmoothingChance, config.paletteSmoothSeconds);
    transitionControllerValue.configureQuietMessages(
        config.quietMessageDurationMs);
    sceneBindingValue.silenceMessages().configure(config.silenceMessages);
}

const FrameGeometry& FrameGeneratorRuntime::geometry() const {
    return geometryValue;
}

SceneGeometry& FrameGeneratorRuntime::sceneGeometry() {
    return geometryValue;
}

FrameStore& FrameGeneratorRuntime::frameStore() {
    return frameStoreValue;
}

SilenceMessage& FrameGeneratorRuntime::silenceMessages() {
    return sceneBindingValue.silenceMessages();
}

void FrameGeneratorRuntime::bindScene(Scene& scene) {
    sceneBindingValue.bindScene(scene);
}

void FrameGeneratorRuntime::unbindScene() {
    sceneBindingValue.unbindScene();
}

int FrameGeneratorRuntime::observeQuiet(int quietLength,
    int quietMessageThresholdMs, int frameBudgetFramesPerSecond) {
    return sceneBindingValue.observeQuiet(quietLength, quietMessageThresholdMs,
        frameBudgetFramesPerSecond);
}

void FrameGeneratorRuntime::initializePipeline() {
    if (pipelineValue.initialized())
        return;

    pipelineValue.initialize(sceneBindingValue.defaultFilterchainSequence());
}

void FrameGeneratorRuntime::resetPipeline() {
    pipelineValue.reset();
}

FramePalette* FrameGeneratorRuntime::framePalette() {
    initializePipeline();
    return pipelineValue.framePalette();
}

const IndexedFrame& FrameGeneratorRuntime::render(
    const FrameGeneratorContext& context) {
    initializePipeline();
    sceneBindingValue.configureFilterchain(pipelineValue.filterchain(), context);

    logValue.trace("frame generator", "running filterchain=%p filters=%d\n",
        &pipelineValue.filterchain(), pipelineValue.filterchain().size());
    return pipelineValue.render(frameStoreValue.renderTarget(), context);
}
