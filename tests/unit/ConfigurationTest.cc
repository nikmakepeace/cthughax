/** @file
 * Unit tests for startup configuration parsing and typed conversion.
 */

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
    assert(result.config.scene.audioProcessing.empty());
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
    assert(result.config.audio.outputDriver == AUDIO_CONFIG_DEFAULT_OUTPUT_DRIVER);
    assert(result.config.audio.pulseLatencyMs == AUDIO_CONFIG_DEFAULT_PULSE_LATENCY_MS);
    assert(result.config.audio.pulseServer == AUDIO_CONFIG_DEFAULT_PULSE_SERVER_TEXT);
    assert(result.config.audio.outputDumpPath == AUDIO_CONFIG_DEFAULT_OUTPUT_DUMP_PATH);
    assert(result.config.audio.miniAudioPlaybackDeviceName == AUDIO_CONFIG_DEFAULT_MINIAUDIO_PLAYBACK_DEVICE_NAME);
    assert(result.config.audio.miniAudioCaptureDeviceName == AUDIO_CONFIG_DEFAULT_MINIAUDIO_CAPTURE_DEVICE_NAME);
    assert(result.config.audio.dspDevicePath == AUDIO_CONFIG_DEFAULT_DSP_DEVICE_PATH);
    assert(result.config.audio.mixerDevicePath == AUDIO_CONFIG_DEFAULT_MIXER_DEVICE_PATH);
    assert(result.config.audio.mixerInitialVolumes.empty());
    assert(result.config.audio.nullOutputTargetLatencyMs == AUDIO_CONFIG_DEFAULT_NULL_TARGET_LATENCY_MS);
    assert(result.config.audio.pulseOutputTargetLatencyMs == AUDIO_CONFIG_DEFAULT_PULSE_TARGET_LATENCY_MS);
    assert(result.config.audio.miniAudioOutputTargetLatencyMs == AUDIO_CONFIG_DEFAULT_MINIAUDIO_TARGET_LATENCY_MS);
    assert(result.config.audio.dspOutputTargetLatencyMs == AUDIO_CONFIG_DEFAULT_DSP_TARGET_LATENCY_MS);
    assert(result.config.display.driver == DisplayDriverAuto);
    assert(result.config.display.displayMode == DISPLAY_CONFIG_DEFAULT_MODE);
    assert(!result.config.display.hasCustomDisplaySize);
    assert(result.config.display.bufferWidth == 160);
    assert(result.config.display.bufferHeight == 100);
    assert(result.config.display.maxFramesPerSecond == DISPLAY_CONFIG_DEFAULT_MAX_FRAMES_PER_SECOND);
    assert(result.config.display.showFpsEnabled == DISPLAY_CONFIG_DEFAULT_SHOW_FPS_ENABLED);
    assert(result.config.display.zoomMode == DISPLAY_CONFIG_DEFAULT_ZOOM_MODE);
    assert(result.config.sdl3.highPixelDensityEnabled == SDL3_CONFIG_DEFAULT_HIGH_PIXEL_DENSITY_ENABLED);
    assert(result.config.sdl3.resizableWindowEnabled == SDL3_CONFIG_DEFAULT_RESIZABLE_WINDOW_ENABLED);
    assert(result.config.sdl3.rendererName == SDL3_CONFIG_DEFAULT_RENDERER_NAME);
    assert(result.config.sdl3.frameDumpDirectory == SDL3_CONFIG_DEFAULT_FRAME_DUMP_DIRECTORY);
    assert(result.config.sdl3.frameDumpLimit == SDL3_CONFIG_DEFAULT_FRAME_DUMP_LIMIT);
    assert(result.config.sdl3.frameDumpEvery == SDL3_CONFIG_DEFAULT_FRAME_DUMP_EVERY);
    assert(result.config.autoChange.quietMs == AUTO_CHANGE_CONFIG_DEFAULT_QUIET_MS);
    assert(result.config.autoChange.waitMinMs == AUTO_CHANGE_CONFIG_DEFAULT_WAIT_MIN_MS);
    assert(result.config.autoChange.waitRandomMs == AUTO_CHANGE_CONFIG_DEFAULT_WAIT_RANDOM_MS);
    assert(result.config.autoChange.waitRandomMinimumMs == AUTO_CHANGE_CONFIG_DEFAULT_WAIT_RANDOM_MIN_MS);
    assert(result.config.autoChange.cumulativeFireLevel == AUTO_CHANGE_CONFIG_DEFAULT_CUMULATIVE_FIRE_LEVEL);
    assert(result.config.autoChange.locked == AUTO_CHANGE_CONFIG_DEFAULT_LOCKED);
    assert(result.config.autoChange.changeLittle == AUTO_CHANGE_CONFIG_DEFAULT_CHANGE_LITTLE);
    assert(result.config.audioAnalysis.minNoise == AUDIO_ANALYSIS_CONFIG_DEFAULT_MIN_NOISE);
    assert(result.config.audioAnalysis.fireSensitivity
        == AUDIO_ANALYSIS_CONFIG_DEFAULT_FIRE_SENSITIVITY);
    assert(result.config.audioAnalysis.fireSource
        == AUDIO_ANALYSIS_CONFIG_DEFAULT_FIRE_SOURCE_TEXT);
    assert(result.config.effectPolicy.imageFilesEnabled == EFFECT_POLICY_DEFAULT_IMAGE_FILES_ENABLED);
    assert(result.config.effectPolicy.paletteSetFilterText == EFFECT_POLICY_DEFAULT_PALETTE_SET_FILTER_TEXT);
    assert(result.config.effectPolicy.useTranslatesEnabled == EFFECT_POLICY_DEFAULT_USE_TRANSLATES_ENABLED);
    assert(result.config.effectPolicy.useObjectsEnabled == EFFECT_POLICY_DEFAULT_USE_OBJECTS_ENABLED);
    assert(result.config.effectPolicy.allowedChoices.empty());
    assert(result.config.effectPolicy.presets.empty());
    assert(result.config.sceneTransition.paletteSmoothingChance == SCENE_TRANSITION_POLICY_DEFAULT_PALETTE_SMOOTHING_CHANCE);
    assert(result.config.sceneTransition.paletteSmoothSeconds == SCENE_TRANSITION_POLICY_DEFAULT_PALETTE_SMOOTH_SECONDS);
    assert(result.config.sceneScript.directory.empty());
    assert(result.config.sceneScript.script.empty());
    assert(result.config.messages.quietMessageMs == MESSAGES_CONFIG_DEFAULT_QUIET_MESSAGE_MS);
    assert(result.config.messages.quietMessageDurationMs == MESSAGES_CONFIG_DEFAULT_QUIET_MESSAGE_DURATION_MS);
    assert(result.config.messages.quietMessageFile == MESSAGES_CONFIG_DEFAULT_QUIET_MESSAGE_FILE_PATH);
    assert(result.config.messages.qotdEnabled == MESSAGES_CONFIG_DEFAULT_QOTD_ENABLED);
    assert(result.config.messages.qotdPrefetchTimeoutMs == MESSAGES_CONFIG_DEFAULT_QOTD_PREFETCH_TIMEOUT_MS);
    assert(result.config.messages.qotdServer == MESSAGES_CONFIG_DEFAULT_QOTD_SERVER_TEXT);
    assert(result.config.messages.qotdPort == MESSAGES_CONFIG_DEFAULT_QOTD_PORT_TEXT);
}

