// Scene wave-object catalog loading.

#ifndef CTHUGHA_SCENE_WAVE_OBJECT_CATALOG_LOADER_H
#define CTHUGHA_SCENE_WAVE_OBJECT_CATALOG_LOADER_H

struct PathConfig;
class LogSink;
class SceneWaveObjectCatalog;

/**
 * Loads built-in and file-backed wave objects into a Scene catalog.
 *
 * This is the native Scene wave-object startup path. The compatibility object
 * EffectControl list may still exist for non-Scene UI, but Scene construction
 * does not need to copy from it.
 *
 * @param catalog Native catalog to replace with loaded entries.
 * @param pathConfig Startup path configuration.
 * @param fileObjectsEnabled Nonzero to scan configured object directories.
 * @param log Diagnostics sink for loader progress and file errors.
 * @return Zero after scanning configured paths.
 */
int loadSceneWaveObjectCatalog(SceneWaveObjectCatalog& catalog,
    const PathConfig& pathConfig, int fileObjectsEnabled, LogSink& log);

#endif
