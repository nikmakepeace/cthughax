/** @file
 * Typed configuration model, acquisition sources, diagnostics, and builder.
 */

#include "Configuration.h"

#include "configuration_defaults.h"
#include "defaults.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>

#ifndef CTH_LIBDIR
#define CTH_LIBDIR ""
#endif

namespace {

static const char* KEY_LOGGING_VERBOSITY = "logging.verbosity";
static const char* KEY_APP_OPTIONS_SAVE_ENABLED = "app.options_save_enabled";
static const char* KEY_INPUT_ESCAPE_ENABLED = "input.escape_key_enabled";
static const char* KEY_INPUT_KEYMAP_FILE = "input.keymap_file";
static const char* KEY_PATH_EXTRA_LIBRARY = "paths.extra_library";
static const char* KEY_PATH_INI_OVERRIDE = "paths.ini_file_override";
static const char* KEY_SCENE_FLAME = "scene.flame";
static const char* KEY_SCENE_GENERAL_FLAME = "scene.general_flame";
static const char* KEY_SCENE_WAVE = "scene.wave";
static const char* KEY_SCENE_WAVE_SCALE = "scene.wave_scale";
static const char* KEY_SCENE_OBJECT = "scene.object";
static const char* KEY_SCENE_TRANSLATION = "scene.translation";
static const char* KEY_SCENE_PALETTE = "scene.palette";
static const char* KEY_SCENE_BORDER = "scene.border";
static const char* KEY_SCENE_FLASHLIGHT = "scene.flashlight";
static const char* KEY_SCENE_TABLE = "scene.table";
static const char* KEY_SCENE_IMAGE = "scene.image";
static const char* KEY_SCENE_PRESENTATION = "scene.presentation";
static const char* KEY_SCENE_AUDIO_PROCESSING = "scene.audio_processing";
static const char* KEY_AUDIO_INPUT_MODE = "audio.input_mode";
static const char* KEY_AUDIO_INPUT_FILE = "audio.input_file";
static const char* KEY_AUDIO_INPUT_LOOP = "audio.input_loop";
static const char* KEY_AUDIO_SAMPLE_RATE = "audio.sample_rate_hz";
static const char* KEY_AUDIO_CHANNELS = "audio.channels";
static const char* KEY_AUDIO_SAMPLE_FORMAT = "audio.sample_format";
static const char* KEY_AUDIO_DSP_METHOD = "audio.dsp_method";
static const char* KEY_AUDIO_DSP_FRAGMENTS = "audio.dsp_fragments";
static const char* KEY_AUDIO_DSP_FRAGMENT_SIZE = "audio.dsp_fragment_size";
static const char* KEY_AUDIO_DSP_SYNC = "audio.dsp_sync";
static const char* KEY_AUDIO_SILENT = "audio.silent";
static const char* KEY_AUDIO_OUTPUT_DRIVER = "audio.output_driver";
static const char* KEY_AUDIO_PULSE_LATENCY = "audio.pulse_latency_ms";
static const char* KEY_AUDIO_PULSE_SERVER = "audio.pulse_server";
static const char* KEY_AUDIO_OUTPUT_DUMP = "audio.output_dump_path";
static const char* KEY_AUDIO_MINIAUDIO_PLAYBACK_DEVICE
    = "audio.miniaudio_playback_device_name";
static const char* KEY_AUDIO_MINIAUDIO_CAPTURE_DEVICE
    = "audio.miniaudio_capture_device_name";
static const char* KEY_AUDIO_DSP_DEVICE = "audio.dsp_device_path";
static const char* KEY_AUDIO_MIXER_DEVICE = "audio.mixer_device_path";
static const char* KEY_AUDIO_MIXER_INITIAL_VOLUME
    = "audio.mixer_initial_volume";
static const char* KEY_AUDIO_NULL_TARGET_LATENCY = "audio.null_target_latency_ms";
static const char* KEY_AUDIO_PULSE_TARGET_LATENCY = "audio.pulse_target_latency_ms";
static const char* KEY_AUDIO_MINIAUDIO_TARGET_LATENCY
    = "audio.miniaudio_target_latency_ms";
static const char* KEY_AUDIO_DSP_TARGET_LATENCY = "audio.dsp_target_latency_ms";
static const char* KEY_DISPLAY_DRIVER = "display.driver";
static const char* KEY_DISPLAY_MODE = "display.mode";
static const char* KEY_DISPLAY_WIDTH = "display.width";
static const char* KEY_DISPLAY_HEIGHT = "display.height";
static const char* KEY_DISPLAY_MAX_FPS = "display.max_frames_per_second";
static const char* KEY_DISPLAY_SHOW_FPS = "display.show_fps";
static const char* KEY_DISPLAY_ZOOM_MODE = "display.zoom_mode";
static const char* KEY_SDL3_HIGH_PIXEL_DENSITY
    = "sdl3.high_pixel_density_enabled";
static const char* KEY_SDL3_RESIZABLE_WINDOW
    = "sdl3.resizable_window_enabled";
static const char* KEY_SDL3_RENDERER_NAME = "sdl3.renderer_name";
static const char* KEY_SDL3_FRAME_DUMP_DIRECTORY
    = "sdl3.frame_dump_directory";
static const char* KEY_SDL3_FRAME_DUMP_LIMIT = "sdl3.frame_dump_limit";
static const char* KEY_SDL3_FRAME_DUMP_EVERY = "sdl3.frame_dump_every";
static const char* KEY_BUFFER_PRESET = "buffer.preset";
static const char* KEY_BUFFER_WIDTH = "buffer.width";
static const char* KEY_BUFFER_HEIGHT = "buffer.height";
static const char* KEY_BUFFER_CUSTOM_SIZE = "buffer.custom_size";
static const char* KEY_EFFECT_IMAGE_FILES_ENABLED
    = "effect.image_files_enabled";
static const char* KEY_EFFECT_PALETTE_SET_FILTER
    = "effect.palette_set_filter";
static const char* KEY_EFFECT_USE_TRANSLATES_ENABLED
    = "effect.use_translates_enabled";
static const char* KEY_EFFECT_USE_OBJECTS_ENABLED
    = "effect.use_objects_enabled";
static const char* KEY_EFFECT_ALLOWED_CHOICE
    = "effect.allowed_choice";
static const char* KEY_EFFECT_PRESET
    = "effect.preset";
static const char* KEY_SCENE_TRANSITION_PALETTE_SMOOTHING_CHANCE
    = "scene_transition.palette_smoothing_chance";
static const char* KEY_SCENE_TRANSITION_PALETTE_SMOOTH_SECONDS
    = "scene_transition.palette_smooth_seconds";
static const char* KEY_SCENE_SCRIPT_DIRECTORY = "scene_script.directory";
static const char* KEY_SCENE_SCRIPT_FILE = "scene_script.file";
static const char* KEY_AUTO_CHANGE_QUIET_MS = "auto_change.quiet_ms";
static const char* KEY_AUTO_CHANGE_WAIT_MIN_MS = "auto_change.wait_min_ms";
static const char* KEY_AUTO_CHANGE_WAIT_RANDOM_MS
    = "auto_change.wait_random_ms";
static const char* KEY_AUTO_CHANGE_CUMULATIVE_FIRE_LEVEL
    = "auto_change.cumulative_fire_level";
static const char* KEY_AUTO_CHANGE_LOCKED = "auto_change.locked";
static const char* KEY_AUTO_CHANGE_CHANGE_LITTLE = "auto_change.change_little";
static const char* KEY_AUDIO_ANALYSIS_MIN_NOISE = "audio_analysis.min_noise";
static const char* KEY_AUDIO_ANALYSIS_FIRE_SENSITIVITY
    = "audio_analysis.fire_sensitivity";
static const char* KEY_AUDIO_ANALYSIS_FIRE_SOURCE
    = "audio_analysis.fire_source";
static const char* KEY_MESSAGES_QUIET_MESSAGE_MS
    = "messages.quiet_message_ms";
static const char* KEY_MESSAGES_QUIET_MESSAGE_DURATION_MS
    = "messages.quiet_message_duration_ms";
static const char* KEY_MESSAGES_QUIET_MESSAGE_FILE
    = "messages.quiet_message_file";
static const char* KEY_MESSAGES_QOTD_ENABLED = "messages.qotd_enabled";
static const char* KEY_MESSAGES_QOTD_PREFETCH_TIMEOUT_MS
    = "messages.qotd_prefetch_timeout_ms";
static const char* KEY_MESSAGES_QOTD_SERVER = "messages.qotd_server";
static const char* KEY_MESSAGES_QOTD_PORT = "messages.qotd_port";
#ifdef CTH_XWIN
static const char* KEY_X11_OVERRIDE_REDIRECT = "x11.override_redirect";
static const char* KEY_X11_PRIVATE_CMAP = "x11.private_cmap";
static const char* KEY_X11_MIT_SHM = "x11.mit_shm";
static const char* KEY_X11_ROOT_WINDOW = "x11.root_window";
static const char* KEY_X11_FULLSCREEN = "x11.fullscreen";
static const char* KEY_X11_WINDOW_POSITION_ENABLED
    = "x11.window_position_enabled";
static const char* KEY_X11_WINDOW_POSITION_X = "x11.window_position_x";
static const char* KEY_X11_WINDOW_POSITION_Y = "x11.window_position_y";
static const char* KEY_X11_PANEL_ENABLED = "x11.panel_enabled";
static const char* KEY_X11_FONT_NAME = "x11.font_name";
static const char* KEY_X11_FRAME_DUMP_DIRECTORY = "x11.frame_dump_directory";
static const char* KEY_X11_FRAME_DUMP_LIMIT = "x11.frame_dump_limit";
static const char* KEY_X11_FRAME_DUMP_EVERY = "x11.frame_dump_every";
#endif

struct ConfigSize {
    int width;
    int height;
};

static const ConfigSize screenSizePresets[] = {
    { 320, 200 },
    { 640, 480 },
    { 800, 600 },
    { 1024, 768 },
    { 1152, 864 },
    { 1280, 1024 }
};

static const ConfigSize bufferSizePresets[] = {
    { 160, 100 },
    { 320, 240 },
    { 400, 300 },
    { 512, 384 },
    { 576, 450 },
    { 600, 512 }
};

static std::string trim(const std::string& value) {
    std::string::size_type begin = 0;
    std::string::size_type end = value.size();

    while (begin < end
        && std::isspace(static_cast<unsigned char>(value[begin])))
        begin++;
    while (end > begin
        && std::isspace(static_cast<unsigned char>(value[end - 1])))
        end--;

    return value.substr(begin, end - begin);
}

static std::string lowercase(const std::string& value) {
    std::string result = value;
    for (std::string::size_type i = 0; i < result.size(); i++)
        result[i] = char(std::tolower(static_cast<unsigned char>(result[i])));
    return result;
}

static bool startsWith(const std::string& value, const std::string& prefix) {
    return value.compare(0, prefix.size(), prefix) == 0;
}

static bool parseDisplayDriverId(const std::string& text,
    DisplayDriverId* driver) {
    std::string cleaned = lowercase(trim(text));
    if (cleaned == "auto") {
        *driver = DisplayDriverAuto;
        return true;
    }
    if (cleaned == "x11") {
        *driver = DisplayDriverX11;
        return true;
    }
    if (cleaned == "sdl3") {
        *driver = DisplayDriverSDL3;
        return true;
    }

    return false;
}

static bool normalizeFireSourceName(
    const std::string& text, std::string* normalized) {
    std::string cleaned = lowercase(trim(text));
    if (cleaned == AUDIO_ANALYSIS_FIRE_SOURCE_RAW_AMPLITUDE_TEXT
        || cleaned == "raw" || cleaned == "amplitude") {
        *normalized = AUDIO_ANALYSIS_FIRE_SOURCE_RAW_AMPLITUDE_TEXT;
        return true;
    }
    if (cleaned == AUDIO_ANALYSIS_FIRE_SOURCE_LOW_PASS_150HZ_AMPLITUDE_TEXT
        || cleaned == "low-pass-150hz" || cleaned == "lowpass150hz"
        || cleaned == "150hz-low-pass") {
        *normalized = AUDIO_ANALYSIS_FIRE_SOURCE_LOW_PASS_150HZ_AMPLITUDE_TEXT;
        return true;
    }

    return false;
}

static bool parseInteger(const std::string& text, int* result) {
    std::string cleaned = trim(text);
    char* end = NULL;
    errno = 0;
    long value = std::strtol(cleaned.c_str(), &end, 0);

    if (cleaned.empty() || end == cleaned.c_str() || errno != 0)
        return false;

    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end)))
            return false;
        end++;
    }

    *result = int(value);
    return true;
}

static bool parseDouble(const std::string& text, double* result) {
    std::string cleaned = trim(text);
    char* end = NULL;
    errno = 0;
    double value = std::strtod(cleaned.c_str(), &end);

    if (cleaned.empty() || end == cleaned.c_str() || errno != 0
        || !std::isfinite(value))
        return false;

    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end)))
            return false;
        end++;
    }

    *result = value;
    return true;
}

static std::string integerText(int value) {
    return std::to_string(value);
}

static std::string doubleText(double value) {
    std::ostringstream output;
    output << value;
    return output.str();
}

static bool parseBoolean(const std::string& text, int* result) {
    std::string cleaned = lowercase(trim(text));

    if (cleaned == "1" || cleaned == "yes" || cleaned == "on"
        || cleaned == "true" || cleaned == "enabled"
        || cleaned == "enable") {
        *result = 1;
        return true;
    }

    if (cleaned == "0" || cleaned == "no" || cleaned == "off"
        || cleaned == "false" || cleaned == "disabled"
        || cleaned == "disable") {
        *result = 0;
        return true;
    }

    return false;
}

static std::string booleanText(int value) {
    return value ? "1" : "0";
}

static bool parseSizeSpec(const std::string& text, int* width, int* height) {
    std::string cleaned = lowercase(trim(text));
    std::string::size_type separator = cleaned.find('x');
    if (separator == std::string::npos)
        return false;

    std::string widthText = cleaned.substr(0, separator);
    std::string heightText = cleaned.substr(separator + 1);
    return parseInteger(widthText, width) && parseInteger(heightText, height);
}

static std::string withTrailingSlash(const std::string& path) {
    if (path.empty())
        return path;
    if (path[path.size() - 1] == '/')
        return path;
    return path + "/";
}

static void setDisplayMode(ConfigPatch& patch, const std::string& source,
    const std::string& value) {
    int width = 0;
    int height = 0;

    if (parseSizeSpec(value, &width, &height)) {
        patch.set(KEY_DISPLAY_MODE, "-1", source);
        patch.set(KEY_DISPLAY_WIDTH, integerText(width), source);
        patch.set(KEY_DISPLAY_HEIGHT, integerText(height), source);
        return;
    }

    patch.set(KEY_DISPLAY_MODE, trim(value), source);
}

static void setBufferSize(ConfigPatch& patch, const std::string& source,
    const std::string& value) {
    int width = 0;
    int height = 0;

    if (parseSizeSpec(value, &width, &height)) {
        patch.set(KEY_BUFFER_WIDTH, integerText(width), source);
        patch.set(KEY_BUFFER_HEIGHT, integerText(height), source);
        patch.set(KEY_BUFFER_CUSTOM_SIZE, "1", source);
        return;
    }

    patch.set(KEY_BUFFER_PRESET, trim(value), source);
}

static void setAudioMode(ConfigPatch& patch, const std::string& source,
    AudioInputMode mode, const std::string& fileName) {
    patch.set(KEY_AUDIO_INPUT_MODE, integerText(int(mode)), source);
    patch.set(KEY_AUDIO_INPUT_FILE, fileName, source);
}

static void setSceneText(ConfigPatch& patch, const std::string& source,
    const char* key, const std::string& value) {
    patch.set(key, trim(value), source);
}

static void setSceneFlashlight(ConfigPatch& patch, const std::string& source,
    const std::string& value, const char* defaultValue) {
    std::string cleanedValue = trim(value);
    patch.set(KEY_SCENE_FLASHLIGHT,
        cleanedValue.empty() ? defaultValue : cleanedValue, source);
}

static void setConfigBoolean(ConfigPatch& patch, const std::string& source,
    const char* key, int enabled) {
    patch.set(key, booleanText(enabled), source);
}

static void setAudioBoolean(ConfigPatch& patch, const std::string& source,
    const char* key, int enabled) {
    setConfigBoolean(patch, source, key, enabled);
}

#ifdef CTH_XWIN
static void setX11Boolean(ConfigPatch& patch, const std::string& source,
    const char* key, int enabled) {
    patch.set(key, booleanText(enabled), source);
}

