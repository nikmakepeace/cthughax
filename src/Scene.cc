#include "cthugha.h"
#include "Scene.h"
#include "SceneDependencies.h"
#include "Configuration.h"
#include "ProcessServices.h"

#include <cstring>
#include <unistd.h>

SceneSettings::SceneSettings()
    : flame(0)
    , generalFlame(0)
    , wave(0)
    , waveConfig()
    , translationTable()
    , translateIndex(0)
    , palette(0)
    , paletteColors(0)
    , paletteIndex(0)
    , borderMode(0)
    , flashlightEnabled(0)
    , flameName("unknown")
    , generalFlameName("unknown")
    , waveName("unknown")
    , waveScaleName("unknown")
    , tableName("unknown")
    , translationName("unknown")
    , paletteName("unknown")
    , objectName("unknown")
    , borderName("unknown")
    , flashlightName("unknown")
    , imageName("unknown") { }

SceneSelectionState::SceneSelectionState()
    : settingsValue() { }

void SceneSelectionState::update(const SceneSettings& settings) {
    settingsValue = settings;
}

const SceneSettings& SceneSelectionState::settings() const {
    return settingsValue;
}

SceneSnapshot::SceneSnapshot()
    : settingsValue()
    , versionValue(0) { }

SceneSnapshot::SceneSnapshot(
    const SceneSettings& settings_, unsigned int version_)
    : settingsValue(settings_)
    , versionValue(version_) { }

const SceneSettings& SceneSnapshot::settings() const {
    return settingsValue;
}

unsigned int SceneSnapshot::version() const {
    return versionValue;
}

SceneCue::SceneCue()
    : type(SceneCueInjectImage)
    , id(0)
    , image(0)
    , text()
    , textFrames(0)
    , textInkColor(-1) { }

SceneCue SceneCue::injectImage(const IndexedImage* image_) {
    SceneCue cue;
    cue.type = SceneCueInjectImage;
    cue.image = image_;
    return cue;
}

SceneCue SceneCue::injectText(const char* text_, int frameCount, int inkColor) {
    SceneCue cue;
    cue.type = SceneCueInjectText;
    if (text_ != 0)
        cue.text = text_;
    cue.textFrames = frameCount;
    cue.textInkColor = inkColor;
    return cue;
}

void SceneObserver::sceneCue(Scene& scene, const SceneCue& cue) {
    (void)scene;
    (void)cue;
}

Scene::Scene()
    : settingsValue()
    , versionValue(0)
    , cueVersionValue(0) { }

const SceneSettings& Scene::settings() const {
    return settingsValue;
}

unsigned int Scene::version() const {
    return versionValue;
}

SceneSnapshot Scene::snapshot() const {
    return SceneSnapshot(settingsValue, versionValue);
}

unsigned int Scene::compareSettings(const SceneSettings& settings) const {
    unsigned int changes = SceneNoChange;

    if ((settingsValue.flame != settings.flame)
        || (settingsValue.generalFlame != settings.generalFlame))
        changes |= SceneFlameChanged;

    if ((settingsValue.wave != settings.wave)
        || !settingsValue.waveConfig.sameAs(settings.waveConfig))
        changes |= SceneWaveChanged;

    if (!settingsValue.translationTable.sameTable(settings.translationTable)
        || (settingsValue.translateIndex != settings.translateIndex))
        changes |= SceneTranslationChanged;

    if ((settingsValue.palette != settings.palette)
        || (settingsValue.paletteColors != settings.paletteColors)
        || (settingsValue.paletteIndex != settings.paletteIndex))
        changes |= ScenePaletteChanged;

    if (settingsValue.borderMode != settings.borderMode)
        changes |= SceneBorderChanged;

    if (settingsValue.flashlightEnabled != settings.flashlightEnabled)
        changes |= SceneFlashlightChanged;

    if (std::strcmp(settingsValue.imageName, settings.imageName) != 0)
        changes |= SceneImageChanged;

    return changes;
}

unsigned int Scene::setSettings(
    const SceneSettings& settings, unsigned int forcedChanges) {
    unsigned int changes = compareSettings(settings) | forcedChanges;
    if (changes == SceneNoChange)
        return changes;

    settingsValue = settings;
    versionValue++;

    std::vector<SceneObserver*> snapshot = observers;
    for (unsigned int i = 0; i < snapshot.size(); i++)
        snapshot[i]->sceneChanged(*this, changes);

    return changes;
}

void Scene::emitCue(SceneCue cue) {
    cue.id = ++cueVersionValue;

    std::vector<SceneObserver*> snapshot = observers;
    for (unsigned int i = 0; i < snapshot.size(); i++)
        snapshot[i]->sceneCue(*this, cue);
}

void Scene::emitImageCue(const IndexedImage* image) {
    if (image != 0)
        emitCue(SceneCue::injectImage(image));
}

void Scene::emitTextCue(const char* text, int frameCount, int inkColor) {
    if (text != 0 && text[0] != '\0' && frameCount > 0)
        emitCue(SceneCue::injectText(text, frameCount, inkColor));
}

void Scene::addObserver(SceneObserver& observer) {
    for (unsigned int i = 0; i < observers.size(); i++)
        if (observers[i] == &observer)
            return;
    observers.push_back(&observer);
}

void Scene::removeObserver(SceneObserver& observer) {
    for (std::vector<SceneObserver*>::iterator it = observers.begin(); it != observers.end();
         ++it) {
        if (*it == &observer) {
            observers.erase(it);
            return;
        }
    }
}

