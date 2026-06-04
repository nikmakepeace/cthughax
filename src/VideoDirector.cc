#include "cthugha.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "Configuration.h"
#include "Flashlight.h"
#include "Image.h"
#include "VideoFilters.h"
#include "VideoDirector.h"
#include "VideoFrameBudget.h"
#include "cth_buffer.h"
#include "display.h"
#include "imath.h"

OptionTime changeMsgTime("change-msg-time", 0);

static double paletteSmoothingChance = 0.0;
static int paletteSmoothSeconds = 1;
static int quietMessageDurationMs = 0;

template <class Filter>
static Filter* stageFilter(VideoFilterchain& filterchain, VideoFilterchainSequence::Stage stage) {
    return dynamic_cast<Filter*>(filterchain.stageFilter(stage));
}

static int frameBudgetFramesPerSecond() {
    return videoFrameBudgetFramesPerSecond(int(maxFramesPerSecond),
        cthughaDisplay != 0 ? cthughaDisplay->rollingFps : 0.0);
}

static int paletteSmoothingFrameBudget() {
    int fps = frameBudgetFramesPerSecond();

    return max(fps * paletteSmoothSeconds, 1);
}

static int shouldSmoothPaletteChange() {
    if (paletteSmoothingChance <= 0.0)
        return 0;

    if (((double)rand() / ((double)RAND_MAX + 1.0)) >= paletteSmoothingChance)
        return 0;

    return 1;
}

static int paletteChangeFrameBudget() {
    return shouldSmoothPaletteChange() ? paletteSmoothingFrameBudget() : 0;
}

static int quietMessageFrameBudget() {
    int fps = frameBudgetFramesPerSecond();
    int durationMs = (quietMessageDurationMs > int(changeMsgTime))
        ? int(changeMsgTime)
        : quietMessageDurationMs;

    return max(1, (fps * (durationMs / 1000)));
}

enum ImageCuePaletteMode {
    ImageCuePaletteSnapThenReturn,
    ImageCuePaletteAdopt,
    ImageCuePaletteIgnore
};

static ImageCuePaletteMode chooseImageCuePaletteMode() {
    if (shouldSmoothPaletteChange())
        return ImageCuePaletteSnapThenReturn;

    return (rand() % 2 == 0) ? ImageCuePaletteAdopt : ImageCuePaletteIgnore;
}

static const ColorPalette* scenePaletteColors(const SceneSettings& settings) {
    return (settings.palette != 0) ? &settings.palette->colors() : 0;
}

VideoDirector::VideoDirector()
    : images(0, "image")
    , imagePlacementStrategy()
    , silenceMessage()
    , scene(0)
    , filterchain(0)
    , buffer(0)
    , pendingSceneChanges(SceneAllChanged)
    , pendingImageCue(0)
    , pendingImageCueId(0)
    , appliedImageCueId(0)
    , pendingTextMessage()
    , pendingTextCueId(0)
    , appliedTextCueId(0)
    , pendingTextFrames(0)
    , pendingTextInkColor(-1) { }

VideoDirector::~VideoDirector() {
    if (scene != 0)
        scene->removeObserver(*this);
}

void VideoDirector::configureTransitions(
    const SceneTransitionPolicy& transitionPolicy) {
    paletteSmoothingChance = transitionPolicy.paletteSmoothingChance;
    paletteSmoothSeconds = transitionPolicy.paletteSmoothSeconds;
}

void VideoDirector::configureQuietMessages(const MessagesConfig& messagesConfig) {
    changeMsgTime.setValue(messagesConfig.quietMessageMs);
    quietMessageDurationMs = messagesConfig.quietMessageDurationMs;
    silenceMessage.configure(messagesConfig);
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
    filterchain = 0;
    buffer = 0;
    pendingSceneChanges = SceneAllChanged;
    pendingImageCue = 0;
    pendingImageCueId = 0;
    appliedImageCueId = 0;
    pendingTextMessage.clear();
    pendingTextCueId = 0;
    appliedTextCueId = 0;
    pendingTextFrames = 0;
    pendingTextInkColor = -1;
}

SilenceMessage& VideoDirector::silenceMessages() {
    return silenceMessage;
}

