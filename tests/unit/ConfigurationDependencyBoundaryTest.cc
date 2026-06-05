/** @file
 * Source-level dependency boundary checks for deglobalisation work.
 */

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
    std::string path = std::string(CTHUGHA_SOURCE_DIR) + "/" + relativePath;
    std::ifstream file(path.c_str());
    if (!file.good())
        return;

    std::ostringstream contentsStream;
    contentsStream << file.rdbuf();
    std::string contents = contentsStream.str();
    if (contents.find(token) != std::string::npos)
        fprintf(stderr, "%s still contains `%s`\n", relativePath, token);
    assert(contents.find(token) == std::string::npos);
}

static void assertSourceDoesNotExist(const char* relativePath) {
    std::string path = std::string(CTHUGHA_SOURCE_DIR) + "/" + relativePath;
    std::ifstream file(path.c_str());
    if (file.good())
        fprintf(stderr, "%s still exists\n", relativePath);
    assert(!file.good());
}

static void assertSourceContains(const char* relativePath, const char* token) {
    std::string contents = readSourceFile(relativePath);
    if (contents.find(token) == std::string::npos)
        fprintf(stderr, "%s does not contain `%s`\n", relativePath, token);
    assert(contents.find(token) != std::string::npos);
}

static void testAudioIngestUsesAudioConfig() {
    assertSourceDoesNotExist("src/AudioRuntime.cc");
    assertSourceDoesNotExist("src/AudioRuntime.h");
    assertSourceContains("src/AudioIngest.cc", "AudioSettings::fromConfig");
    assertSourceContains("src/AudioIngest.cc", "RuntimeFactory runtimeFactory");
    assertSourceDoesNotContain("src/AudioIngest.cc", "fromCurrentOptions");
    assertSourceDoesNotContain("src/AudioIngest.cc", "audio_input_file");
    assertSourceDoesNotContain("src/AudioIngest.cc", "::audioInputMode");
    assertSourceDoesNotContain("src/AudioIngest.cc", "soundSampleRate");
    assertSourceDoesNotContain("src/AudioIngest.cc", "soundChannels");
    assertSourceDoesNotContain("src/AudioIngest.cc", "soundFormat");
    assertSourceDoesNotContain("src/AudioSettings.cc", "fromCurrentOptions");
    assertSourceDoesNotContain("src/AudioSettings.cc", "audio_input_file");
    assertSourceDoesNotContain("src/AudioSettings.cc", "::audioInputMode");
    assertSourceDoesNotContain("src/AudioSettings.cc", "::soundDSPMethod");
    assertSourceDoesNotContain("src/AudioSettings.cc", "soundSilent");
}

static void testAudioDeviceSettingsAreStartupOnly() {
    assertSourceContains("src/AudioOptions.h", "struct AudioDeviceConfig");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& audioInputMode");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundFormat");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundChannels");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundSampleRate");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundDSPMethod");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundDSPFragments");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundDSPFragmentSize");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundDSPSync");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern Option& soundSilent");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern int audioInputLoop");
    assertSourceDoesNotContain("src/AudioOptions.h", "extern char dev_dsp");
    assertSourceDoesNotContain("src/AudioSystem.cc", "InterfaceElementOption");
    assertSourceDoesNotContain("src/AudioSystem.cc", "interfaceAudio");
    assertSourceDoesNotContain("src/AudioSystem.cc", "Audio Interface");
    assertSourceDoesNotContain("src/AudioSystem.cc", "audioFrameChange");
    assertSourceContains("src/AudioSystem.cc",
        "audioDeviceConfigValue.pcmFormat.sampleRate = config.sampleRateHz");
    assertSourceContains("src/PcmSource.cc", "audioSetPcmFormat(format)");
    assertSourceContains("src/DspPcmSource.cc", "audioSetSampleRateHz(sampleRate)");
    assertSourceContains("src/AudioDSPOutput.cc", "audioSetSampleRateHz(sampleRate)");
}

