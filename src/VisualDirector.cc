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
#include "flames.h"
#include "imath.h"
#include "waves.h"

double paletteSmoothingChance = 1.0;

static CthughaBuffer* currentBuffer() {
    return CthughaBuffer::current;
}

template <class Module>
static Module* stageModule(VisualPipeline& pipeline, VisualPipelineSequence::Stage stage) {
    return dynamic_cast<Module*>(pipeline.stageModule(stage));
}

static Wave* selectRunnableWave(const WaveConfig& config) {
    int nEntries = wave.getNEntries();

    for (int i = 0; i < nEntries; i++) {
        Wave* selectedWave = wave.currentWave();
        if (selectedWave == 0 || selectedWave->canRun(config))
            return selectedWave;

        wave.change(+1, 0);
    }

    return 0;
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
    : images(0, "pcx")
    , imagePlacementStrategy()
    , imageLoadingEnabledValue(1)
    , lastImageSelection(-1) { }

VisualDirector& visualDirector() {
    static VisualDirector director;
    return director;
}

ImageOption& VisualDirector::imageOption() {
    return images;
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
    int result = images.loadImages();
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

int VisualDirector::imageSelectionChanged() const {
    return lastImageSelection != images.currentN();
}

void VisualDirector::markImageSelectionSeen() {
    lastImageSelection = images.currentN();
}

void VisualDirector::syncCurrentBuffer() {
    CthughaBuffer::current = &CthughaBuffer::buffer;
}

void VisualDirector::updatePipelineStages(VisualPipeline& pipeline, CthughaBuffer& buffer) {
    ImageStageModule* imageModule
        = stageModule<ImageStageModule>(pipeline, VisualPipelineSequence::ImageStage);
    if (imageModule != 0) {
        const IndexedImage* currentImage = images.currentImage();
        imageModule->setImage(currentImage);
        if (imageSelectionChanged() && currentImage != 0) {
            imageModule->setPlacement(imagePlacementStrategy.choose(*currentImage,
                buffer.width(), buffer.height()));
            imageModule->setOverlayPassiveBuffer(1);
        }
    }

    const Flame* currentFlame = flame.currentFlame();
    FlameStageModule* flameModule
        = stageModule<FlameStageModule>(pipeline, VisualPipelineSequence::FlameStage);
    if (flameModule != 0) {
        flameModule->setFlame(currentFlame);
        flameModule->setGeneralFlame(int(flameGeneral));
    }

    TranslateStageModule* translateModule
        = stageModule<TranslateStageModule>(pipeline, VisualPipelineSequence::TranslateStage);
    if (translateModule != 0)
        translateModule->setTranslateProvider(&buffer.translate);

    WaveStageModule* waveModule
        = stageModule<WaveStageModule>(pipeline, VisualPipelineSequence::WaveStage);
    if (waveModule != 0) {
        WaveConfig waveConfig(int(waveScale), int(table), currentWaveObject(),
            buffer.width(), buffer.height());
        Wave* currentWave = selectRunnableWave(waveConfig);
        if (currentWave != 0)
            currentWave->configure(waveConfig);
        waveModule->setWave(currentWave);
    }

    BorderVisualModule* borderModule
        = stageModule<BorderVisualModule>(pipeline, VisualPipelineSequence::BorderStage);
    if (borderModule != 0)
        borderModule->setBorderMode(int(border));

    FrameCommitModule* frameCommitModule
        = stageModule<FrameCommitModule>(pipeline, VisualPipelineSequence::FrameCommitStage);
    if (frameCommitModule != 0)
        frameCommitModule->setFlameName((currentFlame != 0) ? currentFlame->name() : "unknown");

    PaletteStageModule* paletteModule
        = stageModule<PaletteStageModule>(pipeline, VisualPipelineSequence::PaletteStage);
    PaletteEntry* currentPaletteEntry = palette.currentPaletteEntry();
    if (paletteModule != 0) {
        int frameBudget = paletteModule->needsTarget(currentPaletteEntry)
            ? paletteChangeFrameBudget()
            : 0;
        paletteModule->setTargetPalette(currentPaletteEntry, frameBudget,
            randomPaletteTransitionStrategy());
    }
}

CthughaBuffer* VisualDirector::configurePipeline(VisualPipeline& pipeline) {
    syncCurrentBuffer();
    CthughaBuffer* buffer = currentBuffer();
    if (buffer == 0)
        return 0;

    updatePipelineStages(pipeline, *buffer);

    int selectedImageChanged = imageSelectionChanged();
    const IndexedImage* selectedImage = images.currentImage();
    if (selectedImageChanged && selectedImage != 0)
        pipeline.setStageMode(VisualPipelineSequence::ImageStage, VisualStageArmedOnce);
    if (selectedImageChanged)
        markImageSelectionSeen();
    pipeline.setStageMode(VisualPipelineSequence::FlashlightStage,
        (int(flashlight) != 0) ? VisualStageEnabled : VisualStageDisabled);
    pipeline.setStageMode(VisualPipelineSequence::BorderStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPipelineSequence::FlameStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPipelineSequence::TranslateStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPipelineSequence::WaveStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPipelineSequence::FrameCommitStage, VisualStageEnabled);
    pipeline.setStageMode(VisualPipelineSequence::PaletteStage, VisualStageEnabled);

    return buffer;
}
