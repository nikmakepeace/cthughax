// Scene binding for the Frame Generator module.

#include "FrameGeneratorSceneBinding.h"

#include "IndexedImage.h"
#include "PaletteTransition.h"
#include "FrameFilters.h"

template <class Filter>
static Filter* stageFilter(FrameFilterchain& filterchain,
    FrameFilterchainSequence::Stage stage) {
    return dynamic_cast<Filter*>(filterchain.stageFilter(stage));
}

static const ColorPalette* scenePaletteColors(const SceneSettings& settings) {
    return settings.paletteColors;
}

FrameGeneratorSceneBinding::FrameGeneratorSceneBinding(
    const FrameGeometry& geometry, FrameTransitionController& transitionController_,
    RandomSource& randomSource, CountdownTimerFactory& timerFactory,
    LogSink& log)
    : geometryValue(geometry)
    , transitionController(transitionController_)
    , imagePlacementStrategy()
    , randomSourceValue(randomSource)
    , logValue(log)
    , silenceMessage()
    , scene(0)
    , filterchain(0)
    , pendingSceneChanges(SceneAllChanged)
    , pendingImageCue(0)
    , pendingImageCueId(0)
    , appliedImageCueId(0)
    , pendingTextMessage()
    , pendingTextCueId(0)
    , appliedTextCueId(0)
    , pendingTextFrames(0)
    , pendingTextInkColor(-1) {
    silenceMessage.setRandomSource(randomSourceValue);
    silenceMessage.setTimerFactory(timerFactory);
}

FrameGeneratorSceneBinding::~FrameGeneratorSceneBinding() {
    unbindScene();
}

SilenceMessage& FrameGeneratorSceneBinding::silenceMessages() {
    return silenceMessage;
}

void FrameGeneratorSceneBinding::bindScene(Scene& scene_) {
    if (scene == &scene_)
        return;

    if (scene != 0)
        scene->removeObserver(*this);

    scene = &scene_;
    scene->addObserver(*this);
    pendingSceneChanges |= SceneAllChanged;
}

