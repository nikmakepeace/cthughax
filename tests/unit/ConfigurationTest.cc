#include "Configuration.h"

#include "configuration_defaults.h"
#include "defaults.h"

#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

static const std::string* patchValue(const ConfigPatch& patch,
    const std::string& key) {
    const std::string* value = patch.value(key);
    assert(value != NULL);
    return value;
}

static const std::vector<ConfigEntry>* patchEntries(const ConfigPatch& patch,
    const std::string& key) {
    const std::vector<ConfigEntry>* entries = patch.entriesFor(key);
    assert(entries != NULL);
    return entries;
}

static void defaultsProduceTypedConfig() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults().build();

    assert(result.ok());
    assert(result.config.logging.verbosity == LOGGING_CONFIG_DEFAULT_VERBOSITY);
    assert(result.config.app.optionsSaveEnabled == APP_CONFIG_DEFAULT_OPTIONS_SAVE_ENABLED);
    assert(result.config.input.escapeKeyEnabled == INPUT_CONFIG_DEFAULT_ESCAPE_KEY_ENABLED);
    assert(result.config.input.keymapFile == INPUT_CONFIG_DEFAULT_KEYMAP_FILE_PATH);
    assert(result.config.paths.extraLibraryPath == PATH_CONFIG_DEFAULT_EXTRA_LIBRARY_PATH);
    assert(result.config.paths.iniFileOverride == PATH_CONFIG_DEFAULT_INI_FILE_OVERRIDE_PATH);
    assert(result.config.catalogs.doubleLoadEnabled == CATALOG_CONFIG_DEFAULT_DOUBLE_LOAD_ENABLED);
    assert(result.config.scene.flame.empty());
    assert(result.config.scene.generalFlame.empty());
    assert(result.config.scene.wave.empty());
    assert(result.config.scene.waveScale.empty());
    assert(result.config.scene.object.empty());
    assert(result.config.scene.translation.empty());
    assert(result.config.scene.palette.empty());
    assert(result.config.scene.border.empty());
    assert(result.config.scene.flashlight.empty());
    assert(result.config.scene.table.empty());
    assert(result.config.scene.image.empty());
    assert(result.config.scene.presentation.empty());
    assert(result.config.audio.inputMode == AUDIO_CONFIG_DEFAULT_INPUT_MODE);
    assert(result.config.audio.inputFile == AUDIO_CONFIG_DEFAULT_INPUT_FILE_PATH);
    assert(result.config.audio.inputLoopEnabled == AUDIO_CONFIG_DEFAULT_INPUT_LOOP_ENABLED);
    assert(result.config.audio.sampleRateHz == AUDIO_CONFIG_DEFAULT_SAMPLE_RATE_HZ);
    assert(result.config.audio.channels == AUDIO_CONFIG_DEFAULT_CHANNELS);
    assert(result.config.audio.sampleFormat == AUDIO_CONFIG_DEFAULT_FORMAT);
    assert(result.config.audio.dspMethod == AUDIO_CONFIG_DEFAULT_DSP_METHOD);
    assert(result.config.audio.dspFragments == AUDIO_CONFIG_DEFAULT_DSP_FRAGMENTS);
    assert(result.config.audio.dspFragmentSize == AUDIO_CONFIG_DEFAULT_DSP_FRAGMENT_SIZE);
    assert(result.config.audio.dspSyncEnabled == AUDIO_CONFIG_DEFAULT_DSP_SYNC_ENABLED);
    assert(result.config.audio.silentEnabled == AUDIO_CONFIG_DEFAULT_SILENT_ENABLED);
    assert(result.config.audio.minNoise == AUDIO_CONFIG_DEFAULT_MIN_NOISE);
    assert(result.config.audio.pulseLatencyMs == AUDIO_CONFIG_DEFAULT_PULSE_LATENCY_MS);
    assert(result.config.audio.pulseServer == AUDIO_CONFIG_DEFAULT_PULSE_SERVER_TEXT);
    assert(result.config.audio.outputDumpPath == AUDIO_CONFIG_DEFAULT_OUTPUT_DUMP_PATH);
    assert(result.config.audio.dspDevicePath == AUDIO_CONFIG_DEFAULT_DSP_DEVICE_PATH);
    assert(result.config.audio.mixerDevicePath == AUDIO_CONFIG_DEFAULT_MIXER_DEVICE_PATH);
    assert(result.config.audio.mixerInitialVolumes.empty());
    assert(result.config.audio.nullOutputTargetLatencyMs == AUDIO_CONFIG_DEFAULT_NULL_TARGET_LATENCY_MS);
    assert(result.config.audio.pulseOutputTargetLatencyMs == AUDIO_CONFIG_DEFAULT_PULSE_TARGET_LATENCY_MS);
    assert(result.config.audio.dspOutputTargetLatencyMs == AUDIO_CONFIG_DEFAULT_DSP_TARGET_LATENCY_MS);
    assert(result.config.display.displayMode == DISPLAY_CONFIG_DEFAULT_MODE);
    assert(!result.config.display.hasCustomDisplaySize);
    assert(result.config.display.bufferWidth == 160);
    assert(result.config.display.bufferHeight == 100);
    assert(result.config.display.maxFramesPerSecond == DISPLAY_CONFIG_DEFAULT_MAX_FRAMES_PER_SECOND);
    assert(result.config.display.showFpsEnabled == DISPLAY_CONFIG_DEFAULT_SHOW_FPS_ENABLED);
    assert(result.config.display.zoomMode == DISPLAY_CONFIG_DEFAULT_ZOOM_MODE);
    assert(result.config.display.ncursesEnabled == DISPLAY_CONFIG_DEFAULT_NCURSES_ENABLED);
    assert(result.config.display.screenshotFilePrefix == DISPLAY_CONFIG_DEFAULT_SCREENSHOT_FILE_PREFIX);
    assert(result.config.autoChange.quietMs == AUTO_CHANGE_CONFIG_DEFAULT_QUIET_MS);
    assert(result.config.autoChange.waitMinMs == AUTO_CHANGE_CONFIG_DEFAULT_WAIT_MIN_MS);
    assert(result.config.autoChange.waitRandomMs == AUTO_CHANGE_CONFIG_DEFAULT_WAIT_RANDOM_MS);
    assert(result.config.autoChange.waitRandomMinimumMs == AUTO_CHANGE_CONFIG_DEFAULT_WAIT_RANDOM_MIN_MS);
    assert(result.config.visual.changeMessageMs == VISUAL_CONFIG_DEFAULT_CHANGE_MESSAGE_MS);
    assert(result.config.visual.quietMessageDurationMs == VISUAL_CONFIG_DEFAULT_QUIET_MESSAGE_DURATION_MS);
    assert(result.config.visual.paletteSmoothingChance == VISUAL_CONFIG_DEFAULT_PALETTE_SMOOTHING_CHANCE);
    assert(result.config.visual.paletteSmoothSeconds == VISUAL_CONFIG_DEFAULT_PALETTE_SMOOTH_SECONDS);
    assert(result.config.visual.imageLoadingEnabled == VISUAL_CONFIG_DEFAULT_IMAGE_LOADING_ENABLED);
    assert(result.config.visual.useTranslatesEnabled == VISUAL_CONFIG_DEFAULT_USE_TRANSLATES_ENABLED);
    assert(result.config.visual.useObjectsEnabled == VISUAL_CONFIG_DEFAULT_USE_OBJECTS_ENABLED);
    assert(result.config.messages.qotdPrefetchTimeoutMs == MESSAGES_CONFIG_DEFAULT_QOTD_PREFETCH_TIMEOUT_MS);
    assert(result.config.messages.qotdServer == MESSAGES_CONFIG_DEFAULT_QOTD_SERVER_TEXT);
    assert(result.config.messages.qotdPort == MESSAGES_CONFIG_DEFAULT_QOTD_PORT_TEXT);
}