static void testAudioFrameOwnsPerFrameMetrics() {
    assertSourceContains("src/AudioFrame.h", "struct AudioMetrics");
    assertSourceContains("src/AudioFrame.h", "AudioMetrics metrics");
    assertSourceContains("src/AudioProcessor.cc",
        "AudioProcessor::analyze(const char2* frame, int minNoise)");
    assertSourceContains("src/AudioProcessing.h", "class AudioProcessingState");
    assertSourceContains("src/AudioProcessing.h", "class AudioProcessingSelector");
    assertSourceContains("src/AudioFftProcessor.h", "class AudioFftProcessor");
    assertSourceContains("src/AudioFftProcessor.h",
        "class FixedPointAudioFftProcessor");
    assertSourceContains("src/AudioFftProcessor.cc",
        "static FixedPointAudioFftProcessor processor");
    assertSourceContains("src/Audio.h",
        "explicit AudioProcessor(AudioFftProcessor& fftProcessor)");
    assertSourceContains("src/AudioProcessor.cc",
        "fftProcessorValue->transform(raw, processedWaveData)");
    assertSourceContains("tests/unit/AudioFrameProcessorTest.cc",
        "testAudioProcessorDelegatesFftToInjectedProcessor");
    assertSourceContains("tests/unit/AudioFrameProcessorTest.cc",
        "testFixedPointFftProducesCharacterizedOutput");
    assertSourceContains("tests/benchmarks/AudioPipelineBench.cc",
        "prism-2s-48000-stereo-s16le.raw");
    assertSourceContains("tests/benchmarks/AudioPipelineBench.cc",
        "BM_AudioFft_FixedPoint_PrismPcm");
    assertSourceContains("tests/benchmarks/CMakeLists.txt", "CTH_DISABLE_MINIMP3");
    assertSourceDoesNotContain("src/AudioFftProcessor.h",
        "LookupTableAudioFftProcessor");
    assertSourceDoesNotContain("tests/benchmarks/AudioPipelineBench.cc",
        "LookupTableAudioFftProcessor");
    assertSourceContains("tests/CMakeLists.txt", "AudioFftProcessor.cc");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioProcessorWp");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioProcessorR");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioProcessorFftInit");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "initAudioProcessorFft");
    assertSourceContains("src/AudioVisualBridge.cc",
        "audioProcessingSelectorValue.process(frame)");
    assertSourceContains("src/AudioVisualBridge.cc",
        "audioProcessorValue.analyze(frame, minNoiseValue)");
    assertSourceDoesNotContain("src/AudioVisualBridge.cc",
        "static AudioProcessor");
    assertSourceDoesNotContain("src/AudioProcessor.cc",
        "static AudioProcessor");
    assertSourceContains("src/AudioVisualBridge.cc",
        "acousticContextValue.update(frame.metrics)");
    assertSourceContains("src/Application.h",
        "AcousticContext acousticContextValue");
    assertSourceContains("src/Application.cc",
        "new AudioVisualBridge(acousticContextValue");
    assertSourceContains("src/Application.cc",
        "startupConfigValue.audioAnalysis.minNoise");
    assertSourceContains("src/Application.cc",
        "context.audioMetrics = &frame.metrics");
    assertSourceContains("src/Application.cc",
        "context.acousticContext = &acousticContext");
    assertSourceContains("src/Application.cc",
        "displayValue->present(*indexedFrame, presentationContext)");
    assertSourceContains("src/ScreenRenderContext.h",
        "const AudioMetrics* audioMetrics() const");
    assertSourceContains("src/ScreenRenderContext.h",
        "const AcousticContext* acousticContext() const");
    assertSourceContains("src/PresentationComposer.cc",
        "ScreenRenderContext(source, destination, frameTimeSeconds,");
    assertSourceContains("src/AutoChanger.cc",
        "void AutoChanger::operator()(const AudioMetrics& metrics)");
    assertSourceContains("src/AutoChanger.h",
        "AcousticContext& acousticContext_");
    assertSourceContains("src/AudioVisualBridge.h",
        "std::unique_ptr<AutoChanger> autoChangerValue");
    assertSourceContains("src/AudioVisualBridge.cc",
        "autoChangerValue.reset(new AutoChanger");
    assertSourceContains("src/Interface.cc",
        "autoChangerStatusProviderValue->autoChangerStatus()");
    assertSourceContains("src/Application.cc",
        "Interface::setAutoChangerStatusProvider(audioVisualBridge.get())");
    assertSourceContains("tests/CMakeLists.txt",
        "audio_frame_processor_test");
    assertSourceContains("tests/CMakeLists.txt",
        "audio_visual_bridge_auto_changer_test");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameSetCurrent");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameCurrent");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameRawData");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameProcessedWaveData");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFramePublishMetrics");
    assertSourceDoesNotContain("src/AudioFrame.h", "audioFrameMetrics");
    assertSourceDoesNotContain("src/AudioFrame.cc", "currentAudioFrame");
    assertSourceDoesNotContain("src/Application.cc", "audioFrameSetCurrent");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioFrameCurrent");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioFrameRawData");
    assertSourceDoesNotContain("src/AudioProcessor.cc", "audioFrameProcessedWaveData");
    assertSourceDoesNotContain("src/AutoChanger.cc", "audioFrameMetrics");
    assertSourceDoesNotContain("src/Border.cc", "audioFrameRawData");
    assertSourceDoesNotContain("src/display.cc", "audioFrameMetrics");
    assertSourceDoesNotContain("src/display.cc", "audioFrameProcessedWaveData");
    assertSourceDoesNotContain("src/AudioAnalyzer.h", "class AudioAnalyzer");
    assertSourceDoesNotContain("src/AudioAnalyzer.h", "extern AudioMetrics audioMetrics");
    assertSourceDoesNotContain("src/AudioAnalyzer.h", "extern AcousticContext acousticContext");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "AudioAnalyzer::");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "AudioMetrics audioMetrics");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "AcousticContext acousticContext");
    assertSourceDoesNotContain("src/AudioAnalyzer.h", "sound_minnoise");
    assertSourceDoesNotContain("src/AudioAnalyzer.cc", "sound_minnoise");
    assertSourceDoesNotContain("src/AudioVisualBridge.cc", "sound_minnoise");
    assertSourceDoesNotContain("src/Interface.cc", "sound_minnoise");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "sound_minnoise");
    assertSourceDoesNotContain("src/AudioVisualBridge.cc", "audioAnalyzer");
    assertSourceDoesNotContain("src/AutoChanger.cc", "audioMetrics.");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern AutoChanger* autoChanger");
    assertSourceDoesNotContain("src/AutoChanger.cc", "AutoChanger* autoChanger");
    assertSourceDoesNotContain("src/AudioVisualBridge.cc", "autoChanger =");
    assertSourceDoesNotContain("src/Interface.cc", "autoChanger->");
    assertSourceDoesNotContain("src/display.cc", "audioMetrics.");
}

