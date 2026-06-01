#include "cthugha.h"
#include "Flashlight.h"
#include "FramePalette.h"
#include "VideoPipeline.h"
#include "display.h"
#include "imath.h"

static CoreOptionEntry* flashlight_entries[] = { new OffEntry(), new OnEntry() };
static CoreOptionEntryList flashlightEntries;

CoreOption flashlight(0, "flashlight", flashlightEntries);

void init_flashlight() {
    flashlight.add(flashlight_entries, 2);
}

void apply_flashlight(FramePalette& framePalette, const VideoFrameContext& context) {
    if (context.acousticContext == 0)
        return;

    int i, j, l;
    ColorPalette pal = framePalette.currentPalette();

    // Brighten the palette currently represented by this frame.
    for (l = context.acousticContext->fire() << 3, i = 0; (i < 256) && (l > 0); i++, l -= 8)
        for (j = 0; j < 3; j++)
            pal.setComponent(i, j, pal.component(i, j) + l);

    framePalette.setPalette(pal);
}