static void iniTextSourceProducesPatchWithoutGlobals() {
    DeferredLogBuffer diagnostics;
    IniTextConfigSource source("memory",
        "cthugha.verbose: 3\n"
        "cthugha.save: yes\n"
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
        "cthugha.sound-processing: FFT\n"
        "cthugha.no-images: yes\n"
        "cthugha.palette-smoothing: 0.25\n"
        "cthugha.palette-set: warm,cool\n"
        "cthugha.no-trans: yes\n"
        "cthugha.no-object: yes\n"
        "cthugha.flame.fire: no\n"
        "cthugha.image.0.logo: yes\n"
        "cthugha.preset.1.wave: line\n"
        "cthugha.lock: yes\n"
        "cthugha.little: yes\n"
        "cthugha.min-time: 1200\n"
        "cthugha.random-time: 3400\n"
        "cthugha.quiet-change: 5600\n"
        "cthugha.cumulative-fire-level: 78\n"
        "cthugha.msg-time: 9100\n"
        "cthugha.quiet-message-duration-ms: 4300\n"
        "cthugha.quiet-file: quiet.txt\n"
        "cthugha.qotd: yes\n"
        "cthugha.qotd-server: qotd.local\n"
        "cthugha.qotd-port: 1717\n"
        "cthugha.qotd-prefetch-timeout-ms: 1200\n"
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
        "cthugha.audio-output-driver: miniaudio\n"
        "cthugha.min-noise: 12\n"
        "cthugha.fire-sensitivity: 72\n"
        "cthugha.fire-source: low-pass-150hz\n"
        "cthugha.pulse-latency-ms: 150\n"
        "cthugha.miniaudio-target-latency-ms: 90\n"
        "cthugha.pulse-server: local\n"
        "cthugha.audio-output-dump: dump.raw\n"
        "cthugha.miniaudio-playback-device: Speakers\n"
        "cthugha.miniaudio-capture-device: Microphone\n"
        "cthugha.dev-dsp: /tmp/dsp\n"
        "cthugha.dev-mixer: /tmp/mixer\n"
        "cthugha.line: 12\n"
        "cthugha.mic: 34\n"
        "cthugha.mixer: pcm:56\n"
        "cthugha.display-driver: sdl3\n"
        "cthugha.disp-mode: 800x600\n"
        "cthugha.max-fps: 60\n"
        "cthugha.show-fps: yes\n"
        "cthugha.zoom: 2\n"
        "cthugha.sdl3-high-pixel-density: no\n"
        "cthugha.sdl3-resizable-window: yes\n"
        "cthugha.sdl3-renderer: software\n"
        "cthugha.sdl3-frame-dump-directory: /tmp/cth-sdl3\n"
        "cthugha.sdl3-frame-dump-limit: 4\n"
        "cthugha.sdl3-frame-dump-every: 2\n"
        "cthugha.buff-size: 2\n");
    ConfigPatch patch = source.acquire(diagnostics);

    assert(diagnostics.diagnostics().empty());
    assert(*patchValue(patch, "logging.verbosity") == "3");
    assert(*patchValue(patch, "app.options_save_enabled") == "1");
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
    assert(*patchValue(patch, "scene.audio_processing") == "FFT");
    assert(*patchValue(patch, "effect.image_files_enabled") == "0");
    assert(*patchValue(patch, "scene_transition.palette_smoothing_chance") == "0.25");
    assert(*patchValue(patch, "effect.palette_set_filter") == "warm,cool");
    assert(*patchValue(patch, "effect.use_translates_enabled") == "0");
    assert(*patchValue(patch, "effect.use_objects_enabled") == "0");
    const std::vector<ConfigEntry>* effectChoices
        = patchEntries(patch, "effect.allowed_choice");
    assert(effectChoices->size() == 2);
    assert((*effectChoices)[0].value == "flame.fire\t0");
    assert((*effectChoices)[1].value == "image.0.logo\t1");
    const std::vector<ConfigEntry>* effectPresets
        = patchEntries(patch, "effect.preset");
    assert(effectPresets->size() == 1);
    assert((*effectPresets)[0].value == "1\twave\tline");
    assert(*patchValue(patch, "auto_change.locked") == "1");
    assert(*patchValue(patch, "auto_change.change_little") == "1");
    assert(*patchValue(patch, "auto_change.wait_min_ms") == "1200");
    assert(*patchValue(patch, "auto_change.wait_random_ms") == "3400");
    assert(*patchValue(patch, "auto_change.quiet_ms") == "5600");
    assert(*patchValue(patch, "auto_change.cumulative_fire_level") == "78");
    assert(*patchValue(patch, "messages.quiet_message_ms") == "9100");
    assert(*patchValue(patch, "messages.quiet_message_duration_ms") == "4300");
    assert(*patchValue(patch, "messages.quiet_message_file") == "quiet.txt");
    assert(*patchValue(patch, "messages.qotd_enabled") == "1");
    assert(*patchValue(patch, "messages.qotd_server") == "qotd.local");
    assert(*patchValue(patch, "messages.qotd_port") == "1717");
    assert(*patchValue(patch, "messages.qotd_prefetch_timeout_ms") == "1200");
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
    assert(*patchValue(patch, "audio.output_driver") == "miniaudio");
    assert(*patchValue(patch, "audio_analysis.min_noise") == "12");
    assert(*patchValue(patch, "audio_analysis.fire_sensitivity") == "72");
    assert(*patchValue(patch, "audio_analysis.fire_source")
        == "low-pass-150hz");
    assert(*patchValue(patch, "audio.pulse_latency_ms") == "150");
    assert(*patchValue(patch, "audio.miniaudio_target_latency_ms") == "90");
    assert(*patchValue(patch, "audio.pulse_server") == "local");
    assert(*patchValue(patch, "audio.output_dump_path") == "dump.raw");
    assert(*patchValue(patch, "audio.miniaudio_playback_device_name") == "Speakers");
    assert(*patchValue(patch, "audio.miniaudio_capture_device_name") == "Microphone");
    assert(*patchValue(patch, "audio.dsp_device_path") == "/tmp/dsp");
    assert(*patchValue(patch, "audio.mixer_device_path") == "/tmp/mixer");
    const std::vector<ConfigEntry>* mixerInitials
        = patchEntries(patch, "audio.mixer_initial_volume");
    assert(mixerInitials->size() == 3);
    assert((*mixerInitials)[0].value == "line=3084");
    assert((*mixerInitials)[1].value == "mic=8738");
    assert((*mixerInitials)[2].value == "pcm=56");
    assert(*patchValue(patch, "display.driver") == "sdl3");
    assert(*patchValue(patch, "display.mode") == "-1");
    assert(*patchValue(patch, "display.width") == "800");
    assert(*patchValue(patch, "display.height") == "600");
    assert(*patchValue(patch, "display.max_frames_per_second") == "60");
    assert(*patchValue(patch, "display.show_fps") == "1");
    assert(*patchValue(patch, "display.zoom_mode") == "2");
    assert(*patchValue(patch, "sdl3.high_pixel_density_enabled") == "0");
    assert(*patchValue(patch, "sdl3.resizable_window_enabled") == "1");
    assert(*patchValue(patch, "sdl3.renderer_name") == "software");
    assert(*patchValue(patch, "sdl3.frame_dump_directory") == "/tmp/cth-sdl3");
    assert(*patchValue(patch, "sdl3.frame_dump_limit") == "4");
    assert(*patchValue(patch, "sdl3.frame_dump_every") == "2");
    assert(*patchValue(patch, "buffer.preset") == "2");
}

