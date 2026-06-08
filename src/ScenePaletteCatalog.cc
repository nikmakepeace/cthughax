// Native Scene catalog owner for palette entries.

#include "ScenePaletteCatalog.h"

#include "PaletteEntry.h"

#include <string.h>

namespace {

static void copyBoundedString(char* target, unsigned int targetSize,
    const char* source) {
    if (targetSize == 0)
        return;

    strncpy(target, (source != 0) ? source : "", targetSize);
    target[targetSize - 1] = '\0';
}

static void copyPaletteMetadata(
    PaletteEntry& target, const PaletteEntry& source) {
    static const unsigned int energyCount
        = sizeof(target.metadataEnergies) / sizeof(target.metadataEnergies[0]);

    copyBoundedString(target.sourcePath, sizeof(target.sourcePath),
        source.sourcePath);
    copyBoundedString(target.metadataName, sizeof(target.metadataName),
        source.metadataName);
    copyBoundedString(target.metadataSet, sizeof(target.metadataSet),
        source.metadataSet);
    copyBoundedString(target.metadataEnergy, sizeof(target.metadataEnergy),
        source.metadataEnergy);

    target.metadataSetCount = source.metadataSetCount;
    if (target.metadataSetCount < 0)
        target.metadataSetCount = 0;
    if (target.metadataSetCount > PALETTE_METADATA_MAX_VALUES)
        target.metadataSetCount = PALETTE_METADATA_MAX_VALUES;
    for (int i = 0; i < PALETTE_METADATA_MAX_VALUES; i++) {
        copyBoundedString(target.metadataSets[i], sizeof(target.metadataSets[i]),
            (i < target.metadataSetCount) ? source.metadataSets[i] : "");
    }

    target.metadataEnergyCount = source.metadataEnergyCount;
    if (target.metadataEnergyCount < 0)
        target.metadataEnergyCount = 0;
    if (target.metadataEnergyCount > int(energyCount))
        target.metadataEnergyCount = int(energyCount);
    for (unsigned int i = 0; i < energyCount; i++) {
        copyBoundedString(target.metadataEnergies[i],
            sizeof(target.metadataEnergies[i]),
            (i < unsigned(target.metadataEnergyCount))
                ? source.metadataEnergies[i]
                : "");
    }
}

static PaletteEntry* copyPaletteEntry(const PaletteEntry& palette) {
    PaletteEntry* copy = new PaletteEntry(palette.Name(), palette.Desc());
    copy->colors().copyFrom(palette.colors());
    copyPaletteMetadata(*copy, palette);
    return copy;
}

}

ScenePaletteCatalog::Entry::Entry(
    const PaletteEntry& palette_, int inUse_)
    : paletteValue(copyPaletteEntry(palette_))
    , inUseValue(inUse_) { }

ScenePaletteCatalog::Entry::~Entry() { }

const PaletteEntry* ScenePaletteCatalog::Entry::palette() const {
    return paletteValue.get();
}

int ScenePaletteCatalog::Entry::inUse() const {
    return inUseValue;
}

ScenePaletteCatalog::ScenePaletteCatalog()
    : entries() { }

void ScenePaletteCatalog::clear() {
    entries.clear();
}

void ScenePaletteCatalog::addChoice(const PaletteEntry& palette, int inUse) {
    entries.push_back(std::unique_ptr<Entry>(
        new Entry(palette, inUse)));
}

int ScenePaletteCatalog::entryCount() const {
    return int(entries.size());
}

const PaletteEntry* ScenePaletteCatalog::paletteAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return 0;

    return entries[index]->palette();
}

int ScenePaletteCatalog::inUseAt(int index) const {
    if ((index < 0) || (index >= int(entries.size())))
        return 0;

    return entries[index]->inUse();
}