static void iniTextSourceProducesPatchWithoutGlobals() {
    DeferredLogBuffer diagnostics;
    IniTextConfigSource source("memory",
        "cthugha.verbose: 3\n"
        "cthugha.path: /tmp/cth\n"
        "cthugha.keymap: custom.keymap\n"
        "cthugha.no-esc: yes\n"
        "cthugha.flame: fuzz\n"
        "cthugha.flame-general: 42\n"
        "cthugha.wave: line\n"
        "cthugha.wave-scale: scale1\n"
        "cthugha.object: cube\n"
        "cthugha.translation: swirl\n"
        "cthugha.palette: fire\n"
        "cthugha.border: border1\n"
        "cthugha.flashlight: on\n"
        "cthugha.table: table3\n"
        "cthugha.image: splash\n"
        "cthugha.display: roll\n"
        "cthugha.play: song.wav\n"
        "cthugha.loop: no\n"
        "cthugha.rate: 22050\n"
        "cthugha.no-stereo: yes\n"
        "cthugha.snd-format: s16le\n"
        "cthugha.snd-method: 2\n"
        "cthugha.snd-fragments: 8\n"
        "cthugha.sound-fragment-size: 10\n"
        "cthugha.snd-sync: on\n"
        "cthugha.silent: yes\n"
        "cthugha.min-noise: 12\n"
        "cthugha.pulse-latency-ms: 150\n"
        "cthugha.pulse-server: local\n"
        "cthugha.audio-output-dump: dump.raw\n"
        "cthugha.dev-dsp: /tmp/dsp\n"
        "cthugha.dev-mixer: /tmp/mixer\n"
        "cthugha.line: 12\n"
        "cthugha.mic: 34\n"
        "cthugha.mixer: pcm:56\n"
        "cthugha.disp-mode: 800x600\n"
        "cthugha.buff-size: 2\n");
    ConfigPatch patch = source.acquire(diagnostics);

    assert(diagnostics.diagnostics().empty());
    assert(*patchValue(patch, "logging.verbosity") == "3");
    assert(*patchValue(patch, "paths.extra_library") == "/tmp/cth/");
    assert(*patchValue(patch, "input.keymap_file") == "custom.keymap");
    assert(*patchValue(patch, "input.escape_key_enabled") == "0");
    assert(*patchValue(patch, "scene.flame") == "fuzz");
    assert(*patchValue(patch, "scene.general_flame") == "42");
    assert(*patchValue(patch, "scene.wave") == "line");
    assert(*patchValue(patch, "scene.wave_scale") == "scale1");
    assert(*patchValue(patch, "scene.object") == "cube");
    assert(*patchValue(patch, "scene.translation") == "swirl");
    assert(*patchValue(patch, "scene.palette") == "fire");
    assert(*patchValue(patch, "scene.border") == "border1");
    assert(*patchValue(patch, "scene.flashlight") == "on");
    assert(*patchValue(patch, "scene.table") == "table3");
    assert(*patchValue(patch, "scene.image") == "splash");
    assert(*patchValue(patch, "scene.presentation") == "roll");
    assert(*patchValue(patch, "audio.input_mode") == "2");
    assert(*patchValue(patch, "audio.input_file") == "song.wav");
    assert(*patchValue(patch, "audio.input_loop") == "0");
    assert(*patchValue(patch, "audio.sample_rate_hz") == "22050");
    assert(*patchValue(patch, "audio.channels") == "1");
    assert(*patchValue(patch, "audio.sample_format") == "s16le");
    assert(*patchValue(patch, "audio.dsp_method") == "2");
    assert(*patchValue(patch, "audio.dsp_fragments") == "8");
    assert(*patchValue(patch, "audio.dsp_fragment_size") == "10");
    assert(*patchValue(patch, "audio.dsp_sync") == "1");
    assert(*patchValue(patch, "audio.silent") == "1");
    assert(*patchValue(patch, "audio.min_noise") == "12");
    assert(*patchValue(patch, "audio.pulse_latency_ms") == "150");
    assert(*patchValue(patch, "audio.pulse_server") == "local");
    assert(*patchValue(patch, "audio.output_dump_path") == "dump.raw");
    assert(*patchValue(patch, "audio.dsp_device_path") == "/tmp/dsp");
    assert(*patchValue(patch, "audio.mixer_device_path") == "/tmp/mixer");
    const std::vector<ConfigEntry>* mixerInitials
        = patchEntries(patch, "audio.mixer_initial_volume");
    assert(mixerInitials->size() == 3);
    assert((*mixerInitials)[0].value == "line=3084");
    assert((*mixerInitials)[1].value == "mic=8738");
    assert((*mixerInitials)[2].value == "pcm=56");
    assert(*patchValue(patch, "display.mode") == "-1");
    assert(*patchValue(patch, "display.width") == "800");
    assert(*patchValue(patch, "display.height") == "600");
    assert(*patchValue(patch, "buffer.preset") == "2");
}