static void iniTextSourceBuildsSdl3Config() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addIniText("memory",
            "cthugha.sdl3.high_pixel_density_enabled: off\n"
            "cthugha.sdl3.resizable_window_enabled: no\n"
            "cthugha.sdl3.renderer_name: opengl\n"
            "cthugha.sdl3.frame_dump_directory: /tmp/sdl3-frames\n"
            "cthugha.sdl3.frame_dump_limit: 9\n"
            "cthugha.sdl3.frame_dump_every: 3\n")
        .build();

    assert(result.ok());
    assert(result.config.sdl3.highPixelDensityEnabled == 0);
    assert(result.config.sdl3.resizableWindowEnabled == 0);
    assert(result.config.sdl3.rendererName == "opengl");
    assert(result.config.sdl3.frameDumpDirectory == "/tmp/sdl3-frames");
    assert(result.config.sdl3.frameDumpLimit == 9);
    assert(result.config.sdl3.frameDumpEvery == 3);
}

static void iniPaletteSmoothingAcceptsBooleanValues() {
    ConfigurationBuilder disabledBuilder;
    ConfigBuildResult result = disabledBuilder.addDefaults()
        .addIniText("memory", "cthugha.palette-smoothing: no\n")
        .build();

    assert(result.ok());
    assert(result.config.sceneTransition.paletteSmoothingChance == 0.0);
    assert(result.diagnostics.empty());

    ConfigurationBuilder enabledBuilder;
    result = enabledBuilder.addDefaults()
        .addIniText("memory", "cthugha.palette-smoothing: on\n")
        .build();

    assert(result.ok());
    assert(result.config.sceneTransition.paletteSmoothingChance == 1.0);
    assert(result.diagnostics.empty());
}

