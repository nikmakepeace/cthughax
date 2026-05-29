#include "cthugha.h"
#include "display.h"
#include "Interface.h"
#include "disp-sys.h"
#include "imath.h"
#include "CthughaDisplay.h"

#include <ctype.h>

CoreOptionEntryList paletteEntries;
PaletteOption palette;

int change_palette_imm(int);

unsigned long bitmap_colors0[256]; /* "compiled" palette */
unsigned long bitmap_colors1[256]; /* "compiled" palette */
unsigned long bitmap_colors2[256]; /* "compiled" palette */
unsigned long bitmap_colors3[256]; /* "compiled" palette */

CoreOptionEntry* read_palette(FILE* file, const char* name, const char* dir, const char*);
static const char* palette_path[] = { "./", "./map/", CTH_LIBDIR "/map/", "" };
static int paletteSetFilterCount = 0;
static char paletteSetFilter[PALETTE_METADATA_MAX_VALUES][PALETTE_METADATA_VALUE_SIZE];
static char paletteSetFilterText[256] = "";

static int palette_line_was_truncated(const char* line) {
    size_t len = strlen(line);

    return (len > 0) && (line[len - 1] != '\n');
}

static void discard_rest_of_line(FILE* file) {
    int c;

    do {
        c = fgetc(file);
    } while ((c != EOF) && (c != '\n'));
}

static int clamp_palette_component(long value) {
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return (int)value;
}

static int parse_palette_data_line(const char* line, int* r, int* g, int* b) {
    char* end;
    long components[3];
    const char* pos = line;

    for (int i = 0; i < 3; i++) {
        while ((*pos != '\0') && isspace((unsigned char)*pos))
            pos++;

        errno = 0;
        components[i] = strtol(pos, &end, 10);
        if (end == pos)
            return 0;

        pos = end;
    }

    *r = clamp_palette_component(components[0]);
    *g = clamp_palette_component(components[1]);
    *b = clamp_palette_component(components[2]);
    return 1;
}

static char* trim_metadata_value(char* value) {
    char* end;

    while ((*value != '\0') && isspace((unsigned char)*value))
        value++;

    end = value + strlen(value);
    while ((end > value) && isspace((unsigned char)*(end - 1)))
        end--;
    *end = '\0';

    return value;
}

static int append_metadata_value(
    char values[][PALETTE_METADATA_VALUE_SIZE], int* count, const char* value) {
    if (*count >= (int)PALETTE_METADATA_MAX_VALUES)
        return 0;
    if (strlen(value) >= (size_t)PALETTE_METADATA_VALUE_SIZE)
        return 0;

    strcpy(values[*count], value);
    (*count)++;
    return 1;
}

static int append_metadata_display_value(char* display, size_t display_size, const char* value,
    int quote_value) {
    size_t used = strlen(display);
    size_t value_len = strlen(value);
    size_t needed = value_len + (quote_value ? 2 : 0) + (used ? 1 : 0);

    if (used + needed >= display_size)
        return 0;

    if (used)
        strcat(display, " ");
    if (quote_value)
        strcat(display, "\"");
    strcat(display, value);
    if (quote_value)
        strcat(display, "\"");

    return 1;
}

