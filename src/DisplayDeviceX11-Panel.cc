#include "cthugha.h"
#include "DisplayDevice.h"
#include "cthugha.h"
#include "display.h"
#include "disp-sys.h"
#include "imath.h"
#include "xcthugha.h"
#include "keys.h"
#include "CthughaBuffer.h"
#include "Interface.h"
#include "cth_buffer.h"
#include "CthughaDisplay.h"

#include <unistd.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Simple.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xatom.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <zlib.h>

#if WITH_SAVER == 1
#include <X11/extensions/scrnsaver.h>
#endif
#include <X11/extensions/XShm.h>

#include "xcthugha.h"

//
// Create the panel with buttons and menus
//
void DisplayDeviceX11::key_button(Widget /*w*/, XtPointer data, XtPointer /*data2*/) {
    if (data)
        keys_x11((char*)data);
}
/*
 * handler for activated menu-items
 */
void DisplayDeviceX11::menuCB(Widget /*item*/, XtPointer data, XtPointer /*data2*/) {
    menu_data_t* d = (menu_data_t*)data;

    if (d->opt != 0) {
        d->opt->setValue(d->pos);
        d->opt->change(0, 0);
    }
}

static unsigned int read_be32(const unsigned char* data) {
    return ((unsigned int)data[0] << 24) | ((unsigned int)data[1] << 16)
        | ((unsigned int)data[2] << 8) | (unsigned int)data[3];
}

static int png_paeth(int a, int b, int c) {
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);

    if ((pa <= pb) && (pa <= pc))
        return a;
    if (pb <= pc)
        return b;
    return c;
}

static int load_png_rgb(const char* path, int& width, int& height, std::vector<unsigned char>& rgb) {
    FILE* file = fopen(path, "rb");
    if (file == NULL)
        return 0;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (file_size <= 0) {
        fclose(file);
        return 0;
    }

    std::vector<unsigned char> data(file_size);
    if (fread(&data[0], 1, file_size, file) != (size_t)file_size) {
        fclose(file);
        return 0;
    }
    fclose(file);

    static const unsigned char signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    if ((data.size() < 8) || (memcmp(&data[0], signature, 8) != 0))
        return 0;

    int bit_depth = 0;
    int color_type = 0;
    int interlace = 0;
    std::vector<unsigned char> plte;
    std::vector<unsigned char> compressed;

    size_t pos = 8;
    while (pos + 12 <= data.size()) {
        unsigned int length = read_be32(&data[pos]);
        if (pos + 12 + length > data.size())
            return 0;

        const unsigned char* chunk_type = &data[pos + 4];
        const unsigned char* chunk_data = &data[pos + 8];

        if (memcmp(chunk_type, "IHDR", 4) == 0) {
            if (length != 13)
                return 0;
            width = read_be32(chunk_data);
            height = read_be32(chunk_data + 4);
            bit_depth = chunk_data[8];
            color_type = chunk_data[9];
            interlace = chunk_data[12];
        } else if (memcmp(chunk_type, "PLTE", 4) == 0) {
            plte.assign(chunk_data, chunk_data + length);
        } else if (memcmp(chunk_type, "IDAT", 4) == 0) {
            compressed.insert(compressed.end(), chunk_data, chunk_data + length);
        } else if (memcmp(chunk_type, "IEND", 4) == 0) {
            break;
        }

        pos += 12 + length;
    }

    if ((width <= 0) || (height <= 0) || (bit_depth != 8) || (interlace != 0))
        return 0;

    int bpp;
    switch (color_type) {
    case 2:
        bpp = 3;
        break;
    case 3:
        bpp = 1;
        if (plte.size() < 3)
            return 0;
        break;
    case 6:
        bpp = 4;
        break;
    default:
        return 0;
    }

    size_t row_bytes = width * bpp;
    size_t raw_size = (row_bytes + 1) * height;
    std::vector<unsigned char> raw(raw_size);
    uLongf dest_len = raw_size;
    if (compressed.empty())
        return 0;
    if (uncompress(&raw[0], &dest_len, &compressed[0], compressed.size()) != Z_OK)
        return 0;
    if (dest_len < raw_size)
        return 0;

    std::vector<unsigned char> pixels(row_bytes * height);
    std::vector<unsigned char> previous(row_bytes, 0);
    std::vector<unsigned char> current(row_bytes, 0);

    for (int y = 0; y < height; y++) {
        const unsigned char* src = &raw[y * (row_bytes + 1)];
        int filter = src[0];

        for (size_t x = 0; x < row_bytes; x++) {
            int left = (x >= (size_t)bpp) ? current[x - bpp] : 0;
            int up = previous[x];
            int up_left = (x >= (size_t)bpp) ? previous[x - bpp] : 0;
            int value = src[x + 1];

            switch (filter) {
            case 0:
                break;
            case 1:
                value += left;
                break;
            case 2:
                value += up;
                break;
            case 3:
                value += (left + up) / 2;
                break;
            case 4:
                value += png_paeth(left, up, up_left);
                break;
            default:
                return 0;
            }

            current[x] = (unsigned char)(value & 0xFF);
        }

        memcpy(&pixels[y * row_bytes], &current[0], row_bytes);
        previous.swap(current);
    }

    rgb.resize(width * height * 3);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char* dst = &rgb[(y * width + x) * 3];
            const unsigned char* src = &pixels[y * row_bytes + x * bpp];

            if (color_type == 3) {
                int index = src[0] * 3;
                if (index + 2 >= (int)plte.size())
                    return 0;
                dst[0] = plte[index];
                dst[1] = plte[index + 1];
                dst[2] = plte[index + 2];
            } else {
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
            }
        }
    }

    return 1;
}