static void testDisplayStartupUsesDisplayConfig() {
    assertSourceContains("src/Application.cc", "startupConfigValue.display");
    assertSourceContains("src/DisplayDeviceX11.cc", "checkDisplaySize(config)");
    assertSourceDoesNotContain("src/DisplayDevice.h", "display_mode");
    assertSourceDoesNotContain("src/DisplayDevice.cc", "display_mode");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "display_mode");
}

static void testAutoChangeSettingsAreApplicationOwned() {
    assertSourceContains("src/Application.h",
        "std::unique_ptr<AutoChangeSettings> autoChangeSettingsValue");
    assertSourceContains("src/Application.h",
        "std::unique_ptr<AutoChangeControls> autoChangeControlsValue");
    assertSourceContains("src/Application.cc",
        "new OwnedAutoChangeSettings(startupConfigValue.autoChange)");
    assertSourceContains("src/Application.cc",
        "new AutoChangeControls(*autoChangeSettingsValue)");
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeAutoChangeControls(*autoChangeControlsValue)");
    assertSourceContains("src/Application.cc",
        "Interface::setAutoChangeControls(autoChangeControlsValue.get())");
    assertSourceContains("src/Application.cc",
        "runtimeChangeMediatorValue.get(), autoChangeSettingsValue.get()");
    assertSourceContains("src/AudioVisualBridge.cc",
        "*autoChangeSettings_");
    assertSourceContains("src/AutoChanger.h",
        "const AutoChangeSettings& settings");
    assertSourceContains("src/AutoChanger.cc", "settings.quietMs()");
    assertSourceContains("src/AutoChanger.cc", "settings.waitMinMs()");
    assertSourceContains("src/AutoChanger.cc", "settings.waitRandomMs()");
    assertSourceContains("src/AutoChanger.cc", "settings.cumulativeFireLevel()");
    assertSourceContains("src/AutoChanger.cc", "settings.locked()");
    assertSourceContains("src/AutoChanger.cc", "settings.changeLittle()");
    assertSourceContains("src/RuntimeAutoChangeControls.cc",
        "autoChangeControls.changeOptionBy(option, by)");
    assertSourceContains("src/Interface.cc",
        "InterfaceElementAutoChangeOption");
    assertSourceContains("src/Interface.cc",
        "controls->option(field)");
    assertSourceContains("src/LegacyRuntimeConfigContributor.cc",
        "config.autoChange = autoChangeSettings.config()");
    assertSourceContains("tests/CMakeLists.txt",
        "runtime_auto_change_controls_test");
    assertSourceContains("tests/unit/RuntimeAutoChangeControlsTest.cc",
        "class FakeAutoChangeSettings");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionTime changeQuiet");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionTime changeWaitMin");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionTime changeWaitRandom");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionInt changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionOnOff lock");
    assertSourceDoesNotContain("src/AutoChanger.h", "extern OptionOnOff change_little");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionTime changeQuiet");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionTime changeWaitMin");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionTime changeWaitRandom");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionInt changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionOnOff lock");
    assertSourceDoesNotContain("src/AutoChanger.cc", "OptionOnOff change_little");
    assertSourceDoesNotContain("src/Application.cc",
        "configureAutoChanger(startupConfigValue.autoChange)");
    assertSourceDoesNotContain("src/Interface.cc", "&changeWaitMin");
    assertSourceDoesNotContain("src/Interface.cc", "&changeWaitRandom");
    assertSourceDoesNotContain("src/Interface.cc", "&changeQuiet");
    assertSourceDoesNotContain("src/Interface.cc", "&changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/Interface.cc", "&change_little");
    assertSourceDoesNotContain("src/Interface.cc", "&lock");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "changeWaitMin");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "changeWaitRandom");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "changeQuiet");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc",
        "changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc", "change_little");
    assertSourceDoesNotContain("src/info_title_usage.cc", "changeWaitMin.text()");
    assertSourceDoesNotContain("src/info_title_usage.cc", "changeWaitRandom.text()");
    assertSourceDoesNotContain("src/info_title_usage.cc", "changeQuiet.text()");
    assertSourceDoesNotContain("src/info_title_usage.cc",
        "changeCumulativeFireLevel.text()");
    assertSourceDoesNotContain("src/info_title_usage.cc", "change_little.text()");
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
    assertSourceContains("src/DisplayDeviceX11.cc", "config.frameDumpDirectory");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc",
        "getenv(\"CTHUGHA_DUMP_X11");
    assertSourceDoesNotContain("src/Configuration.h", "textOnTerm");
    assertSourceDoesNotContain("src/Configuration.h", "ncursesEnabled");
    assertSourceDoesNotContain("src/Configuration.cc", "text-on-term");
    assertSourceDoesNotContain("src/Configuration.cc", "KEY_X11_TEXT_ON_TERM");
    assertSourceDoesNotContain("src/configuration_defaults.h", "NCURSES");
    assertSourceDoesNotContain("src/configuration_defaults.h",
        "X11_CONFIG_DEFAULT_TEXT_ON_TERM");
    assertSourceDoesNotContain("src/DisplayDevice.h", "text_on_term");
    assertSourceDoesNotContain("src/DisplayDevice.cc", "text_on_term");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "text_on_term");
    assertSourceDoesNotContain("src/info_title_usage.cc", "text-on-term");
    assertSourceDoesNotContain("src/keys.cc", "getkey_ncurs");
    assertSourceDoesNotContain("src/keys.cc", "ncurses");
    assertSourceDoesNotContain("src/display.h", "init_ncurses");
    assertSourceDoesNotContain("src/display.h", "exit_ncurses");
    assertSourceDoesNotExist("src/disp-ncurses.cc");
    assertSourceDoesNotContain("src/CMakeLists.txt", "disp-ncurses.cc");
    assertSourceDoesNotContain("CMakeLists.txt", "find_package(Curses");
    assertSourceDoesNotContain("CMakeLists.txt", "HAVE_NCURSES");
    assertSourceDoesNotContain("cmake/config.h.in", "HAVE_NCURSES");
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
    assertSourceDoesNotExist("src/options.cc");
    assertSourceDoesNotExist("src/options.h");
    assertSourceDoesNotExist("src/xwin_options.cc");
    assertSourceDoesNotContain("src/Application.cc", "get_pre_params");
    assertSourceDoesNotContain("src/Application.cc", "get_params");
    assertSourceDoesNotContain("src/Application.cc", "params_request_help");
    assertSourceDoesNotContain("src/Application.cc", "startupHelpRequested");
    assertSourceContains("src/Application.cc", "startupConfig.helpRequested");
    assertSourceDoesNotContain("src/options.cc", "audioInputMode.setValue");
    assertSourceDoesNotContain("src/options.cc", "audio_input_file");
    assertSourceDoesNotContain("src/options.cc", "display_mode =");
    assertSourceDoesNotContain("src/options.cc",
        "CthughaBuffer::buffer.setDimensions");
    assertSourceDoesNotContain("src/options.cc", "cthugha_verbose.change");
    assertSourceDoesNotContain("src/options.cc", "&cthugha_verbose.value");
    assertSourceDoesNotContain("src/options.cc", "&options_save.value");
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
    assertSourceDoesNotContain("src/options.cc", "&lock.value");
    assertSourceDoesNotContain("src/options.cc", "&change_little.value");
    assertSourceDoesNotContain("src/options.cc", "lock.setValue");
    assertSourceDoesNotContain("src/options.cc", "changeWaitMin.change");
    assertSourceDoesNotContain("src/options.cc", "changeWaitRandom.change");
    assertSourceDoesNotContain("src/options.cc", "changeQuiet.change");
    assertSourceDoesNotContain("src/options.cc", "changeMsgTime.change");
    assertSourceDoesNotContain("src/options.cc",
        "changeCumulativeFireLevel.change");
    assertSourceDoesNotContain("src/options.cc", "silenceMessages().loadFile");
    assertSourceDoesNotContain("src/options.cc", "setQotdEnabled");
    assertSourceDoesNotContain("src/options.cc", "setQotdServer");
    assertSourceDoesNotContain("src/options.cc", "&use_objects.value");
    assertSourceDoesNotContain("src/options.cc", "&use_translates.value");
    assertSourceDoesNotContain("src/options.cc", "&showFPS.value");
    assertSourceDoesNotContain("src/options.cc", "paletteSmoothingChance =");
    assertSourceDoesNotContain("src/options.cc", "palette_set_filter");
    assertSourceDoesNotContain("src/options.cc",
        "videoDirector().setImageLoadingEnabled");
    assertSourceDoesNotContain("src/options.cc", "zoom.change");
    assertSourceDoesNotContain("src/options.cc", "maxFramesPerSecond.change");
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

