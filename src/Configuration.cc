// Explicit startup configuration acquisition and typed config slices.

#include "Configuration.h"

#include "cth_buffer.h"
#include "configuration_defaults.h"
#include "defaults.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>

#ifndef CTH_LIBDIR
#define CTH_LIBDIR ""
#endif

namespace {

static const char* KEY_LOGGING_VERBOSITY = "logging.verbosity";
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
static const char* KEY_AUDIO_MIN_NOISE = "audio.min_noise";
static const char* KEY_AUDIO_PULSE_LATENCY = "audio.pulse_latency_ms";
static const char* KEY_AUDIO_PULSE_SERVER = "audio.pulse_server";
static const char* KEY_AUDIO_OUTPUT_DUMP = "audio.output_dump_path";
static const char* KEY_AUDIO_DSP_DEVICE = "audio.dsp_device_path";
static const char* KEY_AUDIO_MIXER_DEVICE = "audio.mixer_device_path";
static const char* KEY_AUDIO_MIXER_INITIAL_VOLUME
    = "audio.mixer_initial_volume";
static const char* KEY_AUDIO_NULL_TARGET_LATENCY = "audio.null_target_latency_ms";
static const char* KEY_AUDIO_PULSE_TARGET_LATENCY = "audio.pulse_target_latency_ms";
static const char* KEY_AUDIO_DSP_TARGET_LATENCY = "audio.dsp_target_latency_ms";
static const char* KEY_DISPLAY_MODE = "display.mode";
static const char* KEY_DISPLAY_WIDTH = "display.width";
static const char* KEY_DISPLAY_HEIGHT = "display.height";
static const char* KEY_BUFFER_PRESET = "buffer.preset";
static const char* KEY_BUFFER_WIDTH = "buffer.width";
static const char* KEY_BUFFER_HEIGHT = "buffer.height";
static const char* KEY_BUFFER_CUSTOM_SIZE = "buffer.custom_size";

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

static std::string integerText(int value) {
    return std::to_string(value);
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

static void setAudioBoolean(ConfigPatch& patch, const std::string& source,
    const char* key, int enabled) {
    patch.set(key, booleanText(enabled), source);
}

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
    setAudioBoolean(patch, source, key, enabled);
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
    } else if (key == "min-noise" || key == "minnoise") {
        patch.set(KEY_AUDIO_MIN_NOISE, cleanedValue, source);
    } else if (key == "pulse-latency-ms") {
        patch.set(KEY_AUDIO_PULSE_LATENCY, cleanedValue, source);
    } else if (key == "pulse-server") {
        patch.set(KEY_AUDIO_PULSE_SERVER, cleanedValue, source);
    } else if (key == "audio-output-dump") {
        patch.set(KEY_AUDIO_OUTPUT_DUMP, cleanedValue, source);
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
    } else if (key == "buff-size") {
        setBufferSize(patch, source, cleanedValue);
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
    } else if (arg == "--min-noise") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_MIN_NOISE, value, "command line");
    } else if (startsWith(arg, "--min-noise=")) {
        patch.set(KEY_AUDIO_MIN_NOISE, arg.substr(12), "command line");
    } else if (arg == "--pulse-latency-ms") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            patch.set(KEY_AUDIO_PULSE_LATENCY, value, "command line");
    } else if (startsWith(arg, "--pulse-latency-ms=")) {
        patch.set(KEY_AUDIO_PULSE_LATENCY, arg.substr(19), "command line");
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
    } else if (arg == "--buff-size") {
        std::string value;
        if (readOptionValue(args, index, arg, &value, diagnostics))
            setBufferSize(patch, "command line", value);
    } else if (startsWith(arg, "--buff-size=")) {
        setBufferSize(patch, "command line", arg.substr(12));
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
    } else if (arg == "-s") {
        setSceneFlashlight(patch, "command line", "",
            DEFAULT_FLASHLIGHT_DISABLE_INITIAL_ENTRY);
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

static std::string joinedPath(const std::string& directory,
    const std::string& fileName) {
    if (directory.empty())
        return fileName;
    if (directory[directory.size() - 1] == '/')
        return directory + fileName;
    return directory + "/" + fileName;
}

static void addHomeFile(std::vector<std::string>& files,
    const std::string& fileName) {
    const char* home = std::getenv("HOME");
    if (home == NULL || home[0] == '\0')
        return;

    files.push_back(joinedPath(home, fileName));
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

static std::vector<std::string> startupIniFiles(const ConfigPatch& commandLinePatch,
    std::string* continuationFile) {
    std::vector<std::string> files;
    const std::string* overridePath = commandLinePatch.value(KEY_PATH_INI_OVERRIDE);
    const std::string* extraPath = commandLinePatch.value(KEY_PATH_EXTRA_LIBRARY);

    if (overridePath != NULL && !overridePath->empty()) {
        files.push_back(*overridePath);
    } else {
        std::string libDir = CTH_LIBDIR;
        if (!libDir.empty())
            files.push_back(joinedPath(libDir, "cthugha.ini"));

        addHomeFile(files, ".cthugha.auto");
        addHomeFile(files, ".cthugha.ini");
        files.push_back("./cthugha.ini");

        if (extraPath != NULL && !extraPath->empty())
            files.push_back(joinedPath(*extraPath, "cthugha.ini"));
    }

    const char* home = std::getenv("HOME");
    continuationFile->clear();
    if (home != NULL && home[0] != '\0') {
        *continuationFile = joinedPath(home, ".cthugha.continue");
        files.push_back(*continuationFile);
    }

    return files;
}

static ConfigPatch acquireBootstrapCommandLineConfig(
    const std::vector<std::string>& args, DeferredLogBuffer& diagnostics) {
    ConfigPatch patch;

    for (int i = 1; i < int(args.size()); i++) {
        const std::string& arg = args[i];

        if (arg == "--path") {
            std::string value;
            if (readOptionValue(args, &i, arg, &value, diagnostics))
                patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(value),
                    "command line");
        } else if (startsWith(arg, "--path=")) {
            patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(arg.substr(7)),
                "command line");
        } else if (arg == "--ini-file") {
            std::string value;
            if (readOptionValue(args, &i, arg, &value, diagnostics))
                patch.set(KEY_PATH_INI_OVERRIDE, value, "command line");
        } else if (startsWith(arg, "--ini-file=")) {
            patch.set(KEY_PATH_INI_OVERRIDE, arg.substr(11), "command line");
        } else if (startsWith(arg, "-E")) {
            std::string value;
            if (readShortOptionValue(args, &i, arg, &value, diagnostics))
                patch.set(KEY_PATH_EXTRA_LIBRARY, withTrailingSlash(value),
                    "command line");
        }
    }

    return patch;
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
    : optionsSaveEnabled(APP_CONFIG_DEFAULT_OPTIONS_SAVE_ENABLED)
    , escapeKeyEnabled(APP_CONFIG_DEFAULT_ESCAPE_KEY_ENABLED)
    , keymapFile(PATH_CONFIG_DEFAULT_KEYMAP_FILE_PATH) { }

PathConfig::PathConfig()
    : extraLibraryPath(PATH_CONFIG_DEFAULT_EXTRA_LIBRARY_PATH)
    , iniFileOverride(PATH_CONFIG_DEFAULT_INI_FILE_OVERRIDE_PATH) { }

CatalogConfig::CatalogConfig()
    : doubleLoadEnabled(CATALOG_CONFIG_DEFAULT_DOUBLE_LOAD_ENABLED) { }

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
    , presentation() { }

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
    , minNoise(AUDIO_CONFIG_DEFAULT_MIN_NOISE)
    , mixerVolume(AUDIO_CONFIG_DEFAULT_MIXER_VOLUME)
    , pulseLatencyMs(AUDIO_CONFIG_DEFAULT_PULSE_LATENCY_MS)
    , pulseServer(AUDIO_CONFIG_DEFAULT_PULSE_SERVER_TEXT)
    , outputDumpPath(AUDIO_CONFIG_DEFAULT_OUTPUT_DUMP_PATH)
    , dspDevicePath(AUDIO_CONFIG_DEFAULT_DSP_DEVICE_PATH)
    , mixerDevicePath(AUDIO_CONFIG_DEFAULT_MIXER_DEVICE_PATH)
    , nullOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_NULL_TARGET_LATENCY_MS)
    , pulseOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_PULSE_TARGET_LATENCY_MS)
    , dspOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_DSP_TARGET_LATENCY_MS) { }

