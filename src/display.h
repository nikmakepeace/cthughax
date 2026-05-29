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
#include "CoreOption.h"

/*
 *  initialize and shut down ncurses
 */
int init_ncurses();
void exit_ncurses();

extern int ncurses_use;

extern unsigned long bitmap_colors0[256]; /* "compiled" palette */
extern unsigned long bitmap_colors1[256];
extern unsigned long bitmap_colors2[256];
extern unsigned long bitmap_colors3[256];
extern int rev_byte_order;

/*
 *  Stuff about palettes
 */
typedef unsigned char Palette[256][3]; /* one Palette: 256 entries, each 3 bytes */

const int PALETTE_METADATA_MAX_VALUES = 16;
const int PALETTE_METADATA_VALUE_SIZE = 64;

extern CoreOptionEntryList paletteEntries;

class PaletteEntry : public CoreOptionEntry {
    void random(); // randomize this palette
public:
    Palette pal;
    char sourcePath[PATH_MAX];
    char metadataName[128];
    char metadataSet[256];
    char metadataEnergy[64];
    int metadataSetCount;
    char metadataSets[PALETTE_METADATA_MAX_VALUES][PALETTE_METADATA_VALUE_SIZE];
    int metadataEnergyCount;
    char metadataEnergies[4][16];

    PaletteEntry(const char* name, const char* desc)
        : CoreOptionEntry(name, desc) {
        sourcePath[0] = '\0';
        metadataName[0] = '\0';
        metadataSet[0] = '\0';
        metadataEnergy[0] = '\0';
        metadataSetCount = 0;
        metadataEnergyCount = 0;
    }
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

    static void Random(); // re-randomize the last random palette
    static void addRandom(); // add a new random palette

    friend CoreOptionEntry* read_palette(FILE*, const char*, const char*);
};

int load_palettes(); /* initializiation */
int init_palettes();
int exit_palettes();
int update_palette();
void cth_setpalette(Palette pal, int immed);
int palette_set_filter(const char* value);
void apply_palette_set_filter();
int palette_set_metadata_set(PaletteEntry* palette, const char* value);
int palette_set_metadata_energy(PaletteEntry* palette, const char* value);

/*
 *  Stuff for PCX
 */
class CthughaFrameBuffer;

int init_pcx(); /* initalize pcx */
int show_pcx(); /* request a one-shot PCX image stage */
int show_pcx(CthughaFrameBuffer& frameBuffer); /* execute the PCX image stage */
extern int display_use_pcx; /* allow pcx-usage */
extern int* pcx_palettes; /* index to corresp. palette */

extern char display_prt_file[]; /* filename used by PrtScrn */
char* prtFileName(const char* ext);

int save_pcx(unsigned char* v, int w, int h, Palette pal);

#endif
