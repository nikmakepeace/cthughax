/** @file
 * SDL3 display presentation helpers that do not depend on SDL headers.
 */

#ifndef CTHUGHA_SDL3_PRESENTATION_H
#define CTHUGHA_SDL3_PRESENTATION_H

#include "DisplayGeometry.h"
#include "OverlaySource.h"

class ColorPalette;

/** One RGBA color in the byte order used by SDL_PIXELFORMAT_RGBA32 uploads. */
class Sdl3RgbaColor {
public:
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;

    /** Creates an opaque black color. */
    Sdl3RgbaColor();

    /** Creates an explicit RGBA color. */
    Sdl3RgbaColor(unsigned char red_, unsigned char green_,
        unsigned char blue_, unsigned char alpha_);
};

/**
 * Converts a Cthugha 256-color RGB palette to SDL upload colors.
 *
 * @param palette Source indexed palette.
 * @param colors Destination color table.
 * @param colorCount Number of destination entries available.
 */
void sdl3PopulateRgbaPalette(const ColorPalette& palette,
    Sdl3RgbaColor* colors, int colorCount);

/**
 * Expands indexed pixels to tightly packed RGBA bytes for SDL_UpdateTexture().
 *
 * @param indexed Source indexed pixels.
 * @param frameSize Visible source size.
 * @param indexedPitch Bytes from one indexed row to the next.
 * @param palette 256-entry RGBA palette.
 * @param rgba Destination RGBA bytes.
 * @param rgbaPitch Bytes from one RGBA row to the next.
 */
void sdl3ExpandIndexedPixelsToRgba(const unsigned char* indexed,
    const PixelSize& frameSize, int indexedPitch,
    const Sdl3RgbaColor* palette, unsigned char* rgba, int rgbaPitch);

/**
 * Computes the left pixel for one overlay text line.
 *
 * @param textLength Character count for the line.
 * @param justification Overlay justification: 'l', 'c', or 'r'.
 * @param layout Current text-cell layout.
 */
int sdl3OverlayTextX(int textLength, int justification,
    const OverlayLayout& layout);

/**
 * Computes the top pixel for an overlay text command.
 *
 * Negative y values are relative to the bottom row, matching DisplayDevice.
 */
int sdl3OverlayTextY(double y, const OverlayLayout& layout);

#endif
