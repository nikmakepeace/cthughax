/** @file
 * Runtime ini persistence and continuation ini adapters.
 */

#include "IniFiles.h"

#include "cthugha.h"

#include <sstream>
#include <string>
#include <unistd.h>

static FILE* ini_file = NULL;

static const char* home_ini_file_name(const char* fileName) {
    static std::string fname;
    const char* home = getenv("HOME");

    if (home == NULL)
        return NULL;

    fname = std::string(home) + "/" + fileName;
    return fname.c_str();
}

static const char* automatic_ini_file_name() {
    return home_ini_file_name(".cthugha.auto");
}

static const char* continuation_ini_file_name() {
    return home_ini_file_name(".cthugha.continue");
}

ContinuationIniConfig::ContinuationIniConfig()
    : scene()
    , showFpsEnabled(0) { }

static int putini(const char* entry, const char* value) {
    if (ini_file == NULL)
        return 1;

    fprintf(ini_file, "Cthugha.%s: %s\n", entry, value);
    return 0;
}

int remove_continuation_ini(const PathConfig& paths) {
    if (paths.continuationIniFile.empty())
        return 0;

    if (unlink(paths.continuationIniFile.c_str()) == 0) {
        CTH_DEBUG("Removed continuation ini `%s'.\n",
            paths.continuationIniFile.c_str());
        return 0;
    }

    if (errno != ENOENT)
        CTH_ERRNO(errno, "Can not remove continuation ini `%s'.",
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

static void putini_int(const char* entry, int value) {
    std::string text = integer_text(value);
    putini(entry, text.c_str());
}

static void putini_bool(const char* entry, int value) {
    putini(entry, yes_no(value));
}

static void putini_double(const char* entry, double value) {
    std::string text = double_text(value);
    putini(entry, text.c_str());
}

static void putini_text_if_set(const char* entry, const std::string& value) {
    if (!value.empty())
        putini(entry, value.c_str());
}

static int begin_ini_output(const char* fname, const char* label,
    std::string* tmpName, FILE** previousIniFile) {
    if (fname == NULL) {
        CTH_ERROR("Can not create %s: HOME is not set.\n", label);
        return 1;
    }

    *tmpName = std::string(fname) + ".tmp";
    *previousIniFile = ini_file;
    FILE* out = fopen(tmpName->c_str(), "w");
    if (out == NULL) {
        CTH_ERRNO(errno, "Can not create %s `%s'.", label,
            tmpName->c_str());
        return 1;
    }

    ini_file = out;
    return 0;
}

static int finish_ini_output(const char* fname, const char* label,
    const std::string& tmpName, FILE* previousIniFile) {
    int write_error = ferror(ini_file);
    int close_error = fclose(ini_file);
    ini_file = previousIniFile;

    if (write_error || close_error) {
        unlink(tmpName.c_str());
        CTH_ERROR("Can not write %s `%s'.\n", label, tmpName.c_str());
        return 1;
    }

    if (rename(tmpName.c_str(), fname) != 0) {
        unlink(tmpName.c_str());
        CTH_ERRNO(errno, "Can not install %s `%s'.", label, fname);
        return 1;
    }

    return 0;
}

static void write_scene_config_ini(const SceneConfig& scene) {
    fprintf(ini_file,
        "#\n"
        "# Scene Controls\n"
        "#\n");
    putini_text_if_set("flame", scene.flame);
    putini_text_if_set("flame-general", scene.generalFlame);
    putini_text_if_set("wave", scene.wave);
    putini_text_if_set("wave-scale", scene.waveScale);
    putini_text_if_set("object", scene.object);
    putini_text_if_set("translation", scene.translation);
    putini_text_if_set("palette", scene.palette);
    putini_text_if_set("border", scene.border);
    putini_text_if_set("flashlight", scene.flashlight);
    putini_text_if_set("table", scene.table);
    putini_text_if_set("image", scene.image);
    putini_text_if_set("display", scene.presentation);
    putini_text_if_set("sound-processing", scene.audioProcessing);
}

static void write_transition_policy_ini(const SceneTransitionPolicy& policy) {
    fprintf(ini_file,
        "#\n"
        "# Scene Transition Policy\n"
        "#\n");
    putini_double("palette-smoothing", policy.paletteSmoothingChance);
    putini_int("palette-smooth-seconds", policy.paletteSmoothSeconds);
}

static void write_audio_analysis_config_ini(const AudioAnalysisConfig& audioAnalysis) {
    fprintf(ini_file,
        "#\n"
        "# Audio Analysis\n"
        "#\n");
    putini_int("min-noise", audioAnalysis.minNoise);
}

static void write_auto_change_config_ini(const AutoChangeConfig& autoChange) {
    fprintf(ini_file,
        "#\n"
        "# Timing/Automatic Changer Options\n"
        "#\n");
    putini_int("min-time", autoChange.waitMinMs);
    putini_int("random-time", autoChange.waitRandomMs);
    putini_int("quiet-change", autoChange.quietMs);
    putini_int("cumulative-fire-level", autoChange.cumulativeFireLevel);
    putini_bool("lock", autoChange.locked);
    putini_bool("little", autoChange.changeLittle);
}

static void write_messages_config_ini(const MessagesConfig& messages) {
    fprintf(ini_file,
        "#\n"
        "# Messages\n"
        "#\n");
    putini_int("change-msg-time", messages.quietMessageMs);
    putini_int("quiet-message-duration-ms", messages.quietMessageDurationMs);
    putini_text_if_set("quiet-file", messages.quietMessageFile);
    putini_bool("qotd", messages.qotdEnabled);
    putini_int("qotd-prefetch-timeout-ms", messages.qotdPrefetchTimeoutMs);
    putini_text_if_set("qotd-server", messages.qotdServer);
    putini_text_if_set("qotd-port", messages.qotdPort);
}

static void write_display_config_ini(const DisplayConfig& display) {
    fprintf(ini_file,
        "#\n"
        "# Display\n"
        "#\n");
    if (display.hasCustomDisplaySize)
        putini("disp-mode",
            dimensions_text(display.displayWidth, display.displayHeight).c_str());
    else
        putini_int("disp-mode", display.displayMode);

    putini("buff-size",
        dimensions_text(display.bufferWidth, display.bufferHeight).c_str());
    putini_int("max-fps", display.maxFramesPerSecond);
    putini_bool("show-fps", display.showFpsEnabled);
    putini_int("zoom", display.zoomMode);
}

static void write_effect_policy_ini(const EffectPolicy& policy) {
    fprintf(ini_file,
        "#\n"
        "# Effect Policy\n"
        "#\n");
    putini_bool("images", policy.imageFilesEnabled);
    putini_text_if_set("palette-set", policy.paletteSetFilterText);
    putini_bool("trans", policy.useTranslatesEnabled);
    putini_bool("use-objects", policy.useObjectsEnabled);

    for (std::vector<EffectChoicePolicy>::const_iterator it
             = policy.allowedChoices.begin();
         it != policy.allowedChoices.end(); ++it) {
        putini_bool(it->catalogEntryKey.c_str(), it->enabled);
    }

    fprintf(ini_file,
        "#\n"
        "# Effect Control Preset Slots\n"
        "#\n");
    for (std::vector<EffectPresetPolicy>::const_iterator it
             = policy.presets.begin();
         it != policy.presets.end(); ++it) {
        std::string key = "preset." + integer_text(it->slot) + "."
            + it->catalogName;
        putini(key.c_str(), it->choiceText.c_str());
    }
}

int write_ini(const Config& config) {
    const char* fname = automatic_ini_file_name();
    std::string tmpName;
    FILE* previousIniFile = NULL;
    if (begin_ini_output(fname, "automatic ini", &tmpName, &previousIniFile))
        return 1;

    fprintf(ini_file,
        "#\n"
        "# .cthugha.auto\n"
        "#\n"
        "# This file was created automatically, please do not edit.\n"
        "#\n");

    write_scene_config_ini(config.scene);
    write_transition_policy_ini(config.sceneTransition);
    write_audio_analysis_config_ini(config.audioAnalysis);
    write_auto_change_config_ini(config.autoChange);
    write_messages_config_ini(config.messages);
    write_display_config_ini(config.display);
    write_effect_policy_ini(config.effectPolicy);

    if (finish_ini_output(fname, "automatic ini", tmpName, previousIniFile))
        return 1;

    CTH_INFO("Saved startup configuration to `%s'.\n", fname);
    return 0;
}

int write_continuation_ini(const ContinuationIniConfig& config) {
    const char* fname = continuation_ini_file_name();
    std::string tmpName;
    FILE* previousIniFile = NULL;
    if (begin_ini_output(fname, "continuation ini", &tmpName, &previousIniFile))
        return 1;

    fprintf(ini_file,
        "#\n"
        "# .cthugha.continue\n"
        "#\n"
        "# One-shot stop-and-continue state. Cthugha deletes this file after startup.\n"
        "#\n");

    write_scene_config_ini(config.scene);
    fprintf(ini_file,
        "#\n"
        "# Display\n"
        "#\n");
    putini_bool("show-fps", config.showFpsEnabled);

    if (finish_ini_output(fname, "continuation ini", tmpName, previousIniFile))
        return 1;

    CTH_INFO("Saved continuation state to `%s'.\n", fname);
    return 0;
}
