// Frame Generator module root.

#include "FrameGeneratorRuntime.h"

#include "FrameFilterchain.h"
#include "FrameFilterchainUserStages.h"
#include "IndexedFrame.h"
#include "ProcessServices.h"
#include "SceneGeometry.h"

namespace {

static void applyFilterchainStageGate(FrameFilterchain& filterchain,
    FrameFilterchainSequence::Stage stage, int enabled) {
    filterchain.setStageEnabled(stage, enabled);
}

} // namespace

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
    , pipelineValue(logValue)
    , filterchainSequenceValue(
          sceneBindingValue.defaultFilterchainSequence())
    , filterchainStageNamesValue()
    , filterchainStageEnabledValue()
    , filterchainStagePolicyActive(0) { }

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

void FrameGeneratorRuntime::setPaletteSmoothingChance(double chance) {
    transitionControllerValue.setPaletteSmoothingChance(chance);
}

int FrameGeneratorRuntime::filterchainStageEnabled(
    FrameFilterchainSequence::Stage stage) const {
    if (!filterchainStagePolicyActive)
        return 1;

    for (std::size_t i = 0; i < filterchainStageNamesValue.size(); i++) {
        FrameFilterchainSequence::Stage candidate
            = FrameFilterchainSequence::ImageStage;
        if (!frameFilterchainUserStageFromName(filterchainStageNamesValue[i],
                &candidate)
            || candidate != stage)
            continue;

        if (i >= filterchainStageEnabledValue.size())
            return 1;
        return filterchainStageEnabledValue[i] != 0;
    }

    return 1;
}

void FrameGeneratorRuntime::applyFilterchainStageGates() {
    if (!filterchainStagePolicyActive || !pipelineValue.initialized())
        return;

    FrameFilterchain& filterchain = pipelineValue.filterchain();
    applyFilterchainStageGate(filterchain, FrameFilterchainSequence::ImageStage,
        filterchainStageEnabled(FrameFilterchainSequence::ImageStage));
    applyFilterchainStageGate(filterchain, FrameFilterchainSequence::BorderStage,
        filterchainStageEnabled(FrameFilterchainSequence::BorderStage));
    applyFilterchainStageGate(filterchain, FrameFilterchainSequence::FlameStage,
        filterchainStageEnabled(FrameFilterchainSequence::FlameStage));
    applyFilterchainStageGate(filterchain,
        FrameFilterchainSequence::TranslateStage,
        filterchainStageEnabled(FrameFilterchainSequence::TranslateStage));
    applyFilterchainStageGate(filterchain, FrameFilterchainSequence::WaveStage,
        filterchainStageEnabled(FrameFilterchainSequence::WaveStage));
    applyFilterchainStageGate(filterchain, FrameFilterchainSequence::TextStage,
        filterchainStageEnabled(FrameFilterchainSequence::TextStage));
}

void FrameGeneratorRuntime::setFilterchainSequence(
    const std::vector<std::string>& stages, const std::vector<int>& enabled) {
    filterchainStageNamesValue = stages;
    filterchainStageEnabledValue = enabled;
    filterchainStagePolicyActive = 1;
    filterchainSequenceValue = frameFilterchainSequenceFromUserStageNames(stages);
    pipelineValue.setSequence(filterchainSequenceValue);
    applyFilterchainStageGates();
}

void FrameGeneratorRuntime::setFilterchainStageEnabled(
    const std::vector<std::string>& stages, const std::vector<int>& enabled) {
    filterchainStageNamesValue = stages;
    filterchainStageEnabledValue = enabled;
    filterchainStagePolicyActive = 1;
    applyFilterchainStageGates();
}

double FrameGeneratorRuntime::paletteSmoothingChance() const {
    return transitionControllerValue.paletteSmoothingChance();
}

int FrameGeneratorRuntime::paletteSmoothSeconds() const {
    return transitionControllerValue.paletteSmoothSeconds();
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

    pipelineValue.initialize(filterchainSequenceValue);
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
    applyFilterchainStageGates();

    logValue.trace("frame generator", "running filterchain=%p filters=%d\n",
        &pipelineValue.filterchain(), pipelineValue.filterchain().size());
    return pipelineValue.render(frameStoreValue.renderTarget(), context);
}
