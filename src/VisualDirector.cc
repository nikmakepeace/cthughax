#include "cthugha.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "Flashlight.h"
#include "Image.h"
#include "PipelineStageModules.h"
#include "VisualDirector.h"
#include "cth_buffer.h"
#include "display.h"
#include "imath.h"

double paletteSmoothingChance = 1.0;

template <class Module>
static Module* stageModule(VisualPipeline& pipeline, VisualPipelineSequence::Stage stage) {
    return dynamic_cast<Module*>(pipeline.stageModule(stage));
}

static int paletteSmoothingFrameBudget() {
    const int PALETTE_SMOOTH_SECONDS = 2;
    int fps = 60;

    if (cthughaDisplay != 0 && cthughaDisplay->fps > 0)
        fps = cthughaDisplay->fps;

    return max(fps * PALETTE_SMOOTH_SECONDS, 1);
}

static int paletteChangeFrameBudget() {
    if (paletteSmoothingChance <= 0.0)
        return 0;

    if (((double)rand() / ((double)RAND_MAX + 1.0)) >= paletteSmoothingChance)
        return 0;

    return paletteSmoothingFrameBudget();
}

VisualDirector::VisualDirector()
    : images(0, "image")
    , imagePlacementStrategy()
    , scene(0)
    , pipeline(0)
    , buffer(0)
    , pendingSceneChanges(SceneAllChanged)
    , pendingImageCue(0)
    , pendingImageCueId(0)
    , appliedImageCueId(0)
    , imageLoadingEnabledValue(1) { }

VisualDirector::~VisualDirector() {
    if (scene != 0)
        scene->removeObserver(*this);
}

VisualDirector& visualDirector() {
    static VisualDirector director;
    return director;
}

ImageOption& VisualDirector::imageOption() {
    return images;
}

void VisualDirector::bindScene(Scene& scene_) {
    if (scene == &scene_)
        return;

    if (scene != 0)
        scene->removeObserver(*this);

    scene = &scene_;
    scene->addObserver(*this);
    pendingSceneChanges |= SceneAllChanged;
}

void VisualDirector::unbindScene() {
    if (scene != 0)
        scene->removeObserver(*this);

    scene = 0;
    pipeline = 0;
    buffer = 0;
    pendingSceneChanges = SceneAllChanged;
    pendingImageCue = 0;
    pendingImageCueId = 0;
    appliedImageCueId = 0;
}

int VisualDirector::imageLoadingEnabled() const {
    return imageLoadingEnabledValue;
}

void VisualDirector::setImageLoadingEnabled(int enabled) {
    imageLoadingEnabledValue = enabled ? 1 : 0;
}

int VisualDirector::loadImages() {
    if (!imageLoadingEnabledValue)
        return 0;

    CTH_INFO("  loading image files...\n");
    CthughaBuffer& targetBuffer = CthughaBuffer::buffer;
    int result = images.loadImages(targetBuffer.width(), targetBuffer.height());
    CTH_INFO("  number of loaded image files: %d\n", images.getNEntries());

    return result;
}

VisualPipelineSequence VisualDirector::defaultPipelineSequence() const {
    VisualPipelineSequence sequence;

    sequence.append(VisualPipelineSequence::ImageStage);
    sequence.append(VisualPipelineSequence::BorderStage);
    sequence.append(VisualPipelineSequence::FlameStage);
    sequence.append(VisualPipelineSequence::TranslateStage);
    sequence.append(VisualPipelineSequence::WaveStage);
    sequence.append(VisualPipelineSequence::FrameCommitStage);
    sequence.append(VisualPipelineSequence::PaletteStage);
    sequence.append(VisualPipelineSequence::FlashlightStage);

    CTH_DEBUG("visual director: default stage sequence stages=%d\n",
        int(sequence.sequence().size()));
    return sequence;
}

