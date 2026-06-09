// Scene palette catalog loading.

#include "ScenePaletteCatalogLoader.h"

#include "config.h"

#include "ColorPalette.h"
#include "Configuration.h"
#include "PaletteEntry.h"
#include "ProcessServices.h"
#include "ScenePaletteCatalog.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <strings.h>
#include <vector>

// autoconf suggests this
#if HAVE_DIRENT_H
#include <dirent.h>
#else
#define dirent direct
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

namespace {

struct LoadedPalette {
    std::unique_ptr<PaletteEntry> palette;
    int inUse;

    explicit LoadedPalette(PaletteEntry* palette_)
        : palette(palette_)
        , inUse(1) { }
};

struct PaletteSetFilter {
    std::vector<std::string> values;
    std::string displayText;
};

static const char* palettePath[] = { "./", "./resources/map/",
    CTH_LIBDIR "/map/", "" };

static int compareFeatureFileNames(
    const std::string& left, const std::string& right) {
    int folded = strcasecmp(left.c_str(), right.c_str());

    return folded ? folded < 0 : left < right;
}

static int hasExtension(const std::string& name, const char* extension) {
    size_t extensionLen = std::strlen(extension);
    if (name.size() < extensionLen)
        return 0;

    return strcasecmp(name.c_str() + name.size() - extensionLen, extension) == 0;
}

static std::string featureNameForFile(
    const std::string& name, const char* extension) {
    size_t extensionLen = std::strlen(extension);
    for (size_t i = 0; i < name.size(); i++) {
        if (strncasecmp(name.c_str() + i, extension, extensionLen) == 0)
            return name.substr(0, i);
    }

    return name;
}

static int loadedPalettesHaveName(
    const std::vector<LoadedPalette>& palettes, const char* name) {
    for (std::vector<LoadedPalette>::const_iterator it = palettes.begin();
         it != palettes.end(); ++it) {
        if (strcasecmp(it->palette->Name(), name) == 0)
            return 1;
    }

    return 0;
}

static int appendFilterValue(PaletteSetFilter& filter,
    const char* begin, size_t length) {
    if (filter.values.size() >= PALETTE_METADATA_MAX_VALUES)
        return 0;
    if (length == 0 || length >= PALETTE_METADATA_VALUE_SIZE)
        return 0;

    filter.values.push_back(std::string(begin, length));
    return 1;
}

static int appendFilterDisplayValue(PaletteSetFilter& filter,
    const std::string& value, int quoted) {
    if (!filter.displayText.empty())
        filter.displayText += " ";
    if (quoted)
        filter.displayText += "\"";
    filter.displayText += value;
    if (quoted)
        filter.displayText += "\"";

    return filter.displayText.size() < 256;
}

static int parsePaletteSetFilter(
    const char* text, PaletteSetFilter& filter, LogSink& log) {
    filter.values.clear();
    filter.displayText.clear();

    if (text == NULL || text[0] == '\0')
        return 1;

    const char* pos = text;
    while (1) {
        while ((*pos != '\0') && isspace((unsigned char)*pos))
            pos++;
        if (*pos == '\0')
            break;

        const char* begin = pos;
        size_t length = 0;
        int quoted = 0;

        if ((*pos == '"') || (*pos == '\'')) {
            char quote = *pos;
            quoted = 1;
            pos++;
            begin = pos;
            while ((*pos != '\0') && (*pos != quote)) {
                if (!isalnum((unsigned char)*pos)
                    && !isspace((unsigned char)*pos))
                    goto malformed;
                length++;
                pos++;
            }
            if (*pos != quote)
                goto malformed;
            pos++;
            if ((*pos != '\0') && !isspace((unsigned char)*pos))
                goto malformed;
        } else {
            while ((*pos != '\0') && !isspace((unsigned char)*pos)) {
                if (!isalnum((unsigned char)*pos))
                    goto malformed;
                length++;
                pos++;
            }
        }

        while ((length > 0) && isspace((unsigned char)begin[length - 1]))
            length--;
        if (!appendFilterValue(filter, begin, length))
            goto malformed;
        if (!appendFilterDisplayValue(filter,
                std::string(begin, length), quoted))
            goto malformed;
    }

    if (filter.values.empty())
        goto malformed;

    return 1;

malformed:
    log.warn("Ignoring malformed palette set filter `%s'.\n", text);
    filter.values.clear();
    filter.displayText.clear();
    return 0;
}

static int paletteMatchesSetFilter(
    const PaletteEntry& palette, const PaletteSetFilter& filter) {
    if (filter.values.empty())
        return 1;

    for (std::vector<std::string>::const_iterator it = filter.values.begin();
         it != filter.values.end(); ++it) {
        for (int i = 0; i < palette.metadataSetCount; i++) {
            if (strcasecmp(it->c_str(), palette.metadataSets[i]) == 0)
                return 1;
        }
    }

    return 0;
}

static void applyPaletteSetFilter(
    std::vector<LoadedPalette>& palettes, const PaletteSetFilter& filter,
    LogSink& log) {
    if (filter.values.empty())
        return;

    int usable = 0;
    for (std::vector<LoadedPalette>::iterator it = palettes.begin();
         it != palettes.end(); ++it) {
        it->inUse = paletteMatchesSetFilter(*it->palette, filter);
        if (it->inUse)
            usable++;
    }

    if (usable == 0) {
        log.warn("Palette set filter `%s' matched no palettes; leaving all palettes enabled.\n",
            filter.displayText.c_str());
        for (std::vector<LoadedPalette>::iterator it = palettes.begin();
             it != palettes.end(); ++it)
            it->inUse = 1;
    } else {
        log.info("  palette set filter `%s': %d palettes enabled\n",
            filter.displayText.c_str(), usable);
    }
}

static PaletteEntry* createGeneralPalette() {
    PaletteEntry* palette = new PaletteEntry("general", "");
    ColorPalette& colors = palette->colors();

    for (int i = 0; i < 64; i++) {
        colors.setColor(i, i << 2, 0, 0);
        colors.setColor(i + 64, 0, i << 2, 0);
        colors.setColor(i + 128, 0, 0, i << 2);
        colors.setColor(i + 192, i << 2, i << 2, i << 2);
    }

    return palette;
}

static void brightenPalette(PaletteEntry& palette, int index, LogSink& log) {
    ColorPalette& colors = palette.colors();
    int maxBrightness = 0;

    for (int color = 0; color < 256; color++) {
        maxBrightness = std::max(maxBrightness,
            colors.component(color, 0) + colors.component(color, 1)
                + colors.component(color, 2));
    }

    if (maxBrightness <= 0 || maxBrightness >= 3 * 255)
        return;

    double factor = double(3 * 255) / double(maxBrightness);
    log.debug("brightening palette %d (%s). Faktor: %0.3f\n", index,
        palette.Name(), factor);

    for (int color = 0; color < 256; color++) {
        colors.setComponent(color, 0,
            int(double(colors.component(color, 0)) * factor));
        colors.setComponent(color, 1,
            int(double(colors.component(color, 1)) * factor));
        colors.setComponent(color, 2,
            int(double(colors.component(color, 2)) * factor));
    }
}

static void brightenPalettes(
    std::vector<LoadedPalette>& palettes, LogSink& log) {
    for (size_t i = 0; i < palettes.size(); i++)
        brightenPalette(*palettes[i].palette, int(i), log);
}

static void loadScenePaletteDirectory(std::vector<LoadedPalette>& palettes,
    const char* dir, const char* extension, LogSink& log) {
    DIR* directory;
    struct dirent* entry;
    std::vector<std::string> names;

    if ((directory = opendir(dir)) == NULL)
        return;

    while ((entry = readdir(directory)) != NULL) {
        if (hasExtension(entry->d_name, extension))
            names.push_back(entry->d_name);
    }
    closedir(directory);

    std::sort(names.begin(), names.end(), compareFeatureFileNames);

    for (std::vector<std::string>::const_iterator it = names.begin();
         it != names.end(); ++it) {
        std::string totalName = std::string(dir) + *it;
        std::string featureName = featureNameForFile(*it, extension);

        if (featureName.empty()) {
            log.debug("skipping empty feature name: %s\n", totalName.c_str());
            continue;
        }

        if (loadedPalettesHaveName(palettes, featureName.c_str())) {
            log.debug("already loaded: %s\n", totalName.c_str());
            continue;
        }

        log.debug("loading: %s", totalName.c_str());
        FILE* file = std::fopen(totalName.c_str(), "r");
        if (file == NULL) {
            log.debug(" ... x\n");
            continue;
        }

        PaletteEntry* palette = read_palette_entry(file, featureName.c_str(),
            dir, totalName.c_str());
        std::fclose(file);

        if (palette != NULL) {
            palettes.push_back(LoadedPalette(palette));
            log.debug(" ... OK\n");
        }
    }
}

}

