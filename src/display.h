/*
    CTHUGHA-L							display.h

    Funktions to access the Screen.
        - general stuff
        - screen-functions
        - palette-functions
        - text-functions
*/

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "cthugha.h"
#include "ColorPalette.h"
#include "EffectControl.h"

extern unsigned long bitmap_colors0[256]; /* "compiled" palette */
extern unsigned long bitmap_colors1[256];
extern unsigned long bitmap_colors2[256];
extern unsigned long bitmap_colors3[256];
extern int rev_byte_order;

/*
 *  Stuff about palettes
 */
const int PALETTE_METADATA_MAX_VALUES = 16;
const int PALETTE_METADATA_VALUE_SIZE = 64;

extern EffectChoiceList paletteEntries;

class RandomSource;

class PaletteEntry : public EffectChoice {
    ColorPalette paletteValue;
    void random(RandomSource& randomSource); // randomize this palette
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
    ColorPalette& colors() { return paletteValue; }
    const ColorPalette& colors() const { return paletteValue; }
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
     * Adds a new randomly generated palette using max(random.N) + 1 and saves
     * it under resources/map.
     *
     * @param randomSource Random source used for palette generation.
     */
    static void addRandom(RandomSource& randomSource);

    /**
     * Re-randomizes the currently selected random.N palette and saves that
     * map. If the current selection is not random.N, re-randomizes this
     * process's most recently generated palette, or adds one if none exists.
     *
     * @param randomSource Random source used for palette generation.
     */
    static void randomizeLast(RandomSource& randomSource);

    friend EffectChoice* read_palette(FILE*, const char*, const char*, const char*);
};

class PaletteOption : public EffectControl {
public:
    PaletteOption();

    PaletteEntry* currentPaletteEntry();
    const ColorPalette* currentPalette();
};

extern PaletteOption palette;

struct PathConfig;

int load_palettes(const PathConfig& pathConfig); /* initializiation */
int init_palettes();
int exit_palettes();
int update_palette();
void cth_setpalette(Palette pal, int immed);
int palette_set_filter(const char* value);
void apply_palette_set_filter();
struct EffectPolicy;
void configurePaletteOptions(const EffectPolicy& config);
int palette_set_metadata_set(PaletteEntry* palette, const char* value);
int palette_set_metadata_energy(PaletteEntry* palette, const char* value);

#endif