static unsigned long scale_to_mask(unsigned char value, unsigned long mask) {
    int shift = 0;
    int bits = 0;
    unsigned long shifted = mask;

    if (mask == 0)
        return 0;

    while ((shifted & 1) == 0) {
        shifted >>= 1;
        shift++;
    }
    while (shifted & 1) {
        bits++;
        shifted >>= 1;
    }

    return ((((unsigned long)value * ((1UL << bits) - 1)) / 255UL) << shift) & mask;
}

unsigned long DisplayDeviceX11::palettePreviewPixel(unsigned char r, unsigned char g,
    unsigned char b) {
    if (visual->c_class == TrueColor || visual->c_class == DirectColor) {
        return scale_to_mask(r, visual->red_mask)
            | scale_to_mask(g, visual->green_mask)
            | scale_to_mask(b, visual->blue_mask);
    }

    XColor color;
    color.red = r << 8;
    color.green = g << 8;
    color.blue = b << 8;
    color.flags = DoRed | DoGreen | DoBlue;
    if (XAllocColor(xcth_display, colormap, &color))
        return color.pixel;

    return BlackPixel(xcth_display, DefaultScreen(xcth_display));
}

static int palette_png_path(const char* map_path, const char* palette_name, char* png_path) {
    char dir[PATH_MAX];
    char stem[PATH_MAX];
    const char* slash;

    if ((map_path == NULL) || (map_path[0] == '\0')) {
        snprintf(png_path, PATH_MAX, "map/png/%s.png", palette_name);
        return access(png_path, R_OK) == 0;
    }

    strncpy(dir, map_path, PATH_MAX);
    dir[PATH_MAX - 1] = '\0';
    slash = strrchr(dir, '/');
    if (slash == NULL) {
        strcpy(dir, ".");
        slash = map_path;
    } else {
        dir[slash - dir] = '\0';
        slash++;
    }

    strncpy(stem, slash, PATH_MAX);
    stem[PATH_MAX - 1] = '\0';
    size_t stem_len = strlen(stem);
    if ((stem_len >= 4) && (strcasecmp(stem + stem_len - 4, ".map") == 0))
        stem[stem_len - 4] = '\0';

    snprintf(png_path, PATH_MAX, "%s/png/%s.png", dir, stem);
    if (access(png_path, R_OK) == 0)
        return 1;

    snprintf(png_path, PATH_MAX, "map/png/%s.png", stem);
    if (access(png_path, R_OK) == 0)
        return 1;

    snprintf(png_path, PATH_MAX, "map/png/%s.png", palette_name);
    return access(png_path, R_OK) == 0;
}