DisplayConfig::DisplayConfig()
    : displayMode(DISPLAY_CONFIG_DEFAULT_MODE)
    , hasCustomDisplaySize(false)
    , displayWidth(0)
    , displayHeight(0)
    , bufferWidth(160)
    , bufferHeight(100)
    , hasCustomBufferSize(false)
    , maxFramesPerSecond(DISPLAY_CONFIG_DEFAULT_MAX_FRAMES_PER_SECOND)
    , showFpsEnabled(DISPLAY_CONFIG_DEFAULT_SHOW_FPS_ENABLED)
    , zoomMode(DISPLAY_CONFIG_DEFAULT_ZOOM_MODE)
    , textOnTerm(DISPLAY_CONFIG_DEFAULT_TEXT_ON_TERM)
    , ncursesEnabled(DISPLAY_CONFIG_DEFAULT_NCURSES_ENABLED)
    , screenshotFilePrefix(DISPLAY_CONFIG_DEFAULT_SCREENSHOT_FILE_PREFIX)
    , x11OverrideRedirect(DISPLAY_CONFIG_DEFAULT_X11_OVERRIDE_REDIRECT)
    , x11PrivateCmap(DISPLAY_CONFIG_DEFAULT_X11_PRIVATE_CMAP)
    , x11MitShm(DISPLAY_CONFIG_DEFAULT_X11_MIT_SHM)
    , x11RootWindow(DISPLAY_CONFIG_DEFAULT_X11_ROOT_WINDOW)
    , x11Fullscreen(DISPLAY_CONFIG_DEFAULT_X11_FULLSCREEN)
    , x11WindowPositionEnabled(DISPLAY_CONFIG_DEFAULT_X11_WINDOW_POSITION_ENABLED)
    , x11WindowPositionX(DISPLAY_CONFIG_DEFAULT_X11_WINDOW_POSITION_X)
    , x11WindowPositionY(DISPLAY_CONFIG_DEFAULT_X11_WINDOW_POSITION_Y)
    , x11PanelEnabled(DISPLAY_CONFIG_DEFAULT_X11_PANEL_ENABLED)
    , x11FontName(DISPLAY_CONFIG_DEFAULT_X11_FONT_NAME) { }

