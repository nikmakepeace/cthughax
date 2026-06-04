#include <assert.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <string>

#ifndef CTHUGHA_SOURCE_DIR
#error CTHUGHA_SOURCE_DIR must point at the repository root
#endif

static std::string readSourceFile(const char* relativePath) {
    std::string path = std::string(CTHUGHA_SOURCE_DIR) + "/" + relativePath;
    std::ifstream file(path.c_str());
    assert(file.good());

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

static void assertSourceDoesNotContain(const char* relativePath,
    const char* token) {
    std::string contents = readSourceFile(relativePath);
    if (contents.find(token) != std::string::npos)
        fprintf(stderr, "%s still contains `%s`\n", relativePath, token);
    assert(contents.find(token) == std::string::npos);
}

static void assertSourceContains(const char* relativePath, const char* token) {
    std::string contents = readSourceFile(relativePath);
    if (contents.find(token) == std::string::npos)
        fprintf(stderr, "%s does not contain `%s`\n", relativePath, token);
    assert(contents.find(token) != std::string::npos);
}

static void testAudioRuntimeUsesAudioConfig() {
    assertSourceContains("src/AudioRuntime.cc", "AudioSettings::fromConfig");
    assertSourceDoesNotContain("src/AudioRuntime.cc", "fromCurrentOptions");
    assertSourceDoesNotContain("src/AudioRuntime.cc", "audio_input_file");
    assertSourceDoesNotContain("src/AudioRuntime.cc", "::audioInputMode");
    assertSourceDoesNotContain("src/AudioSettings.cc", "fromCurrentOptions");
    assertSourceDoesNotContain("src/AudioSettings.cc", "audio_input_file");
    assertSourceDoesNotContain("src/AudioSettings.cc", "::audioInputMode");
    assertSourceDoesNotContain("src/AudioSettings.cc", "::soundDSPMethod");
    assertSourceDoesNotContain("src/AudioSettings.cc", "soundSilent");
}

static void testDisplayStartupUsesDisplayConfig() {
    assertSourceContains("src/Application.cc", "startupConfigValue.display");
    assertSourceContains("src/DisplayDeviceX11.cc", "checkDisplaySize(config)");
    assertSourceDoesNotContain("src/DisplayDevice.h", "display_mode");
    assertSourceDoesNotContain("src/DisplayDevice.cc", "display_mode");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "display_mode");
}

static void testX11StartupUsesX11ConfigOnlyForX11Builds() {
    assertSourceContains("src/Configuration.h", "#ifdef CTH_XWIN\nstruct X11Config");
    assertSourceContains("src/Configuration.h", "#ifdef CTH_XWIN\n    X11Config x11;");
    assertSourceContains("src/Application.cc",
        "configureDisplayDeviceX11(startupConfigValue.x11)");
    assertSourceDoesNotContain("src/Application.cc",
        "configureDisplayDevice(startupConfigValue.display)");
    assertSourceContains("src/xcthugha.h",
        "void configureDisplayDeviceX11(const X11Config& config)");
    assertSourceContains("src/DisplayDeviceX11.cc",
        "void configureDisplayDeviceX11(const X11Config& config)");
    assertSourceDoesNotContain("src/Configuration.h", "x11FontName");
    assertSourceDoesNotContain("src/Configuration.h", "x11MitShm");
    assertSourceDoesNotContain("src/options.cc", "&display_mit_shm");
    assertSourceDoesNotContain("src/options.cc", "&display_on_root");
    assertSourceDoesNotContain("src/options.cc", "&private_cmap");
    assertSourceDoesNotContain("src/options.cc", "&display_override_redirect");
    assertSourceDoesNotContain("src/options.cc", "&xcth_panel");
    assertSourceDoesNotContain("src/options.cc", "&DisplayDevice::text_on_term");
    assertSourceDoesNotContain("src/options.cc", "window_pos.x");
    assertSourceDoesNotContain("src/options.cc", "strncpy(xcth_font");
}

static void testLoggingUsesLoggingConfig() {
    assertSourceContains("src/Application.cc", "cthugha_configure_logging");
    assertSourceDoesNotContain("src/misc.cc", "cthugha_verbose");
    assertSourceDoesNotContain("src/Option.h", "cthugha_verbose");
}

