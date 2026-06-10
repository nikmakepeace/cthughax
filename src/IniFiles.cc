/** @file
 * Runtime ini persistence and continuation ini adapters.
 */

#include "IniFiles.h"

#include "ProcessServices.h"

#include <sstream>
#include <string>
#include <unistd.h>

static std::string home_ini_file_name(const char* fileName) {
    const char* home = getenv("HOME");

    if (home == NULL)
        return std::string();

    return std::string(home) + "/" + fileName;
}

static std::string automatic_ini_file_name() {
    return home_ini_file_name(".cthugha.auto");
}

static std::string continuation_ini_file_name() {
    return home_ini_file_name(".cthugha.continue");
}

ContinuationIniConfig::ContinuationIniConfig()
    : scene()
    , showFpsEnabled(0) { }

class IniWriter {
    FILE* fileValue;

public:
    explicit IniWriter(FILE* file)
        : fileValue(file) { }

    void section(const char* text) {
        fprintf(fileValue, "%s", text);
    }

    int put(const char* entry, const char* value) {
        if (fileValue == NULL)
            return 1;

        fprintf(fileValue, "Cthugha.%s: %s\n", entry, value);
        return 0;
    }

    FILE* file() const {
        return fileValue;
    }
};

int remove_continuation_ini(const PathConfig& paths, LogSink& log) {
    if (paths.continuationIniFile.empty())
        return 0;

    if (unlink(paths.continuationIniFile.c_str()) == 0) {
        log.debug("Removed continuation ini `%s'.\n",
            paths.continuationIniFile.c_str());
        return 0;
    }

    if (errno != ENOENT)
        log.errorErrno(errno, "Can not remove continuation ini `%s'.",
            paths.continuationIniFile.c_str());

    return 0;
}

static const char* yes_no(int value) {
    return value ? "yes" : "no";
}

static std::string integer_text(int value) {
    char text[64];
    snprintf(text, sizeof(text), "%d", value);
    return text;
}

static std::string double_text(double value) {
    std::ostringstream out;
    out.precision(12);
    out << value;
    return out.str();
}

static std::string dimensions_text(int width, int height) {
    return integer_text(width) + "x" + integer_text(height);
}

static void putini_int(IniWriter& writer, const char* entry, int value) {
    std::string text = integer_text(value);
    writer.put(entry, text.c_str());
}

static void putini_bool(IniWriter& writer, const char* entry, int value) {
    writer.put(entry, yes_no(value));
}

static void putini_double(IniWriter& writer, const char* entry, double value) {
    std::string text = double_text(value);
    writer.put(entry, text.c_str());
}

static void putini_text_if_set(IniWriter& writer, const char* entry,
    const std::string& value) {
    if (!value.empty())
        writer.put(entry, value.c_str());
}

static int begin_ini_output(const std::string& fname, const char* label,
    std::string* tmpName, FILE** outFile, LogSink& log) {
    if (fname.empty()) {
        log.error("Can not create %s: HOME is not set.\n", label);
        return 1;
    }

    *tmpName = fname + ".tmp";
    FILE* out = fopen(tmpName->c_str(), "w");
    if (out == NULL) {
        log.errorErrno(errno, "Can not create %s `%s'.", label,
            tmpName->c_str());
        return 1;
    }

    *outFile = out;
    return 0;
}

static int finish_ini_output(const std::string& fname, const char* label,
    const std::string& tmpName, FILE* outFile, LogSink& log) {
    int write_error = ferror(outFile);
    int close_error = fclose(outFile);

    if (write_error || close_error) {
        unlink(tmpName.c_str());
        log.error("Can not write %s `%s'.\n", label, tmpName.c_str());
        return 1;
    }

    if (rename(tmpName.c_str(), fname.c_str()) != 0) {
        unlink(tmpName.c_str());
        log.errorErrno(errno, "Can not install %s `%s'.", label,
            fname.c_str());
        return 1;
    }

    return 0;
}

static void write_scene_config_ini(IniWriter& writer, const SceneConfig& scene) {
    writer.section(
        "#\n"
        "# Scene Controls\n"
        "#\n");
    putini_text_if_set(writer, "flame", scene.flame);
    putini_text_if_set(writer, "flame-general", scene.generalFlame);
    putini_text_if_set(writer, "wave", scene.wave);
    putini_text_if_set(writer, "wave-scale", scene.waveScale);
    putini_text_if_set(writer, "object", scene.object);
    putini_text_if_set(writer, "translation", scene.translation);
    putini_text_if_set(writer, "palette", scene.palette);
    putini_text_if_set(writer, "border", scene.border);
    putini_text_if_set(writer, "flashlight", scene.flashlight);
    putini_text_if_set(writer, "table", scene.table);
    putini_text_if_set(writer, "image", scene.image);
    putini_text_if_set(writer, "display", scene.presentation);
    putini_text_if_set(writer, "sound-processing", scene.audioProcessing);
}

static void write_transition_policy_ini(IniWriter& writer,
    const SceneTransitionPolicy& policy) {
    writer.section(
        "#\n"
        "# Scene Transition Policy\n"
        "#\n");
    putini_double(writer, "palette-smoothing", policy.paletteSmoothingChance);
    putini_int(writer, "palette-smooth-seconds", policy.paletteSmoothSeconds);
}

