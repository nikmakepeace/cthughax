#include "cthugha.h"
#include "PipelineStageModules.h"
#include "VideoPipeline.h"
#include "VideoPipelineFactory.h"

VideoPipelineFactory::VideoPipelineFactory() { }

VideoPipeline* VideoPipelineFactory::create(const VideoPipelineSequence& sequence) const {
    VideoPipeline* pipeline = new VideoPipeline();
    PaletteStageModule* paletteModule = 0;

    pipeline->setStageSequence(sequence.sequence());

    if (sequence.includes(VideoPipelineSequence::ImageStage))
        pipeline->add(VideoPipelineSequence::ImageStage, new ImageStageModule(), 1);
    if (sequence.includes(VideoPipelineSequence::BorderStage))
        pipeline->add(VideoPipelineSequence::BorderStage, new BorderVideoModule(), 1);
    if (sequence.includes(VideoPipelineSequence::FlameStage))
        pipeline->add(VideoPipelineSequence::FlameStage, new FlameStageModule(), 1);
    if (sequence.includes(VideoPipelineSequence::TranslateStage))
        pipeline->add(VideoPipelineSequence::TranslateStage, new TranslateStageModule(), 1);
    if (sequence.includes(VideoPipelineSequence::WaveStage))
        pipeline->add(VideoPipelineSequence::WaveStage, new WaveStageModule(), 1);
    if (sequence.includes(VideoPipelineSequence::FrameCommitStage))
        pipeline->add(VideoPipelineSequence::FrameCommitStage, new FrameCommitModule(), 1);
    if (sequence.includes(VideoPipelineSequence::PaletteStage)) {
        paletteModule = new PaletteStageModule();
        pipeline->add(VideoPipelineSequence::PaletteStage, paletteModule, 1);
        pipeline->setFramePalette(&paletteModule->framePalette());
    }
    if (sequence.includes(VideoPipelineSequence::FlashlightStage))
        pipeline->add(VideoPipelineSequence::FlashlightStage, new FlashlightVideoModule(), 1);

    CTH_DEBUG("visual pipeline factory: created pipeline=%p stages=%d modules=%d\n",
        pipeline, int(sequence.sequence().size()), pipeline->size());
    return pipeline;
}

void VideoPipelineFactory::refresh(VideoPipeline& pipeline,
    const VideoPipelineSequence& sequence) const {
    CTH_DEBUG("visual pipeline factory: refreshing pipeline=%p stages=%d modules=%d\n",
        &pipeline, int(sequence.sequence().size()), pipeline.size());
    pipeline.refresh();
}