static void testAudioInputFileGlobalWasDeleted() {
    assertSourceDoesNotContain("src/AudioOptions.h", "audio_input_file");
    assertSourceDoesNotContain("src/AudioOutput.cc", "audio_input_file");
}

static void testLegacyParserDoesNotWritePortedConfigValues() {
    assertSourceDoesNotContain("src/options.cc", "audioInputMode.setValue");
    assertSourceDoesNotContain("src/options.cc", "audio_input_file");
    assertSourceDoesNotContain("src/options.cc", "display_mode =");
    assertSourceDoesNotContain("src/options.cc",
        "CthughaBuffer::buffer.setDimensions");
    assertSourceDoesNotContain("src/options.cc", "cthugha_verbose.change");
    assertSourceDoesNotContain("src/options.cc", "&cthugha_verbose.value");
    assertSourceDoesNotContain("src/options.cc", "snprintf(extra_lib_path");
    assertSourceDoesNotContain("src/options.cc", "strncpy(ini_file_override");
    assertSourceDoesNotContain("src/options.cc", "&audioInputLoop");
    assertSourceDoesNotContain("src/options.cc", "soundChannels.setValue");
    assertSourceDoesNotContain("src/options.cc", "soundSampleRate.setValue");
    assertSourceDoesNotContain("src/options.cc", "soundFormat.change");
    assertSourceDoesNotContain("src/options.cc", "strncpy(pulse_server");
    assertSourceDoesNotContain("src/options.cc", "pulse_latency_msec =");
    assertSourceDoesNotContain("src/options.cc", "strncpy(audio_output_dump");
    assertSourceDoesNotContain("src/options.cc", "sound_minnoise.change");
    assertSourceDoesNotContain("src/options.cc", "soundSilent.setValue");
    assertSourceDoesNotContain("src/options.cc", "strncpy(dev_dsp");
    assertSourceDoesNotContain("src/options.cc", "soundDSPSync.setValue");
    assertSourceDoesNotContain("src/options.cc", "strncpy(dev_mixer");
    assertSourceDoesNotContain("src/options.cc", "soundDSPFragments.change");
    assertSourceDoesNotContain("src/options.cc", "soundDSPMethod.change");
    assertSourceDoesNotContain("src/options.cc", "mixer_initial_volume");
    assertSourceDoesNotContain("src/options.cc", "&key_esc");
    assertSourceDoesNotContain("src/options.cc", "Keymap::keymapFile");
    assertSourceContains("src/AudioSystem.cc", "config.mixerInitialVolumes");
}

static void testCatalogLoadingUsesPathConfig() {
    assertSourceContains("src/EffectChoiceLoader.cc", "pathConfig.extraLibraryPath");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "extra_lib_path");
    assertSourceContains("src/Image.cc", "loadEffectChoices(*this, pathConfig");
    assertSourceContains("src/palettes.cc", "loadEffectChoices(palette, pathConfig");
    assertSourceContains("src/waves.cc", "loadEffectChoices(object, pathConfig");
}

static void testApplicationProvidesStartupConfigSlices() {
    assertSourceContains("src/Application.cc", "configureApplicationOptions(startupConfigValue.app)");
    assertSourceContains("src/Application.cc", "configureKeys(startupConfigValue.input)");
    assertSourceContains("src/Application.cc", "Keymap::init(startupConfigValue.input)");
    assertSourceContains("src/Application.cc", "configureIniFiles(startupConfigValue.paths)");
    assertSourceContains("src/Application.cc", "configureEffectChoiceLoader(startupConfigValue.catalogs)");
    assertSourceContains("src/Application.cc", "configureAudioOptions(startupConfigValue.audio)");
    assertSourceContains("src/Application.cc", "configureCthughaDisplay(startupConfigValue.display)");
    assertSourceContains("src/Application.cc", "configureAutoChanger(startupConfigValue.autoChange)");
    assertSourceContains("src/Application.cc", "configureTranslationOptions(startupConfigValue.visual)");
    assertSourceContains("src/Application.cc", "configureWaveOptions(startupConfigValue.visual)");
    assertSourceContains("src/Application.cc", "videoDirector().configure(startupConfigValue.visual");
    assertSourceContains("src/Application.cc", "sceneCommands().applyStartupConfig(startupConfigValue.scene)");
    assertSourceDoesNotContain("src/Application.cc", "Keymap::configure");
    assertSourceDoesNotContain("src/Application.cc", "EffectControl::changeToInitial");
}

