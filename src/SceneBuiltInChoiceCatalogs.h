// Native Scene choice catalogs for fixed built-in visual selections.

#ifndef CTHUGHA_SCENE_BUILT_IN_CHOICE_CATALOGS_H
#define CTHUGHA_SCENE_BUILT_IN_CHOICE_CATALOGS_H

class SceneChoiceCatalog;
class SceneChoiceLock;

/**
 * Returns the native default availability for a built-in flame choice.
 *
 * The clear flame at index zero is selectable directly but excluded from
 * random/cyclic changes by default, matching the historical catalog without
 * reading legacy option-entry state.
 *
 * @param index Built-in flame catalog index.
 * @return Nonzero when the flame should be enabled by default.
 */
int sceneBuiltInFlameChoiceInUse(int index);

/**
 * Returns the native default availability for a built-in wave choice.
 *
 * Diagnostic/no-op wave entries at the end of the catalog are selectable
 * directly but excluded from random/cyclic changes by default, matching the
 * historical catalog without reading legacy option-entry state.
 *
 * @param index Built-in wave catalog index.
 * @return Nonzero when the wave should be enabled by default.
 */
int sceneBuiltInWaveChoiceInUse(int index);

/**
 * Creates the built-in flame Scene choice catalog.
 *
 * The returned catalog owns its choices and the supplied lock. Choice payloads
 * point at the native built-in Flame catalog.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @return Owned catalog with all built-in flame choices.
 */
SceneChoiceCatalog* createSceneFlameChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock);

/**
 * Creates the built-in wave Scene choice catalog.
 *
 * The returned catalog owns its choices and the supplied lock. Choice payloads
 * point at the native built-in Wave catalog.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @return Owned catalog with all built-in wave choices.
 */
SceneChoiceCatalog* createSceneWaveChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock);

/**
 * Creates the fixed wave-scale Scene choice catalog.
 *
 * The returned catalog owns its choices and the supplied lock.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @return Owned catalog with scale0, scale1, and scale2 choices.
 */
SceneChoiceCatalog* createSceneWaveScaleChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock);

/**
 * Creates the fixed table Scene choice catalog.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @return Owned catalog with table0 through table9 choices.
 */
SceneChoiceCatalog* createSceneTableChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock);

/**
 * Creates the fixed border Scene choice catalog.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @return Owned catalog with border0 through border3 choices.
 */
SceneChoiceCatalog* createSceneBorderChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock);

/**
 * Creates the fixed flashlight Scene choice catalog.
 *
 * @param catalogName Stable Scene catalog name.
 * @param lock Owned lock state for this selection.
 * @return Owned catalog with off/on choices and yes/no/0/1 aliases.
 */
SceneChoiceCatalog* createSceneFlashlightChoiceCatalog(
    const char* catalogName, SceneChoiceLock* lock);

#endif
