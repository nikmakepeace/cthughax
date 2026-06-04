#include "cthugha.h"
#include "Scene.h"
#include "EffectPresetCatalog.h"
#include "Border.h"
#include "Configuration.h"
#include "CthughaBuffer.h"
#include "Flashlight.h"
#include "Image.h"
#include "Screen.h"
#include "display.h"
#include "flames.h"
#include "TranslationOptions.h"
#include "waves.h"

#include <unistd.h>

static SceneCommands* legacySceneCommands = 0;

SceneSettings::SceneSettings()
    : flame(0)
    , generalFlame(0)
    , wave(0)
    , waveConfig()
    , translationTable()
    , translateIndex(0)
    , palette(0)
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
    , flashlightName("unknown") { }

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
        || (settingsValue.paletteIndex != settings.paletteIndex))
        changes |= ScenePaletteChanged;

    if (settingsValue.borderMode != settings.borderMode)
        changes |= SceneBorderChanged;

    if (settingsValue.flashlightEnabled != settings.flashlightEnabled)
        changes |= SceneFlashlightChanged;

    return changes;
}

void Scene::setSettings(const SceneSettings& settings, unsigned int forcedChanges) {
    unsigned int changes = compareSettings(settings) | forcedChanges;
    if (changes == SceneNoChange)
        return;

    settingsValue = settings;
    versionValue++;

    std::vector<SceneObserver*> snapshot = observers;
    for (unsigned int i = 0; i < snapshot.size(); i++)
        snapshot[i]->sceneChanged(*this, changes);
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

SceneCommands::SceneCommands(Scene& scene_, CthughaBuffer& buffer_, ImageOption& images_)
    : scene(scene_)
    , buffer(buffer_)
    , images(images_) { }

Wave* SceneCommands::selectRunnableWave(const WaveConfig& config) {
    int nEntries = wave.getNEntries();

    for (int i = 0; i < nEntries; i++) {
        Wave* selectedWave = wave.currentWave();
        if (selectedWave == 0 || selectedWave->canRun(config))
            return selectedWave;

        wave.change(+1, 0);
    }

    return 0;
}

SceneSettings SceneCommands::settingsFromOptions() {
    SceneSettings settings;

    settings.flame = flame.currentFlame();
    settings.generalFlame = int(flameGeneral);
    settings.flameName = (settings.flame != 0) ? settings.flame->name() : "unknown";
    settings.generalFlameName = flameGeneral.text();

    settings.waveConfig = WaveConfig(int(waveScale), int(table), currentWaveObject(),
        buffer.width(), buffer.height());
    settings.wave = selectRunnableWave(settings.waveConfig);
    settings.waveName = (settings.wave != 0) ? settings.wave->name() : "unknown";
    settings.waveScaleName = waveScale.currentName();
    settings.tableName = table.currentName();
    settings.objectName = object.currentName();

    settings.translationTable = translation.currentTranslationTable();
    settings.translateIndex = translation.currentN();
    settings.translationName = translation.currentName();

    settings.palette = palette.currentPaletteEntry();
    settings.paletteIndex = palette.currentN();
    settings.paletteName = palette.currentName();

    settings.borderMode = int(border);
    settings.flashlightEnabled = int(flashlight) != 0;
    settings.borderName = border.currentName();
    settings.flashlightName = flashlight.currentName();

    return settings;
}

void SceneCommands::syncFromOptions(unsigned int forcedChanges) {
    scene.setSettings(settingsFromOptions(), forcedChanges);
}

void SceneCommands::emitImageCue() {
    scene.emitImageCue(images.currentImage());
}

void SceneCommands::syncFromOptionsAndMaybeCueImage(
    const EffectControl& option, unsigned int forcedChanges) {
    syncFromOptions(forcedChanges);
    if (&option == &images)
        emitImageCue();
}

static void applyStartupChoice(EffectControl& option,
    const std::string& choice) {
    option.change(choice.c_str(), 0);
}

void SceneCommands::applyStartupConfig(const SceneConfig& config) {
    applyStartupChoice(waveScale, config.waveScale);
    applyStartupChoice(table, config.table);
    applyStartupChoice(object, config.object);
    applyStartupChoice(wave, config.wave);
    applyStartupChoice(flame, config.flame);
    applyStartupChoice(flameGeneral, config.generalFlame);
    applyStartupChoice(translation, config.translation);
    applyStartupChoice(palette, config.palette);
    applyStartupChoice(border, config.border);
    applyStartupChoice(flashlight, config.flashlight);
    applyStartupChoice(images, config.image);

    // Presentation screen selection remains a display-side EffectControl until
    // runtime reconfiguration gets its own owner.
    applyStartupChoice(screen, config.presentation);

    initializeFromOptions();
}

void SceneCommands::initializeFromOptions() {
    syncFromOptions(SceneAllChanged);
    emitImageCue();
}

void SceneCommands::refreshFromOptions(unsigned int forcedChanges) {
    syncFromOptions(forcedChanges);
}

int SceneCommands::isSceneOption(const EffectControl& option) const {
    return (&option == &flame)
        || (&option == &flameGeneral)
        || (&option == &wave)
        || (&option == &waveScale)
        || (&option == &object)
        || (&option == &translation)
        || (&option == &border)
        || (&option == &flashlight)
        || (&option == &palette)
        || (&option == &table)
        || (&option == &images);
}

void SceneCommands::change(EffectControl& option, int by, int doSave) {
    option.change(by, doSave);
    syncFromOptionsAndMaybeCueImage(option, SceneNoChange);
}

void SceneCommands::change(EffectControl& option, const char* to, int doSave) {
    option.change(to, doSave);
    syncFromOptionsAndMaybeCueImage(option, SceneNoChange);
}

void SceneCommands::activate(EffectControl& option, int index) {
    if ((index < 0) || (index >= option.getNEntries()))
        return;

    option[index]->setUse(1);
    option.setValue(index);
    option.change(0, 0);
    syncFromOptionsAndMaybeCueImage(option, SceneNoChange);
}

void SceneCommands::changeFlame(int by) { change(flame, by, 0); }
void SceneCommands::changeFlame(const char* to) { change(flame, to, 0); }

void SceneCommands::changeGeneralFlame() {
    flameGeneral.changeRandom();
    syncFromOptions(SceneFlameChanged);
}

void SceneCommands::changeWave(int by) { change(wave, by, 0); }
void SceneCommands::changeWave(const char* to) { change(wave, to, 0); }
void SceneCommands::changeWaveScale(int by) { change(waveScale, by, 0); }
void SceneCommands::changeWaveScale(const char* to) { change(waveScale, to, 0); }
void SceneCommands::changeObject(int by) { change(object, by, 0); }
void SceneCommands::changeObject(const char* to) { change(object, to, 0); }
void SceneCommands::changeTranslation(int by) { change(translation, by, 0); }
void SceneCommands::changeTranslation(const char* to) { change(translation, to, 0); }
void SceneCommands::changeBorder(int by) { change(border, by, 0); }
void SceneCommands::changeBorder(const char* to) { change(border, to, 0); }
void SceneCommands::changeFlashlight(int by) { change(flashlight, by, 0); }
void SceneCommands::changeFlashlight(const char* to) { change(flashlight, to, 0); }
void SceneCommands::changePalette(int by) { change(palette, by, 0); }
void SceneCommands::changePalette(const char* to) { change(palette, to, 0); }

void SceneCommands::randomPalette() {
    PaletteEntry::Random();
    palette.setValue(PaletteEntry::lastRandomPos);
    syncFromOptions(ScenePaletteChanged);
}

void SceneCommands::addRandomPalette() {
    PaletteEntry::addRandom();
    palette.setValue(PaletteEntry::lastRandomPos);
    syncFromOptions(ScenePaletteChanged);
}

void SceneCommands::changeTable(int by) { change(table, by, 0); }
void SceneCommands::changeTable(const char* to) { change(table, to, 0); }
void SceneCommands::changeImage(int by) { change(images, by, 0); }
void SceneCommands::changeImage(const char* to) { change(images, to, 0); }

void SceneCommands::changeAll() {
    EffectControl::changeAll();
    syncFromOptions(SceneAllChanged);
    emitImageCue();
}

void SceneCommands::changeOne() {
    EffectControl* changedOption = EffectControl::changeOne();
    syncFromOptions(SceneNoChange);
    if (changedOption == &images)
        emitImageCue();
}

void SceneCommands::restore() {
    EffectControl::restore();
    syncFromOptions(SceneAllChanged);
    emitImageCue();
}

void SceneCommands::restorePreset(int slot) {
    effectPresetCatalog.restore(slot);
    syncFromOptions(SceneAllChanged);
    emitImageCue();
}

void SceneCommands::savePreset(int slot) {
    effectPresetCatalog.save(slot);
}

void bindSceneCommandsForLegacyCallbacks(SceneCommands* commands) {
    legacySceneCommands = commands;
}

SceneCommands* sceneCommandsForLegacyCallbacks() {
    return legacySceneCommands;
}
