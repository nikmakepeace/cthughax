#ifndef __WAVE_OBJECT_H
#define __WAVE_OBJECT_H

#include "Wave.h"

#include <stdio.h>

class EffectChoice;

extern EffectChoice* _objects[];
extern int _nObjects;

/** @return Built-in Big H object line list used when no file objects load. */
const WObject* builtInWaveObjectBigH();

/**
 * Reads an object file into an owned, axis-normalized WObject line list.
 *
 * @param file Open object file positioned at the beginning.
 * @param name Object name used in diagnostics.
 * @return Newly allocated terminated WObject line list, or NULL on failure.
 */
WObject* read_wave_object(FILE* file, const char* name);

EffectChoice* read_object(FILE* file, const char* name, const char* dir, const char* total_name);
WObject* waveObjectEntryObject(EffectChoice* entry);
int waveObjectEntryOwnsObject(const EffectChoice* entry);

#endif