static void testCatalogLoadingSkipsDuplicateNamesWithoutCompressedAssetShellOut() {
    assertSourceContains("src/EffectChoiceLoader.cc", "option.defined(feat_name)");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "double_load");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "isCompressed");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "popen");
    assertSourceDoesNotContain("src/EffectChoiceLoader.cc", "pclose");
    assertSourceDoesNotContain("src/options.cc", "dbl-load");
    assertSourceDoesNotContain("src/Option.h", "double_load");
    assertSourceDoesNotContain("src/Configuration.h", "CatalogConfig");
}

static void testScreenshotFeatureWasRemoved() {
    assertSourceDoesNotContain("src/options.cc", "prt" "-file");
    assertSourceDoesNotContain("src/DisplayDevice.h", "print" "Screen");
    assertSourceDoesNotContain("src/DisplayDeviceX11.cc", "print" "Screen");
    assertSourceDoesNotContain("src/keymap.cc", "print" "Screen");
    assertSourceDoesNotContain("src/Configuration.h", "screenshot" "FilePrefix");
    assertSourceDoesNotContain("src/configuration_defaults.h",
        "SCREENSHOT" "_FILE_PREFIX");
    assertSourceDoesNotContain("src/CMakeLists.txt", "Screen" "shot.cc");
}

