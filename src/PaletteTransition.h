/** @file
 * Palette transition strategies and per-frame transition state.
 */

#ifndef __PALETTE_TRANSITION_H
#define __PALETTE_TRANSITION_H

#include "ColorPalette.h"

class FramePalette;
class RandomSource;

/**
 * Named algorithm for stepping one palette toward another.
 */
class PaletteTransitionStrategy {
public:
    /** Palette transition stepping function signature. */
    typedef void (*Function)(ColorPalette& current, const ColorPalette& target,
        int remainingFrames);

private:
    const char* nameValue;
    Function functionValue;

public:
    /**
     * Creates a named palette stepping strategy.
     *
     * @param name Stable strategy name for diagnostics.
     * @param function Per-frame stepping implementation.
     */
    PaletteTransitionStrategy(const char* name, Function function);

    /** @return Stable strategy name. */
    const char* name() const;

    /**
     * Advances a current palette toward a target palette.
     *
     * @param current Mutable palette being stepped.
     * @param target Palette to converge on.
     * @param remainingFrames Frames remaining in the transition.
     */
    void step(ColorPalette& current, const ColorPalette& target, int remainingFrames) const;
};

/**
 * Stateful palette transition executor for one frame palette.
 */
class PaletteTransition {
    ColorPalette currentPalette;
    ColorPalette targetPalette;
    const PaletteTransitionStrategy* strategyValue;
    int hasTargetValue;
    int remainingFramesValue;
    int holdCurrentFrameValue;

public:
    /** Creates an idle transition using the linear strategy by default. */
    PaletteTransition();

    /**
     * Checks whether the current transition target already matches.
     *
     * @param target Palette to compare with the current target.
     * @return Nonzero when this transition is already targeting that palette.
     */
    int hasTarget(const ColorPalette& target) const;

    /**
     * Transitions from the current internal palette toward a target palette.
     *
     * @param target Palette to converge on.
     * @param frameBudget Number of visual frames over which to transition.
     * @param strategy Per-frame palette interpolation strategy.
     */
    void achieve(const ColorPalette& target, int frameBudget,
        const PaletteTransitionStrategy& strategy);

    /**
     * Snaps to one palette for the next execution, then transitions to a target.
     *
     * @param current Palette to publish on the next execute() call.
     * @param target Palette to converge on after the snap frame.
     * @param frameBudget Number of visual frames over which to transition.
     * @param strategy Per-frame palette interpolation strategy.
     */
    void snapThenAchieve(const ColorPalette& current, const ColorPalette& target,
        int frameBudget, const PaletteTransitionStrategy& strategy);

    /**
     * Advances and publishes the transition state.
     *
     * @param framePalette Display-facing frame palette to update.
     */
    void execute(FramePalette& framePalette);
};

/** @return Linear RGB transition strategy. */
const PaletteTransitionStrategy& linearPaletteTransitionStrategy();

/** @return Squared RGB transition strategy. */
const PaletteTransitionStrategy& squaredPaletteTransitionStrategy();

/** @return Hue/saturation/lightness transition strategy. */
const PaletteTransitionStrategy& hslPaletteTransitionStrategy();

/**
 * Selects a palette transition strategy using an injected random source.
 *
 * @param randomSource Random source to select the strategy.
 * @return One of the built-in palette transition strategies.
 */
const PaletteTransitionStrategy& randomPaletteTransitionStrategy(
    RandomSource& randomSource);

#endif
