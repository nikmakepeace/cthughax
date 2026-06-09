// Scene palette catalog loading.

#ifndef CTHUGHA_SCENE_PALETTE_CATALOG_LOADER_H
#define CTHUGHA_SCENE_PALETTE_CATALOG_LOADER_H

class LogSink;
class ScenePaletteCatalog;
struct PathConfig;

/**
 * Loads built-in and file-backed palettes into a Scene catalog.
 *
 * This is the native Scene palette startup path. Legacy palette-option loading
 * may still exist for non-Scene UI, but Scene construction does not copy from
 * it.
 *
 * @param catalog Native catalog to replace with loaded entries.
 * @param pathConfig Startup path configuration.
 * @param paletteSetFilterText Optional explicit palette set filter text.
 * @param log Diagnostics sink for loader progress.
 * @return Zero after scanning configured paths.
 */
int loadScenePaletteCatalog(ScenePaletteCatalog& catalog,
    const PathConfig& pathConfig, const char* paletteSetFilterText,
    LogSink& log);

#endif
