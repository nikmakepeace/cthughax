/** @file
 * Unit coverage for SDL3 presentation conversion helpers.
 */

#include "Sdl3Presentation.h"

#include "ColorPalette.h"

#include <assert.h>
#include <string.h>

static void populateRgbaPaletteCopiesRgbComponents() {
    ColorPalette palette;
    palette.setColor(0, 1, 2, 3);
    palette.setColor(7, 70, 80, 90);

    Sdl3RgbaColor colors[8];
    memset(colors, 0, sizeof(colors));

    sdl3PopulateRgbaPalette(palette, colors, 8);

    assert(colors[0].red == 1);
    assert(colors[0].green == 2);
    assert(colors[0].blue == 3);
    assert(colors[0].alpha == 255);
    assert(colors[7].red == 70);
    assert(colors[7].green == 80);
    assert(colors[7].blue == 90);
    assert(colors[7].alpha == 255);
}

static void expandIndexedPixelsHonorsSourceAndDestinationPitch() {
    Sdl3RgbaColor palette[256];
    palette[1] = Sdl3RgbaColor(10, 20, 30, 255);
    palette[2] = Sdl3RgbaColor(40, 50, 60, 255);
    palette[3] = Sdl3RgbaColor(70, 80, 90, 255);
    palette[4] = Sdl3RgbaColor(100, 110, 120, 255);

    const unsigned char indexed[] = {
        1, 2, 99,
        3, 4, 99
    };
    unsigned char rgba[2 * 10];
    memset(rgba, 0xEE, sizeof(rgba));

    sdl3ExpandIndexedPixelsToRgba(indexed, PixelSize(2, 2), 3, palette, rgba,
        10);

    assert(rgba[0] == 10);
    assert(rgba[1] == 20);
    assert(rgba[2] == 30);
    assert(rgba[3] == 255);
    assert(rgba[4] == 40);
    assert(rgba[5] == 50);
    assert(rgba[6] == 60);
    assert(rgba[7] == 255);
    assert(rgba[8] == 0xEE);
    assert(rgba[10] == 70);
    assert(rgba[11] == 80);
    assert(rgba[12] == 90);
    assert(rgba[13] == 255);
    assert(rgba[14] == 100);
    assert(rgba[15] == 110);
    assert(rgba[16] == 120);
    assert(rgba[17] == 255);
    assert(rgba[18] == 0xEE);
}

static void overlayPlacementMatchesDisplayDeviceTextRules() {
    OverlayLayout layout(40, 12, 9, 14);

    assert(sdl3OverlayTextX(5, 'l', layout) == 0);
    assert(sdl3OverlayTextX(10, 'c', layout) == 135);
    assert(sdl3OverlayTextX(10, 'r', layout) == 270);
    assert(sdl3OverlayTextX(80, 'r', layout) == 0);
    assert(sdl3OverlayTextY(2.0, layout) == 28);
    assert(sdl3OverlayTextY(-1.0, layout) == 154);
}

int main() {
    populateRgbaPaletteCopiesRgbComponents();
    expandIndexedPixelsHonorsSourceAndDestinationPitch();
    overlayPlacementMatchesDisplayDeviceTextRules();
    return 0;
}