static void setIniX11BooleanOption(ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const std::string& source,
    const std::string& optionName, const char* key,
    const std::string& value, int defaultWhenPresent, int invert) {
    int enabled = defaultWhenPresent;

    if (!value.empty() && !parseBoolean(value, &enabled)) {
        diagnostics.error(source, optionName, "expected yes/no value");
        return;
    }

    if (invert)
        enabled = !enabled;
    setX11Boolean(patch, source, key, enabled);
}

static bool parsePositionSpec(const std::string& text, int* x, int* y) {
    std::string cleaned = trim(text);
    char* end = NULL;
    const char* begin = cleaned.c_str();
    errno = 0;

    long parsedX = std::strtol(begin, &end, 10);
    if (cleaned.empty() || end == begin || errno != 0)
        return false;

    char* secondBegin = end;
    long parsedY = std::strtol(secondBegin, &end, 10);
    if (end == secondBegin || errno != 0)
        return false;

    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end)))
            return false;
        end++;
    }

    *x = int(parsedX);
    *y = int(parsedY);
    return true;
}

static void setX11Position(ConfigPatch& patch, DeferredLogBuffer& diagnostics,
    const std::string& source, const std::string& optionName,
    const std::string& value) {
    int x = 0;
    int y = 0;

    if (!parsePositionSpec(value, &x, &y)) {
        diagnostics.error(source, optionName,
            "expected X11 position as +X+Y or X Y");
        return;
    }

    patch.set(KEY_X11_WINDOW_POSITION_ENABLED, "1", source);
    patch.set(KEY_X11_WINDOW_POSITION_X, integerText(x), source);
    patch.set(KEY_X11_WINDOW_POSITION_Y, integerText(y), source);
}
#endif

static void setIniBooleanOption(ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const std::string& source,
    const std::string& optionName, const char* key,
    const std::string& value, int defaultWhenPresent, int invert) {
    int enabled = defaultWhenPresent;

    if (!value.empty() && !parseBoolean(value, &enabled)) {
        diagnostics.error(source, optionName, "expected yes/no value");
        return;
    }

    if (invert)
        enabled = !enabled;
    setConfigBoolean(patch, source, key, enabled);
}

static void setIniStereoOption(ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const std::string& source,
    const std::string& optionName, const std::string& value,
    int defaultWhenPresent, int invert) {
    int stereo = defaultWhenPresent;

    if (!value.empty() && !parseBoolean(value, &stereo)) {
        diagnostics.error(source, optionName, "expected yes/no value");
        return;
    }

    if (invert)
        stereo = !stereo;
    patch.set(KEY_AUDIO_CHANNELS, stereo ? "2" : "1", source);
}

static std::string mixerInitialVolumeText(const std::string& name, int volume) {
    return name + "=" + integerText(volume);
}

static void appendMixerInitialVolume(ConfigPatch& patch,
    const std::string& source, const std::string& name, int volume) {
    patch.append(KEY_AUDIO_MIXER_INITIAL_VOLUME,
        mixerInitialVolumeText(name, volume), source);
}

static void appendStereoMixerInitialVolume(ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const std::string& source,
    const std::string& optionName, const std::string& mixerName,
    const std::string& value) {
    int monoVolume = 0;

    if (!parseInteger(value, &monoVolume)) {
        diagnostics.error(source, optionName, "expected an integer mixer volume");
        return;
    }

    appendMixerInitialVolume(patch, source, mixerName,
        monoVolume | (monoVolume << 8));
}

static void appendNamedMixerInitialVolume(ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const std::string& source,
    const std::string& optionName, const std::string& value) {
    std::string cleanedValue = trim(value);
    std::string name;
    std::string volumeText;
    int volume = 0;

    std::string::size_type separator = cleanedValue.find(':');
    if (separator != std::string::npos) {
        name = trim(cleanedValue.substr(0, separator));
        volumeText = trim(cleanedValue.substr(separator + 1));
    } else {
        std::istringstream input(cleanedValue);
        input >> name >> volumeText;
    }

    if (name.empty() || volumeText.empty()) {
        diagnostics.error(source, optionName,
            "expected mixer volume as NAME:VOLUME");
        return;
    }
    if (!parseInteger(volumeText, &volume)) {
        diagnostics.error(source, optionName, "expected an integer mixer volume");
        return;
    }

    appendMixerInitialVolume(patch, source, name, volume);
}

static std::string effectChoicePolicyText(
    const std::string& catalogEntryKey, int enabled) {
    return catalogEntryKey + "\t" + booleanText(enabled);
}

static std::string effectPresetPolicyText(int slot,
    const std::string& catalogName, const std::string& choiceText) {
    return integerText(slot) + "\t" + catalogName + "\t" + choiceText;
}

static bool appendEffectPolicyIniOption(ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const std::string& source,
    const std::string& key, const std::string& value) {
    if (startsWith(key, "preset.")) {
        std::string rest = key.substr(7);
        std::string::size_type separator = rest.find('.');
        int slot = 0;

        if (separator == std::string::npos) {
            diagnostics.error(source, key,
                "expected preset entry as preset.SLOT.CATALOG");
            return true;
        }

        std::string slotText = rest.substr(0, separator);
        std::string catalogName = rest.substr(separator + 1);
        if (!parseInteger(slotText, &slot) || slot < 0
            || catalogName.empty()) {
            diagnostics.error(source, key,
                "expected preset entry as preset.SLOT.CATALOG");
            return true;
        }

        if (!value.empty())
            patch.append(KEY_EFFECT_PRESET,
                effectPresetPolicyText(slot, catalogName, value), source);
        return true;
    }

    if (key.find('.') != std::string::npos) {
        int enabled = 0;
        if (parseBoolean(value, &enabled)) {
            patch.append(KEY_EFFECT_ALLOWED_CHOICE,
                effectChoicePolicyText(key, enabled), source);
            return true;
        }
    }

    return false;
}

static void applyIniOption(ConfigPatch& patch, DeferredLogBuffer& diagnostics,
    const std::string& source, const std::string& name,
    const std::string& value) {
    std::string key = lowercase(trim(name));
    std::string cleanedValue = trim(value);

    if (key == "verbose") {
        patch.set(KEY_LOGGING_VERBOSITY, cleanedValue, source);
    } else if (key == "no-verbose") {
        patch.set(KEY_LOGGING_VERBOSITY, "0", source);
    } else if (key == "path") {
        patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(cleanedValue), source);
    } else if (key == "ini-file") {
        diagnostics.warning(source, key,
            "cthugha.ini-file is ignored inside ini files; use --ini-file before startup config is built");
    } else if (key == "save") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_APP_OPTIONS_SAVE_ENABLED, cleanedValue, 1, 0);
    } else if (key == "no-save") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_APP_OPTIONS_SAVE_ENABLED, cleanedValue, 1, 1);
    } else if (key == "keymap") {
        patch.set(KEY_INPUT_KEYMAP_FILE, cleanedValue, source);
    } else if (key == "esc") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_INPUT_ESCAPE_ENABLED, cleanedValue, 1, 0);
    } else if (key == "no-esc") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_INPUT_ESCAPE_ENABLED, cleanedValue, 1, 1);
    } else if (key == "flame") {
        setSceneText(patch, source, KEY_SCENE_FLAME, cleanedValue);
    } else if (key == "flame-general") {
        setSceneText(patch, source, KEY_SCENE_GENERAL_FLAME, cleanedValue);
    } else if (key == "wave") {
        setSceneText(patch, source, KEY_SCENE_WAVE, cleanedValue);
    } else if (key == "wave-scale") {
        setSceneText(patch, source, KEY_SCENE_WAVE_SCALE, cleanedValue);
    } else if (key == "object") {
        setSceneText(patch, source, KEY_SCENE_OBJECT, cleanedValue);
    } else if (key == "translation" || key == "translate") {
        setSceneText(patch, source, KEY_SCENE_TRANSLATION, cleanedValue);
    } else if (key == "palette") {
        setSceneText(patch, source, KEY_SCENE_PALETTE, cleanedValue);
    } else if (key == "border") {
        setSceneText(patch, source, KEY_SCENE_BORDER, cleanedValue);
    } else if (key == "flashlight") {
        setSceneFlashlight(patch, source, cleanedValue,
            DEFAULT_FLASHLIGHT_ENABLE_INITIAL_ENTRY);
    } else if (key == "no-flashlight") {
        setSceneFlashlight(patch, source, cleanedValue,
            DEFAULT_FLASHLIGHT_DISABLE_INITIAL_ENTRY);
    } else if (key == "table") {
        setSceneText(patch, source, KEY_SCENE_TABLE, cleanedValue);
    } else if (key == "image") {
        setSceneText(patch, source, KEY_SCENE_IMAGE, cleanedValue);
    } else if (key == "display") {
        setSceneText(patch, source, KEY_SCENE_PRESENTATION, cleanedValue);
    } else if (key == "sound-processing") {
        setSceneText(patch, source, KEY_SCENE_AUDIO_PROCESSING, cleanedValue);
    } else if (key == "images") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_EFFECT_IMAGE_FILES_ENABLED, cleanedValue, 1, 0);
    } else if (key == "no-images") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_EFFECT_IMAGE_FILES_ENABLED, cleanedValue, 1, 1);
    } else if (key == "palette-smoothing") {
        int enabled = 0;
        if (parseBoolean(cleanedValue, &enabled))
            cleanedValue = booleanText(enabled);
        patch.set(KEY_SCENE_TRANSITION_PALETTE_SMOOTHING_CHANCE,
            cleanedValue, source);
    } else if (key == "no-palette-smoothing") {
        patch.set(KEY_SCENE_TRANSITION_PALETTE_SMOOTHING_CHANCE, "0",
            source);
    } else if (key == "palette-smooth-seconds") {
        patch.set(KEY_SCENE_TRANSITION_PALETTE_SMOOTH_SECONDS, cleanedValue,
            source);
    } else if (key == "palette-set") {
        patch.set(KEY_EFFECT_PALETTE_SET_FILTER, cleanedValue, source);
    } else if (key == "trans" || key == "use-translate"
        || key == "use-translates") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_EFFECT_USE_TRANSLATES_ENABLED, cleanedValue, 1, 0);
    } else if (key == "no-trans") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_EFFECT_USE_TRANSLATES_ENABLED, cleanedValue, 1, 1);
    } else if (key == "use-object" || key == "use-objects") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_EFFECT_USE_OBJECTS_ENABLED, cleanedValue, 1, 0);
    } else if (key == "no-object" || key == "no-objects") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_EFFECT_USE_OBJECTS_ENABLED, cleanedValue, 1, 1);
    } else if (key == "lock") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUTO_CHANGE_LOCKED, cleanedValue, 1, 0);
    } else if (key == "no-lock") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUTO_CHANGE_LOCKED, cleanedValue, 1, 1);
    } else if (key == "little") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUTO_CHANGE_CHANGE_LITTLE, cleanedValue, 1, 0);
    } else if (key == "no-little") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUTO_CHANGE_CHANGE_LITTLE, cleanedValue, 1, 1);
    } else if (key == "min-time") {
        patch.set(KEY_AUTO_CHANGE_WAIT_MIN_MS, cleanedValue, source);
    } else if (key == "random-time") {
        patch.set(KEY_AUTO_CHANGE_WAIT_RANDOM_MS, cleanedValue, source);
    } else if (key == "quiet-time" || key == "quiet-change") {
        patch.set(KEY_AUTO_CHANGE_QUIET_MS, cleanedValue, source);
    } else if (key == "cumulative-fire-level") {
        patch.set(KEY_AUTO_CHANGE_CUMULATIVE_FIRE_LEVEL, cleanedValue, source);
    } else if (key == "min-noise" || key == "minnoise") {
        patch.set(KEY_AUDIO_ANALYSIS_MIN_NOISE, cleanedValue, source);
    } else if (key == "fire-sensitivity") {
        patch.set(KEY_AUDIO_ANALYSIS_FIRE_SENSITIVITY, cleanedValue, source);
    } else if (key == "fire-source") {
        patch.set(KEY_AUDIO_ANALYSIS_FIRE_SOURCE, cleanedValue, source);
    } else if (key == "msg-time" || key == "change-msg-time") {
        patch.set(KEY_MESSAGES_QUIET_MESSAGE_MS, cleanedValue, source);
    } else if (key == "quiet-file") {
        patch.set(KEY_MESSAGES_QUIET_MESSAGE_FILE, cleanedValue, source);
    } else if (key == "qotd") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_MESSAGES_QOTD_ENABLED, cleanedValue, 1, 0);
    } else if (key == "no-qotd") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_MESSAGES_QOTD_ENABLED, cleanedValue, 1, 1);
    } else if (key == "quiet-message-duration-ms"
        || key == "msg-duration") {
        patch.set(KEY_MESSAGES_QUIET_MESSAGE_DURATION_MS, cleanedValue,
            source);
    } else if (key == "qotd-prefetch-timeout-ms") {
        patch.set(KEY_MESSAGES_QOTD_PREFETCH_TIMEOUT_MS, cleanedValue,
            source);
    } else if (key == "qotd-server") {
        patch.set(KEY_MESSAGES_QOTD_SERVER, cleanedValue, source);
    } else if (key == "qotd-port") {
        patch.set(KEY_MESSAGES_QOTD_PORT, cleanedValue, source);
    } else if (key == "play") {
        setAudioMode(patch, source, AIM_File, cleanedValue);
    } else if (key == "random-noise") {
        setAudioMode(patch, source, AIM_Random, "");
    } else if (key == "no-sound") {
        setAudioMode(patch, source, AIM_None, "");
    } else if (key == "sound-device-number") {
        patch.set(KEY_AUDIO_INPUT_MODE, cleanedValue, source);
    } else if (key == "loop") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUDIO_INPUT_LOOP, cleanedValue, 1, 0);
    } else if (key == "no-loop") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUDIO_INPUT_LOOP, cleanedValue, 1, 1);
    } else if (key == "rate" || key == "sound-sample-rate") {
        patch.set(KEY_AUDIO_SAMPLE_RATE, cleanedValue, source);
    } else if (key == "stereo") {
        setIniStereoOption(patch, diagnostics, source, key, cleanedValue, 1, 0);
    } else if (key == "no-stereo") {
        setIniStereoOption(patch, diagnostics, source, key, cleanedValue, 1, 1);
    } else if (key == "sound-channels") {
        patch.set(KEY_AUDIO_CHANNELS, cleanedValue, source);
    } else if (key == "snd-format" || key == "sound-format") {
        patch.set(KEY_AUDIO_SAMPLE_FORMAT, cleanedValue, source);
    } else if (key == "snd-method" || key == "sound-method") {
        patch.set(KEY_AUDIO_DSP_METHOD, cleanedValue, source);
    } else if (key == "snd-fragments" || key == "sound-fragments") {
        patch.set(KEY_AUDIO_DSP_FRAGMENTS, cleanedValue, source);
    } else if (key == "snd-fragment-size"
        || key == "sound-fragment-size") {
        patch.set(KEY_AUDIO_DSP_FRAGMENT_SIZE, cleanedValue, source);
    } else if (key == "snd-sync" || key == "sound-sync") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUDIO_DSP_SYNC, cleanedValue, 1, 0);
    } else if (key == "no-snd-sync" || key == "no-sound-sync") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUDIO_DSP_SYNC, cleanedValue, 1, 1);
    } else if (key == "silent") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUDIO_SILENT, cleanedValue, 1, 0);
    } else if (key == "no-silent") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_AUDIO_SILENT, cleanedValue, 1, 1);
    } else if (key == "audio-output-driver" || key == "output-driver") {
        patch.set(KEY_AUDIO_OUTPUT_DRIVER, cleanedValue, source);
    } else if (key == "pulse-latency-ms") {
        patch.set(KEY_AUDIO_PULSE_LATENCY, cleanedValue, source);
    } else if (key == "miniaudio-target-latency-ms") {
        patch.set(KEY_AUDIO_MINIAUDIO_TARGET_LATENCY, cleanedValue, source);
    } else if (key == "pulse-server") {
        patch.set(KEY_AUDIO_PULSE_SERVER, cleanedValue, source);
    } else if (key == "audio-output-dump") {
        patch.set(KEY_AUDIO_OUTPUT_DUMP, cleanedValue, source);
    } else if (key == "miniaudio-playback-device"
        || key == "miniaudio-playback-device-name") {
        patch.set(KEY_AUDIO_MINIAUDIO_PLAYBACK_DEVICE, cleanedValue, source);
    } else if (key == "miniaudio-capture-device"
        || key == "miniaudio-capture-device-name") {
        patch.set(KEY_AUDIO_MINIAUDIO_CAPTURE_DEVICE, cleanedValue, source);
    } else if (key == "dev-dsp") {
        patch.set(KEY_AUDIO_DSP_DEVICE, cleanedValue, source);
    } else if (key == "dev-mixer") {
        patch.set(KEY_AUDIO_MIXER_DEVICE, cleanedValue, source);
    } else if (key == "line") {
        appendStereoMixerInitialVolume(patch, diagnostics, source, key,
            "line", cleanedValue);
    } else if (key == "mic") {
        appendStereoMixerInitialVolume(patch, diagnostics, source, key,
            "mic", cleanedValue);
    } else if (key == "mixer") {
        appendNamedMixerInitialVolume(patch, diagnostics, source, key,
            cleanedValue);
    } else if (key == "disp-mode") {
        setDisplayMode(patch, source, cleanedValue);
    } else if (key == "display-driver" || key == "driver") {
        patch.set(KEY_DISPLAY_DRIVER, cleanedValue, source);
    } else if (key == "max-fps" || key == "maxfps") {
        patch.set(KEY_DISPLAY_MAX_FPS, cleanedValue, source);
    } else if (key == "show-fps") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_DISPLAY_SHOW_FPS, cleanedValue, 1, 0);
    } else if (key == "no-show-fps") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_DISPLAY_SHOW_FPS, cleanedValue, 1, 1);
    } else if (key == "zoom") {
        patch.set(KEY_DISPLAY_ZOOM_MODE, cleanedValue, source);
    } else if (key == "sdl3-high-pixel-density"
        || key == "sdl3.high-pixel-density"
        || key == "sdl3.high_pixel_density_enabled") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_SDL3_HIGH_PIXEL_DENSITY, cleanedValue, 1, 0);
    } else if (key == "no-sdl3-high-pixel-density") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_SDL3_HIGH_PIXEL_DENSITY, cleanedValue, 1, 1);
    } else if (key == "sdl3-resizable-window"
        || key == "sdl3.resizable-window"
        || key == "sdl3.resizable_window_enabled") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_SDL3_RESIZABLE_WINDOW, cleanedValue, 1, 0);
    } else if (key == "no-sdl3-resizable-window") {
        setIniBooleanOption(patch, diagnostics, source, key,
            KEY_SDL3_RESIZABLE_WINDOW, cleanedValue, 1, 1);
    } else if (key == "sdl3-renderer" || key == "sdl3-renderer-name"
        || key == "sdl3.renderer_name") {
        patch.set(KEY_SDL3_RENDERER_NAME, cleanedValue, source);
    } else if (key == "sdl3-frame-dump-directory"
        || key == "sdl3.frame_dump_directory") {
        patch.set(KEY_SDL3_FRAME_DUMP_DIRECTORY, cleanedValue, source);
    } else if (key == "sdl3-frame-dump-limit"
        || key == "sdl3.frame_dump_limit") {
        patch.set(KEY_SDL3_FRAME_DUMP_LIMIT, cleanedValue, source);
    } else if (key == "sdl3-frame-dump-every"
        || key == "sdl3.frame_dump_every") {
        patch.set(KEY_SDL3_FRAME_DUMP_EVERY, cleanedValue, source);
    } else if (key == "buff-size") {
        setBufferSize(patch, source, cleanedValue);
#ifdef CTH_XWIN
    } else if (key == "mit-shm") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_MIT_SHM, cleanedValue, 1, 0);
    } else if (key == "no-mit-shm") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_MIT_SHM, cleanedValue, 1, 1);
    } else if (key == "root") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_ROOT_WINDOW, cleanedValue, 1, 0);
    } else if (key == "no-root") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_ROOT_WINDOW, cleanedValue, 1, 1);
    } else if (key == "install") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_PRIVATE_CMAP, cleanedValue, 1, 0);
    } else if (key == "no-install") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_PRIVATE_CMAP, cleanedValue, 1, 1);
    } else if (key == "override-redirect" || key == "no-decorate") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_OVERRIDE_REDIRECT, cleanedValue, 1, 0);
    } else if (key == "no-override-redirect" || key == "decorate") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_OVERRIDE_REDIRECT, cleanedValue, 1, 1);
    } else if (key == "full-screen") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_FULLSCREEN, cleanedValue, 1, 0);
    } else if (key == "no-full-screen") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_FULLSCREEN, cleanedValue, 1, 1);
    } else if (key == "panel") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_PANEL_ENABLED, cleanedValue, 1, 0);
    } else if (key == "no-panel") {
        setIniX11BooleanOption(patch, diagnostics, source, key,
            KEY_X11_PANEL_ENABLED, cleanedValue, 1, 1);
    } else if (key == "position") {
        setX11Position(patch, diagnostics, source, key, cleanedValue);
    } else if (key == "font") {
        patch.set(KEY_X11_FONT_NAME, cleanedValue, source);
#endif
    } else if (key.find('?') != std::string::npos) {
        diagnostics.warning(source, key,
            "wildcard ini entries are no longer supported; entry ignored");
    } else {
        appendEffectPolicyIniOption(patch, diagnostics, source, key,
            cleanedValue);
    }
}