void VisualDirector::applySceneToPipeline(unsigned int changes) {
    if (scene == 0 || pipeline == 0 || buffer == 0)
        return;

    const SceneSettings& settings = scene->settings();

    FlameStageModule* flameModule
        = stageModule<FlameStageModule>(*pipeline, VisualPipelineSequence::FlameStage);
    if (flameModule != 0) {
        flameModule->setFlame(settings.flame);
        flameModule->setGeneralFlame(settings.generalFlame);
    }

    TranslateStageModule* translateModule
        = stageModule<TranslateStageModule>(*pipeline, VisualPipelineSequence::TranslateStage);
    if (translateModule != 0 && (changes & SceneTranslationChanged))
        translateModule->setTranslate(settings.translationTable);

    WaveStageModule* waveModule
        = stageModule<WaveStageModule>(*pipeline, VisualPipelineSequence::WaveStage);
    if (waveModule != 0)
        waveModule->setWave(settings.wave, settings.waveConfig);

    BorderVisualModule* borderModule
        = stageModule<BorderVisualModule>(*pipeline, VisualPipelineSequence::BorderStage);
    if (borderModule != 0)
        borderModule->setBorderMode(settings.borderMode);

    FrameCommitModule* frameCommitModule
        = stageModule<FrameCommitModule>(*pipeline, VisualPipelineSequence::FrameCommitStage);
    if (frameCommitModule != 0)
        frameCommitModule->setSceneNames(settings.flameName, settings.waveName,
            settings.waveScaleName, settings.tableName);

    PaletteStageModule* paletteModule
        = stageModule<PaletteStageModule>(*pipeline, VisualPipelineSequence::PaletteStage);
    if (paletteModule != 0) {
        int frameBudget = paletteModule->needsTarget(settings.palette)
            ? paletteChangeFrameBudget()
            : 0;
        paletteModule->setTargetPalette(settings.palette, frameBudget,
            randomPaletteTransitionStrategy());
    }

    pipeline->setStageMode(VisualPipelineSequence::FlashlightStage,
        settings.flashlightEnabled ? VisualStageEnabled : VisualStageDisabled);
    pipeline->setStageMode(VisualPipelineSequence::BorderStage, VisualStageEnabled);
    pipeline->setStageMode(VisualPipelineSequence::FlameStage, VisualStageEnabled);
    pipeline->setStageMode(VisualPipelineSequence::TranslateStage, VisualStageEnabled);
    pipeline->setStageMode(VisualPipelineSequence::WaveStage, VisualStageEnabled);
    pipeline->setStageMode(VisualPipelineSequence::FrameCommitStage, VisualStageEnabled);
    pipeline->setStageMode(VisualPipelineSequence::PaletteStage, VisualStageEnabled);
}

void VisualDirector::applyPendingImageCue() {
    if (pipeline == 0 || buffer == 0 || pendingImageCue == 0
        || pendingImageCueId == appliedImageCueId)
        return;

    ImageStageModule* imageModule
        = stageModule<ImageStageModule>(*pipeline, VisualPipelineSequence::ImageStage);
    if (imageModule == 0)
        return;

    imageModule->setImage(pendingImageCue);
    imageModule->setPlacement(imagePlacementStrategy.choose(*pendingImageCue,
        buffer->width(), buffer->height()));
    imageModule->setOverlayPassiveBuffer(1);
    pipeline->setStageMode(VisualPipelineSequence::ImageStage, VisualStageArmedOnce);

    appliedImageCueId = pendingImageCueId;
    pendingImageCue = 0;
}

void VisualDirector::sceneChanged(Scene& scene_, unsigned int changes) {
    (void)scene_;

    pendingSceneChanges |= changes;
    if (pipeline != 0 && buffer != 0) {
        applySceneToPipeline(pendingSceneChanges);
        pendingSceneChanges = SceneNoChange;
    }
}

void VisualDirector::sceneCue(Scene& scene_, const SceneCue& cue) {
    (void)scene_;

    if (cue.type == SceneCueInjectImage) {
        pendingImageCue = cue.image;
        pendingImageCueId = cue.id;
        applyPendingImageCue();
    }
}

CthughaBuffer* VisualDirector::configurePipeline(VisualPipeline& pipeline_) {
    CthughaBuffer* targetBuffer = &CthughaBuffer::buffer;

    if (pipeline != &pipeline_) {
        pipeline = &pipeline_;
        pendingSceneChanges |= SceneAllChanged;
    }
    if (buffer != targetBuffer) {
        buffer = targetBuffer;
        pendingSceneChanges |= SceneAllChanged;
    }

    if (pendingSceneChanges != SceneNoChange) {
        applySceneToPipeline(pendingSceneChanges);
        pendingSceneChanges = SceneNoChange;
    }
    applyPendingImageCue();

    return targetBuffer;
}