static int panel_palette_data_line(const char* line) {
    char* end;
    const char* pos = line;

    for (int i = 0; i < 3; i++) {
        while ((*pos != '\0') && isspace((unsigned char)*pos))
            pos++;
        errno = 0;
        strtol(pos, &end, 10);
        if (end == pos)
            return 0;
        pos = end;
    }

    return 1;
}

static int panel_metadata_key_is(const char* line, const char* key) {
    const char* pos = line;
    const char* key_pos = key;

    while ((*pos != '\0') && isspace((unsigned char)*pos))
        pos++;

    while ((*pos != '\0') && (*key_pos != '\0') && isalpha((unsigned char)*pos)) {
        if (tolower((unsigned char)*pos) != tolower((unsigned char)*key_pos))
            return 0;
        pos++;
        key_pos++;
    }
    if (*key_pos != '\0')
        return 0;
    while ((*pos != '\0') && isspace((unsigned char)*pos))
        pos++;

    return *pos == ':';
}

static int panel_known_metadata_key(const char* line) {
    return panel_metadata_key_is(line, "name") || panel_metadata_key_is(line, "set")
        || panel_metadata_key_is(line, "energy");
}

static int write_metadata_line(FILE* file, const char* key, const char* value) {
    if ((value == NULL) || (value[0] == '\0'))
        return 1;

    return fprintf(file, "%s: %s\n", key, value) >= 0;
}

void DisplayDeviceX11::drawPalettePreview() {
    if ((palettePreviewWidget == NULL) || (palettePreviewPixmap == None))
        return;

    XCopyArea(xcth_display, palettePreviewPixmap, XtWindow(palettePreviewWidget), gc, 0, 0,
        palettePreviewWidth, palettePreviewHeight, 0, 0);
}

void DisplayDeviceX11::updatePalettePreview() {
    if ((palettePreviewWidget == NULL) || (CthughaBuffer::current == NULL))
        return;

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(xcth_display, XtWindow(palettePreviewWidget), &attrs))
        return;

    int current_palette = CthughaBuffer::current->palette.currentN();
    int palette_changed = current_palette != palettePreviewPalette;
    int size_changed = (attrs.width != palettePreviewWidth) || (attrs.height != palettePreviewHeight);
    if (!palette_changed && !size_changed)
        return;

    palettePreviewPalette = current_palette;
    palettePreviewWidth = attrs.width;
    palettePreviewHeight = attrs.height;
    if (palette_changed)
        palettePreviewChangedAt = getTime();
    palettePreviewMapPath[0] = '\0';
    if (palette_changed)
        updatePaletteMetadataEditor();

    if (palettePreviewPixmap != None) {
        XFreePixmap(xcth_display, palettePreviewPixmap);
        palettePreviewPixmap = None;
    }

    PaletteEntry* palette = (PaletteEntry*)CthughaBuffer::current->palette.current();
    if (palette == NULL)
        return;

    strncpy(palettePreviewMapPath, palette->sourcePath, PATH_MAX);
    palettePreviewMapPath[PATH_MAX - 1] = '\0';

    char png_path[PATH_MAX];
    int png_width = 0;
    int png_height = 0;
    std::vector<unsigned char> rgb;

    if (!palette_png_path(palette->sourcePath, palette->Name(), png_path)) {
        CTH_WARN("Could not find PNG preview for palette `%s' loaded from `%s'.\n",
            palette->Name(), palette->sourcePath);
        return;
    }
    if (!load_png_rgb(png_path, png_width, png_height, rgb)) {
        CTH_WARN("Could not load PNG preview `%s' for palette `%s'.\n", png_path,
            palette->Name());
        return;
    }

    int preview_width = palettePreviewWidth;
    int preview_height = palettePreviewHeight;
    if ((preview_width <= 0) || (preview_height <= 0))
        return;

    XImage* preview = XCreateImage(xcth_display, visual, planes, ZPixmap, 0, NULL, preview_width,
        preview_height, XBitmapPad(xcth_display), 0);
    if (preview == NULL)
        return;

    preview->data = (char*)malloc(preview->bytes_per_line * preview_height);
    if (preview->data == NULL) {
        XDestroyImage(preview);
        return;
    }

    for (int y = 0; y < preview_height; y++) {
        int src_y = y * png_height / preview_height;
        for (int x = 0; x < preview_width; x++) {
            int src_x = x * png_width / preview_width;
            unsigned char* src = &rgb[(src_y * png_width + src_x) * 3];
            XPutPixel(preview, x, y, palettePreviewPixel(src[0], src[1], src[2]));
        }
    }

    palettePreviewPixmap = XCreatePixmap(
        xcth_display, XtWindow(palettePreviewWidget), preview_width, preview_height, planes);
    if (palettePreviewPixmap != None)
        XPutImage(xcth_display, palettePreviewPixmap, gc, preview, 0, 0, 0, 0, preview_width,
            preview_height);

    XDestroyImage(preview);
    drawPalettePreview();
}