static bool readOptionValue(const std::vector<std::string>& args, int* index,
    const std::string& optionName, std::string* value,
    DeferredLogBuffer& diagnostics) {
    if (*index + 1 >= int(args.size())) {
        diagnostics.error("command line", optionName, "missing required argument");
        return false;
    }

    (*index)++;
    *value = args[*index];
    return true;
}

static bool readShortOptionValue(const std::vector<std::string>& args, int* index,
    const std::string& arg, std::string* value,
    DeferredLogBuffer& diagnostics) {
    if (arg.size() > 2) {
        *value = arg.substr(2);
        return true;
    }

    return readOptionValue(args, index, arg, value, diagnostics);
}

static bool isHelpOption(const std::string& arg) {
    return arg == "--help" || arg == "-?";
}

static bool isVersionOption(const std::string& arg) {
    return arg == "--version";
}

static int commandLineHelpRequested(const std::vector<std::string>& args) {
    for (int i = 1; i < int(args.size()); i++) {
        const std::string& arg = args[i];
        if (arg == "--")
            break;
        if (isHelpOption(arg))
            return 1;
    }

    return 0;
}

static int commandLineVersionRequested(const std::vector<std::string>& args) {
    for (int i = 1; i < int(args.size()); i++) {
        const std::string& arg = args[i];
        if (arg == "--")
            break;
        if (isVersionOption(arg))
            return 1;
    }

    return 0;
}

#ifdef CTH_XWIN
static bool xToolkitOptionWithArg(const std::string& arg) {
    static const char* options[] = {
        "-background",
        "-bg",
        "-display",
        "-fn",
        "-font",
        "-foreground",
        "-fg",
        "-geometry",
        "-name",
        "-selectionTimeout",
        "-title",
        "-xrm",
        "-xnllanguage",
        "-xtsessionID",
        0
    };

    for (int i = 0; options[i] != 0; i++) {
        if (arg == options[i])
            return true;
    }

    return false;
}

static bool xToolkitOptionWithoutArg(const std::string& arg) {
    static const char* options[] = {
        "+rv",
        "+synchronous",
        "-iconic",
        "-reverse",
        "-rv",
        "-synchronous",
        0
    };

    for (int i = 0; options[i] != 0; i++) {
        if (arg == options[i])
            return true;
    }

    return false;
}

static bool consumeXToolkitOption(const std::vector<std::string>& args,
    int* index, const std::string& arg) {
    if (xToolkitOptionWithArg(arg)) {
        if (*index + 1 < int(args.size()))
            (*index)++;
        return true;
    }

    return xToolkitOptionWithoutArg(arg);
}
#else
static bool consumeXToolkitOption(const std::vector<std::string>&,
    int*, const std::string&) {
    return false;
}
#endif

