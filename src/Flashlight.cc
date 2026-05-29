#include "cthugha.h"
#include "Flashlight.h"
#include "CthughaBuffer.h"
#include "VisualPipeline.h"
#include "display.h"
#include "imath.h"

static CoreOptionEntry* flashlight_entries[] = { new OffEntry(), new OnEntry() };
static CoreOptionEntryList flashlightEntries;

CoreOption flashlight(0, "flashlight", flashlightEntries);

void init_flashlight() {
    flashlight.add(flashlight_entries, 2);
}

void apply_flashlight(CthughaBuffer& buffer, const VisualFrameContext& context) {
    if (context.acousticContext == 0)
        return;

    int i, j, l;
    static Palette pal;

    // Brighten the palette currently represented by this buffer.
    memcpy(pal, buffer.currentPalette, sizeof(Palette));

    for (l = context.acousticContext->fire() << 3, i = 0; (i < 256) && (l > 0); i++, l -= 8)
        for (j = 0; j < 3; j++)
            pal[i][j] = min(pal[i][j] + l, 255);

    buffer.setPalette(pal);
}