static void write_audio_analysis_config_ini(IniWriter& writer,
    const AudioAnalysisConfig& audioAnalysis) {
    writer.section(
        "#\n"
        "# Audio Analysis\n"
        "#\n");
    putini_int(writer, "min-noise", audioAnalysis.minNoise);
    putini_int(writer, "fire-sensitivity", audioAnalysis.fireSensitivity);
    putini_text_if_set(writer, "fire-source", audioAnalysis.fireSource);
}

static void write_auto_change_config_ini(IniWriter& writer,
    const AutoChangeConfig& autoChange) {
    writer.section(
        "#\n"
        "# Timing/Automatic Changer Options\n"
        "#\n");
    putini_int(writer, "min-time", autoChange.waitMinMs);
    putini_int(writer, "random-time", autoChange.waitRandomMs);
    putini_int(writer, "quiet-change", autoChange.quietMs);
    putini_int(writer, "cumulative-fire-level",
        autoChange.cumulativeFireLevel);
    putini_bool(writer, "lock", autoChange.locked);
    putini_bool(writer, "little", autoChange.changeLittle);
}

static void write_messages_config_ini(IniWriter& writer,
    const MessagesConfig& messages) {
    writer.section(
        "#\n"
        "# Messages\n"
        "#\n");
    putini_int(writer, "change-msg-time", messages.quietMessageMs);
    putini_int(writer, "quiet-message-duration-ms",
        messages.quietMessageDurationMs);
    putini_text_if_set(writer, "quiet-file", messages.quietMessageFile);
    putini_bool(writer, "qotd", messages.qotdEnabled);
    putini_int(writer, "qotd-prefetch-timeout-ms",
        messages.qotdPrefetchTimeoutMs);
    putini_text_if_set(writer, "qotd-server", messages.qotdServer);
    putini_text_if_set(writer, "qotd-port", messages.qotdPort);
}

static void write_display_config_ini(IniWriter& writer,
    const DisplayConfig& display) {
    writer.section(
        "#\n"
        "# Display\n"
        "#\n");
    if (display.hasCustomDisplaySize)
        writer.put("disp-mode",
            dimensions_text(display.displayWidth, display.displayHeight).c_str());
    else
        putini_int(writer, "disp-mode", display.displayMode);

    writer.put("buff-size",
        dimensions_text(display.bufferWidth, display.bufferHeight).c_str());
    putini_int(writer, "max-fps", display.maxFramesPerSecond);
    putini_bool(writer, "show-fps", display.showFpsEnabled);
    putini_int(writer, "zoom", display.zoomMode);
}

static void write_effect_policy_ini(IniWriter& writer,
    const EffectPolicy& policy) {
    writer.section(
        "#\n"
        "# Effect Policy\n"
        "#\n");
    putini_bool(writer, "images", policy.imageFilesEnabled);
    putini_text_if_set(writer, "palette-set", policy.paletteSetFilterText);
    putini_bool(writer, "trans", policy.useTranslatesEnabled);
    putini_bool(writer, "use-objects", policy.useObjectsEnabled);

    for (std::vector<EffectChoicePolicy>::const_iterator it
             = policy.allowedChoices.begin();
         it != policy.allowedChoices.end(); ++it) {
        putini_bool(writer, it->catalogEntryKey.c_str(), it->enabled);
    }

    writer.section(
        "#\n"
        "# Effect Control Preset Slots\n"
        "#\n");
    for (std::vector<EffectPresetPolicy>::const_iterator it
             = policy.presets.begin();
         it != policy.presets.end(); ++it) {
        std::string key = "preset." + integer_text(it->slot) + "."
            + it->catalogName;
        writer.put(key.c_str(), it->choiceText.c_str());
    }
}

int write_ini(const Config& config, LogSink& log) {
    std::string fname = automatic_ini_file_name();
    std::string tmpName;
    FILE* outFile = NULL;
    if (begin_ini_output(fname, "automatic ini", &tmpName, &outFile, log))
        return 1;
    IniWriter writer(outFile);

    writer.section(
        "#\n"
        "# .cthugha.auto\n"
        "#\n"
        "# This file was created automatically, please do not edit.\n"
        "#\n");

    write_scene_config_ini(writer, config.scene);
    write_transition_policy_ini(writer, config.sceneTransition);
    write_audio_analysis_config_ini(writer, config.audioAnalysis);
    write_auto_change_config_ini(writer, config.autoChange);
    write_messages_config_ini(writer, config.messages);
    write_display_config_ini(writer, config.display);
    write_effect_policy_ini(writer, config.effectPolicy);

    if (finish_ini_output(fname, "automatic ini", tmpName, outFile, log))
        return 1;

    log.info("Saved startup configuration to `%s'.\n", fname.c_str());
    return 0;
}

int write_continuation_ini(const ContinuationIniConfig& config, LogSink& log) {
    std::string fname = continuation_ini_file_name();
    std::string tmpName;
    FILE* outFile = NULL;
    if (begin_ini_output(fname, "continuation ini", &tmpName, &outFile, log))
        return 1;
    IniWriter writer(outFile);

    writer.section(
        "#\n"
        "# .cthugha.continue\n"
        "#\n"
        "# One-shot stop-and-continue state. Cthugha deletes this file after startup.\n"
        "#\n");

    write_scene_config_ini(writer, config.scene);
    writer.section(
        "#\n"
        "# Display\n"
        "#\n");
    putini_bool(writer, "show-fps", config.showFpsEnabled);

    if (finish_ini_output(fname, "continuation ini", tmpName, outFile, log))
        return 1;

    log.info("Saved continuation state to `%s'.\n", fname.c_str());
    return 0;
}