AutoChangeConfig::AutoChangeConfig()
    : quietMs(AUTO_CHANGE_CONFIG_DEFAULT_QUIET_MS)
    , waitMinMs(AUTO_CHANGE_CONFIG_DEFAULT_WAIT_MIN_MS)
    , waitRandomMs(AUTO_CHANGE_CONFIG_DEFAULT_WAIT_RANDOM_MS)
    , waitRandomMinimumMs(AUTO_CHANGE_CONFIG_DEFAULT_WAIT_RANDOM_MIN_MS)
    , cumulativeFireLevel(AUTO_CHANGE_CONFIG_DEFAULT_CUMULATIVE_FIRE_LEVEL)
    , locked(AUTO_CHANGE_CONFIG_DEFAULT_LOCKED)
    , changeLittle(AUTO_CHANGE_CONFIG_DEFAULT_CHANGE_LITTLE) { }

VisualConfig::VisualConfig()
    : changeMessageMs(VISUAL_CONFIG_DEFAULT_CHANGE_MESSAGE_MS)
    , quietMessageDurationMs(VISUAL_CONFIG_DEFAULT_QUIET_MESSAGE_DURATION_MS)
    , paletteSmoothingChance(VISUAL_CONFIG_DEFAULT_PALETTE_SMOOTHING_CHANCE)
    , paletteSmoothSeconds(VISUAL_CONFIG_DEFAULT_PALETTE_SMOOTH_SECONDS)
    , imageLoadingEnabled(VISUAL_CONFIG_DEFAULT_IMAGE_LOADING_ENABLED)
    , paletteSetFilterText(VISUAL_CONFIG_DEFAULT_PALETTE_SET_FILTER_TEXT)
    , paletteSetFilterCount(VISUAL_CONFIG_DEFAULT_PALETTE_SET_FILTER_COUNT)
    , useTranslatesEnabled(VISUAL_CONFIG_DEFAULT_USE_TRANSLATES_ENABLED)
    , useObjectsEnabled(VISUAL_CONFIG_DEFAULT_USE_OBJECTS_ENABLED) { }

MessagesConfig::MessagesConfig()
    : qotdPrefetchTimeoutMs(MESSAGES_CONFIG_DEFAULT_QOTD_PREFETCH_TIMEOUT_MS)
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
    applyClampedIntEntry(patch, diagnostics, KEY_AUDIO_MIN_NOISE, 0,
        SOUND_MINNOISE_MAX_EXCLUSIVE - 1, &config.audio.minNoise);
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
    if (const std::string* value = patch.value(KEY_AUDIO_DSP_DEVICE))
        config.audio.dspDevicePath = *value;
    if (const std::string* value = patch.value(KEY_AUDIO_MIXER_DEVICE))
        config.audio.mixerDevicePath = *value;
    applyMixerInitialVolumeEntries(patch, diagnostics, &config.audio);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_NULL_TARGET_LATENCY, 0,
        &config.audio.nullOutputTargetLatencyMs);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_PULSE_TARGET_LATENCY, 0,
        &config.audio.pulseOutputTargetLatencyMs);
    applyMinimumIntEntry(patch, diagnostics, KEY_AUDIO_DSP_TARGET_LATENCY, 0,
        &config.audio.dspOutputTargetLatencyMs);

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
        } else {
            width = clampIntWithWarning(width, 64, MAX_BUFF_WIDTH,
                *bufferWidthEntry, diagnostics);
            height = clampIntWithWarning(height, 64, MAX_BUFF_HEIGHT,
                *bufferHeightEntry, diagnostics);
            config.display.bufferWidth = width;
            config.display.bufferHeight = height;
            config.display.hasCustomBufferSize
                = patch.has(KEY_BUFFER_CUSTOM_SIZE);
        }
    }

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

