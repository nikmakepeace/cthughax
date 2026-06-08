#include "cthugha.h"
#include "FrameFilters.h"
#include "FrameFilterchain.h"
#include "FrameFilterchainFactory.h"

FrameFilterchainFactory::FrameFilterchainFactory() { }

FrameFilterchain* FrameFilterchainFactory::create(const FrameFilterchainSequence& sequence) const {
    FrameFilterchain* filterchain = new FrameFilterchain();
    PaletteFilter* paletteFilter = 0;

    filterchain->setStageSequence(sequence.sequence());

    if (sequence.includes(FrameFilterchainSequence::ImageStage))
        filterchain->add(FrameFilterchainSequence::ImageStage, new ImageFilter(), 1);
    if (sequence.includes(FrameFilterchainSequence::BorderStage))
        filterchain->add(FrameFilterchainSequence::BorderStage, new BorderFilter(), 1);
    if (sequence.includes(FrameFilterchainSequence::FlameStage))
        filterchain->add(FrameFilterchainSequence::FlameStage, new FlameFilter(), 1);
    if (sequence.includes(FrameFilterchainSequence::TranslateStage))
        filterchain->add(FrameFilterchainSequence::TranslateStage, new TranslateFilter(), 1);
    if (sequence.includes(FrameFilterchainSequence::WaveStage))
        filterchain->add(FrameFilterchainSequence::WaveStage, new WaveFilter(), 1);
    if (sequence.includes(FrameFilterchainSequence::TextStage))
        filterchain->add(FrameFilterchainSequence::TextStage, new TextInjectionFilter(), 1);
    if (sequence.includes(FrameFilterchainSequence::FrameCommitStage))
        filterchain->add(FrameFilterchainSequence::FrameCommitStage, new FrameCommitFilter(), 1);
    if (sequence.includes(FrameFilterchainSequence::PaletteStage)) {
        paletteFilter = new PaletteFilter();
        filterchain->add(FrameFilterchainSequence::PaletteStage, paletteFilter, 1);
        filterchain->setFramePalette(&paletteFilter->framePalette());
    }
    if (sequence.includes(FrameFilterchainSequence::FlashlightStage))
        filterchain->add(FrameFilterchainSequence::FlashlightStage, new FlashlightFilter(), 1);
    if (sequence.includes(FrameFilterchainSequence::IndexedFrameStage))
        filterchain->add(FrameFilterchainSequence::IndexedFrameStage, new IndexedFrameFilter(), 1);

    CTH_DEBUG("frame filterchain factory: created filterchain=%p stages=%d filters=%d\n",
        filterchain, int(sequence.sequence().size()), filterchain->size());
    return filterchain;
}

void FrameFilterchainFactory::refresh(FrameFilterchain& filterchain,
    const FrameFilterchainSequence& sequence) const {
    CTH_DEBUG("frame filterchain factory: refreshing filterchain=%p stages=%d filters=%d\n",
        &filterchain, int(sequence.sequence().size()), filterchain.size());
    filterchain.refresh();
}