static void testLegacyTestOptionWasRemoved() {
    assertSourceDoesNotContain("src/options.cc", "opt_" "test");
    assertSourceDoesNotContain("src/options.cc", "\"test\"");
}

static void testApplicationProvidesStartupConfigSlices() {
    assertSourceContains("src/Application.cc",
        "startupConfigValue.app.optionsSaveEnabled");
    assertSourceContains("src/Application.cc", "configureKeys(startupConfigValue.input)");
    assertSourceContains("src/Application.cc", "Keymap::init(startupConfigValue.input)");
    assertSourceContains("src/Application.cc", "remove_continuation_ini(startupConfigValue.paths)");
    assertSourceContains("src/Application.cc", "configureAudioOptions(startupConfigValue.audio)");
    assertSourceContains("src/Application.cc", "configureCthughaDisplay(startupConfigValue.display)");
    assertSourceContains("src/Application.cc",
        "new OwnedAutoChangeSettings(startupConfigValue.autoChange)");
    assertSourceContains("src/Application.cc",
        "startupConfigValue.audioAnalysis.minNoise");
    assertSourceContains("src/Configuration.h", "struct AudioAnalysisConfig");
    assertSourceContains("src/Configuration.h", "AudioAnalysisConfig audioAnalysis");
    assertSourceDoesNotContain("src/Application.cc", "configureAudioAnalyzer");
    assertSourceContains("src/Application.cc", "configureEffectPolicy(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc", "configureTranslationOptions(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc", "configureWaveOptions(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc", "configurePaletteOptions(startupConfigValue.effectPolicy)");
    assertSourceContains("src/Application.cc",
        "audioProcessingSelectorValue->configureStartup(startupConfigValue.scene)");
    assertSourceContains("src/Application.cc",
        "videoDirector().configureTransitions(startupConfigValue.sceneTransition)");
    assertSourceContains("src/Application.cc",
        "videoDirector().configureQuietMessages(startupConfigValue.messages)");
    assertSourceContains("src/Application.cc", "sceneCommands().applyStartupConfig(startupConfigValue.scene)");
    assertSourceDoesNotContain("src/Application.cc", "Keymap::configure");
    assertSourceDoesNotContain("src/Application.cc", "EffectControl::changeToInitial");
    assertSourceDoesNotContain("src/Application.cc", "audioProcessing.changeToInitial");
    assertSourceDoesNotContain("src/Application.cc", "applyEffectPolicy");
    assertSourceDoesNotContain("src/Application.cc",
        "read_effect_control_usage_and_presets");
    assertSourceDoesNotContain("src/Application.cc", "configureApplicationOptions");
    assertSourceDoesNotContain("src/Option.h", "options_save");
    assertSourceDoesNotContain("src/Option.cc", "options_save");
    assertSourceDoesNotContain("src/Option.h", "configureApplicationOptions");
    assertSourceDoesNotContain("src/Option.cc", "configureApplicationOptions");
    assertSourceDoesNotContain("src/AutoChanger.cc", "configureQuietMessages");
    assertSourceDoesNotContain("src/AutoChanger.h", "MessagesConfig");
    assertSourceDoesNotContain("src/Configuration.h", "AutoChangeConfig {\n"
        "    int quietMs;\n"
        "    int waitMinMs;\n"
        "    int waitRandomMs;\n"
        "    int waitRandomMinimumMs;\n"
        "    int cumulativeFireLevel;\n"
        "    int locked;\n"
        "    int changeLittle;\n"
        "    int minNoise;\n"
        "    int quietMessageMs;");
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
    assertSourceDoesNotContain("src/options.cc", "audioProcessing.setInitialEntry");
    assertSourceDoesNotContain("src/IniFiles.cc", "effectControlGetIniInitials");
}