static int parse_metadata_values(const char* text,
    char values[][PALETTE_METADATA_VALUE_SIZE], int* count, char* display, size_t display_size) {
    const char* pos = text;

    *count = 0;
    display[0] = '\0';

    while (1) {
        char value[PALETTE_METADATA_VALUE_SIZE];
        size_t value_len = 0;
        int quoted = 0;

        while ((*pos != '\0') && isspace((unsigned char)*pos))
            pos++;
        if (*pos == '\0')
            break;

        if ((*pos == '"') || (*pos == '\'')) {
            char quote = *pos;

            quoted = 1;
            pos++;
            while ((*pos != '\0') && (*pos != quote)) {
                if (!isalnum((unsigned char)*pos) && !isspace((unsigned char)*pos))
                    return 0;
                if (value_len + 1 >= sizeof(value))
                    return 0;
                value[value_len++] = *pos;
                pos++;
            }
            if (*pos != quote)
                return 0;
            pos++;
            if ((*pos != '\0') && !isspace((unsigned char)*pos))
                return 0;
        } else {
            while ((*pos != '\0') && !isspace((unsigned char)*pos)) {
                if (!isalnum((unsigned char)*pos))
                    return 0;
                if (value_len + 1 >= sizeof(value))
                    return 0;
                value[value_len++] = *pos;
                pos++;
            }
        }

        while ((value_len > 0) && isspace((unsigned char)value[value_len - 1]))
            value_len--;
        value[value_len] = '\0';
        if (value_len == 0)
            return 0;

        if (!append_metadata_value(values, count, value))
            return 0;
        if (!append_metadata_display_value(display, display_size, value, quoted))
            return 0;
    }

    return *count > 0;
}

static int metadata_energy_allowed(const char* value) {
    static const char* allowed[] = { "low", "medium", "high", "extreme" };

    for (size_t i = 0; i < sizeof(allowed) / sizeof(allowed[0]); i++) {
        if (strcasecmp(value, allowed[i]) == 0)
            return 1;
    }

    return 0;
}

int palette_set_metadata_energy(PaletteEntry* palette, const char* value) {
    char parsed[PALETTE_METADATA_MAX_VALUES][PALETTE_METADATA_VALUE_SIZE];
    char display[sizeof(palette->metadataEnergy)];
    int count;

    if (!parse_metadata_values(value, parsed, &count, display, sizeof(display)))
        return 0;
    if (count > (int)(sizeof(palette->metadataEnergies) / sizeof(palette->metadataEnergies[0])))
        return 0;

    for (int i = 0; i < count; i++) {
        if (!metadata_energy_allowed(parsed[i]))
            return 0;
        if (strlen(parsed[i]) >= sizeof(palette->metadataEnergies[0]))
            return 0;
    }

    palette->metadataEnergyCount = count;
    for (int i = 0; i < count; i++)
        strcpy(palette->metadataEnergies[i], parsed[i]);
    strcpy(palette->metadataEnergy, display);

    return 1;
}

int palette_set_metadata_set(PaletteEntry* palette, const char* value) {
    char parsed[PALETTE_METADATA_MAX_VALUES][PALETTE_METADATA_VALUE_SIZE];
    char display[sizeof(palette->metadataSet)];
    int count;

    if (!parse_metadata_values(value, parsed, &count, display, sizeof(display)))
        return 0;

    palette->metadataSetCount = count;
    for (int i = 0; i < count; i++)
        strcpy(palette->metadataSets[i], parsed[i]);
    strcpy(palette->metadataSet, display);

    return 1;
}

int palette_set_filter(const char* value) {
    char parsed[PALETTE_METADATA_MAX_VALUES][PALETTE_METADATA_VALUE_SIZE];
    char display[sizeof(paletteSetFilterText)];
    int count;

    if ((value == NULL) || (value[0] == '\0')) {
        paletteSetFilterCount = 0;
        paletteSetFilterText[0] = '\0';
        return 1;
    }

    if (!parse_metadata_values(value, parsed, &count, display, sizeof(display))) {
        CTH_WARN("Ignoring malformed palette set filter `%s'.\n", value);
        return 0;
    }

    paletteSetFilterCount = count;
    for (int i = 0; i < count; i++)
        strcpy(paletteSetFilter[i], parsed[i]);
    strcpy(paletteSetFilterText, display);

    return 1;
}

static int palette_matches_set_filter(PaletteEntry* palette) {
    if (paletteSetFilterCount == 0)
        return 1;

    for (int i = 0; i < paletteSetFilterCount; i++) {
        for (int j = 0; j < palette->metadataSetCount; j++) {
            if (strcasecmp(paletteSetFilter[i], palette->metadataSets[j]) == 0)
                return 1;
        }
    }

    return 0;
}

