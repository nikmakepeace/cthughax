// Frame Generator filterchain ownership.

#include "FrameGeneratorPipeline.h"

#include "FramePalette.h"
#include "IndexedFrame.h"
#include "VideoFilterchain.h"
#include "VideoFilterchainSequence.h"
#include "VideoFilters.h"

FrameGeneratorPipeline::FrameGeneratorPipeline()
    : factoryValue()
    , filterchainValue()
    , sequenceValue() { }

FrameGeneratorPipeline::~FrameGeneratorPipeline() { }

void FrameGeneratorPipeline::initialize(
    const VideoFilterchainSequence& sequence) {
    if (filterchainValue.get() != 0)
        return;

    sequenceValue = sequence;
    filterchainValue.reset(factoryValue.create(sequenceValue));
}

void FrameGeneratorPipeline::reset() {
    filterchainValue.reset();
    sequenceValue = VideoFilterchainSequence();
}

VideoFilterchain& FrameGeneratorPipeline::filterchain() {
    return *filterchainValue;
}

FramePalette* FrameGeneratorPipeline::framePalette() const {
    if (filterchainValue.get() == 0)
        return 0;

    return framePaletteFromFilterchain(*filterchainValue);
}

const IndexedFrame& FrameGeneratorPipeline::render(CthughaBuffer& buffer,
    const VideoFrameContext& context) {
    filterchainValue->run(buffer, context);
    return filterchainValue->indexedFrame();
}
