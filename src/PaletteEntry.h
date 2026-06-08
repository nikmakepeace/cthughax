// Palette catalog entry shared by Scene and Frame Generator.

#ifndef CTHUGHA_PALETTE_ENTRY_H
#define CTHUGHA_PALETTE_ENTRY_H

#include "cthugha.h"
#include "ColorPalette.h"
#include "EffectControl.h"

#include <stdio.h>

class RandomSource;

const int PALETTE_METADATA_MAX_VALUES = 16;
const int PALETTE_METADATA_VALUE_SIZE = 64;

class PaletteEntry;

/**
 * Reads a .map palette file into an owned palette entry.
 *
 * @param file Open palette file positioned at the beginning.
 * @param name Palette option/display name.
 * @param dir Directory containing the palette, for diagnostics.
 * @param totalName Full palette path, stored as source metadata.
 * @return Newly allocated palette entry, or NULL on invalid/empty input.
 */
PaletteEntry* read_palette_entry(FILE* file, const char* name, const char* dir,
    const char* totalName);

/**
 * Runtime palette catalog entry with optional file-backed metadata.
 */
class PaletteEntry : public EffectChoice {
    ColorPalette paletteValue;
    void random(RandomSource& randomSource);

public:
    char sourcePath[PATH_MAX];
    char metadataName[128];
    char metadataSet[256];
    char metadataEnergy[64];
    int metadataSetCount;
    char metadataSets[PALETTE_METADATA_MAX_VALUES][PALETTE_METADATA_VALUE_SIZE];
    int metadataEnergyCount;
    char metadataEnergies[4][16];

    PaletteEntry(const char* name, const char* desc)
        : EffectChoice(name, desc) {
        sourcePath[0] = '\0';
        metadataName[0] = '\0';
        metadataSet[0] = '\0';
        metadataEnergy[0] = '\0';
        metadataSetCount = 0;
        metadataEnergyCount = 0;
    }

    /** @return Mutable indexed palette colors. */
    ColorPalette& colors() { return paletteValue; }

    /** @return Immutable indexed palette colors. */
    const ColorPalette& colors() const { return paletteValue; }

    /**
     * Sets display metadata name and mirrors it into the description.
     *
     * @param value Metadata name text.
     */
    void setMetadataName(const char* value) {
        strncpy(metadataName, value, sizeof(metadataName));
        metadataName[sizeof(metadataName) - 1] = '\0';

        char* newDesc = new char[strlen(metadataName) + 1];
        strcpy(newDesc, metadataName);
        delete[] desc;
        desc = newDesc;
    }

    PaletteEntry(FILE* file, const char* name);

    static char randomName[PATH_MAX];
    static int lastRandom;
    static int lastRandomPos;

    /**
     * Creates, randomizes, registers, and persists the next random palette.
     *
     * @param randomSource Random source used to generate palette colors.
     */
    static void addRandom(RandomSource& randomSource);

    /**
     * Re-randomizes the currently selected random.N palette, or creates one.
     *
     * @param randomSource Random source used to generate palette colors.
     */
    static void randomizeLast(RandomSource& randomSource);

    friend EffectChoice* read_palette(FILE*, const char*, const char*, const char*);
};

#endif