void DisplayDeviceX11::setPaletteMetadataStatus(const char* status) {
    if (paletteMetadataStatusWidget == NULL)
        return;

    Arg wargs[1];
    XtSetArg(wargs[0], XtNlabel, status ? status : "");
    XtSetValues(paletteMetadataStatusWidget, wargs, 1);
}

void DisplayDeviceX11::updatePaletteMetadataEditor() {
    if ((paletteNameTextWidget == NULL) || (paletteSetTextWidget == NULL)
        || (paletteEnergyTextWidget == NULL) || (CthughaBuffer::current == NULL))
        return;

    PaletteEntry* palette = (PaletteEntry*)CthughaBuffer::current->palette.current();
    if (palette == NULL)
        return;

    XtVaSetValues(paletteNameTextWidget, XtNstring, palette->metadataName, NULL);
    XtVaSetValues(paletteSetTextWidget, XtNstring, palette->metadataSet, NULL);
    XtVaSetValues(paletteEnergyTextWidget, XtNstring, palette->metadataEnergy, NULL);
    setPaletteMetadataStatus(palette->sourcePath[0] ? palette->Name() : "read-only");
}

int DisplayDeviceX11::savePaletteMetadata() {
    if ((paletteNameTextWidget == NULL) || (paletteSetTextWidget == NULL)
        || (paletteEnergyTextWidget == NULL) || (CthughaBuffer::current == NULL))
        return 0;

    PaletteEntry* palette = (PaletteEntry*)CthughaBuffer::current->palette.current();
    if ((palette == NULL) || (palette->sourcePath[0] == '\0')) {
        setPaletteMetadataStatus("read-only");
        return 0;
    }

    char* name = NULL;
    char* set = NULL;
    char* energy = NULL;
    XtVaGetValues(paletteNameTextWidget, XtNstring, &name, NULL);
    XtVaGetValues(paletteSetTextWidget, XtNstring, &set, NULL);
    XtVaGetValues(paletteEnergyTextWidget, XtNstring, &energy, NULL);
    if ((name == NULL) || (set == NULL) || (energy == NULL)) {
        setPaletteMetadataStatus("could not read fields");
        return 0;
    }

    PaletteEntry scratch(palette->Name(), "");
    if (name[0])
        scratch.setMetadataName(name);
    if (set[0] && !palette_set_metadata_set(&scratch, set)) {
        setPaletteMetadataStatus("bad set");
        return 0;
    }
    if (energy[0] && !palette_set_metadata_energy(&scratch, energy)) {
        setPaletteMetadataStatus("bad energy");
        return 0;
    }

    FILE* in = fopen(palette->sourcePath, "rb");
    if (in == NULL) {
        setPaletteMetadataStatus("open failed");
        return 0;
    }

    std::vector<std::string> prefix;
    std::vector<std::string> body;
    char line[1024];
    int found_data = 0;
    while (fgets(line, sizeof(line), in) != NULL) {
        std::string current(line);
        if (!found_data && panel_palette_data_line(line))
            found_data = 1;
        if (found_data)
            body.push_back(current);
        else if (!panel_known_metadata_key(line))
            prefix.push_back(current);
    }
    int read_error = ferror(in);
    fclose(in);
    if (read_error || body.empty()) {
        setPaletteMetadataStatus("read failed");
        return 0;
    }

    char tmp_path[PATH_MAX];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", palette->sourcePath);
    FILE* out = fopen(tmp_path, "wb");
    if (out == NULL) {
        setPaletteMetadataStatus("write failed");
        return 0;
    }

    int ok = write_metadata_line(out, "name", scratch.metadataName)
        && write_metadata_line(out, "set", scratch.metadataSet)
        && write_metadata_line(out, "energy", scratch.metadataEnergy);
    for (size_t i = 0; ok && (i < prefix.size()); i++)
        ok = fputs(prefix[i].c_str(), out) >= 0;
    for (size_t i = 0; ok && (i < body.size()); i++)
        ok = fputs(body[i].c_str(), out) >= 0;
    if (fclose(out) != 0)
        ok = 0;

    if (!ok) {
        unlink(tmp_path);
        setPaletteMetadataStatus("write failed");
        return 0;
    }
    if (rename(tmp_path, palette->sourcePath) != 0) {
        unlink(tmp_path);
        setPaletteMetadataStatus("save failed");
        return 0;
    }

    palette->setMetadataName(scratch.metadataName);
    if (scratch.metadataSet[0]) {
        palette_set_metadata_set(palette, scratch.metadataSet);
    } else {
        palette->metadataSet[0] = '\0';
        palette->metadataSetCount = 0;
    }
    if (scratch.metadataEnergy[0]) {
        palette_set_metadata_energy(palette, scratch.metadataEnergy);
    } else {
        palette->metadataEnergy[0] = '\0';
        palette->metadataEnergyCount = 0;
    }
    XtVaSetValues(paletteNameTextWidget, XtNstring, palette->metadataName, NULL);
    XtVaSetValues(paletteSetTextWidget, XtNstring, palette->metadataSet, NULL);
    XtVaSetValues(paletteEnergyTextWidget, XtNstring, palette->metadataEnergy, NULL);
    setPaletteMetadataStatus("saved");
    return 1;
}

