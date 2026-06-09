// Legacy palette option declaration.

#ifndef CTHUGHA_PALETTE_OPTION_H
#define CTHUGHA_PALETTE_OPTION_H

#include "EffectControl.h"
#include "PaletteEntry.h"

extern EffectChoiceList paletteEntries;

/**
 * Legacy EffectControl-backed palette selector.
 *
 * This header is the narrow compatibility surface for code that still needs
 * the global palette option without depending on the broader Display API.
 */
class PaletteOption : public EffectControl {
public:
    /** Creates the palette option over the global palette entry list. */
    PaletteOption();

    /** @return Currently selected mutable palette entry, or NULL. */
    PaletteEntry* currentPaletteEntry();

    /** @return Currently selected color palette, or NULL. */
    const ColorPalette* currentPalette();
};

extern PaletteOption palette;

#endif
