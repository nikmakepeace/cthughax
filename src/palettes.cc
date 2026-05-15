#include "cthugha.h"
#include "display.h"
#include "Interface.h"
#include "disp-sys.h"
#include "imath.h"
#include "CthughaDisplay.h"
#include "CthughaBuffer.h"

#include "initial_palettes.cc"

CoreOptionEntryList paletteEntries;

int change_palette_imm(int);

int display_internal_pal = 1; /* use internal pal. */
int display_external_pal = 1; /* use external pal. */

unsigned long bitmap_colors0[256]; /* "compiled" palette */
unsigned long bitmap_colors1[256]; /* "compiled" palette */
unsigned long bitmap_colors2[256]; /* "compiled" palette */
unsigned long bitmap_colors3[256]; /* "compiled" palette */

CoreOptionEntry* read_palette(FILE* file, const char* name, const char* dir, const char*);
static const char* palette_path[] = { "./", "./map/", CTH_LIBDIR "/map/", "" };

int colormapped = 1; /* 0 .. True/Direct color
                        1 .. All 256 color cells
                        2 .. Only some cells */

/* when using only some colorcells (e.g. when drawing on the root window)
   update to the palette are done immediately without smoothing.
   */

int load_palettes() {
    int i, l;

    /* Builtin palettes */
    if (display_internal_pal) {
        char str[128];
        PaletteEntry* new_pal;
        Palette* pal;

        CTH_INFO("  preparing internal palettes...\n");

        for (i = 0; (unsigned int)i < sizeof(initial_palettes) / sizeof(Palette); i++) {
            sprintf(str, "Internal_%d", i);
            new_pal = new PaletteEntry(str, "");
            memcpy(new_pal->pal, initial_palettes[i], sizeof(Palette));
            CthughaBuffer::current->palette.add(new_pal);
        }

        /* create one general palette */
        sprintf(str, "Internal_%d", CthughaBuffer::current->palette.getNEntries());
        new_pal = new PaletteEntry(str, "");
        pal = &(new_pal->pal);
        for (i = 0; i < 64; i++) {
            (*pal)[i][0] = i << 2;
            (*pal)[i][1] = 0;
            (*pal)[i][2] = 0;

            (*pal)[i + 64][0] = 0;
            (*pal)[i + 64][1] = i << 2;
            (*pal)[i + 64][2] = 0;

            (*pal)[i + 128][0] = 0;
            (*pal)[i + 128][1] = 0;
            (*pal)[i + 128][2] = i << 2;

            (*pal)[i + 192][0] = i << 2;
            (*pal)[i + 192][1] = i << 2;
            (*pal)[i + 192][2] = i << 2;
        }
        CthughaBuffer::current->palette.add(new_pal);
    }

    /* read palettes from file */
    if (display_external_pal) {
        CTH_INFO("  loading external palettes...\n");
        CthughaBuffer::current->palette.load(palette_path, "/map/", ".map", read_palette);
        CTH_INFO("  number of loaded palettes: %d\n", CthughaBuffer::current->palette.getNEntries());
    }

    /* brighten up palettes, that are a bit dark */
    for (i = 0; i < CthughaBuffer::current->palette.getNEntries(); i++) {
        Palette* pal = &(((PaletteEntry*)CthughaBuffer::current->palette[i])->pal);
        int m = 0;
        double P = 0;

        for (l = 0; l < 256; l++) {
            m = max(m, (*pal)[l][0] + (*pal)[l][1] + (*pal)[l][2]);
        }
        if ((m > 0) && (m < 3 * 255)) {
            P = double(3 * 255) / double(m);
            CTH_TRACE("    brightening palette %d (%s). Faktor: %0.3f\n", i,
                CthughaBuffer::current->palette[i]->Name(), P);

            for (l = 0; l < 256; l++) {
                (*pal)[l][0] = min((int)((double)(*pal)[l][0] * P), 255);
                (*pal)[l][1] = min((int)((double)(*pal)[l][1] * P), 255);
                (*pal)[l][2] = min((int)((double)(*pal)[l][2] * P), 255);
            }
        }
    }

    return 0;
}

/*
 * store the palette with 8 bytes per color. 
 * palettes are 256 entries of 8 bit rgb. Shorter palettes are filled up with black.
 * longer palettes are truncated.
 */
