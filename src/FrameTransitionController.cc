// Frame Generator transition and duration policy.

#include "FrameTransitionController.h"

#include "ProcessServices.h"
#include "imath.h"

FrameTransitionController::FrameTransitionController()
    : paletteSmoothingChanceValue(0.0)
    , paletteSmoothSecondsValue(1)
    , quietMessageDurationMsValue(0) { }

void FrameTransitionController::configureTransitions(
    double paletteSmoothingChance, int paletteSmoothSeconds) {
    paletteSmoothingChanceValue = paletteSmoothingChance;
    paletteSmoothSecondsValue = paletteSmoothSeconds;
}

void FrameTransitionController::configureQuietMessages(
    int quietMessageDurationMs) {
    quietMessageDurationMsValue = quietMessageDurationMs;
}

int FrameTransitionController::paletteSmoothingFrameBudget(
    int frameBudgetFramesPerSecond) const {
    int fps = max(frameBudgetFramesPerSecond, 1);

    return max(fps * paletteSmoothSecondsValue, 1);
}

int FrameTransitionController::shouldSmoothPaletteChange(
    RandomSource& randomSource) const {
    if (paletteSmoothingChanceValue <= 0.0)
        return 0;

    static const int scale = 1000000;
    if ((double(randomSource.uniformInt(scale)) / double(scale))
        >= paletteSmoothingChanceValue)
        return 0;

    return 1;
}

int FrameTransitionController::paletteChangeFrameBudget(
    RandomSource& randomSource, int frameBudgetFramesPerSecond) const {
    return shouldSmoothPaletteChange(randomSource)
        ? paletteSmoothingFrameBudget(frameBudgetFramesPerSecond)
        : 0;
}

FrameImageCuePaletteMode FrameTransitionController::chooseImageCuePaletteMode(
    RandomSource& randomSource, int frameBudgetFramesPerSecond) const {
    if (shouldSmoothPaletteChange(randomSource))
        return FrameImageCuePaletteSnapThenReturn;

    (void)frameBudgetFramesPerSecond;
    return (randomSource.uniformInt(2) == 0)
        ? FrameImageCuePaletteAdopt
        : FrameImageCuePaletteIgnore;
}

int FrameTransitionController::quietMessageFrameBudget(
    int frameBudgetFramesPerSecond, int quietMessageThresholdMs) const {
    int fps = max(frameBudgetFramesPerSecond, 1);
    int durationMs = (quietMessageDurationMsValue > quietMessageThresholdMs)
        ? quietMessageThresholdMs
        : quietMessageDurationMsValue;

    return max(1, fps * (durationMs / 1000));
}