static void applyCommandLineOption(ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const std::vector<std::string>& args,
    int* index) {
    std::string arg = args[*index];

    if (arg == "--verbose") {
        patch.set(KEY_LOGGING_VERBOSITY, DEFAULT_VERBOSE_COMMAND_LEVEL_TEXT,
            "command line");
    } else if (startsWith(arg, "--verbose=")) {
        patch.set(KEY_LOGGING_VERBOSITY, arg.substr(10), "command line");
    } else if (arg == "--no-verbose") {
        patch.set(KEY_LOGGING_VERBOSITY, "0", "command line");
    } else if (arg == "--path") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(value),
                "command line");
    } else if (startsWith(arg, "--path=")) {
        patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(arg.substr(7)),
            "command line");
    } else if (arg == "--ini-file") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_PATH_INI_OVERRIDE, value, "command line");
    } else if (startsWith(arg, "--ini-file=")) {
        patch.set(KEY_PATH_INI_OVERRIDE, arg.substr(11), "command line");
    } else if (arg == "--save") {
        setConfigBoolean(patch, "command line", KEY_APP_OPTIONS_SAVE_ENABLED,
            1);
    } else if (arg == "--no-save") {
        setConfigBoolean(patch, "command line", KEY_APP_OPTIONS_SAVE_ENABLED,
            0);
    } else if (arg == "--keymap") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_INPUT_KEYMAP_FILE, value, "command line");
    } else if (startsWith(arg, "--keymap=")) {
        patch.set(KEY_INPUT_KEYMAP_FILE, arg.substr(9), "command line");
    } else if (arg == "--esc") {
        setConfigBoolean(patch, "command line", KEY_INPUT_ESCAPE_ENABLED, 1);
    } else if (arg == "--no-esc") {
        setConfigBoolean(patch, "command line", KEY_INPUT_ESCAPE_ENABLED, 0);
    } else if (arg == "--flame") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_FLAME, value);
    } else if (startsWith(arg, "--flame=")) {
        setSceneText(patch, "command line", KEY_SCENE_FLAME, arg.substr(8));
    } else if (arg == "--flame-general") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_GENERAL_FLAME,
                value);
    } else if (startsWith(arg, "--flame-general=")) {
        setSceneText(patch, "command line", KEY_SCENE_GENERAL_FLAME,
            arg.substr(16));
    } else if (arg == "--wave") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_WAVE, value);
    } else if (startsWith(arg, "--wave=")) {
        setSceneText(patch, "command line", KEY_SCENE_WAVE, arg.substr(7));
    } else if (arg == "--wave-scale") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_WAVE_SCALE, value);
    } else if (startsWith(arg, "--wave-scale=")) {
        setSceneText(patch, "command line", KEY_SCENE_WAVE_SCALE,
            arg.substr(13));
    } else if (arg == "--object") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_OBJECT, value);
    } else if (startsWith(arg, "--object=")) {
        setSceneText(patch, "command line", KEY_SCENE_OBJECT,
            arg.substr(9));
    } else if (arg == "--translation" || arg == "--translate") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_TRANSLATION,
                value);
    } else if (startsWith(arg, "--translation=")) {
        setSceneText(patch, "command line", KEY_SCENE_TRANSLATION,
            arg.substr(14));
    } else if (startsWith(arg, "--translate=")) {
        setSceneText(patch, "command line", KEY_SCENE_TRANSLATION,
            arg.substr(12));
    } else if (arg == "--palette") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_PALETTE, value);
    } else if (startsWith(arg, "--palette=")) {
        setSceneText(patch, "command line", KEY_SCENE_PALETTE,
            arg.substr(10));
    } else if (arg == "--border") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_BORDER, value);
    } else if (startsWith(arg, "--border=")) {
        setSceneText(patch, "command line", KEY_SCENE_BORDER, arg.substr(9));
    } else if (arg == "--flashlight") {
        setSceneFlashlight(patch, "command line", "",
            DEFAULT_FLASHLIGHT_ENABLE_INITIAL_ENTRY);
    } else if (startsWith(arg, "--flashlight=")) {
        setSceneFlashlight(patch, "command line", arg.substr(13),
            DEFAULT_FLASHLIGHT_ENABLE_INITIAL_ENTRY);
    } else if (arg == "--no-flashlight") {
        setSceneFlashlight(patch, "command line", "",
            DEFAULT_FLASHLIGHT_DISABLE_INITIAL_ENTRY);
    } else if (startsWith(arg, "--no-flashlight=")) {
        setSceneFlashlight(patch, "command line", arg.substr(16),
            DEFAULT_FLASHLIGHT_DISABLE_INITIAL_ENTRY);
    } else if (arg == "--table") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_TABLE, value);
    } else if (startsWith(arg, "--table=")) {
        setSceneText(patch, "command line", KEY_SCENE_TABLE, arg.substr(8));
    } else if (arg == "--image") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_IMAGE, value);
    } else if (startsWith(arg, "--image=")) {
        setSceneText(patch, "command line", KEY_SCENE_IMAGE, arg.substr(8));
    } else if (arg == "--display") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_PRESENTATION,
                value);
    } else if (startsWith(arg, "--display=")) {
        setSceneText(patch, "command line", KEY_SCENE_PRESENTATION,
            arg.substr(10));
    } else if (arg == "--sound-processing") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_AUDIO_PROCESSING,
                value);
    } else if (startsWith(arg, "--sound-processing=")) {
        setSceneText(patch, "command line", KEY_SCENE_AUDIO_PROCESSING,
            arg.substr(19));
    } else if (arg == "--images") {
        setConfigBoolean(patch, "command line",
            KEY_EFFECT_IMAGE_FILES_ENABLED, 1);
    } else if (arg == "--no-images") {
        setConfigBoolean(patch, "command line",
            KEY_EFFECT_IMAGE_FILES_ENABLED, 0);
    } else if (arg == "--palette-smoothing") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_SCENE_TRANSITION_PALETTE_SMOOTHING_CHANCE,
                value, "command line");
    } else if (startsWith(arg, "--palette-smoothing=")) {
        patch.set(KEY_SCENE_TRANSITION_PALETTE_SMOOTHING_CHANCE,
            arg.substr(20), "command line");
    } else if (arg == "--no-palette-smoothing") {
        patch.set(KEY_SCENE_TRANSITION_PALETTE_SMOOTHING_CHANCE, "0",
            "command line");
    } else if (arg == "--palette-set") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_EFFECT_PALETTE_SET_FILTER, value, "command line");
    } else if (startsWith(arg, "--palette-set=")) {
        patch.set(KEY_EFFECT_PALETTE_SET_FILTER, arg.substr(14),
            "command line");
    } else if (arg == "--trans" || arg == "--use-translate"
        || arg == "--use-translates") {
        setConfigBoolean(patch, "command line",
            KEY_EFFECT_USE_TRANSLATES_ENABLED, 1);
    } else if (arg == "--no-trans") {
        setConfigBoolean(patch, "command line",
            KEY_EFFECT_USE_TRANSLATES_ENABLED, 0);
    } else if (arg == "--use-object" || arg == "--use-objects") {
        setConfigBoolean(patch, "command line",
            KEY_EFFECT_USE_OBJECTS_ENABLED, 1);
    } else if (arg == "--no-object" || arg == "--no-objects") {
        setConfigBoolean(patch, "command line",
            KEY_EFFECT_USE_OBJECTS_ENABLED, 0);
    } else if (arg == "--lock") {
        setConfigBoolean(patch, "command line", KEY_AUTO_CHANGE_LOCKED, 1);
    } else if (arg == "--no-lock") {
        setConfigBoolean(patch, "command line", KEY_AUTO_CHANGE_LOCKED, 0);
    } else if (arg == "--little") {
        setConfigBoolean(patch, "command line", KEY_AUTO_CHANGE_CHANGE_LITTLE,
            1);
    } else if (arg == "--no-little") {
        setConfigBoolean(patch, "command line", KEY_AUTO_CHANGE_CHANGE_LITTLE,
            0);
    } else if (arg == "--min-time") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUTO_CHANGE_WAIT_MIN_MS, value, "command line");
    } else if (startsWith(arg, "--min-time=")) {
        patch.set(KEY_AUTO_CHANGE_WAIT_MIN_MS, arg.substr(11), "command line");
    } else if (arg == "--random-time") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUTO_CHANGE_WAIT_RANDOM_MS, value, "command line");
    } else if (startsWith(arg, "--random-time=")) {
        patch.set(KEY_AUTO_CHANGE_WAIT_RANDOM_MS, arg.substr(14),
            "command line");
    } else if (arg == "--quiet-time") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUTO_CHANGE_QUIET_MS, value, "command line");
    } else if (startsWith(arg, "--quiet-time=")) {
        patch.set(KEY_AUTO_CHANGE_QUIET_MS, arg.substr(13), "command line");
    } else if (arg == "--cumulative-fire-level") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUTO_CHANGE_CUMULATIVE_FIRE_LEVEL, value,
                "command line");
    } else if (startsWith(arg, "--cumulative-fire-level=")) {
        patch.set(KEY_AUTO_CHANGE_CUMULATIVE_FIRE_LEVEL, arg.substr(24),
            "command line");
    } else if (arg == "--min-noise") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_ANALYSIS_MIN_NOISE, value, "command line");
    } else if (startsWith(arg, "--min-noise=")) {
        patch.set(KEY_AUDIO_ANALYSIS_MIN_NOISE, arg.substr(12), "command line");
    } else if (arg == "--fire-sensitivity") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_ANALYSIS_FIRE_SENSITIVITY, value,
                "command line");
    } else if (startsWith(arg, "--fire-sensitivity=")) {
        patch.set(KEY_AUDIO_ANALYSIS_FIRE_SENSITIVITY, arg.substr(19),
            "command line");
    } else if (arg == "--fire-source") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_ANALYSIS_FIRE_SOURCE, value,
                "command line");
    } else if (startsWith(arg, "--fire-source=")) {
        patch.set(KEY_AUDIO_ANALYSIS_FIRE_SOURCE, arg.substr(14),
            "command line");
    } else if (arg == "--msg-time") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_MESSAGES_QUIET_MESSAGE_MS, value, "command line");
    } else if (startsWith(arg, "--msg-time=")) {
        patch.set(KEY_MESSAGES_QUIET_MESSAGE_MS, arg.substr(11),
            "command line");
    } else if (arg == "--quiet-file") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_MESSAGES_QUIET_MESSAGE_FILE, value, "command line");
    } else if (startsWith(arg, "--quiet-file=")) {
        patch.set(KEY_MESSAGES_QUIET_MESSAGE_FILE, arg.substr(13),
            "command line");
    } else if (arg == "--qotd") {
        setConfigBoolean(patch, "command line", KEY_MESSAGES_QOTD_ENABLED,
            1);
    } else if (arg == "--no-qotd") {
        setConfigBoolean(patch, "command line", KEY_MESSAGES_QOTD_ENABLED, 0);
    } else if (arg == "--quiet-message-duration-ms") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_MESSAGES_QUIET_MESSAGE_DURATION_MS, value,
                "command line");
    } else if (startsWith(arg, "--quiet-message-duration-ms=")) {
        patch.set(KEY_MESSAGES_QUIET_MESSAGE_DURATION_MS, arg.substr(28),
            "command line");
    } else if (arg == "--qotd-prefetch-timeout-ms") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_MESSAGES_QOTD_PREFETCH_TIMEOUT_MS, value,
                "command line");
    } else if (startsWith(arg, "--qotd-prefetch-timeout-ms=")) {
        patch.set(KEY_MESSAGES_QOTD_PREFETCH_TIMEOUT_MS, arg.substr(27),
            "command line");
    } else if (arg == "--qotd-server") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_MESSAGES_QOTD_SERVER, value, "command line");
    } else if (startsWith(arg, "--qotd-server=")) {
        patch.set(KEY_MESSAGES_QOTD_SERVER, arg.substr(14), "command line");
    } else if (arg == "--qotd-port") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_MESSAGES_QOTD_PORT, value, "command line");
    } else if (startsWith(arg, "--qotd-port=")) {
        patch.set(KEY_MESSAGES_QOTD_PORT, arg.substr(12), "command line");
    } else if (arg == "--play") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setAudioMode(patch, "command line", AIM_File, value);
    } else if (startsWith(arg, "--play=")) {
        setAudioMode(patch, "command line", AIM_File, arg.substr(7));
    } else if (arg == "--random-noise") {
        setAudioMode(patch, "command line", AIM_Random, "");
    } else if (arg == "--no-sound" || arg == "-x") {
        setAudioMode(patch, "command line", AIM_None, "");
    } else if (arg == "--sound-device-number") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_INPUT_MODE, value, "command line");
    } else if (startsWith(arg, "--sound-device-number=")) {
        patch.set(KEY_AUDIO_INPUT_MODE, arg.substr(22), "command line");
    } else if (arg == "--loop") {
        setAudioBoolean(patch, "command line", KEY_AUDIO_INPUT_LOOP, 1);
    } else if (arg == "--no-loop") {
        setAudioBoolean(patch, "command line", KEY_AUDIO_INPUT_LOOP, 0);
    } else if (arg == "--rate") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_SAMPLE_RATE, value, "command line");
    } else if (startsWith(arg, "--rate=")) {
        patch.set(KEY_AUDIO_SAMPLE_RATE, arg.substr(7), "command line");
    } else if (arg == "--stereo" || arg == "-2") {
        patch.set(KEY_AUDIO_CHANNELS, "2", "command line");
    } else if (arg == "--no-stereo" || arg == "-1") {
        patch.set(KEY_AUDIO_CHANNELS, "1", "command line");
    } else if (arg == "--snd-format") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_SAMPLE_FORMAT, value, "command line");
    } else if (startsWith(arg, "--snd-format=")) {
        patch.set(KEY_AUDIO_SAMPLE_FORMAT, arg.substr(13), "command line");
    } else if (arg == "--snd-method") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_DSP_METHOD, value, "command line");
    } else if (startsWith(arg, "--snd-method=")) {
        patch.set(KEY_AUDIO_DSP_METHOD, arg.substr(13), "command line");
    } else if (arg == "--snd-fragments") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_DSP_FRAGMENTS, value, "command line");
    } else if (startsWith(arg, "--snd-fragments=")) {
        patch.set(KEY_AUDIO_DSP_FRAGMENTS, arg.substr(16), "command line");
    } else if (arg == "--snd-fragment-size"
        || arg == "--sound-fragment-size") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_DSP_FRAGMENT_SIZE, value, "command line");
    } else if (startsWith(arg, "--snd-fragment-size=")) {
        patch.set(KEY_AUDIO_DSP_FRAGMENT_SIZE, arg.substr(20),
            "command line");
    } else if (startsWith(arg, "--sound-fragment-size=")) {
        patch.set(KEY_AUDIO_DSP_FRAGMENT_SIZE, arg.substr(22),
            "command line");
    } else if (arg == "--snd-sync") {
        setAudioBoolean(patch, "command line", KEY_AUDIO_DSP_SYNC, 1);
    } else if (arg == "--no-snd-sync") {
        setAudioBoolean(patch, "command line", KEY_AUDIO_DSP_SYNC, 0);
    } else if (arg == "--silent") {
        setAudioBoolean(patch, "command line", KEY_AUDIO_SILENT, 1);
    } else if (arg == "--no-silent") {
        setAudioBoolean(patch, "command line", KEY_AUDIO_SILENT, 0);
    } else if (arg == "--audio-output-driver") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_OUTPUT_DRIVER, value, "command line");
    } else if (startsWith(arg, "--audio-output-driver=")) {
        patch.set(KEY_AUDIO_OUTPUT_DRIVER, arg.substr(22), "command line");
    } else if (arg == "--pulse-latency-ms") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_PULSE_LATENCY, value, "command line");
    } else if (startsWith(arg, "--pulse-latency-ms=")) {
        patch.set(KEY_AUDIO_PULSE_LATENCY, arg.substr(19), "command line");
    } else if (arg == "--miniaudio-target-latency-ms") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_MINIAUDIO_TARGET_LATENCY, value,
                "command line");
    } else if (startsWith(arg, "--miniaudio-target-latency-ms=")) {
        patch.set(KEY_AUDIO_MINIAUDIO_TARGET_LATENCY, arg.substr(30),
            "command line");
    } else if (arg == "--pulse-server") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_PULSE_SERVER, value, "command line");
    } else if (startsWith(arg, "--pulse-server=")) {
        patch.set(KEY_AUDIO_PULSE_SERVER, arg.substr(15), "command line");
    } else if (arg == "--audio-output-dump") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_OUTPUT_DUMP, value, "command line");
    } else if (startsWith(arg, "--audio-output-dump=")) {
        patch.set(KEY_AUDIO_OUTPUT_DUMP, arg.substr(20), "command line");
    } else if (arg == "--miniaudio-playback-device") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_MINIAUDIO_PLAYBACK_DEVICE, value,
                "command line");
    } else if (startsWith(arg, "--miniaudio-playback-device=")) {
        patch.set(KEY_AUDIO_MINIAUDIO_PLAYBACK_DEVICE, arg.substr(28),
            "command line");
    } else if (arg == "--miniaudio-capture-device") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_MINIAUDIO_CAPTURE_DEVICE, value,
                "command line");
    } else if (startsWith(arg, "--miniaudio-capture-device=")) {
        patch.set(KEY_AUDIO_MINIAUDIO_CAPTURE_DEVICE, arg.substr(27),
            "command line");
    } else if (arg == "--dev-dsp") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_DSP_DEVICE, value, "command line");
    } else if (startsWith(arg, "--dev-dsp=")) {
        patch.set(KEY_AUDIO_DSP_DEVICE, arg.substr(10), "command line");
    } else if (arg == "--dev-mixer") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_MIXER_DEVICE, value, "command line");
    } else if (startsWith(arg, "--dev-mixer=")) {
        patch.set(KEY_AUDIO_MIXER_DEVICE, arg.substr(12), "command line");
    } else if (arg == "--line") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            appendStereoMixerInitialVolume(patch, diagnostics, "command line",
                arg, "line", value);
    } else if (startsWith(arg, "--line=")) {
        appendStereoMixerInitialVolume(patch, diagnostics, "command line",
            "--line", "line", arg.substr(7));
    } else if (arg == "--mic") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            appendStereoMixerInitialVolume(patch, diagnostics, "command line",
                arg, "mic", value);
    } else if (startsWith(arg, "--mic=")) {
        appendStereoMixerInitialVolume(patch, diagnostics, "command line",
            "--mic", "mic", arg.substr(6));
    } else if (arg == "--mixer") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            appendNamedMixerInitialVolume(patch, diagnostics, "command line",
                arg, value);
    } else if (startsWith(arg, "--mixer=")) {
        appendNamedMixerInitialVolume(patch, diagnostics, "command line",
            "--mixer", arg.substr(8));
    } else if (arg == "--disp-mode") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setDisplayMode(patch, "command line", value);
    } else if (startsWith(arg, "--disp-mode=")) {
        setDisplayMode(patch, "command line", arg.substr(12));
    } else if (arg == "--display-driver") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_DISPLAY_DRIVER, value, "command line");
    } else if (startsWith(arg, "--display-driver=")) {
        patch.set(KEY_DISPLAY_DRIVER, arg.substr(17), "command line");
    } else if (arg == "--buff-size") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setBufferSize(patch, "command line", value);
    } else if (startsWith(arg, "--buff-size=")) {
        setBufferSize(patch, "command line", arg.substr(12));
    } else if (arg == "--max-fps") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_DISPLAY_MAX_FPS, value, "command line");
    } else if (startsWith(arg, "--max-fps=")) {
        patch.set(KEY_DISPLAY_MAX_FPS, arg.substr(10), "command line");
    } else if (arg == "--show-fps") {
        setConfigBoolean(patch, "command line", KEY_DISPLAY_SHOW_FPS, 1);
    } else if (arg == "--no-show-fps") {
        setConfigBoolean(patch, "command line", KEY_DISPLAY_SHOW_FPS, 0);
    } else if (arg == "--zoom") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_DISPLAY_ZOOM_MODE, value, "command line");
    } else if (startsWith(arg, "--zoom=")) {
        patch.set(KEY_DISPLAY_ZOOM_MODE, arg.substr(7), "command line");
    } else if (arg == "--scene-script-dir") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_SCENE_SCRIPT_DIRECTORY, value, "command line");
    } else if (startsWith(arg, "--scene-script-dir=")) {
        patch.set(KEY_SCENE_SCRIPT_DIRECTORY, arg.substr(19),
            "command line");
    } else if (arg == "--scene-script") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_SCENE_SCRIPT_FILE, value, "command line");
    } else if (startsWith(arg, "--scene-script=")) {
        patch.set(KEY_SCENE_SCRIPT_FILE, arg.substr(15), "command line");
#ifdef CTH_XWIN
    } else if (arg == "--mit-shm") {
        setX11Boolean(patch, "command line", KEY_X11_MIT_SHM, 1);
    } else if (arg == "--no-mit-shm") {
        setX11Boolean(patch, "command line", KEY_X11_MIT_SHM, 0);
    } else if (arg == "--root") {
        setX11Boolean(patch, "command line", KEY_X11_ROOT_WINDOW, 1);
    } else if (arg == "--no-root") {
        setX11Boolean(patch, "command line", KEY_X11_ROOT_WINDOW, 0);
    } else if (arg == "--install") {
        setX11Boolean(patch, "command line", KEY_X11_PRIVATE_CMAP, 1);
    } else if (arg == "--no-install") {
        setX11Boolean(patch, "command line", KEY_X11_PRIVATE_CMAP, 0);
    } else if (arg == "--override-redirect" || arg == "--no-decorate") {
        setX11Boolean(patch, "command line", KEY_X11_OVERRIDE_REDIRECT, 1);
    } else if (arg == "--no-override-redirect" || arg == "--decorate") {
        setX11Boolean(patch, "command line", KEY_X11_OVERRIDE_REDIRECT, 0);
    } else if (arg == "--full-screen") {
        setX11Boolean(patch, "command line", KEY_X11_FULLSCREEN, 1);
    } else if (arg == "--no-full-screen") {
        setX11Boolean(patch, "command line", KEY_X11_FULLSCREEN, 0);
    } else if (arg == "--panel") {
        setX11Boolean(patch, "command line", KEY_X11_PANEL_ENABLED, 1);
    } else if (arg == "--no-panel") {
        setX11Boolean(patch, "command line", KEY_X11_PANEL_ENABLED, 0);
    } else if (arg == "--position") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setX11Position(patch, diagnostics, "command line", arg, value);
    } else if (startsWith(arg, "--position=")) {
        setX11Position(patch, diagnostics, "command line", "--position",
            arg.substr(11));
    } else if (arg == "--font") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_X11_FONT_NAME, value, "command line");
    } else if (startsWith(arg, "--font=")) {
        patch.set(KEY_X11_FONT_NAME, arg.substr(7), "command line");
#endif
    } else if (arg == "-f") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_FLAME, value);
    } else if (arg == "-w") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_WAVE, value);
    } else if (arg == "-o") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_OBJECT, value);
    } else if (arg == "-t") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_TRANSLATION,
                value);
    } else if (arg == "-p") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_PALETTE, value);
    } else if (arg == "-a") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_TABLE, value);
    } else if (arg == "-d") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_PRESENTATION,
                value);
    } else if (startsWith(arg, "-m")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            setSceneText(patch, "command line", KEY_SCENE_AUDIO_PROCESSING,
                value);
    } else if (arg == "-l") {
        setConfigBoolean(patch, "command line", KEY_AUTO_CHANGE_LOCKED, 1);
    } else if (arg == "-s") {
        setSceneFlashlight(patch, "command line", "",
            DEFAULT_FLASHLIGHT_DISABLE_INITIAL_ENTRY);
    } else if (startsWith(arg, "-T")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUTO_CHANGE_WAIT_MIN_MS, value, "command line");
    } else if (startsWith(arg, "-R")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUTO_CHANGE_WAIT_RANDOM_MS, value, "command line");
    } else if (startsWith(arg, "-Q")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUTO_CHANGE_QUIET_MS, value, "command line");
    } else if (startsWith(arg, "-q")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_MESSAGES_QUIET_MESSAGE_FILE, value,
                "command line");
    } else if (startsWith(arg, "-L")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            appendStereoMixerInitialVolume(patch, diagnostics, "command line",
                "-L", "line", value);
    } else if (startsWith(arg, "-M")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            appendStereoMixerInitialVolume(patch, diagnostics, "command line",
                "-M", "mic", value);
    } else if (startsWith(arg, "-E")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(value),
                "command line");
    } else if (startsWith(arg, "-v")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_SAMPLE_RATE, value, "command line");
    } else if (startsWith(arg, "-D")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            setDisplayMode(patch, "command line", value);
    } else if (startsWith(arg, "-S")) {
        std::string value;
        if (readShortOptionValue(args, index, arg, &value, diagnostics))
            setBufferSize(patch, "command line", value);
    } else if (isHelpOption(arg)) {
        return;
    } else if (isVersionOption(arg)) {
        return;
    } else if (arg == "--") {
        *index = int(args.size());
    } else if (consumeXToolkitOption(args, index, arg)) {
        return;
    } else if (startsWith(arg, "-")) {
        diagnostics.error("command line", arg, "unknown option");
    }
}

static std::string removeInlineComment(const std::string& text) {
    std::string::size_type hash = text.find('#');
    std::string::size_type bang = text.find('!');
    std::string::size_type comment = std::string::npos;

    if (hash != std::string::npos)
        comment = hash;
    if (bang != std::string::npos)
        comment = std::min(comment, bang);

    if (comment == std::string::npos)
        return text;
    return text.substr(0, comment);
}

static bool parseIniLine(const std::string& line, std::string* name,
    std::string* value) {
    std::string cleaned = trim(removeInlineComment(line));
    if (cleaned.empty())
        return false;

    if (!startsWith(lowercase(cleaned), "cthugha."))
        return false;

    std::string::size_type colon = cleaned.find(':');
    if (colon == std::string::npos)
        return false;

    *name = trim(cleaned.substr(8, colon - 8));
    *value = trim(cleaned.substr(colon + 1));
    return !name->empty();
}

