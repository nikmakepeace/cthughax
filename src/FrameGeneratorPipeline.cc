// Frame Generator filterchain ownership.

#include "FrameGeneratorPipeline.h"

#include "FramePalette.h"
#include "IndexedFrame.h"
#include "FrameFilterchain.h"
#include "FrameFilterchainSequence.h"
#include "FrameFilters.h"

FrameGeneratorPipeline::FrameGeneratorPipeline(LogSink& log)
    : factoryValue(log)
    , filterchainValue()
    , sequenceValue() { }

FrameGeneratorPipeline::~FrameGeneratorPipeline() { }

void FrameGeneratorPipeline::initialize(
    const FrameFilterchainSequence& sequence) {
    if (filterchainValue.get() != 0)
        return;

    sequenceValue = sequence;
    filterchainValue.reset(factoryValue.create(sequenceValue));
}

int FrameGeneratorPipeline::initialized() const {
    return filterchainValue.get() != 0;
}

void FrameGeneratorPipeline::reset() {
    filterchainValue.reset();
    sequenceValue = FrameFilterchainSequence();
}

FrameFilterchain& FrameGeneratorPipeline::filterchain() {
    return *filterchainValue;
}

FramePalette* FrameGeneratorPipeline::framePalette() const {
    if (filterchainValue.get() == 0)
        return 0;

    return framePaletteFromFilterchain(*filterchainValue);
}

const IndexedFrame& FrameGeneratorPipeline::render(FrameRenderTarget& target,
    const FrameGeneratorContext& context) {
    filterchainValue->run(target, context);
    return filterchainValue->indexedFrame();
}