int loadScenePaletteCatalog(ScenePaletteCatalog& catalog,
    const PathConfig& pathConfig, const char* paletteSetFilterText,
    LogSink& log) {
    catalog.clear();

    std::vector<LoadedPalette> loadedPalettes;
    PaletteSetFilter filter;

    log.info("  loading Scene palettes...\n");
    for (int path = 0; palettePath[path][0] != '\0'; path++)
        loadScenePaletteDirectory(loadedPalettes, palettePath[path], ".map", log);

    if (!pathConfig.extraLibraryPath.empty()) {
        std::string extraPathDir = pathConfig.extraLibraryPath + "/map/";
        loadScenePaletteDirectory(loadedPalettes, extraPathDir.c_str(), ".map",
            log);
    }

    if (!loadedPalettesHaveName(loadedPalettes, "general"))
        loadedPalettes.push_back(LoadedPalette(createGeneralPalette()));

    parsePaletteSetFilter(paletteSetFilterText, filter, log);
    applyPaletteSetFilter(loadedPalettes, filter, log);
    brightenPalettes(loadedPalettes, log);

    for (std::vector<LoadedPalette>::const_iterator it = loadedPalettes.begin();
         it != loadedPalettes.end(); ++it)
        catalog.addChoice(*it->palette, it->inUse);

    log.info("  number of Scene palettes: %d\n", catalog.entryCount());
    return 0;
}