int VideoDirector::observeQuiet(int quietLength) {
    if (!int(changeMsgTime) || quietLength <= int(changeMsgTime))
        return 0;

    if (scene == 0)
        return 0;

    std::string message = silenceMessage.nextMessage();
    scene->emitTextCue(message.c_str(), quietMessageFrameBudget(), -1);
    return 1;
}

VideoFilterchainSequence VideoDirector::defaultFilterchainSequence() const {
    VideoFilterchainSequence sequence;

    sequence.append(VideoFilterchainSequence::ImageStage);
    sequence.append(VideoFilterchainSequence::BorderStage);
    sequence.append(VideoFilterchainSequence::FlameStage);
    sequence.append(VideoFilterchainSequence::TranslateStage);
    sequence.append(VideoFilterchainSequence::WaveStage);
    sequence.append(VideoFilterchainSequence::TextStage);
    sequence.append(VideoFilterchainSequence::FrameCommitStage);
    sequence.append(VideoFilterchainSequence::PaletteStage);
    sequence.append(VideoFilterchainSequence::FlashlightStage);
    sequence.append(VideoFilterchainSequence::IndexedFrameStage);

    CTH_DEBUG("video director: default stage sequence stages=%d\n",
        int(sequence.sequence().size()));
    return sequence;
}

void VideoDirector::applySceneToFilterchain(unsigned int changes) {
    if (scene == 0 || filterchain == 0 || buffer == 0)
        return;

    const SceneSettings& settings = scene->settings();

    FlameFilter* flameFilter
        = stageFilter<FlameFilter>(*filterchain, VideoFilterchainSequence::FlameStage);
    if (flameFilter != 0) {
        flameFilter->setFlame(settings.flame);
        flameFilter->setGeneralFlame(settings.generalFlame);
    }

    TranslateFilter* translateFilter
        = stageFilter<TranslateFilter>(*filterchain, VideoFilterchainSequence::TranslateStage);
    if (translateFilter != 0 && (changes & SceneTranslationChanged))
        translateFilter->setTranslate(settings.translationTable);

    WaveFilter* waveFilter
        = stageFilter<WaveFilter>(*filterchain, VideoFilterchainSequence::WaveStage);
    if (waveFilter != 0)
        waveFilter->setWave(settings.wave, settings.waveConfig);

    BorderFilter* borderFilter
        = stageFilter<BorderFilter>(*filterchain, VideoFilterchainSequence::BorderStage);
    if (borderFilter != 0)
        borderFilter->setBorderMode(settings.borderMode);

    FrameCommitFilter* frameCommitFilter
        = stageFilter<FrameCommitFilter>(*filterchain, VideoFilterchainSequence::FrameCommitStage);
    if (frameCommitFilter != 0)
        frameCommitFilter->setSceneNames(settings.flameName, settings.waveName,
            settings.waveScaleName, settings.tableName);

    if (changes & ScenePaletteChanged) {
        PaletteFilter* paletteFilter
            = stageFilter<PaletteFilter>(*filterchain, VideoFilterchainSequence::PaletteStage);
        if (paletteFilter != 0) {
            int frameBudget = paletteFilter->needsTarget(settings.palette)
                ? paletteChangeFrameBudget()
                : 0;
            paletteFilter->setTargetPalette(settings.palette, frameBudget,
                randomPaletteTransitionStrategy());
        }
    }

    filterchain->setStageMode(VideoFilterchainSequence::FlashlightStage,
        settings.flashlightEnabled ? VideoFilterEnabled : VideoFilterDisabled);
    filterchain->setStageMode(VideoFilterchainSequence::BorderStage, VideoFilterEnabled);
    filterchain->setStageMode(VideoFilterchainSequence::FlameStage, VideoFilterEnabled);
    filterchain->setStageMode(VideoFilterchainSequence::TranslateStage, VideoFilterEnabled);
    filterchain->setStageMode(VideoFilterchainSequence::WaveStage, VideoFilterEnabled);
    filterchain->setStageMode(VideoFilterchainSequence::FrameCommitStage, VideoFilterEnabled);
    filterchain->setStageMode(VideoFilterchainSequence::PaletteStage, VideoFilterEnabled);
    filterchain->setStageMode(VideoFilterchainSequence::IndexedFrameStage, VideoFilterEnabled);
}

