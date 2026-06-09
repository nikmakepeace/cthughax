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
#include "PaletteOption.h"

extern unsigned long bitmap_colors0[256]; /* "compiled" palette */
extern unsigned long bitmap_colors1[256];
extern unsigned long bitmap_colors2[256];
extern unsigned long bitmap_colors3[256];
extern int rev_byte_order;

/*
 *  Stuff about palettes
 */
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