SceneCommandDependencies::SceneCommandDependencies(
    SceneVisualCatalogs& visualCatalogs_,
    SceneEffectRegistry& effectRegistry_, ScenePresetCatalog& presets_)
    : visualCatalogs(visualCatalogs_)
    , effectRegistry(effectRegistry_)
    , presets(presets_) { }

SceneCommands::SceneCommands(Scene& scene_, SceneGeometry& geometry_,
    const SceneCommandDependencies& dependencies_, RandomSource& randomSource_)
    : scene(scene_)
    , geometry(geometry_)
    , dependencies(dependencies_)
    , randomSource(randomSource_) { }

SceneSettings SceneCommands::settingsFromOptions() {
    return dependencies.visualCatalogs.currentSettings(geometry);
}

unsigned int SceneCommands::syncFromOptions(unsigned int forcedChanges) {
    SceneSettings settings = settingsFromOptions();
    return scene.setSettings(settings, forcedChanges);
}

void SceneCommands::emitImageCue() {
    scene.emitImageCue(dependencies.visualCatalogs.currentImage());
}

void SceneCommands::syncFromOptionsAndMaybeCueImage(
    unsigned int forcedChanges) {
    unsigned int appliedChanges = syncFromOptions(forcedChanges);
    if ((appliedChanges & SceneImageChanged) != 0)
        emitImageCue();
}

void SceneCommands::applyStartupConfig(const SceneConfig& config) {
    dependencies.visualCatalogs.applyStartupConfig(config, randomSource);
    initializeFromOptions();
}

void SceneCommands::initializeFromOptions() {
    syncFromOptions(SceneAllChanged);
    emitImageCue();
}

void SceneCommands::refreshFromOptions(unsigned int forcedChanges) {
    syncFromOptions(forcedChanges);
}

void SceneCommands::refreshFromOptionsAndMaybeCueImage(
    unsigned int forcedChanges) {
    syncFromOptionsAndMaybeCueImage(forcedChanges);
}

void SceneCommands::change(SceneSelectionTarget target, int by) {
    unsigned int forcedChanges
        = dependencies.visualCatalogs.change(target, by, randomSource);
    syncFromOptions(forcedChanges);
    if (target == SceneSelectionImage)
        emitImageCue();
}

void SceneCommands::change(SceneSelectionTarget target, const char* to) {
    unsigned int forcedChanges
        = dependencies.visualCatalogs.change(target, to, randomSource);
    syncFromOptions(forcedChanges);
    if (target == SceneSelectionImage)
        emitImageCue();
}

void SceneCommands::activate(SceneSelectionTarget target, int index) {
    unsigned int forcedChanges
        = dependencies.visualCatalogs.activate(target, index);
    syncFromOptions(forcedChanges);
    if (target == SceneSelectionImage)
        emitImageCue();
}

void SceneCommands::toggleLock(SceneSelectionTarget target) {
    dependencies.visualCatalogs.toggleLock(target);
}

void SceneCommands::toggleChoiceUse(SceneSelectionTarget target, int index) {
    dependencies.visualCatalogs.toggleChoiceUse(target, index);
}

void SceneCommands::randomPalette() {
    unsigned int forcedChanges
        = dependencies.visualCatalogs.randomPalette(randomSource);
    syncFromOptions(forcedChanges);
}

void SceneCommands::addRandomPalette() {
    unsigned int forcedChanges
        = dependencies.visualCatalogs.addRandomPalette(randomSource);
    syncFromOptions(forcedChanges);
}

void SceneCommands::changeAll() {
    dependencies.effectRegistry.changeAll(randomSource);
    syncFromOptionsAndMaybeCueImage(SceneAllChanged);
}

void SceneCommands::changeOne() {
    dependencies.effectRegistry.changeOne(randomSource);
    syncFromOptionsAndMaybeCueImage(SceneNoChange);
}

void SceneCommands::restore() {
    dependencies.effectRegistry.restoreAll();
    syncFromOptionsAndMaybeCueImage(SceneAllChanged);
}

void SceneCommands::restorePreset(int slot) {
    dependencies.presets.restore(slot);
    syncFromOptionsAndMaybeCueImage(SceneAllChanged);
}

void SceneCommands::savePreset(int slot) {
    dependencies.presets.save(slot);
}

SceneCommandsTarget::SceneCommandsTarget(SceneCommands& sceneCommands_)
    : sceneCommands(sceneCommands_) { }

void SceneCommandsTarget::changeAll() {
    sceneCommands.changeAll();
}

void SceneCommandsTarget::changeOne() {
    sceneCommands.changeOne();
}

void SceneCommandsTarget::restore() {
    sceneCommands.restore();
}

void SceneCommandsTarget::restorePreset(int slot) {
    sceneCommands.restorePreset(slot);
}

void SceneCommandsTarget::savePreset(int slot) {
    sceneCommands.savePreset(slot);
}

void SceneCommandsTarget::randomPalette() {
    sceneCommands.randomPalette();
}

void SceneCommandsTarget::addRandomPalette() {
    sceneCommands.addRandomPalette();
}

void SceneCommandsTarget::change(SceneSelectionTarget target, int by) {
    sceneCommands.change(target, by);
}

void SceneCommandsTarget::change(SceneSelectionTarget target, const char* to) {
    sceneCommands.change(target, to);
}

void SceneCommandsTarget::activate(SceneSelectionTarget target, int index) {
    sceneCommands.activate(target, index);
}

void SceneCommandsTarget::toggleLock(SceneSelectionTarget target) {
    sceneCommands.toggleLock(target);
}

void SceneCommandsTarget::toggleChoiceUse(
    SceneSelectionTarget target, int index) {
    sceneCommands.toggleChoiceUse(target, index);
}