static bool parseAudioMode(const std::string& text, AudioInputMode* mode) {
    std::string cleaned = lowercase(trim(text));
    int value = 0;

    if (cleaned == "dsp" || cleaned == "dspin" || cleaned == "line-in"
        || cleaned == "mic" || cleaned == "microphone") {
        *mode = AIM_DSPIn;
        return true;
    }
    if (cleaned == "random" || cleaned == "random-noise") {
        *mode = AIM_Random;
        return true;
    }
    if (cleaned == "file" || cleaned == "play") {
        *mode = AIM_File;
        return true;
    }
    if (cleaned == "none" || cleaned == "no-sound" || cleaned == "off") {
        *mode = AIM_None;
        return true;
    }

    if (!parseInteger(cleaned, &value))
        return false;
    if (value < 0 || value >= AIM_Max)
        return false;

    *mode = AudioInputMode(value);
    return true;
}

static std::string compactFormatName(const std::string& text) {
    std::string cleaned = lowercase(trim(text));
    std::string result;

    for (std::string::size_type i = 0; i < cleaned.size(); i++) {
        char c = cleaned[i];
        if (std::isalnum(static_cast<unsigned char>(c)))
            result += c;
    }

    return result;
}

static bool parseAudioSampleFormat(const std::string& text,
    AudioSampleFormat* format) {
    std::string cleaned = compactFormatName(text);
    int value = 0;

    if (parseInteger(text, &value)) {
        if (value < int(SF_u8) || value > int(SF_s16_be))
            return false;
        *format = AudioSampleFormat(value);
        return true;
    }

    if (cleaned == "u8" || cleaned == "uint8" || cleaned == "unsigned8"
        || cleaned == "unsigned8bit" || cleaned == "8bitunsigned") {
        *format = SF_u8;
        return true;
    }
    if (cleaned == "s8" || cleaned == "int8" || cleaned == "signed8"
        || cleaned == "signed8bit" || cleaned == "8bitsigned") {
        *format = SF_s8;
        return true;
    }
    if (cleaned == "u16le" || cleaned == "uint16le"
        || cleaned == "unsigned16le" || cleaned == "unsigned16bitle"
        || cleaned == "16bitunsignedle") {
        *format = SF_u16_le;
        return true;
    }
    if (cleaned == "s16le" || cleaned == "int16le"
        || cleaned == "signed16le" || cleaned == "signed16bitle"
        || cleaned == "16bitsignedle") {
        *format = SF_s16_le;
        return true;
    }
    if (cleaned == "u16be" || cleaned == "uint16be"
        || cleaned == "unsigned16be" || cleaned == "unsigned16bitbe"
        || cleaned == "16bitunsignedbe") {
        *format = SF_u16_be;
        return true;
    }
    if (cleaned == "s16be" || cleaned == "int16be"
        || cleaned == "signed16be" || cleaned == "signed16bitbe"
        || cleaned == "16bitsignedbe") {
        *format = SF_s16_be;
        return true;
    }

    return false;
}

static bool parseAudioOutputDriver(const std::string& text,
    AudioOutputDriverId* driver) {
    std::string cleaned = compactFormatName(text);
    int value = 0;

    if (cleaned == "auto") {
        *driver = AudioOutputDriverAuto;
        return true;
    }
    if (cleaned == "null" || cleaned == "none" || cleaned == "silent") {
        *driver = AudioOutputDriverNull;
        return true;
    }
    if (cleaned == "pulse" || cleaned == "pulseaudio") {
        *driver = AudioOutputDriverPulse;
        return true;
    }
    if (cleaned == "oss" || cleaned == "dsp") {
        *driver = AudioOutputDriverOss;
        return true;
    }
    if (cleaned == "miniaudio" || cleaned == "mini") {
        *driver = AudioOutputDriverMiniAudio;
        return true;
    }

    if (!parseInteger(text, &value))
        return false;
    if (value < 0 || value >= int(AudioOutputDriverMax))
        return false;

    *driver = AudioOutputDriverId(value);
    return true;
}

static bool parseAudioChannels(const std::string& text, int* channels) {
    std::string cleaned = lowercase(trim(text));

    if (cleaned == "mono") {
        *channels = 1;
        return true;
    }
    if (cleaned == "stereo") {
        *channels = 2;
        return true;
    }

    return parseInteger(cleaned, channels);
}

static void applyIntEntry(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const char* key, int* target) {
    const ConfigEntry* configEntry = patch.entry(key);
    int value = 0;

    if (configEntry == NULL)
        return;

    if (!parseInteger(configEntry->value, &value)) {
        diagnostics.error(configEntry->source, configEntry->key,
            "expected an integer value");
        return;
    }

    *target = value;
}

static void applyBoolEntry(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const char* key, int* target) {
    const ConfigEntry* configEntry = patch.entry(key);
    int value = 0;

    if (configEntry == NULL)
        return;

    if (!parseBoolean(configEntry->value, &value)) {
        diagnostics.error(configEntry->source, configEntry->key,
            "expected a yes/no value");
        return;
    }

    *target = value;
}

static int clampIntWithWarning(int value, int minimum, int maximum,
    const ConfigEntry& entry, DeferredLogBuffer& diagnostics);

static void applyClampedIntEntry(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const char* key, int minimum,
    int maximum, int* target) {
    const ConfigEntry* configEntry = patch.entry(key);
    int value = 0;

    if (configEntry == NULL)
        return;

    if (!parseInteger(configEntry->value, &value)) {
        diagnostics.error(configEntry->source, configEntry->key,
            "expected an integer value");
        return;
    }

    *target = clampIntWithWarning(value, minimum, maximum, *configEntry,
        diagnostics);
}

static void applyClampedDoubleEntry(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const char* key, double minimum,
    double maximum, double* target) {
    const ConfigEntry* configEntry = patch.entry(key);
    double value = 0.0;

    if (configEntry == NULL)
        return;

    if (!parseDouble(configEntry->value, &value)) {
        diagnostics.error(configEntry->source, configEntry->key,
            "expected a decimal value");
        return;
    }

    if (value < minimum) {
        diagnostics.warning(configEntry->source, configEntry->key,
            "value below supported range; clamped to minimum");
        value = minimum;
    } else if (value > maximum) {
        diagnostics.warning(configEntry->source, configEntry->key,
            "value above supported range; clamped to maximum");
        value = maximum;
    }

    *target = value;
}

static void applyMinimumIntEntry(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, const char* key, int minimum,
    int* target) {
    const ConfigEntry* configEntry = patch.entry(key);
    int value = 0;

    if (configEntry == NULL)
        return;

    if (!parseInteger(configEntry->value, &value)) {
        diagnostics.error(configEntry->source, configEntry->key,
            "expected an integer value");
        return;
    }

    if (value < minimum) {
        diagnostics.warning(configEntry->source, configEntry->key,
            "value below supported range; clamped to minimum");
        value = minimum;
    }

    *target = value;
}

static void applyAudioChannelsEntry(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, int* target) {
    const ConfigEntry* configEntry = patch.entry(KEY_AUDIO_CHANNELS);
    int value = 0;

    if (configEntry == NULL)
        return;

    if (!parseAudioChannels(configEntry->value, &value)) {
        diagnostics.error(configEntry->source, configEntry->key,
            "expected mono, stereo, or a channel count");
        return;
    }

    *target = clampIntWithWarning(value, SOUND_CHANNELS_MIN,
        SOUND_CHANNELS_MAX_EXCLUSIVE - 1, *configEntry, diagnostics);
}

static void applyAudioSampleFormatEntry(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, AudioSampleFormat* target) {
    const ConfigEntry* configEntry = patch.entry(KEY_AUDIO_SAMPLE_FORMAT);
    AudioSampleFormat format = AUDIO_CONFIG_DEFAULT_FORMAT;

    if (configEntry == NULL)
        return;

    if (!parseAudioSampleFormat(configEntry->value, &format)) {
        diagnostics.error(configEntry->source, configEntry->key,
            "expected a supported PCM sample format");
        return;
    }

    *target = format;
}

static void applyAudioOutputDriverEntry(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, AudioOutputDriverId* target) {
    const ConfigEntry* configEntry = patch.entry(KEY_AUDIO_OUTPUT_DRIVER);
    AudioOutputDriverId driver = AUDIO_CONFIG_DEFAULT_OUTPUT_DRIVER;

    if (configEntry == NULL)
        return;

    if (!parseAudioOutputDriver(configEntry->value, &driver)) {
        diagnostics.error(configEntry->source, configEntry->key,
            "expected one of auto, null, pulse, oss, or miniaudio");
        return;
    }

    *target = driver;
}

static void applyMixerInitialVolumeEntries(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, AudioConfig* config) {
    const std::vector<ConfigEntry>* entries
        = patch.entriesFor(KEY_AUDIO_MIXER_INITIAL_VOLUME);

    if (entries == NULL)
        return;

    for (std::vector<ConfigEntry>::const_iterator it = entries->begin();
         it != entries->end(); ++it) {
        std::string::size_type separator = it->value.find('=');
        AudioMixerInitialVolumeConfig initialVolume;

        if (separator == std::string::npos) {
            diagnostics.error(it->source, it->key,
                "expected mixer initial volume as NAME=VOLUME");
            continue;
        }

        initialVolume.name = trim(it->value.substr(0, separator));
        std::string volumeText = trim(it->value.substr(separator + 1));
        if (initialVolume.name.empty()) {
            diagnostics.error(it->source, it->key,
                "expected mixer initial volume name");
            continue;
        }
        if (!parseInteger(volumeText, &initialVolume.volume)) {
            diagnostics.error(it->source, it->key,
                "expected an integer mixer volume");
            continue;
        }
        if (initialVolume.volume < 0) {
            diagnostics.warning(it->source, it->key,
                "value below supported range; clamped to minimum");
            initialVolume.volume = 0;
        }

        config->mixerInitialVolumes.push_back(initialVolume);
    }
}

static bool splitTabFields(const std::string& text, int expectedFieldCount,
    std::vector<std::string>* fields) {
    fields->clear();

    std::string::size_type begin = 0;
    while (begin <= text.size()) {
        std::string::size_type separator = text.find('\t', begin);
        if (separator == std::string::npos) {
            fields->push_back(text.substr(begin));
            break;
        }

        fields->push_back(text.substr(begin, separator - begin));
        begin = separator + 1;
    }

    return int(fields->size()) == expectedFieldCount;
}

static void applyEffectChoicePolicyEntries(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, EffectPolicy* config) {
    const std::vector<ConfigEntry>* entries
        = patch.entriesFor(KEY_EFFECT_ALLOWED_CHOICE);

    if (entries == NULL)
        return;

    for (std::vector<ConfigEntry>::const_iterator it = entries->begin();
         it != entries->end(); ++it) {
        std::vector<std::string> fields;
        int enabled = 0;

        if (!splitTabFields(it->value, 2, &fields) || fields[0].empty()) {
            diagnostics.error(it->source, it->key,
                "expected effect choice policy as CATALOG.CHOICE=yes/no");
            continue;
        }
        if (!parseBoolean(fields[1], &enabled)) {
            diagnostics.error(it->source, it->key,
                "expected a yes/no value");
            continue;
        }

        config->allowedChoices.push_back(
            EffectChoicePolicy(fields[0], enabled));
    }
}

static void applyEffectPresetPolicyEntries(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics, EffectPolicy* config) {
    const std::vector<ConfigEntry>* entries
        = patch.entriesFor(KEY_EFFECT_PRESET);

    if (entries == NULL)
        return;

    for (std::vector<ConfigEntry>::const_iterator it = entries->begin();
         it != entries->end(); ++it) {
        std::vector<std::string> fields;
        int slot = 0;

        if (!splitTabFields(it->value, 3, &fields)
            || fields[1].empty() || fields[2].empty()) {
            diagnostics.error(it->source, it->key,
                "expected effect preset policy as preset.SLOT.CATALOG=CHOICE");
            continue;
        }
        if (!parseInteger(fields[0], &slot) || slot < 0) {
            diagnostics.error(it->source, it->key,
                "expected effect preset slot to be a nonnegative integer");
            continue;
        }

        config->presets.push_back(
            EffectPresetPolicy(slot, fields[1], fields[2]));
    }
}

static int clampIntWithWarning(int value, int minimum, int maximum,
    const ConfigEntry& entry, DeferredLogBuffer& diagnostics) {
    if (value < minimum) {
        diagnostics.warning(entry.source, entry.key,
            "value below supported range; clamped to minimum");
        return minimum;
    }
    if (value > maximum) {
        diagnostics.warning(entry.source, entry.key,
            "value above supported range; clamped to maximum");
        return maximum;
    }
    return value;
}

static std::map<std::string, std::string> processEnvironment(
    const std::vector<std::string>& names) {
    std::map<std::string, std::string> values;

    for (std::vector<std::string>::const_iterator it = names.begin();
         it != names.end(); ++it) {
        const char* value = std::getenv(it->c_str());
        if (value != NULL)
            values[*it] = value;
    }

    return values;
}

}

ConfigDiagnostic::ConfigDiagnostic()
    : severity(ConfigDiagnosticInfo) { }

ConfigDiagnostic::ConfigDiagnostic(ConfigDiagnosticSeverity severityValue,
    const std::string& sourceValue, const std::string& keyValue,
    const std::string& messageValue)
    : severity(severityValue)
    , source(sourceValue)
    , key(keyValue)
    , message(messageValue) { }

void DeferredLogBuffer::add(ConfigDiagnosticSeverity severity,
    const std::string& source, const std::string& key,
    const std::string& message) {
    diagnosticsValue.push_back(ConfigDiagnostic(severity, source, key, message));
}

void DeferredLogBuffer::info(const std::string& source, const std::string& key,
    const std::string& message) {
    add(ConfigDiagnosticInfo, source, key, message);
}

void DeferredLogBuffer::warning(const std::string& source,
    const std::string& key, const std::string& message) {
    add(ConfigDiagnosticWarning, source, key, message);
}

void DeferredLogBuffer::error(const std::string& source, const std::string& key,
    const std::string& message) {
    add(ConfigDiagnosticError, source, key, message);
}

const std::vector<ConfigDiagnostic>& DeferredLogBuffer::diagnostics() const {
    return diagnosticsValue;
}

bool DeferredLogBuffer::hasErrors() const {
    for (std::vector<ConfigDiagnostic>::const_iterator it
         = diagnosticsValue.begin();
         it != diagnosticsValue.end(); ++it) {
        if (it->severity == ConfigDiagnosticError)
            return true;
    }
    return false;
}

void DeferredLogBuffer::append(const DeferredLogBuffer& other) {
    diagnosticsValue.insert(diagnosticsValue.end(), other.diagnosticsValue.begin(),
        other.diagnosticsValue.end());
}

ConfigEntry::ConfigEntry() { }

ConfigEntry::ConfigEntry(const std::string& keyValue,
    const std::string& valueValue, const std::string& sourceValue)
    : key(keyValue)
    , value(valueValue)
    , source(sourceValue) { }

void ConfigPatch::set(const std::string& key, const std::string& value,
    const std::string& source) {
    entriesValue[key] = ConfigEntry(key, value, source);
}

void ConfigPatch::append(const std::string& key, const std::string& value,
    const std::string& source) {
    repeatedEntriesValue[key].push_back(ConfigEntry(key, value, source));
}

bool ConfigPatch::has(const std::string& key) const {
    return entriesValue.find(key) != entriesValue.end();
}

const ConfigEntry* ConfigPatch::entry(const std::string& key) const {
    std::map<std::string, ConfigEntry>::const_iterator it
        = entriesValue.find(key);
    if (it == entriesValue.end())
        return NULL;
    return &it->second;
}

const std::string* ConfigPatch::value(const std::string& key) const {
    const ConfigEntry* configEntry = entry(key);
    if (configEntry == NULL)
        return NULL;
    return &configEntry->value;
}

const std::map<std::string, ConfigEntry>& ConfigPatch::entries() const {
    return entriesValue;
}

const std::vector<ConfigEntry>* ConfigPatch::entriesFor(
    const std::string& key) const {
    std::map<std::string, std::vector<ConfigEntry> >::const_iterator it
        = repeatedEntriesValue.find(key);
    if (it == repeatedEntriesValue.end())
        return NULL;
    return &it->second;
}

