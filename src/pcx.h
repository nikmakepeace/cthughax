#ifndef __PCX_H
#define __PCX_H

#include <stdio.h>

class EffectChoice;
class IndexedImage;
struct ImageLoadTarget;

/**
 * Loads an 8-bit indexed PCX image.
 *
 * @param file Open file positioned at the beginning of PCX data.
 * @param name Option/display name for the image.
 * @param dir Directory containing the image, for diagnostics.
 * @param totalName Full image path, for diagnostics.
 * @param target Target buffer dimensions in pixels.
 * @return Newly allocated indexed image, or NULL on unsupported/invalid input.
 */
IndexedImage* read_pcx_indexed_image(FILE* file, const char* name,
    const char* dir, const char* totalName, const ImageLoadTarget& target);

/**
 * Loads an 8-bit indexed PCX image into a legacy ImageEntry.
 *
 * @param file Open file positioned at the beginning of PCX data.
 * @param name Option/display name for the image.
 * @param dir Directory containing the image, for diagnostics.
 * @param totalName Full image path, for diagnostics.
 * @param target Target buffer dimensions in pixels.
 * @return Newly allocated ImageEntry, or NULL on unsupported/invalid input.
 */
EffectChoice* read_pcx_image(FILE* file, const char* name, const char* dir,
    const char* totalName, const ImageLoadTarget& target);

#endif
