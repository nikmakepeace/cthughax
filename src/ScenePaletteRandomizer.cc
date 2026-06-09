// Native random-palette mutation for Scene visual catalogs.

#include "ScenePaletteRandomizer.h"

#include "PaletteEntry.h"
#include "PaletteRandomGenerator.h"
#include "SceneTypedVisualCatalogs.h"
#include "cthugha.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <memory>
#include <strings.h>

namespace {

static const char* randomPaletteOutputDirectory = "resources/map";
static const char* randomPalettePrefix = "random";

static void copyBoundedString(
    char* target, unsigned int targetSize, const char* source) {
    if (targetSize == 0)
        return;

    std::strncpy(target, (source != 0) ? source : "", targetSize);
    target[targetSize - 1] = '\0';
}

static void randomPaletteNameForIndex(int index, char* name, size_t nameSize) {
    std::snprintf(name, nameSize, "%s.%d", randomPalettePrefix, index);
}

static void randomPaletteFileNameForIndex(
    int index, char* fileName, size_t fileNameSize) {
    char name[PATH_MAX];

    randomPaletteNameForIndex(index, name, sizeof(name));
    std::snprintf(fileName, fileNameSize, "%s/%s.map",
        randomPaletteOutputDirectory, name);
}

static int randomPaletteIndexFromName(const char* name, int* index) {
    size_t prefixLen = std::strlen(randomPalettePrefix);
    char* end = NULL;
    long value;

    if (name == 0)
        return 0;
    if (strncasecmp(name, randomPalettePrefix, prefixLen) != 0)
        return 0;
    if (name[prefixLen] != '.')
        return 0;
    if (name[prefixLen + 1] == '\0')
        return 0;

    errno = 0;
    value = std::strtol(name + prefixLen + 1, &end, 10);
    if ((errno != 0) || (end == name + prefixLen + 1) || (*end != '\0')
        || (value < 0) || (value > INT_MAX))
        return 0;

    *index = int(value);
    return 1;
}

static int randomPaletteIndexFromMapFileName(const char* fileName, int* index) {
    char name[PATH_MAX];
    size_t len = std::strlen(fileName);
    static const char extension[] = ".map";
    size_t extensionLen = sizeof(extension) - 1;

    if ((len <= extensionLen)
        || (strcasecmp(fileName + len - extensionLen, extension) != 0))
        return 0;
    if (len - extensionLen >= sizeof(name))
        return 0;

    std::memcpy(name, fileName, len - extensionLen);
    name[len - extensionLen] = '\0';
    return randomPaletteIndexFromName(name, index);
}

static const ScenePaletteChoice* paletteChoiceAt(
    const ScenePaletteChoiceSelection& selection, int index) {
    const SceneOptionSelection& optionSelection = selection;
    return dynamic_cast<const ScenePaletteChoice*>(
        optionSelection.choiceAt(index));
}

static int maxSceneRandomPaletteIndex(
    const ScenePaletteChoiceSelection& selection) {
    int maxIndex = -1;

    for (int i = 0; i < selection.entryCount(); i++) {
        int index;
        const ScenePaletteChoice* choice = paletteChoiceAt(selection, i);
        const PaletteEntry* entry
            = (choice != 0) ? choice->paletteEntry() : 0;
        if ((entry != 0) && randomPaletteIndexFromName(entry->Name(), &index))
            maxIndex = std::max(maxIndex, index);
    }

    return maxIndex;
}

static int maxPersistedRandomPaletteIndex() {
    DIR* directory = opendir(randomPaletteOutputDirectory);
    int maxIndex = -1;

    if (directory == NULL)
        return -1;

    while (struct dirent* entry = readdir(directory)) {
        int index;
        if (randomPaletteIndexFromMapFileName(entry->d_name, &index))
            maxIndex = std::max(maxIndex, index);
    }
    closedir(directory);

    return maxIndex;
}

static int nextRandomPaletteIndex(
    const ScenePaletteChoiceSelection& selection, int lastRandomIndex) {
    int maxIndex = std::max(maxSceneRandomPaletteIndex(selection),
        maxPersistedRandomPaletteIndex());
    maxIndex = std::max(maxIndex, lastRandomIndex);

    return maxIndex + 1;
}

static void randomPaletteOutputPathForEntry(
    const PaletteEntry& entry, int index, char* fileName, size_t fileNameSize) {
    if (entry.sourcePath[0] != '\0') {
        copyBoundedString(fileName, fileNameSize, entry.sourcePath);
        return;
    }

    randomPaletteFileNameForIndex(index, fileName, fileNameSize);
}

static void persistPaletteEntry(PaletteEntry& entry, int index) {
    char fileName[PATH_MAX];
    randomPaletteOutputPathForEntry(entry, index, fileName, sizeof(fileName));

    FILE* file = std::fopen(fileName, "w");
    if (file == NULL) {
        CTH_ERRNO(errno, "Can not open '%s' for random palette.", fileName);
        return;
    }

    std::fprintf(file, "%d %d %d  Random Palette from Cthugha\n",
        entry.colors().component(0, 0), entry.colors().component(0, 1),
        entry.colors().component(0, 2));
    for (int i = 1; i < 256; i++) {
        std::fprintf(file, "%d %d %d\n", entry.colors().component(i, 0),
            entry.colors().component(i, 1), entry.colors().component(i, 2));
    }
    std::fclose(file);

    copyBoundedString(entry.sourcePath, sizeof(entry.sourcePath), fileName);
}

static std::unique_ptr<PaletteEntry> createRandomPaletteEntry(
    int index, RandomSource& randomSource, const char* sourcePath) {
    char name[PATH_MAX];
    randomPaletteNameForIndex(index, name, sizeof(name));

    std::unique_ptr<PaletteEntry> entry(new PaletteEntry(name, "random"));
    copyBoundedString(entry->sourcePath, sizeof(entry->sourcePath), sourcePath);
    generateRandomPalette(entry->colors(), randomSource);
    persistPaletteEntry(*entry, index);
    return entry;
}

}

