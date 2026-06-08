#ifndef __FRAME_PALETTE_H
#define __FRAME_PALETTE_H

#include "ColorPalette.h"

/**
 * Palette state published with an indexed frame.
 *
 * Display backends can use paletteDirty() to avoid uploading unchanged palette
 * data every frame.
 */
class FramePalette {
    ColorPalette currentPaletteValue;
    int paletteDirtyValue;

public:
    FramePalette();

    /** @return Current 256-color RGB palette. */
    const ColorPalette& currentPalette() const;

    /** @return Nonzero when currentPalette() changed since the dirty flag was cleared. */
    int paletteDirty() const;

    /** Marks the current palette as acknowledged by the display backend. */
    void clearPaletteDirty();

    /**
     * Replaces the current palette when it actually differs.
     *
     * @param palette New 256-color RGB palette.
     */
    void setPalette(const ColorPalette& palette);
};

#endif
