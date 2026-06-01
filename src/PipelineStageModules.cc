#include "cthugha.h"
#include "Border.h"
#include "CthughaBuffer.h"
#include "Flame.h"
#include "Flashlight.h"
#include "Image.h"
#include "PipelineStageModules.h"
#include "cth_buffer.h"
#include "display.h"
#include "Wave.h"

ImageStageModule::ImageStageModule()
    : image(0)
    , placement()
    , overlayPassiveBuffer(1) { }

void ImageStageModule::setImage(const IndexedImage* image_) {
    image = image_;
}

void ImageStageModule::setPlacement(const ImagePlacement& placement_) {
    placement = placement_;
}

void ImageStageModule::setOverlayPassiveBuffer(int enabled) {
    overlayPassiveBuffer = enabled;
}

void ImageStageModule::execute(VideoFrame& frame) {
    CTH_TRACE("executing image stage\n", "visual pipeline");
    if (image == 0 || !placement.visible())
        return;

    CthughaBuffer& buffer = frame.buffer();
    unsigned char* active = buffer.activePixels();
    unsigned char* passive = buffer.passivePixels();
    const unsigned char* sourcePixels = image->pixels();
    if (active == 0 || sourcePixels == 0)
        return;

    for (int row = 0; row < placement.height; row++) {
        const unsigned char* source = sourcePixels
            + (placement.sourceY + row) * image->width()
            + placement.sourceX;
        unsigned char* activeDestination = active
            + (placement.destinationY + row) * buffer.pitch()
            + placement.destinationX;

        memcpy(activeDestination, source, placement.width);

        if (overlayPassiveBuffer && passive != 0) {
            unsigned char* passiveDestination = passive
                + (placement.destinationY + row) * buffer.pitch()
                + placement.destinationX;
            memcpy(passiveDestination, source, placement.width);
        }
    }
}

FlameStageModule::FlameStageModule()
    : flame(0)
    , generalFlame(0) { }

void FlameStageModule::setFlame(const Flame* flame_) {
    flame = flame_;
}

void FlameStageModule::setGeneralFlame(int generalFlame_) {
    generalFlame = generalFlame_;
}

void FlameStageModule::execute(VideoFrame& frame) {
    CTH_TRACE("executing flame stage\n", "visual pipeline");

    if (flame != 0)
        flame->execute(frame.buffer(), frame.context(), generalFlame, lookupTables);
}

TranslateStageModule::TranslateStageModule()
    : translate() { }

void TranslateStageModule::setTranslate(const TranslationTable& table) {
    translate = Translate(table);
}

void TranslateStageModule::execute(VideoFrame& frame) {
    CTH_TRACE("executing translate stage\n", "visual pipeline");
    translate.execute(frame.buffer(), frame.context());
}

WaveStageModule::WaveStageModule()
    : wave(0)
    , config()
    , state()
    , lookupTables()
    , configured(0)
    , needsConfiguration(1) { }

void WaveStageModule::setWave(Wave* wave_, const WaveConfig& config_) {
    if (wave != wave_) {
        state.clear();
        needsConfiguration = 1;
    } else if (!configured || !config.sameAs(config_)) {
        needsConfiguration = 1;
    }

    wave = wave_;
    config = config_;
    configured = 1;
}

void WaveStageModule::execute(VideoFrame& frame) {
    CTH_TRACE("executing wave stage\n", "visual pipeline");
    if (wave != NULL) {
        wave->execute(frame.buffer(), frame.context(), config,
            needsConfiguration, state, lookupTables);
        needsConfiguration = 0;
    }
}

FrameCommitModule::FrameCommitModule()
    : flameName("unknown")
    , waveName("unknown")
    , waveScaleName("unknown")
    , tableName("unknown") { }

void FrameCommitModule::setSceneNames(const char* flameName_, const char* waveName_,
    const char* waveScaleName_, const char* tableName_) {
    flameName = (flameName_ != 0) ? flameName_ : "unknown";
    waveName = (waveName_ != 0) ? waveName_ : "unknown";
    waveScaleName = (waveScaleName_ != 0) ? waveScaleName_ : "unknown";
    tableName = (tableName_ != 0) ? tableName_ : "unknown";
}

void FrameCommitModule::execute(VideoFrame& frame) {
    CTH_TRACE("committing indexed buffer frame\n", "visual pipeline");
    static int debugReports = 0;
    CthughaBuffer& buffer = frame.buffer();

    if (CTH_LOG_ENABLED(CTH_LOG_DEBUG) && (debugReports < 16)) {
        int nonzero = 0;
        int peak = 0;
        for (int i = 0; i < buffer.size(); i++) {
            int value = buffer.activePixels()[i];
            if (value != 0)
                nonzero++;
            if (value > peak)
                peak = value;
        }
        debugReports++;
        CTH_DEBUG("visual buffer: wave=%s wave-scale=%s flame=%s table=%s nonzero-pixels=%d peak-pixel=%d size=%d\n",
            waveName,
            waveScaleName,
            flameName,
            tableName,
            nonzero, peak, buffer.size());
    }

    buffer.swapBuffers();
}

FlashlightVideoModule::FlashlightVideoModule() { }

void FlashlightVideoModule::execute(VideoFrame& frame) {
    CTH_TRACE("executing flashlight stage\n", "visual pipeline");
    FramePalette* framePalette = frame.framePalette();
    if (framePalette != 0)
        apply_flashlight(*framePalette, frame.context());
}

BorderVideoModule::BorderVideoModule()
    : borderMode(0) { }

void BorderVideoModule::setBorderMode(int borderMode_) {
    borderMode = borderMode_;
}

void BorderVideoModule::execute(VideoFrame& frame) {
    CTH_TRACE("executing border stage mode=%d\n", "visual pipeline", borderMode);
    apply_border(frame.buffer(), frame.context(), borderMode);
}

PaletteStageModule::PaletteStageModule() { }

FramePalette& PaletteStageModule::framePalette() {
    return framePaletteValue;
}

int PaletteStageModule::needsTarget(PaletteEntry* paletteEntry) const {
    return paletteEntry != 0 && !transition.hasTarget(paletteEntry->colors());
}

void PaletteStageModule::setTargetPalette(PaletteEntry* paletteEntry, int frameBudget,
    const PaletteTransitionStrategy& strategy) {
    if (paletteEntry != 0)
        transition.achieve(paletteEntry->colors(), frameBudget, strategy);
}

void PaletteStageModule::execute(VideoFrame& frame) {
    CTH_TRACE("executing palette stage\n", "visual pipeline");
    FramePalette* framePalette = frame.framePalette();
    transition.execute((framePalette != 0) ? *framePalette : framePaletteValue);
}

FramePalette* framePaletteFromPipeline(VideoPipeline& pipeline) {
    return pipeline.framePalette();
}
