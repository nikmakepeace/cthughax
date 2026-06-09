#include "cthugha.h"
#include "FramePalette.h"
#include "PaletteTransition.h"
#include "ProcessServices.h"

#include <math.h>

static int absoluteValue(int value) {
    return (value < 0) ? -value : value;
}

static double maxDouble(double lhs, double rhs) {
    return (lhs < rhs) ? rhs : lhs;
}

static double minDouble(double lhs, double rhs) {
    return (lhs < rhs) ? lhs : rhs;
}

static int paletteStepForDelta(int delta, double fraction) {
    int step = int(ceil(double(absoluteValue(delta)) * fraction));
    if (step < 1)
        step = 1;
    if (step > absoluteValue(delta))
        step = absoluteValue(delta);
    return (delta < 0) ? -step : step;
}

static void stepRgbPalette(ColorPalette& current, const ColorPalette& target,
    double fraction) {
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 3; j++) {
            int delta = target.component(i, j) - current.component(i, j);
            if (delta == 0)
                continue;

            current.setComponent(i, j,
                current.component(i, j) + paletteStepForDelta(delta, fraction));
        }
    }
}

static void stepLinearPalette(ColorPalette& current, const ColorPalette& target,
    int remainingFrames) {
    double fraction = 1.0 / double(remainingFrames);
    stepRgbPalette(current, target, fraction);
}

static void stepSquaredPalette(ColorPalette& current, const ColorPalette& target,
    int remainingFrames) {
    double fraction = 1.0 / double(remainingFrames);
    stepRgbPalette(current, target, fraction * fraction);
}

struct HslColor {
    double h;
    double s;
    double l;
};

static double hueToRgb(double p, double q, double t) {
    if (t < 0.0)
        t += 1.0;
    if (t > 1.0)
        t -= 1.0;
    if (t < 1.0 / 6.0)
        return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0)
        return q;
    if (t < 2.0 / 3.0)
        return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}

static HslColor rgbToHsl(int red, int green, int blue) {
    double r = double(red) / 255.0;
    double g = double(green) / 255.0;
    double b = double(blue) / 255.0;
    double maxValue = maxDouble(r, maxDouble(g, b));
    double minValue = minDouble(r, minDouble(g, b));
    HslColor hsl;

    hsl.h = 0.0;
    hsl.s = 0.0;
    hsl.l = (maxValue + minValue) / 2.0;

    if (maxValue == minValue)
        return hsl;

    double delta = maxValue - minValue;
    hsl.s = hsl.l > 0.5
        ? delta / (2.0 - maxValue - minValue)
        : delta / (maxValue + minValue);

    if (maxValue == r)
        hsl.h = (g - b) / delta + (g < b ? 6.0 : 0.0);
    else if (maxValue == g)
        hsl.h = (b - r) / delta + 2.0;
    else
        hsl.h = (r - g) / delta + 4.0;
    hsl.h /= 6.0;

    return hsl;
}

static void hslToRgb(const HslColor& hsl, int& red, int& green, int& blue) {
    double r, g, b;

    if (hsl.s == 0.0) {
        r = g = b = hsl.l;
    } else {
        double q = hsl.l < 0.5
            ? hsl.l * (1.0 + hsl.s)
            : hsl.l + hsl.s - hsl.l * hsl.s;
        double p = 2.0 * hsl.l - q;

        r = hueToRgb(p, q, hsl.h + 1.0 / 3.0);
        g = hueToRgb(p, q, hsl.h);
        b = hueToRgb(p, q, hsl.h - 1.0 / 3.0);
    }

    red = int(r * 255.0 + 0.5);
    green = int(g * 255.0 + 0.5);
    blue = int(b * 255.0 + 0.5);
}