static void iniTextSourceWarnsAndIgnoresUnsupportedWildcards() {
    DeferredLogBuffer diagnostics;
    IniTextConfigSource source("memory",
        "cthugha.flame.?: no\n");
    ConfigPatch patch = source.acquire(diagnostics);

    assert(diagnostics.diagnostics().size() == 1);
    assert(diagnostics.diagnostics()[0].severity == ConfigDiagnosticWarning);
    assert(diagnostics.diagnostics()[0].key == "flame.?");
    assert(patch.entriesFor("effect.allowed_choice") == NULL);
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

static void commandLineSourceBuildsAppConfig() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--save",
        })
        .build();

    assert(result.ok());
    assert(result.config.app.optionsSaveEnabled == 1);

    ConfigurationBuilder noSaveBuilder;
    result = noSaveBuilder.addDefaults()
                 .addCommandLine(std::vector<std::string>{
                     "cthugha",
                     "--save",
                     "--no-save",
                 })
                 .build();

    assert(result.ok());
    assert(result.config.app.optionsSaveEnabled == 0);
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

static void commandLineSourceReportsUnknownOptions() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--does-not-exist",
            "-Z",
        })
        .build();

    assert(!result.ok());
    assert(!result.helpRequested);
    assert(result.diagnostics.size() == 2);
    assert(result.diagnostics[0].severity == ConfigDiagnosticError);
    assert(result.diagnostics[0].key == "--does-not-exist");
    assert(result.diagnostics[1].severity == ConfigDiagnosticError);
    assert(result.diagnostics[1].key == "-Z");

    ConfigurationBuilder helpBuilder;
    result = helpBuilder.addDefaults()
                 .addCommandLine(std::vector<std::string>{
                     "cthugha",
                     "--help",
                     "-?",
                 })
                 .build();

    assert(result.ok());
    assert(result.helpRequested);

    ConfigurationBuilder helpWithErrorBuilder;
    result = helpWithErrorBuilder.addDefaults()
                 .addCommandLine(std::vector<std::string>{
                     "cthugha",
                     "--does-not-exist",
                     "--help",
                 })
                 .build();

    assert(!result.ok());
    assert(result.helpRequested);
    assert(result.diagnostics.size() == 1);
    assert(result.diagnostics[0].key == "--does-not-exist");

    ConfigurationBuilder terminatorBuilder;
    result = terminatorBuilder.addDefaults()
                 .addCommandLine(std::vector<std::string>{
                     "cthugha",
                     "--",
                     "--not-config",
                 })
                 .build();

    assert(result.ok());
    assert(!result.helpRequested);
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
            "--audio-output-driver=miniaudio",
            "--pulse-latency-ms=40",
            "--miniaudio-target-latency-ms",
            "80",
            "--pulse-server",
            "local",
            "--audio-output-dump",
            "dump.raw",
            "--miniaudio-playback-device",
            "Speakers",
            "--miniaudio-capture-device=Microphone",
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
    assert(result.config.audio.outputDriver == AudioOutputDriverMiniAudio);
    assert(result.config.audio.pulseLatencyMs == 50);
    assert(result.config.audio.miniAudioOutputTargetLatencyMs == 80);
    assert(result.config.audio.pulseServer == "local");
    assert(result.config.audio.outputDumpPath == "dump.raw");
    assert(result.config.audio.miniAudioPlaybackDeviceName == "Speakers");
    assert(result.config.audio.miniAudioCaptureDeviceName == "Microphone");
    assert(result.config.audio.dspDevicePath == "/tmp/dsp");
    assert(result.config.audio.mixerDevicePath == "/tmp/mixer");
    assert(result.config.audio.mixerInitialVolumes.size() == 3);
    assert(result.config.audio.mixerInitialVolumes[0].name == "line");
    assert(result.config.audio.mixerInitialVolumes[0].volume == 3084);
    assert(result.config.audio.mixerInitialVolumes[1].name == "mic");
    assert(result.config.audio.mixerInitialVolumes[1].volume == 8738);
    assert(result.config.audio.mixerInitialVolumes[2].name == "pcm");
    assert(result.config.audio.mixerInitialVolumes[2].volume == 56);
    assert(result.diagnostics.size() == 1);
    assert(result.diagnostics[0].severity == ConfigDiagnosticWarning);
}