CoreOptionEntry* read_palette(
    FILE* file, const char* name, const char* /*dir*/, const char* /*total_name*/) {
    char line[256];
    int i, j, r, g, b;
    bool read_all_entries = true;
    Palette* pal;
    PaletteEntry* new_pal = new PaletteEntry(name, "");

    pal = &(new_pal->pal);

    for (i = 0; i < 256; i++) {
        if (fgets(line, sizeof(line), file) == NULL) {
            CTH_DEBUG("\n    Reached end of palette after %d entries (%s) ... filling with black", i,
                name);
            read_all_entries = false;
            if (i == 0) { /* nothing read */
                CTH_DEBUG(" ... skipping file");
                delete new_pal;
                return NULL;
            }
            for (; i < 256; i++) /* fill with black */
                for (j = 0; j < 3; j++)
                    (*pal)[i][j] = 0;
            break;
        }

        if (sscanf(line, "%d %d %d", &r, &g, &b) < 3) {
            CTH_WARN("\n    Malformed palette line: %d (%s) ... replacing with black", i + 1, name);
            r = g = b = 0;
        }

        (*pal)[i][0] = r;
        (*pal)[i][1] = g;
        (*pal)[i][2] = b;
    }

    if (read_all_entries) {
        while (fgets(line, sizeof(line), file) != NULL) {
            char extra;

            if (sscanf(line, " %c", &extra) == 1) {
                CTH_WARN("\n    Palette has more than 256 lines (%s)", name);
                break;
            }
        }
    }

    return new_pal;
}

int PaletteEntry::lastRandom = -1;
int PaletteEntry::lastRandomPos = -1; // index of the last random palette
char PaletteEntry::randomName[PATH_MAX] = "random";

void PaletteEntry::random() {
    int N = 1 << (1 + ::Random(3));
    int h = 256 / N;

    char P[257][3]; // during generation one extra cell is needed

    for (int c = 0; c < 3; c++) { // R,G,B

        for (int i = 0; i <= N; i++) // give values at "Stuetzstellen"
            P[i * h][c] = ::Random(256); //  (what is the correct enlish word?)

        //
        // do interplation
        //  this is just linear interpolation, the results are good enough.
        //  nore complicated interplations, like polynomials or splines are not necessary
        //
        for (int i = 0; i < N; i++)
            for (int j = 0; j < h; j++) {
                double J = double(j) / double(h);
                P[i * h + j][c] = int((1.0 - J) * P[i * h][c] + J * P[(i + 1) * h][c]);
            }

        // copy to real palette
        for (int i = 0; i < 256; i++)
            pal[i][c] = P[i][c];
    }

    char str[512];
    delete[] name;
    sprintf(str, "%s.%d", randomName, lastRandom);
    name = new char[strlen(str) + 1];
    strcpy(name, str);

    //
    // save random palette to disk
    //
    FILE* f;
    char fname[PATH_MAX];
    sprintf(fname, "%s.map", name);

    CTH_DEBUG("  saving '%s'.\n", fname);

    if ((f = fopen(fname, "w")) == NULL) {
        printfee("Can not open '%s' for random palette.", fname);
        return;
    }
    fprintf(f, "%d %d %d  Random Palette from Cthugha\n", pal[0][0], pal[0][1], pal[0][2]);
    for (int i = 1; i < 256; i++)
        fprintf(f, "%d %d %d\n", pal[i][0], pal[i][1], pal[i][2]);
    fclose(f);
}

void PaletteEntry::addRandom() {
    lastRandom++;

    PaletteEntry* new_pal = new PaletteEntry("", "random");
    new_pal->random();
    CthughaBuffer::current->palette.add(new_pal);

    lastRandomPos = CthughaBuffer::current->palette.getNEntries() - 1;

    CthughaBuffer::current->lastPalette = -1; // force redraw
}

void PaletteEntry::Random() {
    if (lastRandomPos == -1)
        addRandom();
    else {
        ((PaletteEntry*)CthughaBuffer::current->palette[lastRandomPos])->random();
        //	((PaletteEntry*)CthughaBuffer::current->palette[
        //	    CthughaBuffer::current->palette.getNEntries()-1 ])->random();
    }

    CthughaBuffer::current->lastPalette = -1; // force redraw
}
