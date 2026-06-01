#ifndef __PIPELINE_STAGE_MODULES_H
#define __PIPELINE_STAGE_MODULES_H

#include "Flame.h"
#include "FramePalette.h"
#include "Image.h"
#include "PaletteTransition.h"
#include "Translate.h"
#include "VideoPipeline.h"
#include "VideoPipelineSequence.h"
#include "Wave.h"

class PaletteEntry;

// Contract: one-shot pixel injector. Writes the selected image into the active
// buffer, and can mirror the same pixels into passive for immediate display.
class ImageStageModule : public VideoModule {
    const IndexedImage* image;
    ImagePlacement placement;
    int overlayPassiveBuffer;

public:
    ImageStageModule();

    void setImage(const IndexedImage* image_);
    void setPlacement(const ImagePlacement& placement_);
    void setOverlayPassiveBuffer(int enabled);
    void execute(VideoFrame& frame);
};

// Contract: feedback filter. Runs the selected Flame over the buffer, usually
// reading passive pixels and hidden border rows while writing active pixels.
class FlameStageModule : public VideoModule {
    const Flame* flame;
    int generalFlame;
    FlameLookupTables lookupTables;

public:
    FlameStageModule();

    void setFlame(const Flame* flame_);
    void setGeneralFlame(int generalFlame_);
    void execute(VideoFrame& frame);
};

// Contract: coordinate remap filter. The Translate executor owns any
// active/passive swap it needs before remapping passive pixels into active.
class TranslateStageModule : public VideoModule {
    Translate translate;

public:
    TranslateStageModule();

    void setTranslate(const TranslationTable& table);
    void execute(VideoFrame& frame);
};

// Contract: sound-reactive drawing filter. Draws into active pixels using the
// current frame context plus wave-local state; it does not commit the frame.
class WaveStageModule : public VideoModule {
    Wave* wave;
    WaveConfig config;
    WaveState state;
    WaveLookupTables lookupTables;
    int configured;
    int needsConfiguration;

public:
    WaveStageModule();

    void setWave(Wave* wave_, const WaveConfig& config_);
    void execute(VideoFrame& frame);
};

// Contract: frame boundary. Emits optional diagnostics, then swaps active and
// passive so the finished indexed image becomes the display source.
class FrameCommitModule : public VideoModule {
    const char* flameName;
    const char* waveName;
    const char* waveScaleName;
    const char* tableName;

public:
    FrameCommitModule();

    void setSceneNames(const char* flameName_, const char* waveName_,
        const char* waveScaleName_, const char* tableName_);
    void execute(VideoFrame& frame);
};

// Contract: palette post-filter. Reads acoustic context and writes temporary
// flashlight output into the frame palette; it ignores indexed pixels.
class FlashlightVideoModule : public VideoModule {
public:
    FlashlightVideoModule();

    void execute(VideoFrame& frame);
};

// Contract: hidden-row writer. Fills the active buffer border rows used by
// flame feedback; visible pixels are left to later stages.
class BorderVideoModule : public VideoModule {
    int borderMode;

public:
    BorderVideoModule();

    void setBorderMode(int borderMode_);
    void execute(VideoFrame& frame);
};

// Contract: palette transition filter. Advances the display-facing
// FramePalette toward the configured target; it ignores indexed pixels.
class PaletteStageModule : public VideoModule {
    PaletteTransition transition;
    FramePalette framePaletteValue;

public:
    PaletteStageModule();

    FramePalette& framePalette();
    int needsTarget(PaletteEntry* paletteEntry) const;
    void setTargetPalette(PaletteEntry* paletteEntry, int frameBudget,
        const PaletteTransitionStrategy& strategy);
    void execute(VideoFrame& frame);
};

FramePalette* framePaletteFromPipeline(VideoPipeline& pipeline);

#endif