static void commandLineSourceBuildsAutoChangeConfig() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--lock",
            "--little",
            "--min-time",
            "1200",
            "--random-time=3400",
            "-Q5600",
            "--cumulative-fire-level",
            "78",
            "--min-noise=300",
            "--fire-sensitivity=37",
            "--fire-source=lowpass150hz",
        })
        .build();

    assert(result.ok());
    assert(result.config.autoChange.locked == 1);
    assert(result.config.autoChange.changeLittle == 1);
    assert(result.config.autoChange.waitMinMs == 1200);
    assert(result.config.autoChange.waitRandomMs == 3400);
    assert(result.config.autoChange.quietMs == 5600);
    assert(result.config.autoChange.cumulativeFireLevel == 78);
    assert(result.config.audioAnalysis.minNoise == 255);
    assert(result.config.audioAnalysis.fireSensitivity == 37);
    assert(result.config.audioAnalysis.fireSource
        == AUDIO_ANALYSIS_FIRE_SOURCE_LOW_PASS_150HZ_AMPLITUDE_TEXT);
    assert(result.diagnostics.size() == 1);
    assert(result.diagnostics[0].severity == ConfigDiagnosticWarning);
}

static void commandLineSourceBuildsMessagesConfig() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--msg-time",
            "9100",
            "--quiet-message-duration-ms=4300",
            "--quiet-file",
            "quiet.txt",
            "--qotd",
            "--qotd-server=qotd.local",
            "--qotd-port",
            "1717",
            "--qotd-prefetch-timeout-ms=1200",
        })
        .build();

    assert(result.ok());
    assert(result.config.messages.quietMessageMs == 9100);
    assert(result.config.messages.quietMessageDurationMs == 4300);
    assert(result.config.messages.quietMessageFile == "quiet.txt");
    assert(result.config.messages.qotdEnabled == 1);
    assert(result.config.messages.qotdServer == "qotd.local");
    assert(result.config.messages.qotdPort == "1717");
    assert(result.config.messages.qotdPrefetchTimeoutMs == 1200);
    assert(result.diagnostics.empty());
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
            "--sound-processing",
            "Filter2",
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
    assert(result.config.scene.audioProcessing == "Filter2");

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

