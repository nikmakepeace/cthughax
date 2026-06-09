// Scene image catalog loading.

#ifndef CTHUGHA_SCENE_IMAGE_CATALOG_LOADER_H
#define CTHUGHA_SCENE_IMAGE_CATALOG_LOADER_H

class LogSink;
class SceneImageCatalog;
struct PathConfig;

/**
 * Loads built-in and file-backed indexed images into a Scene catalog.
 *
 * This is the native Scene image startup path. Legacy image-option loading may
 * still exist for non-Scene UI, but Scene construction does not copy from it.
 *
 * @param catalog Native catalog to replace with loaded entries.
 * @param pathConfig Startup path configuration.
 * @param fileImagesEnabled Nonzero to scan configured image directories.
 * @param targetWidth Target display/buffer width in pixels.
 * @param targetHeight Target display/buffer height in pixels.
 * @param log Diagnostics sink for loader progress.
 * @return Zero after scanning configured paths.
 */
int loadSceneImageCatalog(SceneImageCatalog& catalog,
    const PathConfig& pathConfig, int fileImagesEnabled, int targetWidth,
    int targetHeight, LogSink& log);

#endif
