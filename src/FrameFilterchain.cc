#include "FrameFilterchain.h"

#include "ProcessServices.h"

FrameFilter::~FrameFilter() { }

const char* FrameFilter::name() const {
    return "frame-filter";
}

FrameFilterFrame::FrameFilterFrame(FrameRenderTarget& buffer_, const FrameGeneratorContext& context_,
    FramePalette* framePalette_, IndexedFrame* indexedFrame_, LogSink& log_)
    : bufferValue(&buffer_)
    , contextValue(&context_)
    , framePaletteValue(framePalette_)
    , indexedFrameValue(indexedFrame_)
    , logValue(&log_) { }

FrameRenderTarget& FrameFilterFrame::buffer() {
    return *bufferValue;
}

const FrameGeneratorContext& FrameFilterFrame::context() const {
    return *contextValue;
}

FramePalette* FrameFilterFrame::framePalette() {
    return framePaletteValue;
}

const FramePalette* FrameFilterFrame::framePalette() const {
    return framePaletteValue;
}

void FrameFilterFrame::publishIndexedFrame(const IndexedFrame& indexedFrame) {
    if (indexedFrameValue != 0)
        *indexedFrameValue = indexedFrame;
}

const IndexedFrame& FrameFilterFrame::indexedFrame() const {
    return *indexedFrameValue;
}

LogSink& FrameFilterFrame::log() const {
    return *logValue;
}

static int findStageIndex(const std::vector<unsigned int>& sequence, unsigned int stage) {
    for (unsigned int i = 0; i < sequence.size(); i++) {
        if (sequence[i] == stage)
            return int(i);
    }

    return -1;
}

FrameFilterchain::FrameFilterchain(LogSink& log)
    : framePaletteValue(0)
    , logValue(&log) { }

FrameFilterchain::~FrameFilterchain() {
    clear();
}

void FrameFilterchain::clear() {
    for (unsigned int i = 0; i < filters.size(); i++) {
        if (filters[i].owned)
            delete filters[i].filter;
    }
    filters.clear();
    sequence.clear();
    framePaletteValue = 0;
    indexedFrameValue = IndexedFrame();
}

void FrameFilterchain::add(unsigned int stage, FrameFilter* filter, int takeOwnership) {
    if (filter == 0)
        return;
    filters.push_back(Entry(stage, filter, takeOwnership));
    logValue->debug("frame filterchain: added stage=%u filter=%s ptr=%p owned=%d mode=%d size=%d\n",
        stage, filter->name(), filter, takeOwnership, int(FrameFilterDisabled), size());
}

void FrameFilterchain::setStageSequence(const std::vector<unsigned int>& stages) {
    sequence = stages;
    logValue->debug("frame filterchain: set sequence stages=%d\n", int(sequence.size()));
}

int FrameFilterchain::moveStageBefore(unsigned int stage, unsigned int beforeStage) {
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

    logValue->debug("frame filterchain: moved stage=%u before stage=%u\n",
        stage, beforeStage);
    return 1;
}

int FrameFilterchain::moveStageAfter(unsigned int stage, unsigned int afterStage) {
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

    logValue->debug("frame filterchain: moved stage=%u after stage=%u\n",
        stage, afterStage);
    return 1;
}

int FrameFilterchain::setStageMode(unsigned int stage, FrameFilterRunMode mode) {
    int matched = 0;

    for (unsigned int i = 0; i < filters.size(); i++) {
        if (filters[i].stage == stage) {
            matched++;
            if (filters[i].mode != mode)
                filters[i].mode = mode;
        }
    }

    logValue->trace("frame filterchain", "set stage=%u mode=%d entries=%d\n",
        stage, int(mode), matched);
    return matched;
}

FrameFilterRunMode FrameFilterchain::stageMode(unsigned int stage) const {
    for (unsigned int i = 0; i < filters.size(); i++) {
        if (filters[i].stage == stage)
            return filters[i].mode;
    }

    return FrameFilterDisabled;
}

FrameFilter* FrameFilterchain::stageFilter(unsigned int stage) {
    for (unsigned int i = 0; i < filters.size(); i++) {
        if (filters[i].stage == stage)
            return filters[i].filter;
    }

    return 0;
}

void FrameFilterchain::setFramePalette(FramePalette* framePalette) {
    framePaletteValue = framePalette;
}

FramePalette* FrameFilterchain::framePalette() const {
    return framePaletteValue;
}

const IndexedFrame& FrameFilterchain::indexedFrame() const {
    return indexedFrameValue;
}

void FrameFilterchain::refresh() {
    for (unsigned int i = 0; i < filters.size(); i++)
        filters[i].filter->refresh();
}

void FrameFilterchain::run(FrameRenderTarget& buffer, const FrameGeneratorContext& context) {
    indexedFrameValue = IndexedFrame();
    FrameFilterFrame frame(buffer, context, framePaletteValue, &indexedFrameValue,
        *logValue);

    for (unsigned int stageIndex = 0; stageIndex < sequence.size(); stageIndex++) {
        unsigned int stage = sequence[stageIndex];
        for (unsigned int filterIndex = 0; filterIndex < filters.size(); filterIndex++) {
            if (filters[filterIndex].stage != stage)
                continue;

            if (filters[filterIndex].mode == FrameFilterDisabled) {
                logValue->trace("frame filterchain",
                    "skipping disabled stage=%u filter=%s ptr=%p\n",
                    filters[filterIndex].stage, filters[filterIndex].filter->name(),
                    filters[filterIndex].filter);
                continue;
            }

            filters[filterIndex].filter->execute(frame);

            if (filters[filterIndex].mode == FrameFilterArmedOnce) {
                logValue->trace("frame filterchain",
                    "disarming one-shot stage=%u filter=%s ptr=%p\n",
                    filters[filterIndex].stage, filters[filterIndex].filter->name(),
                    filters[filterIndex].filter);
                filters[filterIndex].mode = FrameFilterDisabled;
            }
        }
    }
}

int FrameFilterchain::size() const {
    return int(filters.size());
}