void DisplayDeviceX11::nextUntaggedPalette() {
    if (CthughaBuffer::current == NULL)
        return;

    int n = CthughaBuffer::current->palette.getNEntries();
    int start = CthughaBuffer::current->palette.currentN();
    for (int step = 1; step <= n; step++) {
        int candidate = (start + step) % n;
        PaletteEntry* palette = (PaletteEntry*)CthughaBuffer::current->palette[candidate];
        if ((palette != NULL)
            && ((palette->metadataName[0] == '\0') || (palette->metadataSetCount == 0)
                || (palette->metadataEnergyCount == 0))) {
            CthughaBuffer::current->palette.setValue(candidate);
            CthughaBuffer::current->palette.change(0, 0);
            palettePreviewPalette = -1;
            updatePalettePreview();
            setPaletteMetadataStatus("next untagged");
            return;
        }
    }

    setPaletteMetadataStatus("all tagged");
}

void DisplayDeviceX11::deletePaletteCB(Widget /*item*/, XtPointer data, XtPointer /*data2*/) {
    DisplayDeviceX11* device = (DisplayDeviceX11*)data;
    if (device == NULL)
        return;

    if (CthughaBuffer::current->palette.currentN() != device->palettePreviewPalette) {
        device->palettePreviewChangedAt = getTime();
        CTH_WARN("Palette changed too recently; not deleting.\n");
        return;
    }

    if ((getTime() - device->palettePreviewChangedAt) < 1.0) {
        CTH_WARN("Palette changed too recently; not deleting.\n");
        return;
    }

    PaletteEntry* palette = (PaletteEntry*)CthughaBuffer::current->palette.current();
    if ((palette == NULL) || (palette->sourcePath[0] == '\0'))
        return;

    unlink(palette->sourcePath);
}

