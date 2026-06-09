// Explicit collaborators used by SceneCommands while legacy catalogs migrate.

#ifndef CTHUGHA_SCENE_DEPENDENCIES_H
#define CTHUGHA_SCENE_DEPENDENCIES_H

#include "Scene.h"
#include "SceneGeometry.h"

class IndexedImage;
class RandomSource;
struct SceneConfig;

/**
 * Registry operations over scene-editable effect controls.
 *
 * This keeps SceneCommands independent of the EffectControl-backed registry
 * while the legacy control model remains in use.
 */
class SceneEffectRegistry {
public:
    virtual ~SceneEffectRegistry();
    virtual void saveAll() = 0;
    virtual void restoreAll() = 0;
    virtual void changeAll(RandomSource& randomSource) = 0;
    virtual void changeOne(RandomSource& randomSource) = 0;
};

/**
 * Preset operations over scene-editable effect values.
 */
class ScenePresetCatalog {
public:
    virtual ~ScenePresetCatalog();
    virtual void restore(int slot) = 0;
    virtual void save(int slot) = 0;
};

/**
 * Scene-facing visual catalog and selection port.
 *
 * SceneCommands mutates this port, while adapters decide how those changes map
 * to legacy option catalogs during the migration to scene-owned selection state.
 */
class SceneVisualCatalogs {
public:
    virtual ~SceneVisualCatalogs();

    virtual const SceneSettings& currentSettings(SceneGeometry& geometry) = 0;
    virtual const IndexedImage* currentImage() = 0;

    virtual void applyStartupConfig(
        const SceneConfig& config, RandomSource& randomSource) = 0;
    virtual unsigned int change(
        SceneSelectionTarget target, int by, RandomSource& randomSource) = 0;
    virtual unsigned int change(SceneSelectionTarget target, const char* to,
        RandomSource& randomSource) = 0;

    /**
     * Activates one indexed choice for a Scene visual selection.
     *
     * @return SceneChange flags forced by the activation.
     */
    virtual unsigned int activate(SceneSelectionTarget target, int index) = 0;

    /** Toggles the lock for a Scene visual selection. */
    virtual void toggleLock(SceneSelectionTarget target) = 0;

    /** Toggles use for one indexed choice on a Scene visual selection. */
    virtual void toggleChoiceUse(SceneSelectionTarget target, int index) = 0;

    virtual unsigned int randomPalette(RandomSource& randomSource) = 0;
    virtual unsigned int addRandomPalette(RandomSource& randomSource) = 0;
};

#endif