PaletteOption::PaletteOption()
    : CoreOption(-1, "palette", paletteEntries) { }

PaletteEntry* PaletteOption::currentPaletteEntry() {
    return dynamic_cast<PaletteEntry*>(current());
}

const ColorPalette* PaletteOption::currentPalette() {
    PaletteEntry* entry = currentPaletteEntry();
    return (entry != 0) ? &entry->colors() : 0;
}

void apply_palette_set_filter() {
    int usable = 0;

    if (paletteSetFilterCount == 0)
        return;

    for (int i = 0; i < ::palette.getNEntries(); i++) {
        PaletteEntry* entry = (PaletteEntry*)::palette[i];
        int matches = (entry != NULL) && palette_matches_set_filter(entry);

        if (entry != NULL)
            entry->setUse(matches);
        if (matches)
            usable++;
    }

    if (usable == 0) {
        CTH_WARN("Palette set filter `%s' matched no palettes; leaving all palettes enabled.\n",
            paletteSetFilterText);
        for (int i = 0; i < ::palette.getNEntries(); i++)
            ::palette[i]->setUse(1);
    } else {
        CTH_INFO("  palette set filter `%s': %d palettes enabled\n", paletteSetFilterText, usable);
    }
}

static int parse_palette_metadata_line(char* line, char* key, size_t key_size, char* value,
    size_t value_size) {
    char* pos = line;
    char* colon;
    char* value_start;
    size_t key_len;
    size_t value_len;

    while ((*pos != '\0') && isspace((unsigned char)*pos))
        pos++;
    if (!isalpha((unsigned char)*pos))
        return 0;

    colon = strchr(pos, ':');
    if (colon == NULL)
        return 0;

    char* key_end = colon;
    while ((key_end > pos) && isspace((unsigned char)*(key_end - 1)))
        key_end--;
    for (char* p = pos; p < key_end; p++) {
        if (!isalpha((unsigned char)*p))
            return 0;
    }

    key_len = key_end - pos;
    if ((key_len == 0) || (key_len >= key_size))
        return 0;

    value_start = trim_metadata_value(colon + 1);
    for (char* p = value_start; *p != '\0'; p++) {
        if (!isalnum((unsigned char)*p) && !isspace((unsigned char)*p) && (*p != '"')
            && (*p != '\''))
            return 0;
    }

    value_len = strlen(value_start);
    if ((value_len == 0) || (value_len >= value_size))
        return 0;

    memcpy(key, pos, key_len);
    key[key_len] = '\0';
    strcpy(value, value_start);

    return 1;
}

static void apply_palette_metadata(PaletteEntry* palette, const char* key, const char* value) {
    if (strcasecmp(key, "name") == 0) {
        palette->setMetadataName(value);
    } else if (strcasecmp(key, "set") == 0) {
        if (!palette_set_metadata_set(palette, value))
            CTH_WARN("\n    Ignoring malformed palette set metadata for `%s'", palette->Name());
    } else if (strcasecmp(key, "energy") == 0) {
        if (!palette_set_metadata_energy(palette, value))
            CTH_WARN("\n    Ignoring malformed palette energy metadata for `%s'", palette->Name());
    }
}

int colormapped = 1; /* 0 .. True/Direct color
                        1 .. All 256 color cells
                        2 .. Only some cells */

/* when using only some colorcells (e.g. when drawing on the root window)
   update to the palette are done immediately without smoothing.
   */