static void sourcePrecedenceIsDefaultsIniEnvironmentCommandLine() {
    std::map<std::string, std::string> environment;
    environment["CTH_VERBOSE"] = "5";

    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addIniText("memory", "cthugha.verbose: 2\n")
        .addEnvironment(environment)
        .addCommandLine(std::vector<std::string>{ "cthugha", "--verbose=7" })
        .build();

    assert(result.ok());
    assert(result.config.logging.verbosity == 7);

    ConfigurationBuilder noVerboseBuilder;
    result = noVerboseBuilder.addDefaults()
                 .addIniText("memory", "cthugha.verbose: 2\n")
                 .addEnvironment(environment)
                 .addCommandLine(
                     std::vector<std::string>{ "cthugha", "--no-verbose" })
                 .build();

    assert(result.ok());
    assert(result.config.logging.verbosity == 0);
}

static void commandLineSourceBuildsInputConfig() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--no-esc",
            "--keymap",
            "custom.keymap",
        })
        .build();

    assert(result.ok());
    assert(result.config.input.escapeKeyEnabled == 0);
    assert(result.config.input.keymapFile == "custom.keymap");

    ConfigurationBuilder escBuilder;
    result = escBuilder.addDefaults()
                 .addCommandLine(std::vector<std::string>{
                     "cthugha",
                     "--no-esc",
                     "--esc",
                     "--keymap=other.keymap",
                 })
                 .build();

    assert(result.ok());
    assert(result.config.input.escapeKeyEnabled == 1);
    assert(result.config.input.keymapFile == "other.keymap");
}