void DisplayDeviceX11::savePaletteMetadataCB(
    Widget /*item*/, XtPointer data, XtPointer /*data2*/) {
    DisplayDeviceX11* device = (DisplayDeviceX11*)data;
    if (device != NULL)
        device->savePaletteMetadata();
}

void DisplayDeviceX11::revertPaletteMetadataCB(
    Widget /*item*/, XtPointer data, XtPointer /*data2*/) {
    DisplayDeviceX11* device = (DisplayDeviceX11*)data;
    if (device != NULL) {
        device->updatePaletteMetadataEditor();
        device->setPaletteMetadataStatus("reverted");
    }
}

void DisplayDeviceX11::nextUntaggedPaletteCB(
    Widget /*item*/, XtPointer data, XtPointer /*data2*/) {
    DisplayDeviceX11* device = (DisplayDeviceX11*)data;
    if (device != NULL)
        device->nextUntaggedPalette();
}

void DisplayDeviceX11::palettePreviewExpose(
    Widget /*item*/, XtPointer data, XEvent* event, Boolean* /*cont*/) {
    DisplayDeviceX11* device = (DisplayDeviceX11*)data;
    if (device == NULL)
        return;

    if (event->type == ConfigureNotify) {
        device->updatePalettePreview();
    } else if (event->type == Expose) {
        device->drawPalettePreview();
    }
}