static void stepHslPalette(ColorPalette& current, const ColorPalette& target,
    int remainingFrames) {
    double fraction = 1.0 / double(remainingFrames);

    for (int i = 0; i < 256; i++) {
        HslColor from = rgbToHsl(current.component(i, 0),
            current.component(i, 1), current.component(i, 2));
        HslColor to = rgbToHsl(target.component(i, 0),
            target.component(i, 1), target.component(i, 2));

        double hueDelta = to.h - from.h;
        if (hueDelta > 0.5)
            hueDelta -= 1.0;
        else if (hueDelta < -0.5)
            hueDelta += 1.0;

        HslColor next;
        next.h = from.h + hueDelta * fraction;
        if (next.h < 0.0)
            next.h += 1.0;
        if (next.h > 1.0)
            next.h -= 1.0;
        next.s = from.s + (to.s - from.s) * fraction;
        next.l = from.l + (to.l - from.l) * fraction;

        int red, green, blue;
        hslToRgb(next, red, green, blue);
        current.setColor(i, red, green, blue);
    }
}

PaletteTransitionStrategy::PaletteTransitionStrategy(const char* name, Function function)
    : nameValue(name)
    , functionValue(function) { }

const char* PaletteTransitionStrategy::name() const {
    return nameValue;
}

void PaletteTransitionStrategy::step(ColorPalette& current, const ColorPalette& target,
    int remainingFrames) const {
    if (functionValue != 0)
        (*functionValue)(current, target, remainingFrames);
}

static const PaletteTransitionStrategy paletteTransitionStrategies[] = {
    PaletteTransitionStrategy("linear", stepLinearPalette),
    PaletteTransitionStrategy("squared", stepSquaredPalette),
    PaletteTransitionStrategy("hsl", stepHslPalette),
};

static const int nPaletteTransitionStrategies
    = sizeof(paletteTransitionStrategies) / sizeof(PaletteTransitionStrategy);

const PaletteTransitionStrategy& linearPaletteTransitionStrategy() {
    return paletteTransitionStrategies[0];
}

const PaletteTransitionStrategy& squaredPaletteTransitionStrategy() {
    return paletteTransitionStrategies[1];
}

const PaletteTransitionStrategy& hslPaletteTransitionStrategy() {
    return paletteTransitionStrategies[2];
}

const PaletteTransitionStrategy& randomPaletteTransitionStrategy(
    RandomSource& randomSource) {
    return paletteTransitionStrategies[randomSource.uniformInt(nPaletteTransitionStrategies)];
}

PaletteTransition::PaletteTransition()
    : strategyValue(&linearPaletteTransitionStrategy())
    , hasTargetValue(0)
    , remainingFramesValue(0)
    , holdCurrentFrameValue(0) { }

int PaletteTransition::hasTarget(const ColorPalette& target) const {
    return hasTargetValue && targetPalette.equals(target);
}

void PaletteTransition::achieve(const ColorPalette& target, int frameBudget,
    const PaletteTransitionStrategy& strategy) {
    if (hasTarget(target))
        return;

    targetPalette.copyFrom(target);
    strategyValue = &strategy;
    remainingFramesValue = (frameBudget > 0) ? frameBudget : 0;
    holdCurrentFrameValue = 0;
    hasTargetValue = 1;

    CTH_DEBUG("palette transition target set strategy=%s frames=%d\n",
        strategyValue->name(), remainingFramesValue);
}

void PaletteTransition::snapThenAchieve(const ColorPalette& current,
    const ColorPalette& target, int frameBudget, const PaletteTransitionStrategy& strategy) {
    currentPalette.copyFrom(current);
    targetPalette.copyFrom(target);
    strategyValue = &strategy;
    remainingFramesValue = (frameBudget > 0) ? frameBudget : 0;
    holdCurrentFrameValue = 1;
    hasTargetValue = 1;

    CTH_DEBUG("palette transition snapped strategy=%s frames=%d\n",
        strategyValue->name(), remainingFramesValue);
}

void PaletteTransition::execute(FramePalette& framePalette) {
    if (!hasTargetValue) {
        framePalette.clearPaletteDirty();
        return;
    }

    if (holdCurrentFrameValue) {
        holdCurrentFrameValue = 0;
        framePalette.setPalette(currentPalette);
        return;
    }

    if (currentPalette.equals(targetPalette)) {
        framePalette.setPalette(currentPalette);
        return;
    }

    if (remainingFramesValue <= 1) {
        currentPalette.copyFrom(targetPalette);
        remainingFramesValue = 0;
    } else {
        strategyValue->step(currentPalette, targetPalette, remainingFramesValue);
        remainingFramesValue--;
    }

    framePalette.setPalette(currentPalette);
}
