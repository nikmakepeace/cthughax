#include "cthugha.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "Flashlight.h"
#include "Image.h"
#include "PipelineStageModules.h"
#include "VideoDirector.h"
#include "cth_buffer.h"
#include "display.h"
#include "imath.h"

double paletteSmoothingChance = 1.0;

template <class Module>
static Module* stageModule(VideoPipeline& pipeline, VideoPipelineSequence::Stage stage) {
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

VideoDirector::VideoDirector()
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

VideoDirector::~VideoDirector() {
    if (scene != 0)
        scene->removeObserver(*this);
}

VideoDirector& videoDirector() {
    static VideoDirector director;
    return director;
}

ImageOption& VideoDirector::imageOption() {
    return images;
}

void VideoDirector::bindScene(Scene& scene_) {
    if (scene == &scene_)
        return;

    if (scene != 0)
        scene->removeObserver(*this);

    scene = &scene_;
    scene->addObserver(*this);
    pendingSceneChanges |= SceneAllChanged;
}

void VideoDirector::unbindScene() {
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

int VideoDirector::imageLoadingEnabled() const {
    return imageLoadingEnabledValue;
}

void VideoDirector::setImageLoadingEnabled(int enabled) {
    imageLoadingEnabledValue = enabled ? 1 : 0;
}

int VideoDirector::loadImages() {
    if (!imageLoadingEnabledValue)
        return 0;

    CTH_INFO("  loading image files...\n");
    CthughaBuffer& targetBuffer = CthughaBuffer::buffer;
    int result = images.loadImages(targetBuffer.width(), targetBuffer.height());
    CTH_INFO("  number of loaded image files: %d\n", images.getNEntries());

    return result;
}

VideoPipelineSequence VideoDirector::defaultPipelineSequence() const {
    VideoPipelineSequence sequence;

    sequence.append(VideoPipelineSequence::ImageStage);
    sequence.append(VideoPipelineSequence::BorderStage);
    sequence.append(VideoPipelineSequence::FlameStage);
    sequence.append(VideoPipelineSequence::TranslateStage);
    sequence.append(VideoPipelineSequence::WaveStage);
    sequence.append(VideoPipelineSequence::FrameCommitStage);
    sequence.append(VideoPipelineSequence::PaletteStage);
    sequence.append(VideoPipelineSequence::FlashlightStage);

    CTH_DEBUG("visual director: default stage sequence stages=%d\n",
        int(sequence.sequence().size()));
    return sequence;
}

void VideoDirector::applySceneToPipeline(unsigned int changes) {
    if (scene == 0 || pipeline == 0 || buffer == 0)
        return;

    const SceneSettings& settings = scene->settings();

    FlameStageModule* flameModule
        = stageModule<FlameStageModule>(*pipeline, VideoPipelineSequence::FlameStage);
    if (flameModule != 0) {
        flameModule->setFlame(settings.flame);
        flameModule->setGeneralFlame(settings.generalFlame);
    }

    TranslateStageModule* translateModule
        = stageModule<TranslateStageModule>(*pipeline, VideoPipelineSequence::TranslateStage);
    if (translateModule != 0 && (changes & SceneTranslationChanged))
        translateModule->setTranslate(settings.translationTable);

    WaveStageModule* waveModule
        = stageModule<WaveStageModule>(*pipeline, VideoPipelineSequence::WaveStage);
    if (waveModule != 0)
        waveModule->setWave(settings.wave, settings.waveConfig);

    BorderVideoModule* borderModule
        = stageModule<BorderVideoModule>(*pipeline, VideoPipelineSequence::BorderStage);
    if (borderModule != 0)
        borderModule->setBorderMode(settings.borderMode);

    FrameCommitModule* frameCommitModule
        = stageModule<FrameCommitModule>(*pipeline, VideoPipelineSequence::FrameCommitStage);
    if (frameCommitModule != 0)
        frameCommitModule->setSceneNames(settings.flameName, settings.waveName,
            settings.waveScaleName, settings.tableName);

    PaletteStageModule* paletteModule
        = stageModule<PaletteStageModule>(*pipeline, VideoPipelineSequence::PaletteStage);
    if (paletteModule != 0) {
        int frameBudget = paletteModule->needsTarget(settings.palette)
            ? paletteChangeFrameBudget()
            : 0;
        paletteModule->setTargetPalette(settings.palette, frameBudget,
            randomPaletteTransitionStrategy());
    }

    pipeline->setStageMode(VideoPipelineSequence::FlashlightStage,
        settings.flashlightEnabled ? VideoStageEnabled : VideoStageDisabled);
    pipeline->setStageMode(VideoPipelineSequence::BorderStage, VideoStageEnabled);
    pipeline->setStageMode(VideoPipelineSequence::FlameStage, VideoStageEnabled);
    pipeline->setStageMode(VideoPipelineSequence::TranslateStage, VideoStageEnabled);
    pipeline->setStageMode(VideoPipelineSequence::WaveStage, VideoStageEnabled);
    pipeline->setStageMode(VideoPipelineSequence::FrameCommitStage, VideoStageEnabled);
    pipeline->setStageMode(VideoPipelineSequence::PaletteStage, VideoStageEnabled);
}

void VideoDirector::applyPendingImageCue() {
    if (pipeline == 0 || buffer == 0 || pendingImageCue == 0
        || pendingImageCueId == appliedImageCueId)
        return;

    ImageStageModule* imageModule
        = stageModule<ImageStageModule>(*pipeline, VideoPipelineSequence::ImageStage);
    if (imageModule == 0)
        return;

    imageModule->setImage(pendingImageCue);
    imageModule->setPlacement(imagePlacementStrategy.choose(*pendingImageCue,
        buffer->width(), buffer->height()));
    imageModule->setOverlayPassiveBuffer(1);
    pipeline->setStageMode(VideoPipelineSequence::ImageStage, VideoStageArmedOnce);

    appliedImageCueId = pendingImageCueId;
    pendingImageCue = 0;
}

void VideoDirector::sceneChanged(Scene& scene_, unsigned int changes) {
    (void)scene_;

    pendingSceneChanges |= changes;
    if (pipeline != 0 && buffer != 0) {
        applySceneToPipeline(pendingSceneChanges);
        pendingSceneChanges = SceneNoChange;
    }
}

void VideoDirector::sceneCue(Scene& scene_, const SceneCue& cue) {
    (void)scene_;

    if (cue.type == SceneCueInjectImage) {
        pendingImageCue = cue.image;
        pendingImageCueId = cue.id;
        applyPendingImageCue();
    }
}

CthughaBuffer* VideoDirector::configurePipeline(VideoPipeline& pipeline_) {
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
