// Current visual scene state and mutation commands.

#ifndef __SCENE_H
#define __SCENE_H

#include "Wave.h"
#include "TranslationTable.h"

#include <string>
#include <vector>

class Flame;
class ColorPalette;
class IndexedImage;
class PaletteEntry;
class RandomSource;
class SceneEffectRegistry;
class SceneGeometry;
class ScenePresetCatalog;
class SceneVisualCatalogs;
struct SceneConfig;
class Wave;

enum SceneChange {
    SceneNoChange = 0,
    SceneFlameChanged = 1 << 0,
    SceneWaveChanged = 1 << 1,
    SceneTranslationChanged = 1 << 2,
    ScenePaletteChanged = 1 << 3,
    SceneBorderChanged = 1 << 4,
    SceneFlashlightChanged = 1 << 5,
    SceneImageChanged = 1 << 6,
    SceneAllChanged = 0x7fffffff
};

enum SceneCueType {
    SceneCueInjectImage,
    SceneCueInjectText
};

enum SceneSelectionTarget {
    SceneSelectionFlame,
    SceneSelectionGeneralFlame,
    SceneSelectionWave,
    SceneSelectionWaveScale,
    SceneSelectionObject,
    SceneSelectionTranslation,
    SceneSelectionBorder,
    SceneSelectionFlashlight,
    SceneSelectionPalette,
    SceneSelectionTable,
    SceneSelectionImage
};

class SceneSettings {
public:
    const Flame* flame;
    int generalFlame;

    Wave* wave;
    WaveConfig waveConfig;

    TranslationTable translationTable;
    int translateIndex;

    PaletteEntry* palette;
    const ColorPalette* paletteColors;
    int paletteIndex;

    int borderMode;
    int flashlightEnabled;

    const char* flameName;
    const char* generalFlameName;
    const char* waveName;
    const char* waveScaleName;
    const char* tableName;
    const char* translationName;
    const char* paletteName;
    const char* objectName;
    const char* borderName;
    const char* flashlightName;
    const char* imageName;

    SceneSettings();
};

class SceneSelectionState {
    SceneSettings settingsValue;

public:
    SceneSelectionState();

    void update(const SceneSettings& settings);
    const SceneSettings& settings() const;
};

class SceneSnapshot {
    SceneSettings settingsValue;
    unsigned int versionValue;

public:
    SceneSnapshot();
    SceneSnapshot(const SceneSettings& settings_, unsigned int version_);

    const SceneSettings& settings() const;
    unsigned int version() const;
};

class SceneCue {
public:
    SceneCueType type;
    unsigned int id;
    const IndexedImage* image;
    std::string text;
    int textFrames;
    int textInkColor;

    SceneCue();
    static SceneCue injectImage(const IndexedImage* image_);
    static SceneCue injectText(const char* text_, int frameCount, int inkColor);
};

class Scene;

class SceneCommandDependencies {
public:
    SceneVisualCatalogs& visualCatalogs;
    SceneEffectRegistry& effectRegistry;
    ScenePresetCatalog& presets;

    SceneCommandDependencies(SceneVisualCatalogs& visualCatalogs_,
        SceneEffectRegistry& effectRegistry_, ScenePresetCatalog& presets_);
};

class SceneObserver {
public:
    virtual ~SceneObserver() { }
    virtual void sceneChanged(Scene& scene, unsigned int changes) = 0;
    virtual void sceneCue(Scene& scene, const SceneCue& cue);
};

class Scene {
    SceneSettings settingsValue;
    unsigned int versionValue;
    unsigned int cueVersionValue;
    std::vector<SceneObserver*> observers;

    unsigned int compareSettings(const SceneSettings& settings) const;

public:
    Scene();

    const SceneSettings& settings() const;
    unsigned int version() const;
    SceneSnapshot snapshot() const;

    /**
     * Applies a new settings snapshot and notifies observers when it changes.
     *
     * @param settings New current Scene settings.
     * @param forcedChanges Additional SceneChange bits to report.
     * @return SceneChange bits that were applied.
     */
    unsigned int setSettings(
        const SceneSettings& settings, unsigned int forcedChanges = 0);
    void emitCue(SceneCue cue);
    void emitImageCue(const IndexedImage* image);
    void emitTextCue(const char* text, int frameCount, int inkColor);