static void commandLineSourceBuildsEffectAndSceneTransitionPolicyConfig() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--no-images",
            "--palette-smoothing",
            "0.25",
            "--palette-set=warm,cool",
            "--no-trans",
            "--no-object",
            "--max-fps",
            "60",
            "--show-fps",
            "--zoom=2",
        })
        .build();

    assert(result.ok());
    assert(result.config.effectPolicy.imageFilesEnabled == 0);
    assert(result.config.sceneTransition.paletteSmoothingChance == 0.25);
    assert(result.config.effectPolicy.paletteSetFilterText == "warm,cool");
    assert(result.config.effectPolicy.useTranslatesEnabled == 0);
    assert(result.config.effectPolicy.useObjectsEnabled == 0);
    assert(result.config.display.maxFramesPerSecond == 60);
    assert(result.config.display.showFpsEnabled == 1);
    assert(result.config.display.zoomMode == 2);
    assert(result.diagnostics.empty());

    ConfigurationBuilder clampBuilder;
    result = clampBuilder.addDefaults()
                 .addCommandLine(std::vector<std::string>{
                     "cthugha",
                     "--palette-smoothing=2.5",
                     "--zoom=7",
                 })
                 .build();

    assert(result.ok());
    assert(result.config.sceneTransition.paletteSmoothingChance == 1.0);
    assert(result.config.display.zoomMode == ZOOM_MODE_MAX_EXCLUSIVE - 1);
    assert(result.diagnostics.size() == 2);
    assert(result.diagnostics[0].severity == ConfigDiagnosticWarning);
    assert(result.diagnostics[1].severity == ConfigDiagnosticWarning);
}

static void iniTextSourceBuildsEffectCatalogPolicy() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addIniText("policy",
            "cthugha.flame.fire: no\n"
            "cthugha.image.0.logo: yes\n"
            "cthugha.preset.2.palette: sunset\n")
        .build();

    assert(result.ok());
    assert(result.config.effectPolicy.allowedChoices.size() == 2);
    assert(result.config.effectPolicy.allowedChoices[0].catalogEntryKey
        == "flame.fire");
    assert(result.config.effectPolicy.allowedChoices[0].enabled == 0);
    assert(result.config.effectPolicy.allowedChoices[1].catalogEntryKey
        == "image.0.logo");
    assert(result.config.effectPolicy.allowedChoices[1].enabled == 1);
    assert(result.config.effectPolicy.presets.size() == 1);
    assert(result.config.effectPolicy.presets[0].slot == 2);
    assert(result.config.effectPolicy.presets[0].catalogName == "palette");
    assert(result.config.effectPolicy.presets[0].choiceText == "sunset");
}

