#include "cthugha.h"
#include "PipelineStageModules.h"
#include "VisualPipeline.h"
#include "VisualPipelineFactory.h"

VisualPipelineFactory::VisualPipelineFactory() { }

VisualPipeline* VisualPipelineFactory::create(const VisualPipelineSequence& sequence) const {
    VisualPipeline* pipeline = new VisualPipeline();
    PaletteStageModule* paletteModule = 0;

    pipeline->setStageSequence(sequence.sequence());

    if (sequence.includes(VisualPipelineSequence::ImageStage))
        pipeline->add(VisualPipelineSequence::ImageStage, new ImageStageModule(), 1);
    if (sequence.includes(VisualPipelineSequence::BorderStage))
        pipeline->add(VisualPipelineSequence::BorderStage, new BorderVisualModule(), 1);
    if (sequence.includes(VisualPipelineSequence::FlameStage))
        pipeline->add(VisualPipelineSequence::FlameStage, new FlameStageModule(), 1);
    if (sequence.includes(VisualPipelineSequence::TranslateStage))
        pipeline->add(VisualPipelineSequence::TranslateStage, new TranslateStageModule(), 1);
    if (sequence.includes(VisualPipelineSequence::WaveStage))
        pipeline->add(VisualPipelineSequence::WaveStage, new WaveStageModule(), 1);
    if (sequence.includes(VisualPipelineSequence::FrameCommitStage))
        pipeline->add(VisualPipelineSequence::FrameCommitStage, new FrameCommitModule(), 1);
    if (sequence.includes(VisualPipelineSequence::PaletteStage)) {
        paletteModule = new PaletteStageModule();
        pipeline->add(VisualPipelineSequence::PaletteStage, paletteModule, 1);
        pipeline->setFramePalette(&paletteModule->framePalette());
    }
    if (sequence.includes(VisualPipelineSequence::FlashlightStage))
        pipeline->add(VisualPipelineSequence::FlashlightStage, new FlashlightVisualModule(), 1);

    CTH_DEBUG("visual pipeline factory: created pipeline=%p stages=%d modules=%d\n",
        pipeline, int(sequence.sequence().size()), pipeline->size());
    return pipeline;
}

void VisualPipelineFactory::refresh(VisualPipeline& pipeline,
    const VisualPipelineSequence& sequence) const {
    CTH_DEBUG("visual pipeline factory: refreshing pipeline=%p stages=%d modules=%d\n",
        &pipeline, int(sequence.sequence().size()), pipeline.size());
    pipeline.refresh();
}