void FrameGeneratorSceneBinding::unbindScene() {
    if (scene != 0)
        scene->removeObserver(*this);

    scene = 0;
    filterchain = 0;
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

int FrameGeneratorSceneBinding::observeQuiet(int quietLength,
    int quietMessageThresholdMs, int frameBudgetFramesPerSecond) {
    if (!quietMessageThresholdMs || quietLength <= quietMessageThresholdMs)
        return 0;

    if (scene == 0)
        return 0;

    std::string message = silenceMessage.nextMessage();
    scene->emitTextCue(message.c_str(),
        transitionController.quietMessageFrameBudget(frameBudgetFramesPerSecond,
            quietMessageThresholdMs),
        -1);
    return 1;
}

FrameFilterchainSequence FrameGeneratorSceneBinding::defaultFilterchainSequence() const {
    FrameFilterchainSequence sequence;

    sequence.append(FrameFilterchainSequence::ImageStage);
    sequence.append(FrameFilterchainSequence::BorderStage);
    sequence.append(FrameFilterchainSequence::FlameStage);
    sequence.append(FrameFilterchainSequence::TranslateStage);
    sequence.append(FrameFilterchainSequence::WaveStage);
    sequence.append(FrameFilterchainSequence::TextStage);
    sequence.append(FrameFilterchainSequence::FrameCommitStage);
    sequence.append(FrameFilterchainSequence::PaletteStage);
    sequence.append(FrameFilterchainSequence::FlashlightStage);
    sequence.append(FrameFilterchainSequence::IndexedFrameStage);

    logValue.debug("frame generator: default stage sequence stages=%d\n",
        int(sequence.sequence().size()));
    return sequence;
}

void FrameGeneratorSceneBinding::applySceneToFilterchain(unsigned int changes,
    int frameBudgetFramesPerSecond) {
    if (scene == 0 || filterchain == 0)
        return;

    const SceneSettings& settings = scene->settings();

    FlameFilter* flameFilter
        = stageFilter<FlameFilter>(*filterchain, FrameFilterchainSequence::FlameStage);
    if (flameFilter != 0) {
        flameFilter->setFlame(settings.flame);
        flameFilter->setGeneralFlame(settings.generalFlame);
    }

    TranslateFilter* translateFilter
        = stageFilter<TranslateFilter>(*filterchain,
            FrameFilterchainSequence::TranslateStage);
    if (translateFilter != 0 && (changes & SceneTranslationChanged))
        translateFilter->setTranslate(settings.translationTable);

    WaveFilter* waveFilter
        = stageFilter<WaveFilter>(*filterchain, FrameFilterchainSequence::WaveStage);
    if (waveFilter != 0) {
        waveFilter->setRandomSource(randomSourceValue);
        waveFilter->setWave(settings.wave, settings.waveConfig);
    }

    BorderFilter* borderFilter
        = stageFilter<BorderFilter>(*filterchain, FrameFilterchainSequence::BorderStage);
    if (borderFilter != 0)
        borderFilter->setBorderMode(settings.borderMode);

    FrameCommitFilter* frameCommitFilter
        = stageFilter<FrameCommitFilter>(*filterchain,
            FrameFilterchainSequence::FrameCommitStage);
    if (frameCommitFilter != 0)
        frameCommitFilter->setSceneNames(settings.flameName, settings.waveName,
            settings.waveScaleName, settings.tableName);

    if (changes & ScenePaletteChanged) {
        PaletteFilter* paletteFilter
            = stageFilter<PaletteFilter>(*filterchain,
                FrameFilterchainSequence::PaletteStage);
        const ColorPalette* paletteColors = scenePaletteColors(settings);
        if (paletteFilter != 0 && paletteColors != 0) {
            int frameBudget = paletteFilter->needsTarget(*paletteColors)
                ? transitionController.paletteChangeFrameBudget(randomSourceValue,
                    frameBudgetFramesPerSecond)
                : 0;
            paletteFilter->setTargetPalette(*paletteColors, frameBudget,
                randomPaletteTransitionStrategy(randomSourceValue));
        }
    }

    filterchain->setStageMode(FrameFilterchainSequence::FlashlightStage,
        settings.flashlightEnabled ? FrameFilterEnabled : FrameFilterDisabled);
    filterchain->setStageMode(FrameFilterchainSequence::BorderStage,
        FrameFilterEnabled);
    filterchain->setStageMode(FrameFilterchainSequence::FlameStage,
        FrameFilterEnabled);
    filterchain->setStageMode(FrameFilterchainSequence::TranslateStage,
        FrameFilterEnabled);
    filterchain->setStageMode(FrameFilterchainSequence::WaveStage,
        FrameFilterEnabled);
    filterchain->setStageMode(FrameFilterchainSequence::FrameCommitStage,
        FrameFilterEnabled);
    filterchain->setStageMode(FrameFilterchainSequence::PaletteStage,
        FrameFilterEnabled);
    filterchain->setStageMode(FrameFilterchainSequence::IndexedFrameStage,
        FrameFilterEnabled);
}

void FrameGeneratorSceneBinding::applyPendingImageCue(
    int frameBudgetFramesPerSecond) {
    if (filterchain == 0 || pendingImageCue == 0
        || pendingImageCueId == appliedImageCueId)
        return;

    ImageFilter* imageFilter
        = stageFilter<ImageFilter>(*filterchain, FrameFilterchainSequence::ImageStage);
    if (imageFilter == 0)
        return;

    imageFilter->setImage(pendingImageCue);
    imageFilter->setPlacement(imagePlacementStrategy.choose(*pendingImageCue,
        geometryValue.width(), geometryValue.height(), randomSourceValue));
    imageFilter->setOverlayPassiveBuffer(1);
    filterchain->setStageMode(FrameFilterchainSequence::ImageStage,
        FrameFilterArmedOnce);
    applyImageCuePalette(*pendingImageCue, frameBudgetFramesPerSecond);

    appliedImageCueId = pendingImageCueId;
    pendingImageCue = 0;
}

void FrameGeneratorSceneBinding::applyImageCuePalette(const IndexedImage& image,
    int frameBudgetFramesPerSecond) {
    if (scene == 0 || filterchain == 0)
        return;

    const ColorPalette* imagePalette = image.palette();
    if (imagePalette == 0)
        return;

    PaletteFilter* paletteFilter
        = stageFilter<PaletteFilter>(*filterchain,
            FrameFilterchainSequence::PaletteStage);
    if (paletteFilter == 0)
        return;

    FrameImageCuePaletteMode mode = transitionController.chooseImageCuePaletteMode(
        randomSourceValue, frameBudgetFramesPerSecond);
    if (mode == FrameImageCuePaletteIgnore)
        return;

    if (mode == FrameImageCuePaletteAdopt) {
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
        transitionController.paletteSmoothingFrameBudget(frameBudgetFramesPerSecond),
        linearPaletteTransitionStrategy());
}

void FrameGeneratorSceneBinding::applyPendingTextCue() {
    if (filterchain == 0 || pendingTextCueId == appliedTextCueId
        || pendingTextMessage.empty() || pendingTextFrames <= 0)
        return;

    TextInjectionFilter* textFilter
        = stageFilter<TextInjectionFilter>(*filterchain,
            FrameFilterchainSequence::TextStage);
    if (textFilter == 0)
        return;

    textFilter->setInkColor(pendingTextInkColor);
    textFilter->setMessage(pendingTextMessage.c_str(), pendingTextFrames);
    filterchain->setStageMode(FrameFilterchainSequence::TextStage,
        FrameFilterEnabled);

    appliedTextCueId = pendingTextCueId;
}

void FrameGeneratorSceneBinding::configureFilterchain(
    FrameFilterchain& filterchain_, const FrameGeneratorContext& context) {
    if (filterchain != &filterchain_) {
        filterchain = &filterchain_;
        pendingSceneChanges |= SceneAllChanged;
    }

    if (pendingSceneChanges != SceneNoChange) {
        applySceneToFilterchain(pendingSceneChanges,
            context.frameBudgetFramesPerSecond());
        pendingSceneChanges = SceneNoChange;
    }
    applyPendingImageCue(context.frameBudgetFramesPerSecond());
    applyPendingTextCue();
}

void FrameGeneratorSceneBinding::sceneChanged(Scene& scene_,
    unsigned int changes) {
    (void)scene_;

    pendingSceneChanges |= changes;
}

void FrameGeneratorSceneBinding::sceneCue(Scene& scene_, const SceneCue& cue) {
    (void)scene_;

    if (cue.type == SceneCueInjectImage) {
        pendingImageCue = cue.image;
        pendingImageCueId = cue.id;
    } else if (cue.type == SceneCueInjectText) {
        pendingTextMessage = cue.text;
        pendingTextFrames = cue.textFrames;
        pendingTextInkColor = cue.textInkColor;
        pendingTextCueId = cue.id;
    }
}
