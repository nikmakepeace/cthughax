#include "cthugha.h"
#include "VideoPipeline.h"

VideoFrameContext::VideoFrameContext()
    : audioFrame(0)
    , rawAudioData(0)
    , processedWaveData(0)
    , audioMetrics(0)
    , acousticContext(0)
    , now(0.0)
    , deltaT(0.0) { }

VideoModule::~VideoModule() { }

VideoFrame::VideoFrame(CthughaBuffer& buffer_, const VideoFrameContext& context_,
    FramePalette* framePalette_)
    : bufferValue(&buffer_)
    , contextValue(&context_)
    , framePaletteValue(framePalette_) { }

CthughaBuffer& VideoFrame::buffer() {
    return *bufferValue;
}

const VideoFrameContext& VideoFrame::context() const {
    return *contextValue;
}

FramePalette* VideoFrame::framePalette() {
    return framePaletteValue;
}

const FramePalette* VideoFrame::framePalette() const {
    return framePaletteValue;
}

static int findStageIndex(const std::vector<unsigned int>& sequence, unsigned int stage) {
    for (unsigned int i = 0; i < sequence.size(); i++) {
        if (sequence[i] == stage)
            return int(i);
    }

    return -1;
}

VideoPipeline::VideoPipeline()
    : framePaletteValue(0) { }

VideoPipeline::~VideoPipeline() {
    clear();
}

void VideoPipeline::clear() {
    for (unsigned int i = 0; i < modules.size(); i++) {
        if (modules[i].owned)
            delete modules[i].module;
    }
    modules.clear();
    sequence.clear();
    framePaletteValue = 0;
}

void VideoPipeline::add(unsigned int stage, VideoModule* module, int takeOwnership) {
    if (module == 0)
        return;
    modules.push_back(Entry(stage, module, takeOwnership));
    CTH_DEBUG("visual pipeline: added stage=%u module=%p owned=%d mode=%d size=%d\n",
        stage, module, takeOwnership, int(VideoStageDisabled), size());
}

void VideoPipeline::setStageSequence(const std::vector<unsigned int>& stages) {
    sequence = stages;
    CTH_DEBUG("visual pipeline: set sequence stages=%d\n", int(sequence.size()));
}

int VideoPipeline::moveStageBefore(unsigned int stage, unsigned int beforeStage) {
    if (stage == beforeStage)
        return 1;

    int stageIndex = findStageIndex(sequence, stage);
    int beforeIndex = findStageIndex(sequence, beforeStage);
    if (stageIndex < 0 || beforeIndex < 0)
        return 0;

    unsigned int movingStage = sequence[stageIndex];
    sequence.erase(sequence.begin() + stageIndex);
    beforeIndex = findStageIndex(sequence, beforeStage);
    sequence.insert(sequence.begin() + beforeIndex, movingStage);

    CTH_DEBUG("visual pipeline: moved stage=%u before stage=%u\n",
        stage, beforeStage);
    return 1;
}

int VideoPipeline::moveStageAfter(unsigned int stage, unsigned int afterStage) {
    if (stage == afterStage)
        return 1;

    int stageIndex = findStageIndex(sequence, stage);
    int afterIndex = findStageIndex(sequence, afterStage);
    if (stageIndex < 0 || afterIndex < 0)
        return 0;

    unsigned int movingStage = sequence[stageIndex];
    sequence.erase(sequence.begin() + stageIndex);
    afterIndex = findStageIndex(sequence, afterStage);
    sequence.insert(sequence.begin() + afterIndex + 1, movingStage);

    CTH_DEBUG("visual pipeline: moved stage=%u after stage=%u\n",
        stage, afterStage);
    return 1;
}

int VideoPipeline::setStageMode(unsigned int stage, VideoStageRunMode mode) {
    int matched = 0;

    for (unsigned int i = 0; i < modules.size(); i++) {
        if (modules[i].stage == stage) {
            matched++;
            if (modules[i].mode != mode)
                modules[i].mode = mode;
        }
    }

    CTH_TRACE("set stage=%u mode=%d entries=%d\n", "visual pipeline",
        stage, int(mode), matched);
    return matched;
}

VideoStageRunMode VideoPipeline::stageMode(unsigned int stage) const {
    for (unsigned int i = 0; i < modules.size(); i++) {
        if (modules[i].stage == stage)
            return modules[i].mode;
    }

    return VideoStageDisabled;
}

VideoModule* VideoPipeline::stageModule(unsigned int stage) {
    for (unsigned int i = 0; i < modules.size(); i++) {
        if (modules[i].stage == stage)
            return modules[i].module;
    }

    return 0;
}

void VideoPipeline::setFramePalette(FramePalette* framePalette) {
    framePaletteValue = framePalette;
}

FramePalette* VideoPipeline::framePalette() const {
    return framePaletteValue;
}

void VideoPipeline::refresh() {
    for (unsigned int i = 0; i < modules.size(); i++)
        modules[i].module->refresh();
}

void VideoPipeline::run(CthughaBuffer& buffer, const VideoFrameContext& context) {
    VideoFrame frame(buffer, context, framePaletteValue);

    for (unsigned int stageIndex = 0; stageIndex < sequence.size(); stageIndex++) {
        unsigned int stage = sequence[stageIndex];
        for (unsigned int moduleIndex = 0; moduleIndex < modules.size(); moduleIndex++) {
            if (modules[moduleIndex].stage != stage)
                continue;

            if (modules[moduleIndex].mode == VideoStageDisabled) {
                CTH_TRACE("skipping disabled stage=%u module=%p\n",
                    "visual pipeline", modules[moduleIndex].stage, modules[moduleIndex].module);
                continue;
            }

            modules[moduleIndex].module->execute(frame);

            if (modules[moduleIndex].mode == VideoStageArmedOnce) {
                CTH_TRACE("disarming one-shot stage=%u module=%p\n",
                    "visual pipeline", modules[moduleIndex].stage, modules[moduleIndex].module);
                modules[moduleIndex].mode = VideoStageDisabled;
            }
        }
    }
}

int VideoPipeline::size() const {
    return int(modules.size());
}