static void commandLineSourceHandlesAudioLastWriterWins() {
    DeferredLogBuffer diagnostics;
    CommandLineConfigSource source(std::vector<std::string>{
        "cthugha",
        "--play",
        "song.wav",
        "--random-noise",
        "--no-sound",
    });
    ConfigPatch patch = source.acquire(diagnostics);

    assert(diagnostics.diagnostics().empty());
    assert(*patchValue(patch, "audio.input_mode") == "3");
    assert(*patchValue(patch, "audio.input_file") == "");

    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults().addSource(source).build();
    assert(result.ok());
    assert(result.config.audio.inputMode == AIM_None);
    assert(result.config.audio.inputFile.empty());
}

static void commandLineSourceBuildsFullAudioConfig() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--play",
            "song.wav",
            "--no-loop",
            "--rate",
            "48000",
            "--no-stereo",
            "--snd-format",
            "16bit signed (le)",
            "--snd-method",
            "2",
            "--snd-fragments",
            "8",
            "--snd-fragment-size",
            "10",
            "--snd-sync",
            "--silent",
            "--min-noise=300",
            "--pulse-latency-ms=40",
            "--pulse-server",
            "local",
            "--audio-output-dump",
            "dump.raw",
            "--dev-dsp",
            "/tmp/dsp",
            "--dev-mixer",
            "/tmp/mixer",
            "--line",
            "12",
            "-M34",
            "--mixer",
            "pcm:56",
        })
        .build();

    assert(result.ok());
    assert(result.config.audio.inputMode == AIM_File);
    assert(result.config.audio.inputFile == "song.wav");
    assert(result.config.audio.inputLoopEnabled == 0);
    assert(result.config.audio.sampleRateHz == 48000);
    assert(result.config.audio.channels == 1);
    assert(result.config.audio.sampleFormat == SF_s16_le);
    assert(result.config.audio.dspMethod == 2);
    assert(result.config.audio.dspFragments == 8);
    assert(result.config.audio.dspFragmentSize == 10);
    assert(result.config.audio.dspSyncEnabled == 1);
    assert(result.config.audio.silentEnabled == 1);
    assert(result.config.audio.minNoise == 255);
    assert(result.config.audio.pulseLatencyMs == 50);
    assert(result.config.audio.pulseServer == "local");
    assert(result.config.audio.outputDumpPath == "dump.raw");
    assert(result.config.audio.dspDevicePath == "/tmp/dsp");
    assert(result.config.audio.mixerDevicePath == "/tmp/mixer");
    assert(result.config.audio.mixerInitialVolumes.size() == 3);
    assert(result.config.audio.mixerInitialVolumes[0].name == "line");
    assert(result.config.audio.mixerInitialVolumes[0].volume == 3084);
    assert(result.config.audio.mixerInitialVolumes[1].name == "mic");
    assert(result.config.audio.mixerInitialVolumes[1].volume == 8738);
    assert(result.config.audio.mixerInitialVolumes[2].name == "pcm");
    assert(result.config.audio.mixerInitialVolumes[2].volume == 56);
    assert(result.diagnostics.size() == 2);
    assert(result.diagnostics[0].severity == ConfigDiagnosticWarning);
    assert(result.diagnostics[1].severity == ConfigDiagnosticWarning);
}