    void addObserver(SceneObserver& observer);
    void removeObserver(SceneObserver& observer);
};

class SceneCommandTarget {
public:
    virtual ~SceneCommandTarget() { }

    /** Restores the live scene controls from their saved values. */
    virtual void restore() = 0;

    /** Restores one saved effect preset into the live scene. */
    virtual void restorePreset(int slot) = 0;

    /** Saves the live scene effect values into one preset slot. */
    virtual void savePreset(int slot) = 0;

    /** Randomizes the currently selected random palette. */
    virtual void randomPalette() = 0;

    /** Adds and selects a new random palette entry. */
    virtual void addRandomPalette() = 0;

    /** Applies a whole-scene automatic/random change. */
    virtual void changeAll() = 0;

    /** Applies a one-option automatic/random scene change. */
    virtual void changeOne() = 0;

    /** Changes one named scene selection by relative offset. */
    virtual void change(SceneSelectionTarget target, int by) = 0;

    /** Changes one named scene selection by value text. */
    virtual void change(SceneSelectionTarget target, const char* to) = 0;

    /** Activates one indexed choice on a named scene selection. */
    virtual void activate(SceneSelectionTarget target, int index) = 0;

    /** Toggles the lock state for a named scene selection. */
    virtual void toggleLock(SceneSelectionTarget target) = 0;

    /** Toggles choice-use state for a named scene selection choice. */
    virtual void toggleChoiceUse(SceneSelectionTarget target, int index) = 0;
};

class SceneCommands {
    Scene& scene;
    SceneGeometry& geometry;
    SceneCommandDependencies dependencies;
    RandomSource& randomSource;

    SceneSettings settingsFromOptions();
    unsigned int syncFromOptions(unsigned int forcedChanges);
    void emitImageCue();
    void syncFromOptionsAndMaybeCueImage(unsigned int forcedChanges);

public:
    /**
     * Creates a command facade over scene-related runtime option state.
     *
     * @param scene_ Scene state object to update.
     * @param geometry_ Frame geometry used to validate wave/image context.
     * @param dependencies_ Explicit scene-editing controls and support ports.
     * @param randomSource_ Application-owned source for scene randomization.
     */
    SceneCommands(Scene& scene_, SceneGeometry& geometry_,
        const SceneCommandDependencies& dependencies_,
        RandomSource& randomSource_);

    Scene& sceneState() { return scene; }
    const Scene& sceneState() const { return scene; }

    void applyStartupConfig(const SceneConfig& config);
    void initializeFromOptions();
    void refreshFromOptions(unsigned int forcedChanges = 0);
    void refreshFromOptionsAndMaybeCueImage(unsigned int forcedChanges);

    void change(SceneSelectionTarget target, int by);
    void change(SceneSelectionTarget target, const char* to);
    void activate(SceneSelectionTarget target, int index);
    void toggleLock(SceneSelectionTarget target);
    void toggleChoiceUse(SceneSelectionTarget target, int index);

    void randomPalette();
    void addRandomPalette();

    void changeAll();
    void changeOne();
    void restore();
    void restorePreset(int slot);
    void savePreset(int slot);
};

class SceneCommandsTarget : public SceneCommandTarget {
    SceneCommands& sceneCommands;

public:
    explicit SceneCommandsTarget(SceneCommands& sceneCommands_);

    virtual void changeAll();
    virtual void changeOne();
    virtual void restore();
    virtual void restorePreset(int slot);
    virtual void savePreset(int slot);
    virtual void randomPalette();
    virtual void addRandomPalette();
    virtual void change(SceneSelectionTarget target, int by);
    virtual void change(SceneSelectionTarget target, const char* to);
    virtual void activate(SceneSelectionTarget target, int index);
    virtual void toggleLock(SceneSelectionTarget target);
    virtual void toggleChoiceUse(SceneSelectionTarget target, int index);
};

#endif
