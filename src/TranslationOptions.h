// Translation option/catalog layer.

#ifndef __TRANSLATION_OPTIONS_H__
#define __TRANSLATION_OPTIONS_H__

#include "TranslationOption.h"

class RandomSource;
class SceneTranslationCatalog;
class SceneGeometry;

/**
 * Generates and registers translation tables for a buffer size.
 *
 * @param geometry Frame geometry whose width/height define table dimensions.
 * @param randomSource Seed source for generated translation tables that request
 *        per-run randomization.
 * @return Zero on completion.
 */
int init_translate(const SceneGeometry& geometry, RandomSource& randomSource);

/**
 * Registers generated translation entries from a native Scene catalog into the
 * temporary legacy TranslateOption mirror.
 *
 * @param catalog Native Scene-owned translation catalog.
 * @return Zero on completion.
 */
int init_translate(const SceneTranslationCatalog& catalog);

#endif