void ConfigPatch::mergeFrom(const ConfigPatch& patch) {
    for (std::map<std::string, ConfigEntry>::const_iterator it
         = patch.entriesValue.begin();
         it != patch.entriesValue.end(); ++it) {
        entriesValue[it->first] = it->second;
    }

    for (std::map<std::string, std::vector<ConfigEntry> >::const_iterator it
         = patch.repeatedEntriesValue.begin();
         it != patch.repeatedEntriesValue.end(); ++it) {
        std::vector<ConfigEntry>& entries = repeatedEntriesValue[it->first];
        entries.insert(entries.end(), it->second.begin(), it->second.end());
    }
}

LoggingConfig::LoggingConfig()
    : verbosity(LOGGING_CONFIG_DEFAULT_VERBOSITY) { }

AppConfig::AppConfig()
    : optionsSaveEnabled(APP_CONFIG_DEFAULT_OPTIONS_SAVE_ENABLED) { }

InputConfig::InputConfig()
    : escapeKeyEnabled(INPUT_CONFIG_DEFAULT_ESCAPE_KEY_ENABLED)
    , keymapFile(INPUT_CONFIG_DEFAULT_KEYMAP_FILE_PATH) { }

PathConfig::PathConfig()
    : extraLibraryPath(PATH_CONFIG_DEFAULT_EXTRA_LIBRARY_PATH)
    , iniFileOverride(PATH_CONFIG_DEFAULT_INI_FILE_OVERRIDE_PATH) { }

SceneConfig::SceneConfig()
    : flame()
    , generalFlame()
    , wave()
    , waveScale()
    , object()
    , translation()
    , palette()
    , border()
    , flashlight()
    , table()
    , image()
    , presentation()
    , audioProcessing() { }

AudioConfig::AudioConfig()
    : inputMode(AUDIO_CONFIG_DEFAULT_INPUT_MODE)
    , inputFile(AUDIO_CONFIG_DEFAULT_INPUT_FILE_PATH)
    , inputLoopEnabled(AUDIO_CONFIG_DEFAULT_INPUT_LOOP_ENABLED)
    , sampleRateHz(AUDIO_CONFIG_DEFAULT_SAMPLE_RATE_HZ)
    , channels(AUDIO_CONFIG_DEFAULT_CHANNELS)
    , sampleFormat(AUDIO_CONFIG_DEFAULT_FORMAT)
    , dspMethod(AUDIO_CONFIG_DEFAULT_DSP_METHOD)
    , dspFragments(AUDIO_CONFIG_DEFAULT_DSP_FRAGMENTS)
    , dspFragmentSize(AUDIO_CONFIG_DEFAULT_DSP_FRAGMENT_SIZE)
    , dspSyncEnabled(AUDIO_CONFIG_DEFAULT_DSP_SYNC_ENABLED)
    , silentEnabled(AUDIO_CONFIG_DEFAULT_SILENT_ENABLED)
    , outputDriver(AUDIO_CONFIG_DEFAULT_OUTPUT_DRIVER)
    , mixerVolume(AUDIO_CONFIG_DEFAULT_MIXER_VOLUME)
    , pulseLatencyMs(AUDIO_CONFIG_DEFAULT_PULSE_LATENCY_MS)
    , pulseServer(AUDIO_CONFIG_DEFAULT_PULSE_SERVER_TEXT)
    , outputDumpPath(AUDIO_CONFIG_DEFAULT_OUTPUT_DUMP_PATH)
    , miniAudioPlaybackDeviceName(
        AUDIO_CONFIG_DEFAULT_MINIAUDIO_PLAYBACK_DEVICE_NAME)
    , miniAudioCaptureDeviceName(
        AUDIO_CONFIG_DEFAULT_MINIAUDIO_CAPTURE_DEVICE_NAME)
    , dspDevicePath(AUDIO_CONFIG_DEFAULT_DSP_DEVICE_PATH)
    , mixerDevicePath(AUDIO_CONFIG_DEFAULT_MIXER_DEVICE_PATH)
    , nullOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_NULL_TARGET_LATENCY_MS)
    , pulseOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_PULSE_TARGET_LATENCY_MS)
    , miniAudioOutputTargetLatencyMs(
        AUDIO_CONFIG_DEFAULT_MINIAUDIO_TARGET_LATENCY_MS)
    , dspOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_DSP_TARGET_LATENCY_MS) { }

DisplayConfig::DisplayConfig()
    : driver(DisplayDriverAuto)
    , displayMode(DISPLAY_CONFIG_DEFAULT_MODE)
    , hasCustomDisplaySize(false)
    , displayWidth(0)
    , displayHeight(0)
    , bufferWidth(160)
    , bufferHeight(100)
    , hasCustomBufferSize(false)
    , maxFramesPerSecond(DISPLAY_CONFIG_DEFAULT_MAX_FRAMES_PER_SECOND)
    , showFpsEnabled(DISPLAY_CONFIG_DEFAULT_SHOW_FPS_ENABLED)
    , zoomMode(DISPLAY_CONFIG_DEFAULT_ZOOM_MODE) { }

#ifdef CTH_XWIN
X11Config::X11Config()
    : overrideRedirect(X11_CONFIG_DEFAULT_OVERRIDE_REDIRECT)
    , privateCmap(X11_CONFIG_DEFAULT_PRIVATE_CMAP)
    , mitShm(X11_CONFIG_DEFAULT_MIT_SHM)
    , rootWindow(X11_CONFIG_DEFAULT_ROOT_WINDOW)
    , fullscreen(X11_CONFIG_DEFAULT_FULLSCREEN)
    , windowPositionEnabled(X11_CONFIG_DEFAULT_WINDOW_POSITION_ENABLED)
    , windowPositionX(X11_CONFIG_DEFAULT_WINDOW_POSITION_X)
    , windowPositionY(X11_CONFIG_DEFAULT_WINDOW_POSITION_Y)
    , panelEnabled(X11_CONFIG_DEFAULT_PANEL_ENABLED)
    , fontName(X11_CONFIG_DEFAULT_FONT_NAME)
    , frameDumpDirectory(X11_CONFIG_DEFAULT_FRAME_DUMP_DIRECTORY)
    , frameDumpLimit(X11_CONFIG_DEFAULT_FRAME_DUMP_LIMIT)
    , frameDumpEvery(X11_CONFIG_DEFAULT_FRAME_DUMP_EVERY) { }
#endif

SDL3Config::SDL3Config()
    : highPixelDensityEnabled(SDL3_CONFIG_DEFAULT_HIGH_PIXEL_DENSITY_ENABLED)
    , resizableWindowEnabled(SDL3_CONFIG_DEFAULT_RESIZABLE_WINDOW_ENABLED)
    , rendererName(SDL3_CONFIG_DEFAULT_RENDERER_NAME)
    , frameDumpDirectory(SDL3_CONFIG_DEFAULT_FRAME_DUMP_DIRECTORY)
    , frameDumpLimit(SDL3_CONFIG_DEFAULT_FRAME_DUMP_LIMIT)
    , frameDumpEvery(SDL3_CONFIG_DEFAULT_FRAME_DUMP_EVERY) { }

AutoChangeConfig::AutoChangeConfig()
    : quietMs(AUTO_CHANGE_CONFIG_DEFAULT_QUIET_MS)
    , waitMinMs(AUTO_CHANGE_CONFIG_DEFAULT_WAIT_MIN_MS)
    , waitRandomMs(AUTO_CHANGE_CONFIG_DEFAULT_WAIT_RANDOM_MS)
    , waitRandomMinimumMs(AUTO_CHANGE_CONFIG_DEFAULT_WAIT_RANDOM_MIN_MS)
    , cumulativeFireLevel(AUTO_CHANGE_CONFIG_DEFAULT_CUMULATIVE_FIRE_LEVEL)
    , locked(AUTO_CHANGE_CONFIG_DEFAULT_LOCKED)
    , changeLittle(AUTO_CHANGE_CONFIG_DEFAULT_CHANGE_LITTLE) { }

AudioAnalysisConfig::AudioAnalysisConfig()
    : minNoise(AUDIO_ANALYSIS_CONFIG_DEFAULT_MIN_NOISE)
    , fireSensitivity(AUDIO_ANALYSIS_CONFIG_DEFAULT_FIRE_SENSITIVITY)
    , fireSource(AUDIO_ANALYSIS_CONFIG_DEFAULT_FIRE_SOURCE_TEXT) { }

EffectChoicePolicy::EffectChoicePolicy()
    : catalogEntryKey()
    , enabled(1) { }

EffectChoicePolicy::EffectChoicePolicy(
    const std::string& catalogEntryKeyValue, int enabledValue)
    : catalogEntryKey(catalogEntryKeyValue)
    , enabled(enabledValue) { }

EffectPresetPolicy::EffectPresetPolicy()
    : slot(0)
    , catalogName()
    , choiceText() { }

EffectPresetPolicy::EffectPresetPolicy(int slotValue,
    const std::string& catalogNameValue,
    const std::string& choiceTextValue)
    : slot(slotValue)
    , catalogName(catalogNameValue)
    , choiceText(choiceTextValue) { }

EffectPolicy::EffectPolicy()
    : imageFilesEnabled(EFFECT_POLICY_DEFAULT_IMAGE_FILES_ENABLED)
    , paletteSetFilterText(EFFECT_POLICY_DEFAULT_PALETTE_SET_FILTER_TEXT)
    , useTranslatesEnabled(EFFECT_POLICY_DEFAULT_USE_TRANSLATES_ENABLED)
    , useObjectsEnabled(EFFECT_POLICY_DEFAULT_USE_OBJECTS_ENABLED)
    , allowedChoices()
    , presets() { }

SceneTransitionPolicy::SceneTransitionPolicy()
    : paletteSmoothingChance(
        SCENE_TRANSITION_POLICY_DEFAULT_PALETTE_SMOOTHING_CHANCE)
    , paletteSmoothSeconds(
        SCENE_TRANSITION_POLICY_DEFAULT_PALETTE_SMOOTH_SECONDS) { }

SceneScriptConfig::SceneScriptConfig()
    : directory()
    , script() { }

MessagesConfig::MessagesConfig()
    : quietMessageMs(MESSAGES_CONFIG_DEFAULT_QUIET_MESSAGE_MS)
    , quietMessageDurationMs(MESSAGES_CONFIG_DEFAULT_QUIET_MESSAGE_DURATION_MS)
    , quietMessageFile(MESSAGES_CONFIG_DEFAULT_QUIET_MESSAGE_FILE_PATH)
    , qotdEnabled(MESSAGES_CONFIG_DEFAULT_QOTD_ENABLED)
    , qotdPrefetchTimeoutMs(MESSAGES_CONFIG_DEFAULT_QOTD_PREFETCH_TIMEOUT_MS)
    , qotdServer(MESSAGES_CONFIG_DEFAULT_QOTD_SERVER_TEXT)
    , qotdPort(MESSAGES_CONFIG_DEFAULT_QOTD_PORT_TEXT) { }

bool ConfigBuildResult::ok() const {
    for (std::vector<ConfigDiagnostic>::const_iterator it = diagnostics.begin();
         it != diagnostics.end(); ++it) {
        if (it->severity == ConfigDiagnosticError)
            return false;
    }
    return true;
}

