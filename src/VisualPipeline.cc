#include "cthugha.h"
#include "CthughaFrameBuffer.h"
#include "VisualPipeline.h"

VisualFrameContext::VisualFrameContext()
    : audioFrame(0)
    , audioAnalysis(0)
    , acousticContext(0)
    , now(0.0)
    , deltaT(0.0) { }

VisualModule::~VisualModule() { }

static int findStageIndex(const std::vector<unsigned int>& sequence, unsigned int stage) {
    for (unsigned int i = 0; i < sequence.size(); i++) {
        if (sequence[i] == stage)
            return int(i);
    }

    return -1;
}

VisualPipeline::VisualPipeline() { }

VisualPipeline::~VisualPipeline() {
    clear();
}

void VisualPipeline::clear() {
    for (unsigned int i = 0; i < modules.size(); i++) {
        if (modules[i].owned)
            delete modules[i].module;
    }
    modules.clear();
    sequence.clear();
}

void VisualPipeline::add(unsigned int stage, VisualModule* module, int takeOwnership) {
    if (module == 0)
        return;
    modules.push_back(Entry(stage, module, takeOwnership));
    CTH_TRACE("added stage=%u module=%p owned=%d mode=%d size=%d\n",
        "visual pipeline", stage, module, takeOwnership, int(VisualStageDisabled), size());
}

void VisualPipeline::setStageSequence(const std::vector<unsigned int>& stages) {
    sequence = stages;
    CTH_TRACE("set sequence stages=%d\n", "visual pipeline", int(sequence.size()));
}

int VisualPipeline::moveStageBefore(unsigned int stage, unsigned int beforeStage) {
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

    CTH_TRACE("moved stage=%u before stage=%u\n", "visual pipeline",
        stage, beforeStage);
    return 1;
}

int VisualPipeline::moveStageAfter(unsigned int stage, unsigned int afterStage) {
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

    CTH_TRACE("moved stage=%u after stage=%u\n", "visual pipeline",
        stage, afterStage);
    return 1;
}

int VisualPipeline::setStageMode(unsigned int stage, VisualStageRunMode mode) {
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

VisualStageRunMode VisualPipeline::stageMode(unsigned int stage) const {
    for (unsigned int i = 0; i < modules.size(); i++) {
        if (modules[i].stage == stage)
            return modules[i].mode;
    }

    return VisualStageDisabled;
}

void VisualPipeline::refresh() {
    for (unsigned int i = 0; i < modules.size(); i++)
        modules[i].module->refresh();
}

void VisualPipeline::run(CthughaFrameBuffer& frameBuffer, const VisualFrameContext& context) {
    for (unsigned int stageIndex = 0; stageIndex < sequence.size(); stageIndex++) {
        unsigned int stage = sequence[stageIndex];
        for (unsigned int moduleIndex = 0; moduleIndex < modules.size(); moduleIndex++) {
            if (modules[moduleIndex].stage != stage)
                continue;

            if (modules[moduleIndex].mode == VisualStageDisabled) {
                CTH_TRACE("skipping disabled stage=%u module=%p\n",
                    "visual pipeline", modules[moduleIndex].stage, modules[moduleIndex].module);
                continue;
            }

            modules[moduleIndex].module->execute(frameBuffer, context);

            if (modules[moduleIndex].mode == VisualStageArmedOnce) {
                CTH_TRACE("disarming one-shot stage=%u module=%p\n",
                    "visual pipeline", modules[moduleIndex].stage, modules[moduleIndex].module);
                modules[moduleIndex].mode = VisualStageDisabled;
            }
        }
    }
}

int VisualPipeline::size() const {
    return int(modules.size());
}