static void testInputStartupUsesInputConfig() {
    assertSourceContains("src/Configuration.h", "struct InputConfig");
    assertSourceContains("src/Configuration.h", "InputConfig input");
    assertSourceContains("src/keys.h", "void configureKeys(const InputConfig& config)");
    assertSourceContains("src/keymap.h", "static void init(const InputConfig& config)");
    assertSourceDoesNotContain("src/Configuration.h",
        "struct AppConfig {\n    int optionsSaveEnabled;\n    int escapeKeyEnabled");
    assertSourceDoesNotContain("src/keymap.h", "keymapFile[PATH_MAX]");
    assertSourceDoesNotContain("src/keymap.cc", "Keymap::configure");
}

static void testSceneStartupUsesSceneConfig() {
    assertSourceContains("src/Configuration.h", "SceneConfig scene");
    assertSourceContains("src/Scene.cc", "SceneCommands::applyStartupConfig");
    assertSourceContains("src/Scene.cc", "config.flame");
    assertSourceContains("src/Scene.cc", "config.wave");
    assertSourceContains("src/Scene.cc", "config.palette");
    assertSourceDoesNotContain("src/options.cc", "flame.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "wave.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "waveScale.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "object.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "translation.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "palette.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "border.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "flashlight.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "table.setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "imageOption().setInitialEntry");
    assertSourceDoesNotContain("src/options.cc", "screen.setInitialEntry");
    assertSourceDoesNotContain("src/IniFiles.cc", "effectControlGetIniInitials");
}

static void testConfigDefaultsAreNotConsumedAsLegacyDefaults() {
    assertSourceContains("src/configuration_defaults.h", "AUDIO_CONFIG_DEFAULT_INPUT_MODE");
    assertSourceDoesNotContain("src/AudioSystem.cc", "DEFAULT_AUDIO_INPUT_MODE");
    assertSourceDoesNotContain("src/AudioSystem.cc", "DEFAULT_SOUND_SAMPLE_RATE_HZ");
    assertSourceDoesNotContain("src/AudioSettings.cc", "DEFAULT_SOUND_DSP_METHOD");
    assertSourceDoesNotContain("src/AudioOutput.cc", "DEFAULT_PULSE_LATENCY_MS");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "DEFAULT_SOUND_MINNOISE");
    assertSourceDoesNotContain("src/AutoChanger.cc", "DEFAULT_CHANGE_QUIET_MS");
    assertSourceDoesNotContain("src/CthughaDisplay.cc", "DEFAULT_MAX_FRAMES_PER_SECOND");
    assertSourceDoesNotContain("src/DisplayDevice.cc", "DEFAULT_DISPLAY_TEXT_ON_TERM");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "DEFAULT_X11_MIT_SHM");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "DEFAULT_DOUBLE_LOAD_ENABLED");
    assertSourceDoesNotContain("src/TranslationOptions.cc", "DEFAULT_USE_TRANSLATES_ENABLED");
    assertSourceDoesNotContain("src/waves.cc", "DEFAULT_USE_OBJECTS_ENABLED");
    assertSourceDoesNotContain("src/keymap.cc", "DEFAULT_KEYMAP_FILE_PATH");
    assertSourceDoesNotContain("src/keys.cc", "DEFAULT_ESCAPE_KEY_ENABLED");
    assertSourceContains("src/configuration_defaults.h", "INPUT_CONFIG_DEFAULT_KEYMAP_FILE_PATH");
    assertSourceContains("src/configuration_defaults.h", "INPUT_CONFIG_DEFAULT_ESCAPE_KEY_ENABLED");
    assertSourceDoesNotContain("src/Screenshot.cc", "DEFAULT_SCREENSHOT_FILE_PREFIX");
}

int main() {
    testAudioRuntimeUsesAudioConfig();
    testDisplayStartupUsesDisplayConfig();
    testX11StartupUsesX11ConfigOnlyForX11Builds();
    testLoggingUsesLoggingConfig();
    testAudioInputFileGlobalWasDeleted();
    testLegacyParserDoesNotWritePortedConfigValues();
    testCatalogLoadingUsesPathConfig();
    testApplicationProvidesStartupConfigSlices();
    testInputStartupUsesInputConfig();
    testSceneStartupUsesSceneConfig();
    testConfigDefaultsAreNotConsumedAsLegacyDefaults();
    return 0;
}