ConfigPatch EnvironmentConfigSource::acquire(
    DeferredLogBuffer& /* diagnostics */) const {
    ConfigPatch patch;
    std::map<std::string, std::string>::const_iterator verbose
        = environmentValue.find("CTH_VERBOSE");

    if (verbose != environmentValue.end())
        patch.set(KEY_LOGGING_VERBOSITY, verbose->second, "environment:CTH_VERBOSE");

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

ConfigurationBuilder::ConfigurationBuilder() { }

ConfigurationBuilder::ConfigurationBuilder(const DeferredLogBuffer& diagnostics)
    : diagnosticsValue(diagnostics) { }

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
    return addSource(source);
}

ConfigurationBuilder& ConfigurationBuilder::addCommandLine(
    std::vector<std::string>&& args) {
    CommandLineConfigSource source(std::move(args));
    return addSource(source);
}

ConfigurationBuilder& ConfigurationBuilder::addCommandLine(int argc,
    char* argv[]) {
    CommandLineConfigSource source(argc, argv);
    return addSource(source);
}

const ConfigPatch& ConfigurationBuilder::patch() const {
    return patchValue;
}

ConfigBuildResult ConfigurationBuilder::build() const {
    DeferredLogBuffer diagnostics = diagnosticsValue;
    ConfigBuildResult result;

    result.config = schemaValue.build(patchValue, diagnostics);
    result.diagnostics = diagnostics.diagnostics();
    return result;
}

ConfigPatch hardcodedDefaultConfigPatch() {
    ConfigPatch defaults;

    defaults.set(KEY_LOGGING_VERBOSITY, integerText(LOGGING_CONFIG_DEFAULT_VERBOSITY),
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
    defaults.set(KEY_AUDIO_MIN_NOISE,
        integerText(AUDIO_CONFIG_DEFAULT_MIN_NOISE), "defaults");
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
    defaults.set(KEY_AUDIO_DSP_TARGET_LATENCY,
        integerText(AUDIO_CONFIG_DEFAULT_DSP_TARGET_LATENCY_MS), "defaults");
    defaults.set(KEY_DISPLAY_MODE, integerText(DISPLAY_CONFIG_DEFAULT_MODE),
        "defaults");

    return defaults;
}

std::vector<std::string> configArgumentsFromArgv(int argc, char* argv[]) {
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++)
        args.push_back(argv[i] ? argv[i] : "");
    return args;
}

ConfigBuildResult buildStartupConfig(int argc, char* argv[]) {
    std::vector<std::string> args = configArgumentsFromArgv(argc, argv);
    DeferredLogBuffer bootstrapDiagnostics;

    // 1. Peek at CLI to determine paths
    ConfigPatch commandLinePatch
        = acquireBootstrapCommandLineConfig(args, bootstrapDiagnostics);
    std::string continuationFile;
    std::vector<std::string> iniFiles
        = startupIniFiles(commandLinePatch, &continuationFile);

    // 2. Supply the builder with our diagnostics buffer
    ConfigurationBuilder builder(bootstrapDiagnostics);
    builder.addDefaults();

    // Clean range-based loop with index tracking
    bool isFirst = true;
    for (const std::string& iniPath : iniFiles) {
        bool optional = true;
        if (isFirst && commandLinePatch.has(KEY_PATH_INI_OVERRIDE))
            optional = false;

        builder.addIniFile(iniPath, optional);
        isFirst = false;
    }

    // Modern initializer list
    builder.addEnvironmentVariables({ "CTH_VERBOSE" });

    // Move args into the builder since we don't need them anymore
    builder.addCommandLine(std::move(args));

    // 3. Assemble result
    ConfigBuildResult result = builder.build();
    result.config.paths.iniFiles = std::move(iniFiles);
    result.config.paths.continuationIniFile = std::move(continuationFile);
    return result;
}