static void testIniPersistenceUsesRuntimePersistenceAdapter() {
    assertSourceContains("src/IniFiles.h", "struct ContinuationIniConfig");
    assertSourceContains("src/IniFiles.h", "int write_ini(const Config& config)");
    assertSourceDoesNotContain("src/IniFiles.h", "void configure_ini_persistence");
    assertSourceDoesNotContain("src/IniFiles.h", "int write_ini();");
    assertSourceDoesNotContain("src/IniFiles.h", "getini");
    assertSourceDoesNotContain("src/IniFiles.h", "putini");
    assertSourceContains("src/IniFiles.cc",
        "int write_ini(const Config& config)");
    assertSourceDoesNotContain("src/IniFiles.cc", "int write_ini()");
    assertSourceDoesNotContain("src/IniFiles.cc", "persisted_startup_config");
    assertSourceDoesNotContain("src/IniFiles.cc", "has_persisted_startup_config");
    assertSourceContains("src/IniFiles.cc",
        "int write_continuation_ini(const ContinuationIniConfig& config)");
    assertSourceContains("src/IniFiles.cc",
        "write_scene_config_ini(config.scene)");
    assertSourceContains("src/IniFiles.cc",
        "write_audio_analysis_config_ini(config.audioAnalysis)");
    assertSourceContains("src/IniFiles.cc",
        "write_auto_change_config_ini(config.autoChange)");
    assertSourceContains("src/IniFiles.cc",
        "write_effect_policy_ini(config.effectPolicy)");
    assertSourceContains("src/Application.cc",
        "RuntimeConfigRegistry(startupConfigValue)");
    assertSourceContains("src/Application.cc",
        "new IniRuntimePersistence(*runtimeConfigRegistryValue)");
    assertSourceContains("src/Application.cc",
        "runtimePersistenceValue->writeCurrentConfig()");
    assertSourceDoesNotContain("src/Application.cc", "configure_ini_persistence");
    assertSourceDoesNotContain("src/Application.cc", "write_ini(startupConfigValue)");
    assertSourceDoesNotContain("src/Application.cc",
        "write_ini(runtimeConfigRegistryValue->currentConfig())");
    assertSourceContains("src/RuntimeChangeMediator.h",
        "RuntimePersistence& runtimePersistence_");
    assertSourceContains("src/RuntimeChangeMediator.h",
        "RuntimeShutdown& runtimeShutdown_");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "runtimePersistence.writeCurrentConfig()");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "IniFiles.h");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "RuntimeConfigRegistry.h");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "write_ini()");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "write_ini(");
    assertSourceContains("src/keymap.cc",
        "RuntimeCommand::stopAndContinue");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "runtimePersistence.writeContinuation(command.continuation)");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "write_continuation_ini(");
    assertSourceContains("src/RuntimePersistence.cc",
        "write_ini(runtimeConfigRegistry.currentConfig())");
    assertSourceContains("src/RuntimePersistence.cc",
        "write_continuation_ini(");
    assertSourceContains("src/LegacyRuntimeConfigContributor.cc",
        "audioProcessingState.text()");
    assertSourceDoesNotContain("src/LegacyRuntimeConfigContributor.cc",
        "audioProcessing.text()");
    assertSourceContains("src/keymap.cc",
        "Keymap::audioProcessingState()");
    assertSourceDoesNotContain("src/keymap.cc",
        "audioProcessing.text()");
    assertSourceDoesNotContain("src/AutoChanger.cc", "write_ini");
    assertSourceDoesNotContain("src/AutoChanger.cc", "options_save");
    assertSourceDoesNotExist("src/EffectControlIni.cc");
    assertSourceDoesNotExist("src/EffectControlIni.h");
    assertSourceDoesNotContain("src/CMakeLists.txt", "EffectControlIni.cc");
    assertSourceDoesNotContain("src/IniFiles.cc", "int getini");
    assertSourceDoesNotContain("src/IniFiles.cc", "move_ini_file");
    assertSourceDoesNotContain("src/IniFiles.cc", "is_in_ini");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(audioProcessing)");
    assertSourceDoesNotContain("src/IniFiles.cc", "showFPS");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(changeWait");
    assertSourceDoesNotContain("src/IniFiles.cc",
        "putini(changeCumulativeFireLevel");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(changeQuiet");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(changeMsgTime");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(lock)");
    assertSourceDoesNotContain("src/IniFiles.cc", "putini(change_little)");
    assertSourceDoesNotContain("src/IniFiles.cc", "effectControlPutIniUsages");
    assertSourceDoesNotContain("src/IniFiles.cc", "effectControlPutPresetIni");
}