ScenePaletteRandomizer::ScenePaletteRandomizer()
    : lastRandomIndexValue(-1)
    , lastRandomSelectionIndexValue(-1) { }

int ScenePaletteRandomizer::randomizeLast(
    ScenePaletteChoiceSelection& selection, RandomSource& randomSource) {
    int replacementIndex = -1;
    int randomIndex = -1;
    PaletteEntry* currentEntry = selection.currentPaletteEntry();

    if ((currentEntry != 0)
        && randomPaletteIndexFromName(currentEntry->Name(), &randomIndex)) {
        replacementIndex = selection.currentValue();
    } else if ((lastRandomSelectionIndexValue >= 0)
        && (lastRandomSelectionIndexValue < selection.entryCount())) {
        replacementIndex = lastRandomSelectionIndexValue;
        randomIndex = lastRandomIndexValue;
    } else {
        return addRandom(selection, randomSource);
    }

    if (randomIndex < 0)
        return addRandom(selection, randomSource);

    const ScenePaletteChoice* existingChoice
        = paletteChoiceAt(selection, replacementIndex);
    const PaletteEntry* existingEntry
        = (existingChoice != 0) ? existingChoice->paletteEntry() : 0;
    const char* sourcePath
        = (existingEntry != 0) ? existingEntry->sourcePath : "";
    int inUse = (existingChoice != 0) ? existingChoice->inUse() : 1;

    std::unique_ptr<PaletteEntry> entry
        = createRandomPaletteEntry(randomIndex, randomSource, sourcePath);
    if (!selection.replacePaletteEntry(replacementIndex, *entry, inUse))
        return -1;

    lastRandomIndexValue = randomIndex;
    lastRandomSelectionIndexValue = replacementIndex;
    return replacementIndex;
}

int ScenePaletteRandomizer::addRandom(
    ScenePaletteChoiceSelection& selection, RandomSource& randomSource) {
    int randomIndex = nextRandomPaletteIndex(selection, lastRandomIndexValue);
    std::unique_ptr<PaletteEntry> entry
        = createRandomPaletteEntry(randomIndex, randomSource, "");
    int selectionIndex = selection.appendPaletteEntry(*entry, 1);
    if (selectionIndex < 0)
        return selectionIndex;

    lastRandomIndexValue = randomIndex;
    lastRandomSelectionIndexValue = selectionIndex;
    return selectionIndex;
}

int ScenePaletteRandomizer::lastRandomIndex() const {
    return lastRandomIndexValue;
}

int ScenePaletteRandomizer::lastRandomSelectionIndex() const {
    return lastRandomSelectionIndexValue;
}
