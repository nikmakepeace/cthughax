/** @file
 * Random palette generation independent of palette catalog ownership.
 */

#ifndef CTHUGHA_PALETTE_RANDOM_GENERATOR_H
#define CTHUGHA_PALETTE_RANDOM_GENERATOR_H

class ColorPalette;
class RandomSource;

/**
 * Fills a palette with the legacy randomized control-point interpolation.
 *
 * @param palette Palette to overwrite.
 * @param randomSource Random source used for control-point count and colors.
 */
void generateRandomPalette(ColorPalette& palette, RandomSource& randomSource);

#endif