Config ConfigSchema::build(const ConfigPatch& patch,
    DeferredLogBuffer& diagnostics) const {
    Config config;

    applyIntEntry(patch, diagnostics, KEY_LOGGING_VERBOSITY,
        &config.logging.verbosity);

    applyBoolEntry(patch, diagnostics, KEY_APP_OPTIONS_SAVE_ENABLED,
        &config.app.optionsSaveEnabled);

    applyBoolEntry(patch, diagnostics, KEY_INPUT_ESCAPE_ENABLED,
        &config.input.escapeKeyEnabled);
    if (const std::string* value = patch.value(KEY_INPUT_KEYMAP_FILE))
        config.input.keymapFile = *value;

    if (const std::string* value = patch.value(KEY_PATH_EXTRA_LIBRARY))
        config.paths.extraLibraryPath = *value;
    if (const std::string* value = patch.value(KEY_PATH_INI_OVERRIDE))
        config.paths.iniFileOverride = *value;

    if (const std::string* value = patch.value(KEY_SCENE_FLAME))
        config.scene.flame = *value;
    if (const std::string* value = patch.value(KEY_SCENE_GENERAL_FLAME))
        config.scene.generalFlame = *value;
    if (const std::string* value = patch.value(KEY_SCENE_WAVE))
        config.scene.wave = *value;
    if (const std::string* value = patch.value(KEY_SCENE_WAVE_SCALE))
        config.scene.waveScale = *value;
    if (const std::string* value = patch.value(KEY_SCENE_OBJECT))
        config.scene.object = *value;
    if (const std::string* value = patch.value(KEY_SCENE_TRANSLATION))
        config.scene.translation = *value;
    if (const std::string* value = patch.value(KEY_SCENE_PALETTE))
        config.scene.palette = *value;
    if (const std::string* value = patch.value(KEY_SCENE_BORDER))
        config.scene.border = *value;
    if (const std::string* value = patch.value(KEY_SCENE_FLASHLIGHT))
        config.scene.flashlight = *value;
    if (const std::string* value = patch.value(KEY_SCENE_TABLE))
        config.scene.table = *value;
    if (const std::string* value = patch.value(KEY_SCENE_IMAGE))
        config.scene.image = *value;
    if (const std::string* value = patch.value(KEY_SCENE_PRESENTATION))
        config.scene.presentation = *value;
    if (const std::string* value = patch.value(KEY_SCENE_AUDIO_PROCESSING))
        config.scene.audioProcessing = *value;

    if (const ConfigEntry* entry = patch.entry(KEY_AUDIO_INPUT_MODE)) {
        AudioInputMode mode = AUDIO_CONFIG_DEFAULT_INPUT_MODE;
        if (parseAudioMode(entry->value, &mode)) {
            config.audio.inputMode = mode;
        } else {
            diagnostics.error(entry->source, entry->key,
                "expected a supported audio input mode");
        }
    }

    if (const std::string* value = patch.value(KEY_AUDIO_INPUT_FILE))
        config.audio.inputFile = *value;

    applyBoolEntry(patch, diagnostics, KEY_AUDIO_INPUT_LOOP,
        &config.audio.inputLoopEnabled);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_SAMPLE_RATE, 0,
        &config.audio.sampleRateHz);
    applyAudioChannelsEntry(patch, diagnostics, &config.audio.channels);
    applyAudioSampleFormatEntry(patch, diagnostics, &config.audio.sampleFormat);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_DSP_METHOD, 0,
        &config.audio.dspMethod);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_DSP_FRAGMENTS, 0,
        &config.audio.dspFragments);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_DSP_FRAGMENT_SIZE, 0,
        &config.audio.dspFragmentSize);
    applyBoolEntry(patch, diagnostics, KEY_AUDIO_DSP_SYNC,
        &config.audio.dspSyncEnabled);
    applyBoolEntry(patch, diagnostics, KEY_AUDIO_SILENT,
        &config.audio.silentEnabled);
    applyAudioOutputDriverEntry(patch, diagnostics,
        &config.audio.outputDriver);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_PULSE_LATENCY, 50,
        &config.audio.pulseLatencyMs);
    if (const ConfigEntry* entry = patch.entry(KEY_AUDIO_PULSE_LATENCY)) {
        config.audio.pulseLatencyMs = clampIntWithWarning(
            config.audio.pulseLatencyMs, 50, 10000, *entry, diagnostics);
    }
    if (const std::string* value = patch.value(KEY_AUDIO_PULSE_SERVER))
        config.audio.pulseServer = *value;
    if (const std::string* value = patch.value(KEY_AUDIO_OUTPUT_DUMP))
        config.audio.outputDumpPath = *value;
    if (const std::string* value
        = patch.value(KEY_AUDIO_MINIAUDIO_PLAYBACK_DEVICE))
        config.audio.miniAudioPlaybackDeviceName = *value;
    if (const std::string* value
        = patch.value(KEY_AUDIO_MINIAUDIO_CAPTURE_DEVICE))
        config.audio.miniAudioCaptureDeviceName = *value;
    if (const std::string* value = patch.value(KEY_AUDIO_DSP_DEVICE))
        config.audio.dspDevicePath = *value;
    if (const std::string* value = patch.value(KEY_AUDIO_MIXER_DEVICE))
        config.audio.mixerDevicePath = *value;
    applyMixerInitialVolumeEntries(patch, diagnostics, &config.audio);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_NULL_TARGET_LATENCY, 0,
        &config.audio.nullOutputTargetLatencyMs);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_PULSE_TARGET_LATENCY, 0,
        &config.audio.pulseOutputTargetLatencyMs);
    applyMinimumIntEntry(patch, diagnostics,
        KEY_AUDIO_MINIAUDIO_TARGET_LATENCY, 0,
        &config.audio.miniAudioOutputTargetLatencyMs);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_DSP_TARGET_LATENCY, 0,
        &config.audio.dspOutputTargetLatencyMs);

    if (const ConfigEntry* entry = patch.entry(KEY_DISPLAY_DRIVER)) {
        DisplayDriverId driver = DisplayDriverAuto;
        if (parseDisplayDriverId(entry->value, &driver)) {
            config.display.driver = driver;
        } else {
            diagnostics.error(entry->source, entry->key,
                "expected one of: auto, x11, sdl3");
        }
    }

    if (const ConfigEntry* entry = patch.entry(KEY_DISPLAY_MODE)) {
        int mode = DISPLAY_CONFIG_DEFAULT_MODE;
        if (parseInteger(entry->value, &mode)) {
            int maxMode = int(sizeof(screenSizePresets) / sizeof(screenSizePresets[0])) - 1;
            if (mode != -1)
                mode = clampIntWithWarning(mode, 0, maxMode, *entry,
                    diagnostics);
            config.display.displayMode = mode;
        } else {
            diagnostics.error(entry->source, entry->key,
                "expected a display mode number or WIDTHxHEIGHT");
        }
    }

    const ConfigEntry* displayWidthEntry = patch.entry(KEY_DISPLAY_WIDTH);
    const ConfigEntry* displayHeightEntry = patch.entry(KEY_DISPLAY_HEIGHT);
    if (displayWidthEntry != NULL || displayHeightEntry != NULL) {
        int width = 0;
        int height = 0;
        if (displayWidthEntry == NULL || displayHeightEntry == NULL) {
            diagnostics.error("configuration", "display.size",
                "display width and height must be supplied together");
        } else if (!parseInteger(displayWidthEntry->value, &width)
            || !parseInteger(displayHeightEntry->value, &height)) {
            diagnostics.error(displayWidthEntry->source, "display.size",
                "expected WIDTHxHEIGHT display dimensions");
        } else if (width <= 0 || height <= 0) {
            diagnostics.error(displayWidthEntry->source, "display.size",
                "display dimensions must be positive");
        } else {
            config.display.hasCustomDisplaySize = true;
            config.display.displayMode = -1;
            config.display.displayWidth = width;
            config.display.displayHeight = height;
        }
    }

    if (const ConfigEntry* entry = patch.entry(KEY_BUFFER_PRESET)) {
        int preset = 0;
        if (parseInteger(entry->value, &preset)) {
            int maxPreset
                = int(sizeof(bufferSizePresets) / sizeof(bufferSizePresets[0])) - 1;
            preset = clampIntWithWarning(preset, 0, maxPreset, *entry,
                diagnostics);
            config.display.bufferWidth = bufferSizePresets[preset].width;
            config.display.bufferHeight = bufferSizePresets[preset].height;
            if (!config.display.hasCustomDisplaySize)
                config.display.displayMode
                    = std::max(config.display.displayMode, preset);
        } else {
            diagnostics.error(entry->source, entry->key,
                "expected a buffer preset number or WIDTHxHEIGHT");
        }
    }

    const ConfigEntry* bufferWidthEntry = patch.entry(KEY_BUFFER_WIDTH);
    const ConfigEntry* bufferHeightEntry = patch.entry(KEY_BUFFER_HEIGHT);
    if (bufferWidthEntry != NULL || bufferHeightEntry != NULL) {
        int width = 0;
        int height = 0;
        if (bufferWidthEntry == NULL || bufferHeightEntry == NULL) {
            diagnostics.error("configuration", "buffer.size",
                "buffer width and height must be supplied together");
        } else if (!parseInteger(bufferWidthEntry->value, &width)
            || !parseInteger(bufferHeightEntry->value, &height)) {
            diagnostics.error(bufferWidthEntry->source, "buffer.size",
                "expected WIDTHxHEIGHT buffer dimensions");
        } else if (width <= 0 || height <= 0) {
            diagnostics.error(bufferWidthEntry->source, "buffer.size",
                "buffer dimensions must be positive");
        } else {
            config.display.bufferWidth = width;
            config.display.bufferHeight = height;
            config.display.hasCustomBufferSize
                = patch.has(KEY_BUFFER_CUSTOM_SIZE);
        }
    }

    applyMinimumIntEntry(patch, diagnostics, KEY_DISPLAY_MAX_FPS, 0,
        &config.display.maxFramesPerSecond);
    applyBoolEntry(patch, diagnostics, KEY_DISPLAY_SHOW_FPS,
        &config.display.showFpsEnabled);
    applyClampedIntEntry(patch, diagnostics, KEY_DISPLAY_ZOOM_MODE, 0,
        ZOOM_MODE_MAX_EXCLUSIVE - 1, &config.display.zoomMode);

    applyBoolEntry(patch, diagnostics, KEY_EFFECT_IMAGE_FILES_ENABLED,
        &config.effectPolicy.imageFilesEnabled);
    if (const std::string* value = patch.value(KEY_EFFECT_PALETTE_SET_FILTER))
        config.effectPolicy.paletteSetFilterText = *value;
    applyBoolEntry(patch, diagnostics, KEY_EFFECT_USE_TRANSLATES_ENABLED,
        &config.effectPolicy.useTranslatesEnabled);
    applyBoolEntry(patch, diagnostics, KEY_EFFECT_USE_OBJECTS_ENABLED,
        &config.effectPolicy.useObjectsEnabled);
    applyEffectChoicePolicyEntries(patch, diagnostics, &config.effectPolicy);
    applyEffectPresetPolicyEntries(patch, diagnostics, &config.effectPolicy);

    applyClampedDoubleEntry(patch, diagnostics,
        KEY_SCENE_TRANSITION_PALETTE_SMOOTHING_CHANCE, 0.0, 1.0,
        &config.sceneTransition.paletteSmoothingChance);
    applyMinimumIntEntry(patch, diagnostics,
        KEY_SCENE_TRANSITION_PALETTE_SMOOTH_SECONDS, 0,
        &config.sceneTransition.paletteSmoothSeconds);
    if (const std::string* value = patch.value(KEY_SCENE_SCRIPT_DIRECTORY))
        config.sceneScript.directory = *value;
    if (const std::string* value = patch.value(KEY_SCENE_SCRIPT_FILE))
        config.sceneScript.script = *value;

    applyMinimumIntEntry(patch, diagnostics, KEY_AUTO_CHANGE_QUIET_MS, 0,
        &config.autoChange.quietMs);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUTO_CHANGE_WAIT_MIN_MS, 0,
        &config.autoChange.waitMinMs);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUTO_CHANGE_WAIT_RANDOM_MS, 0,
        &config.autoChange.waitRandomMs);
    applyMinimumIntEntry(patch, diagnostics,
        KEY_AUTO_CHANGE_CUMULATIVE_FIRE_LEVEL, 0,
        &config.autoChange.cumulativeFireLevel);
    applyBoolEntry(patch, diagnostics, KEY_AUTO_CHANGE_LOCKED,
        &config.autoChange.locked);
    applyBoolEntry(patch, diagnostics, KEY_AUTO_CHANGE_CHANGE_LITTLE,
        &config.autoChange.changeLittle);
    applyClampedIntEntry(patch, diagnostics, KEY_AUDIO_ANALYSIS_MIN_NOISE, 0,
        SOUND_MINNOISE_MAX_EXCLUSIVE - 1, &config.audioAnalysis.minNoise);
    applyClampedIntEntry(patch, diagnostics,
        KEY_AUDIO_ANALYSIS_FIRE_SENSITIVITY, 0, 100,
        &config.audioAnalysis.fireSensitivity);
    if (const ConfigEntry* entry = patch.entry(KEY_AUDIO_ANALYSIS_FIRE_SOURCE)) {
        std::string fireSource;
        if (normalizeFireSourceName(entry->value, &fireSource)) {
            config.audioAnalysis.fireSource = fireSource;
        } else {
            diagnostics.error(entry->source, entry->key,
                "expected one of: raw-amplitude, low-pass-150hz-amplitude");
        }
    }

    applyMinimumIntEntry(patch, diagnostics, KEY_MESSAGES_QUIET_MESSAGE_MS,
        0, &config.messages.quietMessageMs);
    applyMinimumIntEntry(patch, diagnostics,
        KEY_MESSAGES_QUIET_MESSAGE_DURATION_MS, 0,
        &config.messages.quietMessageDurationMs);
    if (const std::string* value = patch.value(KEY_MESSAGES_QUIET_MESSAGE_FILE))
        config.messages.quietMessageFile = *value;
    applyBoolEntry(patch, diagnostics, KEY_MESSAGES_QOTD_ENABLED,
        &config.messages.qotdEnabled);
    applyMinimumIntEntry(patch, diagnostics,
        KEY_MESSAGES_QOTD_PREFETCH_TIMEOUT_MS, 0,
        &config.messages.qotdPrefetchTimeoutMs);
    if (const std::string* value = patch.value(KEY_MESSAGES_QOTD_SERVER))
        config.messages.qotdServer = *value;
    if (const std::string* value = patch.value(KEY_MESSAGES_QOTD_PORT))
        config.messages.qotdPort = *value;

#ifdef CTH_XWIN
    applyBoolEntry(patch, diagnostics, KEY_X11_OVERRIDE_REDIRECT,
        &config.x11.overrideRedirect);
    applyBoolEntry(patch, diagnostics, KEY_X11_PRIVATE_CMAP,
        &config.x11.privateCmap);
    applyBoolEntry(patch, diagnostics, KEY_X11_MIT_SHM, &config.x11.mitShm);
    applyBoolEntry(patch, diagnostics, KEY_X11_ROOT_WINDOW,
        &config.x11.rootWindow);
    applyBoolEntry(patch, diagnostics, KEY_X11_FULLSCREEN,
        &config.x11.fullscreen);
    applyBoolEntry(patch, diagnostics, KEY_X11_WINDOW_POSITION_ENABLED,
        &config.x11.windowPositionEnabled);
    applyIntEntry(patch, diagnostics, KEY_X11_WINDOW_POSITION_X,
        &config.x11.windowPositionX);
    applyIntEntry(patch, diagnostics, KEY_X11_WINDOW_POSITION_Y,
        &config.x11.windowPositionY);
    applyBoolEntry(patch, diagnostics, KEY_X11_PANEL_ENABLED,
        &config.x11.panelEnabled);
    if (const std::string* value = patch.value(KEY_X11_FONT_NAME))
        config.x11.fontName = *value;
    if (const std::string* value = patch.value(KEY_X11_FRAME_DUMP_DIRECTORY))
        config.x11.frameDumpDirectory = *value;
    applyMinimumIntEntry(patch, diagnostics, KEY_X11_FRAME_DUMP_LIMIT, 1,
        &config.x11.frameDumpLimit);
    applyMinimumIntEntry(patch, diagnostics, KEY_X11_FRAME_DUMP_EVERY, 1,
        &config.x11.frameDumpEvery);
#endif

    applyBoolEntry(patch, diagnostics, KEY_SDL3_HIGH_PIXEL_DENSITY,
        &config.sdl3.highPixelDensityEnabled);
    applyBoolEntry(patch, diagnostics, KEY_SDL3_RESIZABLE_WINDOW,
        &config.sdl3.resizableWindowEnabled);
    if (const std::string* value = patch.value(KEY_SDL3_RENDERER_NAME))
        config.sdl3.rendererName = *value;
    if (const std::string* value = patch.value(KEY_SDL3_FRAME_DUMP_DIRECTORY))
        config.sdl3.frameDumpDirectory = *value;
    applyMinimumIntEntry(patch, diagnostics, KEY_SDL3_FRAME_DUMP_LIMIT, 1,
        &config.sdl3.frameDumpLimit);
    applyMinimumIntEntry(patch, diagnostics, KEY_SDL3_FRAME_DUMP_EVERY, 1,
        &config.sdl3.frameDumpEvery);

    return config;
}

ConfigAcquisitionStrategy::~ConfigAcquisitionStrategy() { }

DefaultsConfigSource::DefaultsConfigSource()
    : defaultsValue(hardcodedDefaultConfigPatch()) { }

DefaultsConfigSource::DefaultsConfigSource(const ConfigPatch& defaults)
    : defaultsValue(defaults) { }

ConfigPatch DefaultsConfigSource::acquire(DeferredLogBuffer& /* diagnostics */) const {
    return defaultsValue;
}

IniTextConfigSource::IniTextConfigSource(const std::string& sourceName,
    const std::string& text)
    : sourceNameValue(sourceName)
    , textValue(text) { }

ConfigPatch IniTextConfigSource::acquire(DeferredLogBuffer& diagnostics) const {
    ConfigPatch patch;
    std::istringstream input(textValue);
    std::string line;
    int lineNumber = 0;

    while (std::getline(input, line)) {
        std::string name;
        std::string value;
        lineNumber++;
        if (parseIniLine(line, &name, &value)) {
            applyIniOption(patch, diagnostics,
                sourceNameValue + ":" + integerText(lineNumber), name, value);
        }
    }

    return patch;
}

IniFileConfigSource::IniFileConfigSource(const std::string& path, bool optional)
    : pathValue(path)
    , optionalValue(optional) { }

ConfigPatch IniFileConfigSource::acquire(DeferredLogBuffer& diagnostics) const {
    std::ifstream input(pathValue.c_str(), std::ios::in | std::ios::binary);
    std::stringstream text;

    if (!input) {
        if (!optionalValue)
            diagnostics.error(pathValue, "ini-file", "could not open ini file");
        return ConfigPatch();
    }

    text << input.rdbuf();
    return IniTextConfigSource(pathValue, text.str()).acquire(diagnostics);
}

EnvironmentConfigSource::EnvironmentConfigSource(
    const std::map<std::string, std::string>& environment)
    : environmentValue(environment) { }

static void setEnvironmentPatchEntry(ConfigPatch& patch,
    const std::map<std::string, std::string>& environment,
    const char* environmentName, const char* configKey) {
    std::map<std::string, std::string>::const_iterator value
        = environment.find(environmentName);
    if (value == environment.end())
        return;

    patch.set(configKey, value->second,
        std::string("environment:") + environmentName);
}

ConfigPatch EnvironmentConfigSource::acquire(
    DeferredLogBuffer& /* diagnostics */) const {
    ConfigPatch patch;
    setEnvironmentPatchEntry(patch, environmentValue, "CTH_VERBOSE",
        KEY_LOGGING_VERBOSITY);
#ifdef CTH_XWIN
    setEnvironmentPatchEntry(patch, environmentValue, "CTHUGHA_DUMP_X11_FRAMES",
        KEY_X11_FRAME_DUMP_DIRECTORY);
    setEnvironmentPatchEntry(patch, environmentValue,
        "CTHUGHA_DUMP_X11_FRAME_LIMIT", KEY_X11_FRAME_DUMP_LIMIT);
    setEnvironmentPatchEntry(patch, environmentValue,
        "CTHUGHA_DUMP_X11_FRAME_EVERY", KEY_X11_FRAME_DUMP_EVERY);
#endif

    return patch;
}

CommandLineConfigSource::CommandLineConfigSource(
    const std::vector<std::string>& arguments)
    : argumentsValue(arguments) { }

CommandLineConfigSource::CommandLineConfigSource(
    std::vector<std::string>&& arguments)
    : argumentsValue(std::move(arguments)) { }

CommandLineConfigSource::CommandLineConfigSource(int argc, char* argv[])
    : argumentsValue(configArgumentsFromArgv(argc, argv)) { }

ConfigPatch CommandLineConfigSource::acquire(DeferredLogBuffer& diagnostics) const {
    ConfigPatch patch;

    for (int i = 1; i < int(argumentsValue.size()); i++)
        applyCommandLineOption(patch, diagnostics, argumentsValue, &i);

    return patch;
}

ConfigurationBuilder::ConfigurationBuilder()
    : helpRequestedValue(0)
    , versionRequestedValue(0) { }

ConfigurationBuilder::ConfigurationBuilder(const DeferredLogBuffer& diagnostics)
    : diagnosticsValue(diagnostics)
    , helpRequestedValue(0)
    , versionRequestedValue(0) { }

ConfigurationBuilder& ConfigurationBuilder::addSource(
    const ConfigAcquisitionStrategy& source) {
    ConfigPatch sourcePatch = source.acquire(diagnosticsValue);
    patchValue.mergeFrom(sourcePatch);
    return *this;
}

ConfigurationBuilder& ConfigurationBuilder::addDefaults(
    const ConfigPatch& defaults) {
    DefaultsConfigSource source(defaults);
    return addSource(source);
}

ConfigurationBuilder& ConfigurationBuilder::addDefaults() {
    DefaultsConfigSource source;
    return addSource(source);
}

ConfigurationBuilder& ConfigurationBuilder::addIniText(
    const std::string& sourceName, const std::string& text) {
    IniTextConfigSource source(sourceName, text);
    return addSource(source);
}

ConfigurationBuilder& ConfigurationBuilder::addIniFile(const std::string& path,
    bool optional) {
    IniFileConfigSource source(path, optional);
    return addSource(source);
}

