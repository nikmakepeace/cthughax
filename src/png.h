#ifndef __PNG_H
#define __PNG_H

#include <stdio.h>

class EffectChoice;
class IndexedImage;
struct ImageLoadTarget;

/**
 * Loads an indexed PNG image.
 *
 * Only color type 3 indexed PNG files are supported. Pixel data is expanded to
 * one 8-bit palette index per pixel, and the PNG palette is stored on the image.
 *
 * @param file Open file positioned at the PNG signature.
 * @param name Option/display name for the image.
 * @param dir Directory containing the image, for diagnostics.
 * @param totalName Full image path, for diagnostics.
 * @param target Target buffer dimensions in pixels.
 * @return Newly allocated indexed image, or NULL on unsupported/invalid input.
 */
IndexedImage* read_png_indexed_image(FILE* file, const char* name,
    const char* dir, const char* totalName, const ImageLoadTarget& target);

/**
 * Loads an indexed PNG image into a legacy ImageEntry.
 *
 * Only color type 3 indexed PNG files are supported. Pixel data is expanded to
 * one 8-bit palette index per pixel, and the PNG palette is stored on the entry.
 *
 * @param file Open file positioned at the PNG signature.
 * @param name Option/display name for the image.
 * @param dir Directory containing the image, for diagnostics.
 * @param totalName Full image path, for diagnostics.
 * @param target Target buffer dimensions in pixels.
 * @return Newly allocated ImageEntry, or NULL on unsupported/invalid input.
 */
EffectChoice* read_png_image(FILE* file, const char* name, const char* dir,
    const char* totalName, const ImageLoadTarget& target);

#endif
