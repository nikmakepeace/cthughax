// Filesystem/catalog loader for Effect choices.

#ifndef __EFFECT_CHOICE_LOADER_H
#define __EFFECT_CHOICE_LOADER_H

#include "EffectControl.h"

struct PathConfig;

#include <stdio.h>

typedef EffectChoice* (*EffectChoiceLoader)(FILE*, const char*, const char*, const char*);
typedef EffectChoice* (*EffectChoiceContextLoader)(
    FILE*, const char*, const char*, const char*, void*);

/**
 * Loads matching files from search paths and appends successful choices.
 *
 * @param option Effect control to receive loaded choices.
 * @param searchPath Null-terminated list of directories.
 * @param extraPath Subdirectory appended to the command-line extra library path.
 * @param extension Extension to match, such as ".map" or ".png".
 * @param loader File parser returning a new EffectChoice, or NULL to skip.
 * @return Zero after scanning configured paths.
 */
int loadEffectChoices(EffectControl& option, const PathConfig& pathConfig,
    const char* searchPath[], const char* extraPath, const char* extension,
    EffectChoiceLoader loader);

/**
 * Context-aware variant of loadEffectChoices().
 *
 * @param context Borrowed parser context passed through to loader.
 */
int loadEffectChoices(EffectControl& option, const PathConfig& pathConfig,
    const char* searchPath[], const char* extraPath, const char* extension,
    EffectChoiceContextLoader loader, void* context);

#endif