int load_palettes() {
    int i, l;
    PaletteEntry* new_pal;
    ColorPalette* colors;

    CTH_INFO("  loading palettes...\n");
    palette.load(palette_path, "/map/", ".map", read_palette);

    /* create one general palette */
    new_pal = new PaletteEntry("general", "");
    colors = &new_pal->colors();
    for (i = 0; i < 64; i++) {
        colors->setColor(i, i << 2, 0, 0);
        colors->setColor(i + 64, 0, i << 2, 0);
        colors->setColor(i + 128, 0, 0, i << 2);
        colors->setColor(i + 192, i << 2, i << 2, i << 2);
    }
    palette.add(new_pal);

    CTH_INFO("  number of loaded palettes: %d\n", palette.getNEntries());
    apply_palette_set_filter();

    /* brighten up palettes, that are a bit dark */
    for (i = 0; i < palette.getNEntries(); i++) {
        PaletteEntry* entry = (PaletteEntry*)palette[i];
        ColorPalette& colors = entry->colors();
        int m = 0;
        double P = 0;

        for (l = 0; l < 256; l++) {
            m = max(m, colors.component(l, 0) + colors.component(l, 1)
                    + colors.component(l, 2));
        }
        if ((m > 0) && (m < 3 * 255)) {
            P = double(3 * 255) / double(m);
            CTH_DEBUG("brightening palette %d (%s). Faktor: %0.3f\n", i,
                palette[i]->Name(), P);

            for (l = 0; l < 256; l++) {
                colors.setComponent(l, 0, int(double(colors.component(l, 0)) * P));
                colors.setComponent(l, 1, int(double(colors.component(l, 1)) * P));
                colors.setComponent(l, 2, int(double(colors.component(l, 2)) * P));
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
    FILE* file, const char* name, const char* /*dir*/, const char* total_name) {
    char line[256];
    int i = 0;
    int r, g, b;
    int line_number = 0;
    bool read_all_entries = true;
    bool in_metadata = true;
    PaletteEntry* new_pal = new PaletteEntry(name, "");
    ColorPalette& colors = new_pal->colors();

    strncpy(new_pal->sourcePath, total_name, PATH_MAX);
    new_pal->sourcePath[PATH_MAX - 1] = '\0';

    while (i < 256) {
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
                colors.setColor(i, 0, 0, 0);
            break;
        }
        line_number++;

        int line_truncated = palette_line_was_truncated(line);

        if (parse_palette_data_line(line, &r, &g, &b)) {
            if (line_truncated)
                discard_rest_of_line(file);
            in_metadata = false;
        } else if (line_truncated) {
            discard_rest_of_line(file);
            CTH_WARN("\n    Overlong palette line: %d (%s) ... ignoring", line_number, name);
            if (in_metadata)
                continue;

            r = g = b = 0;
        } else if (in_metadata) {
            char key[32];
            char value[256];

            if (parse_palette_metadata_line(line, key, sizeof(key), value, sizeof(value))) {
                apply_palette_metadata(new_pal, key, value);
            } else {
                CTH_WARN("\n    Ignoring malformed palette metadata line: %d (%s)", line_number,
                    name);
            }
            continue;
        } else {
            CTH_WARN("\n    Malformed palette line: %d (%s) ... replacing with black", i + 1, name);
            r = g = b = 0;
        }

        colors.setColor(i, r, g, b);
        i++;
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
            colors().setComponent(i, c, P[i][c]);
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
        CTH_ERRNO(errno, "Can not open '%s' for random palette.", fname);
        return;
    }
    fprintf(f, "%d %d %d  Random Palette from Cthugha\n",
        colors().component(0, 0), colors().component(0, 1), colors().component(0, 2));
    for (int i = 1; i < 256; i++)
        fprintf(f, "%d %d %d\n",
            colors().component(i, 0), colors().component(i, 1), colors().component(i, 2));
    fclose(f);
}

void PaletteEntry::addRandom() {
    lastRandom++;

    PaletteEntry* new_pal = new PaletteEntry("", "random");
    new_pal->random();
    palette.add(new_pal);

    lastRandomPos = palette.getNEntries() - 1;

}

void PaletteEntry::Random() {
    if (lastRandomPos == -1)
        addRandom();
    else {
        ((PaletteEntry*)palette[lastRandomPos])->random();
    }

}
