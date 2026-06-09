/** @file
 * Random palette generation.
 */

#include "PaletteRandomGenerator.h"

#include "ColorPalette.h"
#include "ProcessServices.h"

void generateRandomPalette(ColorPalette& palette, RandomSource& randomSource) {
    int controlPoints = 1 << (1 + randomSource.uniformInt(3));
    int step = 256 / controlPoints;

    char generated[257][3]; // Extra final control point preserves legacy interpolation.

    for (int channel = 0; channel < 3; channel++) {
        for (int i = 0; i <= controlPoints; i++)
            generated[i * step][channel] = randomSource.uniformInt(256);

        for (int i = 0; i < controlPoints; i++) {
            for (int j = 0; j < step; j++) {
                double fraction = double(j) / double(step);
                generated[i * step + j][channel] = int(
                    (1.0 - fraction) * generated[i * step][channel]
                    + fraction * generated[(i + 1) * step][channel]);
            }
        }

        for (int i = 0; i < 256; i++)
            palette.setComponent(i, channel, generated[i][channel]);
    }
}