void VideoDirector::applyPendingImageCue() {
    if (filterchain == 0 || buffer == 0 || pendingImageCue == 0
        || pendingImageCueId == appliedImageCueId)
        return;

    ImageFilter* imageFilter
        = stageFilter<ImageFilter>(*filterchain, VideoFilterchainSequence::ImageStage);
    if (imageFilter == 0)
        return;

    imageFilter->setImage(pendingImageCue);
    imageFilter->setPlacement(imagePlacementStrategy.choose(*pendingImageCue,
        buffer->width(), buffer->height()));
    imageFilter->setOverlayPassiveBuffer(1);
    filterchain->setStageMode(VideoFilterchainSequence::ImageStage, VideoFilterArmedOnce);
    applyImageCuePalette(*pendingImageCue);

    appliedImageCueId = pendingImageCueId;
    pendingImageCue = 0;
}

void VideoDirector::applyImageCuePalette(const IndexedImage& image) {
    if (scene == 0 || filterchain == 0)
        return;

    const ColorPalette* imagePalette = image.palette();
    if (imagePalette == 0)
        return;

    PaletteFilter* paletteFilter
        = stageFilter<PaletteFilter>(*filterchain, VideoFilterchainSequence::PaletteStage);
    if (paletteFilter == 0)
        return;

    ImageCuePaletteMode mode = chooseImageCuePaletteMode();
    if (mode == ImageCuePaletteIgnore)
        return;

    if (mode == ImageCuePaletteAdopt) {
        paletteFilter->setTargetPalette(*imagePalette, 0,
            linearPaletteTransitionStrategy());
        return;
    }

    const ColorPalette* returnPalette = scenePaletteColors(scene->settings());
    if (returnPalette == 0) {
        paletteFilter->setTargetPalette(*imagePalette, 0,
            linearPaletteTransitionStrategy());
        return;
    }

    paletteFilter->snapThenTransitionPalette(*imagePalette, *returnPalette,
        paletteSmoothingFrameBudget(), linearPaletteTransitionStrategy());
}

void VideoDirector::applyPendingTextCue() {
    if (filterchain == 0 || pendingTextCueId == appliedTextCueId
        || pendingTextMessage.empty() || pendingTextFrames <= 0)
        return;

    TextInjectionFilter* textFilter
        = stageFilter<TextInjectionFilter>(*filterchain, VideoFilterchainSequence::TextStage);
    if (textFilter == 0)
        return;

    textFilter->setInkColor(pendingTextInkColor);
    textFilter->setMessage(pendingTextMessage.c_str(), pendingTextFrames);
    filterchain->setStageMode(VideoFilterchainSequence::TextStage, VideoFilterEnabled);

    appliedTextCueId = pendingTextCueId;
}

void VideoDirector::sceneChanged(Scene& scene_, unsigned int changes) {
    (void)scene_;

    pendingSceneChanges |= changes;
    if (filterchain != 0 && buffer != 0) {
        applySceneToFilterchain(pendingSceneChanges);
        pendingSceneChanges = SceneNoChange;
    }
}

void VideoDirector::sceneCue(Scene& scene_, const SceneCue& cue) {
    (void)scene_;

    if (cue.type == SceneCueInjectImage) {
        pendingImageCue = cue.image;
        pendingImageCueId = cue.id;
        applyPendingImageCue();
    } else if (cue.type == SceneCueInjectText) {
        pendingTextMessage = cue.text;
        pendingTextFrames = cue.textFrames;
        pendingTextInkColor = cue.textInkColor;
        pendingTextCueId = cue.id;
        applyPendingTextCue();
    }
}

CthughaBuffer* VideoDirector::configureFilterchain(VideoFilterchain& filterchain_) {
    CthughaBuffer* targetBuffer = &CthughaBuffer::buffer;

    if (filterchain != &filterchain_) {
        filterchain = &filterchain_;
        pendingSceneChanges |= SceneAllChanged;
    }
    if (buffer != targetBuffer) {
        buffer = targetBuffer;
        pendingSceneChanges |= SceneAllChanged;
    }

    if (pendingSceneChanges != SceneNoChange) {
        applySceneToFilterchain(pendingSceneChanges);
        pendingSceneChanges = SceneNoChange;
    }
    applyPendingImageCue();
    applyPendingTextCue();

    return targetBuffer;
}