static void testRuntimeLifecycleRequestsUseMediator() {
    assertSourceContains("src/keymap.cc", "RuntimeCommand::requestClose()");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "RuntimeCommand::requestClose()");
    assertSourceContains("src/InterfaceCredits.cc",
        "RuntimeCommand::requestClose()");
    assertSourceDoesNotContain("src/DisplayDeviceX11-Panel.cc", "cthugha_close++");
    assertSourceDoesNotContain("src/InterfaceCredits.cc", "cthugha_close++");
    assertSourceDoesNotExist("src/AudioRuntime.cc");
    assertSourceContains("src/Application.cc", "new RuntimeCloseState()");
    assertSourceContains("src/Application.cc",
        "runtimeShutdownValue->requestClose()");
    assertSourceContains("src/Application.cc", "closeRequested()");
    assertSourceDoesNotContain("src/Application.cc", "cthugha_close++");
    assertSourceDoesNotContain("src/Application.cc", "cthugha_close == 0");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "runtimeShutdown.requestClose()");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "cthugha_close++");
    assertSourceContains("src/RuntimeShutdown.cc", "closeRequestedValue");
    assertSourceDoesNotContain("src/RuntimeShutdown.cc", "cthugha_close++");
}

static void testRuntimeCommandsUseSubsystemControlPorts() {
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeDisplayControls()");
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeAudioControls(*audioProcessingSelectorValue)");
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeAutoChangeControls(*autoChangeControlsValue)");
    assertSourceContains("src/Application.cc",
        "new DefaultRuntimeEffectControls()");
    assertSourceDoesNotContain("src/Application.cc",
        "new DefaultRuntimeOptionControls()");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeDisplayControls.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeAudioControls.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeAutoChangeControls.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeEffectControls.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"RuntimeOptionControls.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "displayControls.changePresentationBy(command.value)");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "audioControls.changeSoundProcessingBy(command.value)");
    assertSourceDoesNotContain("src/RuntimeCommand.h",
        "RuntimeCommandResetAudioFrame");
    assertSourceDoesNotContain("src/RuntimeCommand.h",
        "resetAudioFrame");
    assertSourceDoesNotContain("src/RuntimeCommandSink.h",
        "audioResetRequested");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "RuntimeCommandResetAudioFrame");
    assertSourceDoesNotContain("src/keymap.cc", "soundReset");
    assertSourceDoesNotContain("src/default.keymap", "soundReset");
    assertSourceDoesNotContain("src/keymap.cc",
        "#include \"AudioAnalyzer.h\"");
    assertSourceDoesNotContain("src/keymap.cc",
        "#include \"AudioFrame.h\"");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "autoChangeControls.toggleLock()");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "displayControls.changeDisplayOptionBy(option, by, changes)");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "audioControls.changeAudioOptionBy(option, by, changes)");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "autoChangeControls.changeAutoChangeOptionBy(option, by, changes)");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "effectControls.changeEffectControlBy(");
    assertSourceContains("src/RuntimeChangeMediator.cc",
        "effectControls.toggleEffectChoiceUse(");
    assertSourceContains("src/InterfaceList.cc",
        "RuntimeCommand::toggleEffectChoiceUse");
    assertSourceDoesNotExist("src/RuntimeOptionControls.cc");
    assertSourceDoesNotExist("src/RuntimeOptionControls.h");
    assertSourceDoesNotExist("src/RuntimeEffectCatalogControls.cc");
    assertSourceDoesNotExist("src/RuntimeEffectCatalogControls.h");
    assertSourceDoesNotContain("src/CMakeLists.txt",
        "RuntimeOptionControls.cc");
    assertSourceDoesNotContain("src/CMakeLists.txt",
        "RuntimeEffectCatalogControls.cc");
    assertSourceContains("src/CMakeLists.txt",
        "RuntimeEffectControls.cc");
    assertSourceDoesNotContain("src/RuntimeAudioControls.cc",
        "#include \"AudioAnalyzer.h\"");
    assertSourceDoesNotContain("src/RuntimeAudioControls.cc",
        "#include \"AudioFrame.h\"");
    assertSourceDoesNotContain("src/RuntimeAudioControls.cc",
        "sound_minnoise");
    assertSourceContains("src/RuntimeAudioControls.cc",
        "audioProcessingSelector.changeBy");
    assertSourceContains("src/RuntimeAudioControls.cc",
        "audioProcessingSelector.changeTo");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"AudioFrame.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"AudioProcessor.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"AutoChanger.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"CthughaDisplay.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"EffectControl.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"Option.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "#include \"Screen.h\"");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "audioProcessing.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "audioFrameChange");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "lock.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "showFPS.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "screen.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc",
        "zoom.change");
    assertSourceDoesNotContain("src/RuntimeChangeMediator.cc", "&screen");
}

