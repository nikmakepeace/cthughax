/** @file
 * Testable SDL3 presentation data conversion helpers.
 */

#include "Sdl3Presentation.h"

#include "ColorPalette.h"

#include <algorithm>

Sdl3RgbaColor::Sdl3RgbaColor()
    : red(0)
    , green(0)
    , blue(0)
    , alpha(255) {
}

Sdl3RgbaColor::Sdl3RgbaColor(unsigned char red_, unsigned char green_,
    unsigned char blue_, unsigned char alpha_)
    : red(red_)
    , green(green_)
    , blue(blue_)
    , alpha(alpha_) {
}

void sdl3PopulateRgbaPalette(const ColorPalette& palette,
    Sdl3RgbaColor* colors, int colorCount) {
    if (colors == 0 || colorCount <= 0)
        return;

    int count = std::min(colorCount, 256);
    for (int i = 0; i < count; ++i) {
        colors[i] = Sdl3RgbaColor(palette.component(i, 0),
            palette.component(i, 1), palette.component(i, 2), 255);
    }
}

void sdl3ExpandIndexedPixelsToRgba(const unsigned char* indexed,
    const PixelSize& frameSize, int indexedPitch,
    const Sdl3RgbaColor* palette, unsigned char* rgba, int rgbaPitch) {
    if (indexed == 0 || palette == 0 || rgba == 0 || !frameSize.valid()
        || indexedPitch < frameSize.width || rgbaPitch < frameSize.width * 4)
        return;

    for (int y = 0; y < frameSize.height; ++y) {
        const unsigned char* source = indexed + y * indexedPitch;
        unsigned char* destination = rgba + y * rgbaPitch;
        for (int x = 0; x < frameSize.width; ++x) {
            const Sdl3RgbaColor& color = palette[source[x]];
            destination[x * 4 + 0] = color.red;
            destination[x * 4 + 1] = color.green;
            destination[x * 4 + 2] = color.blue;
            destination[x * 4 + 3] = color.alpha;
        }
    }
}

int sdl3OverlayTextX(int textLength, int justification,
    const OverlayLayout& layout) {
    int length = std::max(textLength, 0);
    int columns = std::max(layout.columns, 0);
    int fontWidth = std::max(layout.fontWidth, 0);

    switch (justification) {
    case 'c':
        return std::max(fontWidth * (columns - length) / 2, 0);
    case 'r':
        return std::max(fontWidth * (columns - length), 0);
    case 'l':
    default:
        return 0;
    }
}

int sdl3OverlayTextY(double y, const OverlayLayout& layout) {
    int fontHeight = std::max(layout.fontHeight, 0);
    int rows = std::max(layout.rows, 0);
    return (y >= 0.0) ? int(y * fontHeight) : int(fontHeight * (rows + y));
}