ConfigurationBuilder& ConfigurationBuilder::addEnvironment(
    const std::map<std::string, std::string>& environment) {
    EnvironmentConfigSource source(environment);
    return addSource(source);
}

ConfigurationBuilder& ConfigurationBuilder::addEnvironmentVariables(
    const std::vector<std::string>& names) {
    return addEnvironment(processEnvironment(names));
}

ConfigurationBuilder& ConfigurationBuilder::addCommandLine(
    const std::vector<std::string>& args) {
    CommandLineConfigSource source(args);
    helpRequestedValue = helpRequestedValue || commandLineHelpRequested(args);
    versionRequestedValue
        = versionRequestedValue || commandLineVersionRequested(args);
    return addSource(source);
}

ConfigurationBuilder& ConfigurationBuilder::addCommandLine(
    std::vector<std::string>&& args) {
    helpRequestedValue = helpRequestedValue || commandLineHelpRequested(args);
    versionRequestedValue
        = versionRequestedValue || commandLineVersionRequested(args);
    CommandLineConfigSource source(std::move(args));
    return addSource(source);
}

ConfigurationBuilder& ConfigurationBuilder::addCommandLine(int argc,
    char* argv[]) {
    return addCommandLine(configArgumentsFromArgv(argc, argv));
}

const ConfigPatch& ConfigurationBuilder::patch() const {
    return patchValue;
}

ConfigBuildResult ConfigurationBuilder::build() const {
    DeferredLogBuffer diagnostics = diagnosticsValue;
    ConfigBuildResult result;

    result.config = schemaValue.build(patchValue, diagnostics);
    result.diagnostics = diagnostics.diagnostics();
    result.helpRequested = helpRequestedValue;
    result.versionRequested = versionRequestedValue;
    return result;
}

ConfigPatch hardcodedDefaultConfigPatch() {
    ConfigPatch defaults;

    defaults.set(KEY_LOGGING_VERBOSITY, integerText(LOGGING_CONFIG_DEFAULT_VERBOSITY),
        "defaults");
    defaults.set(KEY_APP_OPTIONS_SAVE_ENABLED,
        booleanText(APP_CONFIG_DEFAULT_OPTIONS_SAVE_ENABLED), "defaults");
    defaults.set(KEY_INPUT_ESCAPE_ENABLED,
        booleanText(INPUT_CONFIG_DEFAULT_ESCAPE_KEY_ENABLED), "defaults");
    defaults.set(KEY_INPUT_KEYMAP_FILE, INPUT_CONFIG_DEFAULT_KEYMAP_FILE_PATH,
        "defaults");
    defaults.set(KEY_PATH_EXTRA_LIBRARY, PATH_CONFIG_DEFAULT_EXTRA_LIBRARY_PATH,
        "defaults");
    defaults.set(KEY_PATH_INI_OVERRIDE, PATH_CONFIG_DEFAULT_INI_FILE_OVERRIDE_PATH,
        "defaults");
    defaults.set(KEY_SCENE_FLAME, "", "defaults");
    defaults.set(KEY_SCENE_GENERAL_FLAME, "", "defaults");
    defaults.set(KEY_SCENE_WAVE, "", "defaults");
    defaults.set(KEY_SCENE_WAVE_SCALE, "", "defaults");
    defaults.set(KEY_SCENE_OBJECT, "", "defaults");
    defaults.set(KEY_SCENE_TRANSLATION, "", "defaults");
    defaults.set(KEY_SCENE_PALETTE, "", "defaults");
    defaults.set(KEY_SCENE_BORDER, "", "defaults");
    defaults.set(KEY_SCENE_FLASHLIGHT, "", "defaults");
    defaults.set(KEY_SCENE_TABLE, "", "defaults");
    defaults.set(KEY_SCENE_IMAGE, "", "defaults");
    defaults.set(KEY_SCENE_PRESENTATION, "", "defaults");
    defaults.set(KEY_SCENE_AUDIO_PROCESSING, "", "defaults");
    defaults.set(KEY_AUTO_CHANGE_QUIET_MS,
        integerText(AUTO_CHANGE_CONFIG_DEFAULT_QUIET_MS), "defaults");
    defaults.set(KEY_AUTO_CHANGE_WAIT_MIN_MS,
        integerText(AUTO_CHANGE_CONFIG_DEFAULT_WAIT_MIN_MS), "defaults");
    defaults.set(KEY_AUTO_CHANGE_WAIT_RANDOM_MS,
        integerText(AUTO_CHANGE_CONFIG_DEFAULT_WAIT_RANDOM_MS), "defaults");
    defaults.set(KEY_AUTO_CHANGE_CUMULATIVE_FIRE_LEVEL,
        integerText(AUTO_CHANGE_CONFIG_DEFAULT_CUMULATIVE_FIRE_LEVEL),
        "defaults");
    defaults.set(KEY_AUTO_CHANGE_LOCKED,
        booleanText(AUTO_CHANGE_CONFIG_DEFAULT_LOCKED), "defaults");
    defaults.set(KEY_AUTO_CHANGE_CHANGE_LITTLE,
        booleanText(AUTO_CHANGE_CONFIG_DEFAULT_CHANGE_LITTLE), "defaults");
    defaults.set(KEY_AUDIO_ANALYSIS_MIN_NOISE,
        integerText(AUDIO_ANALYSIS_CONFIG_DEFAULT_MIN_NOISE), "defaults");
    defaults.set(KEY_AUDIO_ANALYSIS_FIRE_SENSITIVITY,
        integerText(AUDIO_ANALYSIS_CONFIG_DEFAULT_FIRE_SENSITIVITY),
        "defaults");
    defaults.set(KEY_AUDIO_ANALYSIS_FIRE_SOURCE,
        AUDIO_ANALYSIS_CONFIG_DEFAULT_FIRE_SOURCE_TEXT, "defaults");
    defaults.set(KEY_MESSAGES_QUIET_MESSAGE_MS,
        integerText(MESSAGES_CONFIG_DEFAULT_QUIET_MESSAGE_MS), "defaults");
    defaults.set(KEY_MESSAGES_QUIET_MESSAGE_DURATION_MS,
        integerText(MESSAGES_CONFIG_DEFAULT_QUIET_MESSAGE_DURATION_MS),
        "defaults");
    defaults.set(KEY_MESSAGES_QUIET_MESSAGE_FILE,
        MESSAGES_CONFIG_DEFAULT_QUIET_MESSAGE_FILE_PATH, "defaults");
    defaults.set(KEY_MESSAGES_QOTD_ENABLED,
        booleanText(MESSAGES_CONFIG_DEFAULT_QOTD_ENABLED), "defaults");
    defaults.set(KEY_MESSAGES_QOTD_PREFETCH_TIMEOUT_MS,
        integerText(MESSAGES_CONFIG_DEFAULT_QOTD_PREFETCH_TIMEOUT_MS),
        "defaults");
    defaults.set(KEY_MESSAGES_QOTD_SERVER,
        MESSAGES_CONFIG_DEFAULT_QOTD_SERVER_TEXT, "defaults");
    defaults.set(KEY_MESSAGES_QOTD_PORT,
        MESSAGES_CONFIG_DEFAULT_QOTD_PORT_TEXT, "defaults");
    defaults.set(KEY_AUDIO_INPUT_MODE,
        integerText(int(AUDIO_CONFIG_DEFAULT_INPUT_MODE)),
        "defaults");
    defaults.set(KEY_AUDIO_INPUT_FILE, AUDIO_CONFIG_DEFAULT_INPUT_FILE_PATH,
        "defaults");
    defaults.set(KEY_AUDIO_INPUT_LOOP,
        booleanText(AUDIO_CONFIG_DEFAULT_INPUT_LOOP_ENABLED), "defaults");
    defaults.set(KEY_AUDIO_SAMPLE_RATE,
        integerText(AUDIO_CONFIG_DEFAULT_SAMPLE_RATE_HZ), "defaults");
    defaults.set(KEY_AUDIO_CHANNELS, integerText(AUDIO_CONFIG_DEFAULT_CHANNELS),
        "defaults");
    defaults.set(KEY_AUDIO_SAMPLE_FORMAT,
        integerText(int(AUDIO_CONFIG_DEFAULT_FORMAT)), "defaults");
    defaults.set(KEY_AUDIO_DSP_METHOD,
        integerText(AUDIO_CONFIG_DEFAULT_DSP_METHOD), "defaults");
    defaults.set(KEY_AUDIO_DSP_FRAGMENTS,
        integerText(AUDIO_CONFIG_DEFAULT_DSP_FRAGMENTS), "defaults");
    defaults.set(KEY_AUDIO_DSP_FRAGMENT_SIZE,
        integerText(AUDIO_CONFIG_DEFAULT_DSP_FRAGMENT_SIZE), "defaults");
    defaults.set(KEY_AUDIO_DSP_SYNC,
        booleanText(AUDIO_CONFIG_DEFAULT_DSP_SYNC_ENABLED), "defaults");
    defaults.set(KEY_AUDIO_SILENT,
        booleanText(AUDIO_CONFIG_DEFAULT_SILENT_ENABLED), "defaults");
    defaults.set(KEY_AUDIO_OUTPUT_DRIVER,
        integerText(AUDIO_CONFIG_DEFAULT_OUTPUT_DRIVER), "defaults");
    defaults.set(KEY_AUDIO_PULSE_LATENCY,
        integerText(AUDIO_CONFIG_DEFAULT_PULSE_LATENCY_MS), "defaults");
    defaults.set(KEY_AUDIO_PULSE_SERVER, AUDIO_CONFIG_DEFAULT_PULSE_SERVER_TEXT,
        "defaults");
    defaults.set(KEY_AUDIO_OUTPUT_DUMP, AUDIO_CONFIG_DEFAULT_OUTPUT_DUMP_PATH,
        "defaults");
    defaults.set(KEY_AUDIO_DSP_DEVICE, AUDIO_CONFIG_DEFAULT_DSP_DEVICE_PATH,
        "defaults");
    defaults.set(KEY_AUDIO_MIXER_DEVICE, AUDIO_CONFIG_DEFAULT_MIXER_DEVICE_PATH,
        "defaults");
    defaults.set(KEY_AUDIO_NULL_TARGET_LATENCY,
        integerText(AUDIO_CONFIG_DEFAULT_NULL_TARGET_LATENCY_MS), "defaults");
    defaults.set(KEY_AUDIO_PULSE_TARGET_LATENCY,
        integerText(AUDIO_CONFIG_DEFAULT_PULSE_TARGET_LATENCY_MS), "defaults");
    defaults.set(KEY_AUDIO_MINIAUDIO_TARGET_LATENCY,
        integerText(AUDIO_CONFIG_DEFAULT_MINIAUDIO_TARGET_LATENCY_MS),
        "defaults");
    defaults.set(KEY_AUDIO_DSP_TARGET_LATENCY,
        integerText(AUDIO_CONFIG_DEFAULT_DSP_TARGET_LATENCY_MS), "defaults");
    defaults.set(KEY_DISPLAY_DRIVER, displayDriverIdName(DisplayDriverAuto),
        "defaults");
    defaults.set(KEY_DISPLAY_MODE, integerText(DISPLAY_CONFIG_DEFAULT_MODE),
        "defaults");
    defaults.set(KEY_DISPLAY_MAX_FPS,
        integerText(DISPLAY_CONFIG_DEFAULT_MAX_FRAMES_PER_SECOND),
        "defaults");
    defaults.set(KEY_DISPLAY_SHOW_FPS,
        booleanText(DISPLAY_CONFIG_DEFAULT_SHOW_FPS_ENABLED), "defaults");
    defaults.set(KEY_DISPLAY_ZOOM_MODE,
        integerText(DISPLAY_CONFIG_DEFAULT_ZOOM_MODE), "defaults");
    defaults.set(KEY_SDL3_HIGH_PIXEL_DENSITY,
        booleanText(SDL3_CONFIG_DEFAULT_HIGH_PIXEL_DENSITY_ENABLED),
        "defaults");
    defaults.set(KEY_SDL3_RESIZABLE_WINDOW,
        booleanText(SDL3_CONFIG_DEFAULT_RESIZABLE_WINDOW_ENABLED), "defaults");
    defaults.set(KEY_SDL3_RENDERER_NAME, SDL3_CONFIG_DEFAULT_RENDERER_NAME,
        "defaults");
    defaults.set(KEY_SDL3_FRAME_DUMP_DIRECTORY,
        SDL3_CONFIG_DEFAULT_FRAME_DUMP_DIRECTORY, "defaults");
    defaults.set(KEY_SDL3_FRAME_DUMP_LIMIT,
        integerText(SDL3_CONFIG_DEFAULT_FRAME_DUMP_LIMIT), "defaults");
    defaults.set(KEY_SDL3_FRAME_DUMP_EVERY,
        integerText(SDL3_CONFIG_DEFAULT_FRAME_DUMP_EVERY), "defaults");
    defaults.set(KEY_EFFECT_IMAGE_FILES_ENABLED,
        booleanText(EFFECT_POLICY_DEFAULT_IMAGE_FILES_ENABLED),
        "defaults");
    defaults.set(KEY_EFFECT_PALETTE_SET_FILTER,
        EFFECT_POLICY_DEFAULT_PALETTE_SET_FILTER_TEXT, "defaults");
    defaults.set(KEY_EFFECT_USE_TRANSLATES_ENABLED,
        booleanText(EFFECT_POLICY_DEFAULT_USE_TRANSLATES_ENABLED),
        "defaults");
    defaults.set(KEY_EFFECT_USE_OBJECTS_ENABLED,
        booleanText(EFFECT_POLICY_DEFAULT_USE_OBJECTS_ENABLED),
        "defaults");
    defaults.set(KEY_SCENE_TRANSITION_PALETTE_SMOOTHING_CHANCE,
        doubleText(SCENE_TRANSITION_POLICY_DEFAULT_PALETTE_SMOOTHING_CHANCE),
        "defaults");
    defaults.set(KEY_SCENE_TRANSITION_PALETTE_SMOOTH_SECONDS,
        integerText(SCENE_TRANSITION_POLICY_DEFAULT_PALETTE_SMOOTH_SECONDS),
        "defaults");
    defaults.set(KEY_SCENE_SCRIPT_DIRECTORY, "", "defaults");
    defaults.set(KEY_SCENE_SCRIPT_FILE, "", "defaults");
#ifdef CTH_XWIN
    defaults.set(KEY_X11_OVERRIDE_REDIRECT,
        booleanText(X11_CONFIG_DEFAULT_OVERRIDE_REDIRECT), "defaults");
    defaults.set(KEY_X11_PRIVATE_CMAP,
        booleanText(X11_CONFIG_DEFAULT_PRIVATE_CMAP), "defaults");
    defaults.set(KEY_X11_MIT_SHM, booleanText(X11_CONFIG_DEFAULT_MIT_SHM),
        "defaults");
    defaults.set(KEY_X11_ROOT_WINDOW,
        booleanText(X11_CONFIG_DEFAULT_ROOT_WINDOW), "defaults");
    defaults.set(KEY_X11_FULLSCREEN,
        booleanText(X11_CONFIG_DEFAULT_FULLSCREEN), "defaults");
    defaults.set(KEY_X11_WINDOW_POSITION_ENABLED,
        booleanText(X11_CONFIG_DEFAULT_WINDOW_POSITION_ENABLED), "defaults");
    defaults.set(KEY_X11_WINDOW_POSITION_X,
        integerText(X11_CONFIG_DEFAULT_WINDOW_POSITION_X), "defaults");
    defaults.set(KEY_X11_WINDOW_POSITION_Y,
        integerText(X11_CONFIG_DEFAULT_WINDOW_POSITION_Y), "defaults");
    defaults.set(KEY_X11_PANEL_ENABLED,
        booleanText(X11_CONFIG_DEFAULT_PANEL_ENABLED), "defaults");
    defaults.set(KEY_X11_FONT_NAME, X11_CONFIG_DEFAULT_FONT_NAME,
        "defaults");
    defaults.set(KEY_X11_FRAME_DUMP_DIRECTORY,
        X11_CONFIG_DEFAULT_FRAME_DUMP_DIRECTORY, "defaults");
    defaults.set(KEY_X11_FRAME_DUMP_LIMIT,
        integerText(X11_CONFIG_DEFAULT_FRAME_DUMP_LIMIT), "defaults");
    defaults.set(KEY_X11_FRAME_DUMP_EVERY,
        integerText(X11_CONFIG_DEFAULT_FRAME_DUMP_EVERY), "defaults");
#endif

    return defaults;
}

std::vector<std::string> configArgumentsFromArgv(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++)
        args.push_back(argv[i] ? argv[i] : "");
    return args;
}
