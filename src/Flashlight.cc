// Palette flashlight option and acoustic palette brightening.

#include "cthugha.h"
#include "Flashlight.h"
#include "FramePalette.h"
#include "FrameGeneratorContext.h"
#include "display.h"
#include "imath.h"

static EffectChoice* flashlight_entries[] = { new OffEntry(), new OnEntry() };
static EffectChoiceList flashlightEntries;

EffectControl flashlight(0, "flashlight", flashlightEntries, EFFECT_CONTROL_AUTO_CHANGE);

void init_flashlight() {
    flashlight.add(flashlight_entries, 2);
}

void apply_flashlight(FramePalette& framePalette, const FrameGeneratorContext& context) {
    int fire = context.audioAnalysis().fire();
    if (fire <= 0)
        return;

    int i, j, l;
    ColorPalette pal = framePalette.currentPalette();

    // Brighten the palette currently represented by this frame.
    for (l = fire << 3, i = 0; (i < 256) && (l > 0); i++, l -= 8)
        for (j = 0; j < 3; j++)
            pal.setComponent(i, j, pal.component(i, j) + l);

    framePalette.setPalette(pal);
}