static void commandLineSourceHandlesDisplayAndBufferSettings() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--disp-mode",
            "800x600",
            "--display-driver=x11",
            "--buff-size",
            "2",
        })
        .build();

    assert(result.ok());
    assert(result.config.display.hasCustomDisplaySize);
    assert(result.config.display.driver == DisplayDriverX11);
    assert(result.config.display.displayMode == -1);
    assert(result.config.display.displayWidth == 800);
    assert(result.config.display.displayHeight == 600);
    assert(result.config.display.bufferWidth == 400);
    assert(result.config.display.bufferHeight == 300);
    assert(!result.config.display.hasCustomBufferSize);
}

static void customDisplayAndBufferSizesAreAcceptedWithoutLegacyClamps() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--display-driver=sdl3",
            "--disp-mode=2560x1440",
            "--buff-size=2000x12",
        })
        .build();

    assert(result.ok());
    assert(result.config.display.driver == DisplayDriverSDL3);
    assert(result.config.display.hasCustomDisplaySize);
    assert(result.config.display.displayWidth == 2560);
    assert(result.config.display.displayHeight == 1440);
    assert(result.config.display.bufferWidth == 2000);
    assert(result.config.display.bufferHeight == 12);
    assert(result.config.display.hasCustomBufferSize);
    assert(result.diagnostics.empty());
}

static void commandLineSourceBuildsSceneScriptConfig() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addCommandLine(std::vector<std::string>{
            "cthugha",
            "--scene-script-dir",
            "tests/fixtures/ini/perf",
            "--scene-script=perf.scenescript",
        })
        .build();

    assert(result.ok());
    assert(result.config.sceneScript.directory == "tests/fixtures/ini/perf");
    assert(result.config.sceneScript.script == "perf.scenescript");
}

static void invalidTypedValueProducesDeferredError() {
    ConfigurationBuilder builder;
    ConfigBuildResult result = builder.addDefaults()
        .addIniText("broken", "cthugha.verbose: nope\n")
        .build();

    assert(!result.ok());
    assert(!result.diagnostics.empty());
    assert(result.diagnostics[0].severity == ConfigDiagnosticError);

    ConfigurationBuilder driverBuilder;
    result = driverBuilder.addDefaults()
                 .addCommandLine(std::vector<std::string>{
                     "cthugha",
                     "--display-driver=wayland",
                 })
                 .build();

    assert(!result.ok());
    assert(!result.diagnostics.empty());
    assert(result.diagnostics[0].severity == ConfigDiagnosticError);
    assert(result.diagnostics[0].key == "display.driver");
}

int main() {
    fprintf(stderr, "%s", "");
    fflush(stderr);
    assert(configArgumentsFromArgv(0, NULL).empty());
    defaultsProduceTypedConfig();
    iniTextSourceProducesPatchWithoutGlobals();
    iniPaletteSmoothingAcceptsBooleanValues();
    iniTextSourceWarnsAndIgnoresUnsupportedWildcards();
    sourcePrecedenceIsDefaultsIniEnvironmentCommandLine();
    commandLineSourceBuildsAppConfig();
    commandLineSourceBuildsInputConfig();
    commandLineSourceReportsUnknownOptions();
    commandLineSourceHandlesAudioLastWriterWins();
    commandLineSourceBuildsFullAudioConfig();
    commandLineSourceBuildsAutoChangeConfig();
    commandLineSourceBuildsMessagesConfig();
    commandLineSourceBuildsSceneConfig();
    commandLineSourceBuildsEffectAndSceneTransitionPolicyConfig();
    iniTextSourceBuildsEffectCatalogPolicy();
    iniTextSourceBuildsSdl3Config();
    commandLineSourceHandlesDisplayAndBufferSettings();
    customDisplayAndBufferSizesAreAcceptedWithoutLegacyClamps();
    commandLineSourceBuildsSceneScriptConfig();
    invalidTypedValueProducesDeferredError();

    return 0;
}
