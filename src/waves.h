/*
 *  sound-display (wave-functions)
 */
#ifndef __WAVES_H
#define __WAVES_H

#include "cthugha.h"
#include "WaveOptions.h"
#include "Wave.h"

class FrameRenderTarget;

/** Initializes palette color tables used by wave drawing. */
int init_tables();

/**
 * Initializes wave options, color tables, and optional 3D object files.
 *
 * @return Zero on completion.
 */
struct PathConfig;
struct EffectPolicy;
class LogSink;

int init_wave(const PathConfig& pathConfig, LogSink& log);

/** @return Currently selected 3D object line list, or NULL. */
WObject* currentWaveObject();

/** Palette remap table used by wave renderers: input intensity -> palette index. */
typedef unsigned char pal_table[256];

/** Built-in palette color tables for wave drawing. */
extern pal_table tables[];

/** Number of entries in tables. */
extern int nr_tables;

#endif