Widget DisplayDeviceX11::add_menu(
    char* name, CoreOption* what, Widget parent, Widget under, Widget right) {
    Arg wargs[3];
    int n;
    Widget button, menu, item;
    int i;

    if (parent == NULL)
        return NULL;

    /* create menu button */
    n = 0;
    XtSetArg(wargs[n], XtNlabel, name);
    n++;
    if (under) {
        XtSetArg(wargs[n], XtNfromVert, under);
        n++;
    }
    if (right) {
        XtSetArg(wargs[n], XtNfromHoriz, right);
        n++;
    }
    button = XtCreateManagedWidget(name, menuButtonWidgetClass, parent, wargs, n);
    if (button == NULL)
        return NULL;

    /* create the menu popup */
    menu = XtCreatePopupShell("menu", simpleMenuWidgetClass, button, NULL, 0);
    if (menu == NULL) {
        XtDestroyWidget(button);
        return NULL;
    }

    if (what->getNEntries() <= 0) {
        XtSetArg(wargs[0], XtNlabel, "none");
        XtCreateManagedWidget("none", smeBSBObjectClass, menu, wargs, 1);
    }

    /* create menu items */
    for (i = 0; i < what->getNEntries(); i++) {
        menu_data_t* md = new menu_data_t;
        const char* label = (*what)[i]->Desc()[0] ? (*what)[i]->Desc() : (*what)[i]->Name();

        md->opt = what;
        md->pos = i;

        XtSetArg(wargs[0], XtNlabel, label);
        item = XtCreateManagedWidget((*what)[i]->Name(), smeBSBObjectClass, menu, wargs, 1);
        XtAddCallback(item, XtNcallback, menuCB, md);
    }
    return button;
}
void DisplayDeviceX11::xcth_create_panel() {
    Widget panel;
    Widget quit_button, change_button, delete_palette_button;
    Widget name_label, set_label, energy_label;
    Widget save_metadata_button, revert_metadata_button, next_untagged_button;
    Widget menu[8];
    Arg wargs[8];

    /* create the panel formWidget */
    panel = XtCreateManagedWidget("panel", formWidgetClass, xcth_toplevel, NULL, 0);

    /* create the quit button */
    XtSetArg(wargs[0], XtNlabel, "Quit!");
    quit_button = XtCreateManagedWidget("quit", commandWidgetClass, panel, wargs, 1);
    XtAddCallback(quit_button, XtNcallback, (XtCallbackProc)quit, NULL);

    /* create the change button */
    XtSetArg(wargs[0], XtNlabel, "Change!");
    XtSetArg(wargs[1], XtNfromHoriz, quit_button);
    change_button = XtCreateManagedWidget("change", commandWidgetClass, panel, wargs, 2);
    XtAddCallback(change_button, XtNcallback, key_button, (char*)" ");

    XtSetArg(wargs[0], XtNlabel, "Delete Palette");
    XtSetArg(wargs[1], XtNfromHoriz, change_button);
    delete_palette_button
        = XtCreateManagedWidget("deletePalette", commandWidgetClass, panel, wargs, 2);
    XtAddCallback(delete_palette_button, XtNcallback, deletePaletteCB, this);

    /* create the menus */
    menu[0] = add_menu("Display", &::screen, panel, quit_button, NULL);
    menu[1] = add_menu("Wave", &(CthughaBuffer::current->wave), panel, quit_button, menu[0]);
    menu[2] = add_menu("Flame", &CthughaBuffer::current->flame, panel, quit_button, menu[1]);
    menu[3]
        = add_menu("Translation", &CthughaBuffer::current->translate, panel, quit_button, menu[2]);
    menu[4] = add_menu("Palette", &CthughaBuffer::current->palette, panel, quit_button, menu[3]);
    menu[5] = add_menu("Table", &CthughaBuffer::current->table, panel, quit_button, menu[4]);
    menu[6] = add_menu("PCX", &CthughaBuffer::current->pcx, panel, quit_button, menu[5]);
    menu[7] = add_menu("Objects", &CthughaBuffer::current->object, panel, quit_button, menu[6]);

    // create the panelText Widget
    text_size.x = 80;
    text_size.y = 40 / fontSize.y;
    if (text_size.y < 1)
        text_size.y = 1;
    XtSetArg(wargs[0], XtNfromVert, menu[0]);
    XtSetArg(wargs[1], XtNwidth, fontSize.x * text_size.x);
    XtSetArg(wargs[2], XtNheight, 40);
    panelTextWidget = XtCreateManagedWidget("panelText", labelWidgetClass, panel, wargs, 3);

    XtSetArg(wargs[0], XtNfromVert, panelTextWidget);
    XtSetArg(wargs[1], XtNwidth, fontSize.x * text_size.x);
    XtSetArg(wargs[2], XtNheight, 256);
    XtSetArg(wargs[3], XtNright, XawChainRight);
    palettePreviewWidget = XtCreateManagedWidget("palettePreview", simpleWidgetClass, panel, wargs, 4);
    XtAddEventHandler(
        palettePreviewWidget, ExposureMask | StructureNotifyMask, False, palettePreviewExpose, this);

    XtSetArg(wargs[0], XtNlabel, "Name");
    XtSetArg(wargs[1], XtNfromVert, palettePreviewWidget);
    name_label = XtCreateManagedWidget("paletteNameLabel", labelWidgetClass, panel, wargs, 2);

    XtSetArg(wargs[0], XtNfromVert, palettePreviewWidget);
    XtSetArg(wargs[1], XtNfromHoriz, name_label);
    XtSetArg(wargs[2], XtNwidth, fontSize.x * 64);
    XtSetArg(wargs[3], XtNeditType, XawtextEdit);
    XtSetArg(wargs[4], XtNstring, "");
    paletteNameTextWidget
        = XtCreateManagedWidget("paletteNameText", asciiTextWidgetClass, panel, wargs, 5);

    XtSetArg(wargs[0], XtNlabel, "Set");
    XtSetArg(wargs[1], XtNfromVert, name_label);
    set_label = XtCreateManagedWidget("paletteSetLabel", labelWidgetClass, panel, wargs, 2);

    XtSetArg(wargs[0], XtNfromVert, paletteNameTextWidget);
    XtSetArg(wargs[1], XtNfromHoriz, set_label);
    XtSetArg(wargs[2], XtNwidth, fontSize.x * 64);
    XtSetArg(wargs[3], XtNeditType, XawtextEdit);
    XtSetArg(wargs[4], XtNstring, "");
    paletteSetTextWidget
        = XtCreateManagedWidget("paletteSetText", asciiTextWidgetClass, panel, wargs, 5);

    XtSetArg(wargs[0], XtNlabel, "Energy");
    XtSetArg(wargs[1], XtNfromVert, set_label);
    energy_label = XtCreateManagedWidget("paletteEnergyLabel", labelWidgetClass, panel, wargs, 2);

    XtSetArg(wargs[0], XtNfromVert, paletteSetTextWidget);
    XtSetArg(wargs[1], XtNfromHoriz, energy_label);
    XtSetArg(wargs[2], XtNwidth, fontSize.x * 64);
    XtSetArg(wargs[3], XtNeditType, XawtextEdit);
    XtSetArg(wargs[4], XtNstring, "");
    paletteEnergyTextWidget
        = XtCreateManagedWidget("paletteEnergyText", asciiTextWidgetClass, panel, wargs, 5);

    XtSetArg(wargs[0], XtNlabel, "Save Metadata");
    XtSetArg(wargs[1], XtNfromVert, energy_label);
    save_metadata_button
        = XtCreateManagedWidget("savePaletteMetadata", commandWidgetClass, panel, wargs, 2);
    XtAddCallback(save_metadata_button, XtNcallback, savePaletteMetadataCB, this);

    XtSetArg(wargs[0], XtNlabel, "Revert");
    XtSetArg(wargs[1], XtNfromVert, energy_label);
    XtSetArg(wargs[2], XtNfromHoriz, save_metadata_button);
    revert_metadata_button
        = XtCreateManagedWidget("revertPaletteMetadata", commandWidgetClass, panel, wargs, 3);
    XtAddCallback(revert_metadata_button, XtNcallback, revertPaletteMetadataCB, this);

    XtSetArg(wargs[0], XtNlabel, "Next Untagged");
    XtSetArg(wargs[1], XtNfromVert, energy_label);
    XtSetArg(wargs[2], XtNfromHoriz, revert_metadata_button);
    next_untagged_button
        = XtCreateManagedWidget("nextUntaggedPalette", commandWidgetClass, panel, wargs, 3);
    XtAddCallback(next_untagged_button, XtNcallback, nextUntaggedPaletteCB, this);

    XtSetArg(wargs[0], XtNlabel, "");
    XtSetArg(wargs[1], XtNfromVert, energy_label);
    XtSetArg(wargs[2], XtNfromHoriz, next_untagged_button);
    XtSetArg(wargs[3], XtNwidth, fontSize.x * 18);
    XtSetArg(wargs[4], XtNborderWidth, 0);
    XtSetArg(wargs[5], XtNjustify, XtJustifyLeft);
    paletteMetadataStatusWidget
        = XtCreateManagedWidget("paletteMetadataStatus", labelWidgetClass, panel, wargs, 6);

    /* realize window */
    XtRealizeWidget(xcth_toplevel);
    updatePaletteMetadataEditor();
}
