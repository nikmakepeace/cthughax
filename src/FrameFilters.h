#ifndef CTHUGHA_FRAME_FILTERS_H
#define CTHUGHA_FRAME_FILTERS_H

#include "Flame.h"
#include "FramePalette.h"
#include "ImagePlacement.h"
#include "PaletteTransition.h"
#include "Translate.h"
#include "FrameFilterchain.h"
#include "FrameFilterchainSequence.h"
#include "Wave.h"

#include <string>

class BitmapFont;
class IndexedImage;
class RandomSource;

enum TextInjectionHorizontalAlign {
    /** Align text block to the left edge of the available rectangle. */
    TextInjectionAlignLeft,

    /** Center text block horizontally in the available rectangle. */
    TextInjectionAlignCenter,

    /** Align text block to the right edge of the available rectangle. */
    TextInjectionAlignRight
};

enum TextInjectionVerticalAlign {
    /** Align text block to the top edge of the available rectangle. */
    TextInjectionAlignTop,

    /** Center text block vertically in the available rectangle. */
    TextInjectionAlignMiddle,

    /** Align text block to the bottom edge of the available rectangle. */
    TextInjectionAlignBottom
};

// Contract: one-shot pixel injector. The framework copies source to
// destination first, then this overlays the selected image into destination.
class ImageFilter : public FrameFilter {
    const IndexedImage* image;
    ImagePlacement placement;

public:
    ImageFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /**
     * Selects the image to draw.
     *
     * @param image_ Borrowed indexed image pointer, or NULL to disable drawing.
     */
    void setImage(const IndexedImage* image_);

    /**
     * Sets the clipped source/destination rectangle.
     *
     * @param placement_ Pixel coordinates produced by an ImagePlacementStrategy.
     */
    void setPlacement(const ImagePlacement& placement_);

    /**
     * Draws the selected image into destination indexed pixels.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

// Contract: feedback filter. The framework copies source to destination first;
// the selected Flame reads immutable source where needed and writes destination.
class FlameFilter : public FrameFilter {
    const Flame* flame;
    int generalFlame;
    FlameLookupTables lookupTables;

public:
    FlameFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /**
     * Selects the flame feedback implementation.
     *
     * @param flame_ Borrowed flame pointer, or NULL to skip flame execution.
     */
    void setFlame(const Flame* flame_);

    /**
     * Configures whether the flame runs in general-flame mode.
     *
     * @param generalFlame_ Nonzero when using the general flame path.
     */
    void setGeneralFlame(int generalFlame_);

    /**
     * Runs the selected flame against source/destination indexed pixels.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

// Contract: coordinate remap filter. The framework prepares immutable source
// and mutable destination before Translate remaps pixels.
class TranslateFilter : public FrameFilter {
    Translate translate;

public:
    TranslateFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /**
     * Replaces the coordinate translation table used by this stage.
     *
     * @param table Translation table in buffer-pixel coordinates.
     */
    void setTranslate(const TranslationTable& table);

    /**
     * Applies the configured coordinate remap to the frame buffer.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

// Contract: sound-reactive drawing filter. The framework copies source to
// destination first; this draws into destination pixels.
class WaveFilter : public FrameFilter {
    Wave* wave;
    WaveConfig config;
    WaveState state;
    WaveLookupTables lookupTables;
    RandomSource* randomSourceValue;
    int configured;
    int needsConfiguration;

public:
    WaveFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /**
     * Selects the wave renderer and its per-scene configuration.
     *
     * @param wave_ Borrowed wave pointer, or NULL to skip wave drawing.
     * @param config_ Wave configuration for scale/object/audio behavior.
     */
    void setWave(Wave* wave_, const WaveConfig& config_);

    /**
     * Supplies the random source used by wave renderers.
     *
     * @param randomSource Random source owned by the application lifecycle.
     */
    void setRandomSource(RandomSource& randomSource);

    /**
     * Draws the configured wave using the frame audio/time context.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

// Contract: indexed-buffer text injector. The framework copies source to
// destination first; this wraps CP437 text and draws it into destination.
class TextInjectionFilter : public FrameFilter {
    const BitmapFont* font;
    std::string message;
    int framesRemaining;
    int inkColor;
    int marginPixels;
    TextInjectionHorizontalAlign horizontalAlign;
    TextInjectionVerticalAlign verticalAlign;

public:
    TextInjectionFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /**
     * Arms or clears the text cue.
     *
     * @param message_ CP437-compatible text to draw; NULL clears the cue.
     * @param frameCount Number of visual frames to keep drawing the message.
     */
    void setMessage(const char* message_, int frameCount);

