// Frame Generator transition and duration policy.

#ifndef CTHUGHA_FRAME_TRANSITION_CONTROLLER_H
#define CTHUGHA_FRAME_TRANSITION_CONTROLLER_H

class RandomSource;

enum FrameImageCuePaletteMode {
    FrameImageCuePaletteSnapThenReturn,
    FrameImageCuePaletteAdopt,
    FrameImageCuePaletteIgnore
};

/**
 * Owns frame-generation transition settings and duration calculations.
 *
 * Palette smoothing, image-cue palette behavior, and quiet-message durations
 * are stateful runtime policy. Keeping them here removes the historical
 * file-scope statics from the frame generation path.
 */
class FrameTransitionController {
    double paletteSmoothingChanceValue;
    int paletteSmoothSecondsValue;
    int quietMessageDurationMsValue;

    int shouldSmoothPaletteChange(RandomSource& randomSource) const;

public:
    /** Creates transition policy with startup defaults. */
    FrameTransitionController();

    /**
     * Applies startup palette-transition configuration.
     *
     * @param paletteSmoothingChance Probability that palette changes smooth.
     * @param paletteSmoothSeconds Duration of smoothed palette changes.
     */
    void configureTransitions(double paletteSmoothingChance,
        int paletteSmoothSeconds);

    /**
     * Applies startup quiet-message configuration.
     *
     * @param quietMessageDurationMs Maximum quiet-message display duration.
     */
    void configureQuietMessages(int quietMessageDurationMs);

    /**
     * Calculates the palette smoothing duration for a frame budget.
     *
     * @param frameBudgetFramesPerSecond Explicit frames-per-second budget.
     * @return At least one frame.
     */
    int paletteSmoothingFrameBudget(int frameBudgetFramesPerSecond) const;

    /**
     * Chooses a palette transition duration for a scene palette change.
     *
     * @param randomSource Random source used for smoothing probability.
     * @param frameBudgetFramesPerSecond Explicit frames-per-second budget.
     * @return Frame count, or zero to snap immediately.
     */
    int paletteChangeFrameBudget(RandomSource& randomSource,
        int frameBudgetFramesPerSecond) const;

    /**
     * Chooses image-cue palette behavior for one injected image.
     *
     * @param randomSource Random source used for policy variation.
     * @param frameBudgetFramesPerSecond Explicit frames-per-second budget.
     * @return Palette behavior for the cue.
     */
    FrameImageCuePaletteMode chooseImageCuePaletteMode(RandomSource& randomSource,
        int frameBudgetFramesPerSecond) const;

    /**
     * Converts quiet-message duration settings into frames.
     *
     * @param frameBudgetFramesPerSecond Explicit frames-per-second budget.
     * @param quietMessageThresholdMs Runtime quiet-message threshold.
     * @return At least one frame.
     */
    int quietMessageFrameBudget(int frameBudgetFramesPerSecond,
        int quietMessageThresholdMs) const;
};

#endif