static void commandLineSourceBuildsSceneConfig() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--flame",
            "fuzz",
            "--flame-general=42",
            "--wave",
            "line",
            "--wave-scale",
            "scale1",
            "--object",
            "cube",
            "--translation",
            "swirl",
            "--palette",
            "fire",
            "--border",
            "border1",
            "--flashlight",
            "--table",
            "table3",
            "--image",
            "splash",
            "--display",
            "roll",
        })
        .build();

    assert(result.ok());
    assert(result.config.scene.flame == "fuzz");
    assert(result.config.scene.generalFlame == "42");
    assert(result.config.scene.wave == "line");
    assert(result.config.scene.waveScale == "scale1");
    assert(result.config.scene.object == "cube");
    assert(result.config.scene.translation == "swirl");
    assert(result.config.scene.palette == "fire");
    assert(result.config.scene.border == "border1");
    assert(result.config.scene.flashlight
        == DEFAULT_FLASHLIGHT_ENABLE_INITIAL_ENTRY);
    assert(result.config.scene.table == "table3");
    assert(result.config.scene.image == "splash");
    assert(result.config.scene.presentation == "roll");

    ConfigurationBuilder noFlashlightBuilder;
    result = noFlashlightBuilder.addDefaults()
                 .addCommandLine(std::vector<std::string>{
                     "cthugha",
                     "--no-flashlight",
                 })
                 .build();
    assert(result.ok());
    assert(result.config.scene.flashlight
        == DEFAULT_FLASHLIGHT_DISABLE_INITIAL_ENTRY);
}

static void commandLineSourceHandlesDisplayAndBufferSettings() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--disp-mode",
            "800x600",
            "--buff-size",
            "2",
        })
        .build();

    assert(result.ok());
    assert(result.config.display.hasCustomDisplaySize);
    assert(result.config.display.displayMode == -1);
    assert(result.config.display.displayWidth == 800);
    assert(result.config.display.displayHeight == 600);
    assert(result.config.display.bufferWidth == 400);
    assert(result.config.display.bufferHeight == 300);
    assert(!result.config.display.hasCustomBufferSize);
}

static void customBufferSizeIsClampedWithDeferredWarnings() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--buff-size=2000x12",
        })
        .build();

    assert(result.ok());
    assert(result.config.display.bufferWidth == 1024);
    assert(result.config.display.bufferHeight == 64);
    assert(result.config.display.hasCustomBufferSize);
    assert(result.diagnostics.size() == 2);
    assert(result.diagnostics[0].severity == ConfigDiagnosticWarning);
    assert(result.diagnostics[1].severity == ConfigDiagnosticWarning);
}

static void invalidTypedValueProducesDeferredError() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addIniText("broken", "cthugha.verbose: nope\n")
        .build();

    assert(!result.ok());
    assert(!result.diagnostics.empty());
    assert(result.diagnostics[0].severity == ConfigDiagnosticError);
}

int main() {
    fprintf(stderr, "%s", "");
    fflush(stderr);
    assert(configArgumentsFromArgv(0, NULL).empty());
    defaultsProduceTypedConfig();
    iniTextSourceProducesPatchWithoutGlobals();
    sourcePrecedenceIsDefaultsIniEnvironmentCommandLine();
    commandLineSourceBuildsInputConfig();
    commandLineSourceHandlesAudioLastWriterWins();
    commandLineSourceBuildsFullAudioConfig();
    commandLineSourceBuildsSceneConfig();
    commandLineSourceHandlesDisplayAndBufferSettings();
    customBufferSizeIsClampedWithDeferredWarnings();
    invalidTypedValueProducesDeferredError();

    return 0;
}