    /**
     * Selects the text color.
     *
     * @param color Palette index 0-255, or negative to choose a bright color
     *        from the current frame palette.
     */
    void setInkColor(int color);

    /**
     * Sets text alignment and safe inset.
     *
     * @param horizontalAlign_ Horizontal alignment within the available rectangle.
     * @param verticalAlign_ Vertical alignment within the available rectangle.
     * @param marginPixels_ Pixel inset from each buffer edge; clamped to zero.
     */
    void setPlacement(TextInjectionHorizontalAlign horizontalAlign_,
        TextInjectionVerticalAlign verticalAlign_, int marginPixels_);

    /**
     * Draws the armed text cue and decrements its remaining frame count.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

// Contract: frame boundary. Emits optional diagnostics; the framework then
// commits the finished indexed image so it becomes the display source.
class FrameCommitFilter : public FrameFilter {
    const char* flameName;
    const char* waveName;
    const char* waveScaleName;
    const char* tableName;
    int debugReports;

public:
    FrameCommitFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /**
     * Sets borrowed scene names for frame diagnostics.
     *
     * @param flameName_ Current flame display name, or NULL.
     * @param waveName_ Current wave display name, or NULL.
     * @param waveScaleName_ Current wave-scale display name, or NULL.
     * @param tableName_ Current translation-table display name, or NULL.
     */
    void setSceneNames(const char* flameName_, const char* waveName_,
        const char* waveScaleName_, const char* tableName_);

    /**
     * Reports diagnostics for destination indexed pixels before framework commit.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

// Contract: palette post-filter. Reads acoustic context and writes temporary
// flashlight output into the frame palette; it ignores indexed pixels.
class FlashlightFilter : public FrameFilter {
public:
    FlashlightFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /**
     * Applies the acoustic flashlight effect to the frame palette.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

// Contract: hidden-row writer. The framework copies source to destination
// first; this fills destination border rows used by flame feedback.
class BorderFilter : public FrameFilter {
    int borderMode;

public:
    BorderFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /**
     * Selects the hidden-border drawing mode.
     *
     * @param borderMode_ Border option value used by apply_border().
     */
    void setBorderMode(int borderMode_);

    /**
     * Writes hidden border rows into destination indexed storage.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

// Contract: palette transition filter. Advances the display-facing
// FramePalette toward the configured target; it ignores indexed pixels.
class PaletteFilter : public FrameFilter {
    PaletteTransition transition;
    FramePalette framePaletteValue;

public:
    PaletteFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /** @return Owned frame palette supplied to the filterchain. */
    FramePalette& framePalette();

    /**
     * @param palette Palette to compare against current target.
     * @return Nonzero when a new transition target is needed.
     */
    int needsTarget(const ColorPalette& palette) const;

    /**
     * Starts or replaces the target palette transition.
     *
     * @param palette Palette to transition toward.
     * @param frameBudget Number of visual frames over which to transition.
     * @param strategy Per-frame palette interpolation strategy.
     */
    void setTargetPalette(const ColorPalette& palette, int frameBudget,
        const PaletteTransitionStrategy& strategy);

    /**
     * Displays one palette for one palette-stage execution, then returns to a target.
     *
     * @param immediatePalette Palette shown on the next palette-stage execution.
     * @param targetPalette Palette to transition toward after the one-frame snap.
     * @param frameBudget Number of visual frames over which to transition back.
     * @param strategy Per-frame palette interpolation strategy.
     */
    void snapThenTransitionPalette(const ColorPalette& immediatePalette,
        const ColorPalette& targetPalette, int frameBudget,
        const PaletteTransitionStrategy& strategy);

    /**
     * Advances the active frame palette by one visual frame.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

// Contract: final display export. Publishes the committed indexed pixels and
// frame palette as a driver-facing frame descriptor.
class IndexedFrameFilter : public FrameFilter {
public:
    IndexedFrameFilter();

    /** @return Stable human-readable filter name for diagnostics. */
    virtual const char* name() const;

    virtual FrameFilterBufferContract bufferContract() const;

    /**
     * Publishes the committed source indexed pixels as the display-facing frame.
     *
     * @param frame Current frame filter wrapper.
     */
    void execute(FrameFilterFrame& frame);
};

/**
 * Extracts the frame palette installed in a filterchain.
 *
 * @param filterchain Filterchain configured by FrameFilterchainFactory.
 * @return FramePalette owned by PaletteFilter, or NULL when no palette stage exists.
 */
FramePalette* framePaletteFromFilterchain(FrameFilterchain& filterchain);

#endif