static void testX11PanelInputsUseRuntimeCommands() {
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "RuntimeCommand::savePaletteMetadata");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "RuntimeCommand::revertPaletteMetadata");
    assertSourceDoesNotContain("src/DisplayDeviceX11-Panel.cc",
        "d->opt->setValue");
}

static void testSelectionDisplaysUseRuntimeConfigRegistry() {
    assertSourceContains("src/Interface.cc",
        "registry->currentConfig()");
    assertSourceContains("src/Interface.cc",
        "runtimeConfigSelectionTextOrFallback");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "runtimeConfigRegistry.currentConfig()");
    assertSourceContains("src/DisplayDeviceX11-Panel.cc",
        "updatePanelSelectionLabels()");
    assertSourceContains("src/Application.cc",
        "Interface::setRuntimeConfigRegistry(runtimeConfigRegistryValue.get())");
    assertSourceContains("src/Application.cc",
        "*runtimeConfigRegistryValue,\n        startupConfigValue.display");
}

static void testInterfaceInputsDoNotUseLegacyFallbacks() {
    assertSourceDoesNotContain("src/Interface.cc",
        "currentEffectControl->change");
    assertSourceDoesNotContain("src/Interface.cc",
        "currentOption->change");
    assertSourceDoesNotContain("src/Interface.cc",
        "currentEffectControl->lock.change");
    assertSourceDoesNotContain("src/InterfaceList.cc",
        "currentOption->change");
    assertSourceDoesNotContain("src/InterfaceList.cc",
        "currentEffectControl->setValue");
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
}

int main() {
    testAudioIngestUsesAudioConfig();
    testAudioDeviceSettingsAreStartupOnly();
    testAudioFrameOwnsPerFrameMetrics();
    testDisplayStartupUsesDisplayConfig();
    testX11StartupUsesX11ConfigOnlyForX11Builds();
    testLoggingUsesLoggingConfig();
    testAudioInputFileGlobalWasDeleted();
    testLegacyParserDoesNotWritePortedConfigValues();
    testCatalogLoadingUsesPathConfig();
    testCatalogLoadingSkipsDuplicateNamesWithoutCompressedAssetShellOut();
    testScreenshotFeatureWasRemoved();
    testLegacyTestOptionWasRemoved();
    testApplicationProvidesStartupConfigSlices();
    testAutoChangeSettingsAreApplicationOwned();
    testInputStartupUsesInputConfig();
    testSceneStartupUsesSceneConfig();
    testIniPersistenceUsesRuntimePersistenceAdapter();
    testRuntimeLifecycleRequestsUseMediator();
    testRuntimeCommandsUseSubsystemControlPorts();
    testX11PanelInputsUseRuntimeCommands();
    testSelectionDisplaysUseRuntimeConfigRegistry();
    testInterfaceInputsDoNotUseLegacyFallbacks();
    testConfigDefaultsAreNotConsumedAsLegacyDefaults();
    return 0;
}
