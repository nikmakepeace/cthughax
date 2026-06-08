#include "cthugha.h"
#include "DisplayDevice.h"
#include "cthugha.h"
#include "display.h"
#include "disp-sys.h"
#include "imath.h"
#include "xcthugha.h"
#include "InputQueue.h"
#include "Interface.h"
#include "Image.h"
#include "FramePalette.h"
#include "FlameOptions.h"
#include "RuntimeConfigRegistry.h"
#include "RuntimeConfigSelection.h"
#include "RuntimeCommandSink.h"
#include "RuntimeCommandTargets.h"
#include "cth_buffer.h"
#include "Scene.h"
#include "TranslationOption.h"
#include "WaveOptions.h"

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
#include <cstring>
#include <string>
#include <vector>

#if WITH_SAVER == 1
#include <X11/extensions/scrnsaver.h>
#endif
#include <X11/extensions/XShm.h>

#include "xcthugha.h"

template <class T>
static void xawSetArg(Arg& arg, const char* name, T value) {
    arg.name = const_cast<String>(name);
    arg.value = (XtArgVal)value;
}

static const int PalettePreviewStripHeight = 80;

//
// Create the panel with buttons and menus
//
void DisplayDeviceX11::key_button(Widget /*w*/, XtPointer data, XtPointer /*data2*/) {
    KeyButtonData* keyData = (KeyButtonData*)data;
    if ((keyData != 0) && (keyData->device != 0))
        keyData->device->enqueuePanelKey(keyData->keyText);
}

void DisplayDeviceX11::enqueuePanelKey(const char* keyText) {
    if (currentInputSink != 0)
        currentInputSink->pushRawKey(keyText, 0);
}

void DisplayDeviceX11::quit(Widget /*w*/, XtPointer data, XtPointer /*data2*/) {
    RuntimeCommandSink* sink = (RuntimeCommandSink*)data;
    if (sink != 0)
        sink->apply(RuntimeCommand::requestClose());
}

/*
 * handler for activated menu-items
 */
void DisplayDeviceX11::menuCB(Widget /*item*/, XtPointer data, XtPointer /*data2*/) {
    menu_data_t* d = (menu_data_t*)data;

    if ((d->hasSceneTarget != 0) && (d->runtimeCommands != 0)) {
        d->runtimeCommands->apply(
            RuntimeCommand::activateScene(d->sceneTarget, d->pos));
    } else if ((d->opt != 0) && (d->runtimeCommandRouter != 0)) {
        d->runtimeCommandRouter->activateEffectControl(*d->opt, d->pos);
    }
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
    drawPalettePreviewRect(0, 0, palettePreviewWidth, palettePreviewHeight);
}

void DisplayDeviceX11::drawPalettePreviewRect(int x, int y, int width, int height) {
    if ((palettePreviewWidget == NULL) || (palettePreviewPixmap == None)
        || (palettePreviewWidth <= 0) || (palettePreviewHeight <= 0))
        return;

    Window previewWindow = XtWindow(palettePreviewWidget);
    if (previewWindow == 0)
        return;

    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x + width > palettePreviewWidth)
        width = palettePreviewWidth - x;
    if (y + height > palettePreviewHeight)
        height = palettePreviewHeight - y;
    if ((width <= 0) || (height <= 0))
        return;

    XCopyArea(xcth_display, palettePreviewPixmap, previewWindow, gc, x, y,
        width, height, x, y);
}

static unsigned long palettePreviewFingerprint(const Palette& palette) {
    unsigned long hash = 2166136261UL;
    const unsigned char* bytes = &palette[0][0];

    for (int i = 0; i < 256 * 3; i++) {
        hash ^= bytes[i];
        hash *= 16777619UL;
    }

    return hash;
}

void DisplayDeviceX11::updatePalettePreview() {
    if (palettePreviewWidget == NULL)
        return;

    Window previewWindow = XtWindow(palettePreviewWidget);
    if (previewWindow == 0)
        return;

    int current_palette = scene.settings().paletteIndex;
    int palette_changed = current_palette != palettePreviewPalette;
    if (palette_changed) {
        palettePreviewPalette = current_palette;
        updatePaletteMetadataEditor();
    }

    PaletteEntry* paletteEntry = scene.settings().palette;
    const ColorPalette* targetPalette
        = (paletteEntry != NULL) ? &paletteEntry->colors() : palette.currentPalette();
    const ColorPalette* currentPalette
        = (framePalette != NULL) ? &framePalette->currentPalette() : targetPalette;
    if (targetPalette == NULL)
        targetPalette = currentPalette;
    if ((currentPalette == NULL) || (targetPalette == NULL))
        return;

    unsigned long currentFingerprint = palettePreviewFingerprint(currentPalette->raw());
    unsigned long targetFingerprint = palettePreviewFingerprint(targetPalette->raw());
    int currentChanged = !palettePreviewFingerprintValid
        || (currentFingerprint != palettePreviewCurrentFingerprint);
    int targetChanged = !palettePreviewFingerprintValid
        || (targetFingerprint != palettePreviewTargetFingerprint);

    if ((palettePreviewPixmap != None) && palettePreviewFingerprintValid
        && !currentChanged && !targetChanged)
        return;

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(xcth_display, previewWindow, &attrs))
        return;
    if ((attrs.width <= 0) || (attrs.height <= 0))
        return;

    int size_changed = (attrs.width != palettePreviewWidth) || (attrs.height != palettePreviewHeight);
    if (size_changed || (palettePreviewPixmap == None)) {
        if (palettePreviewPixmap != None) {
            XFreePixmap(xcth_display, palettePreviewPixmap);
            palettePreviewPixmap = None;
        }
        palettePreviewWidth = attrs.width;
        palettePreviewHeight = attrs.height;
        palettePreviewPixmap
            = XCreatePixmap(xcth_display, previewWindow, attrs.width, attrs.height, planes);
        palettePreviewFingerprintValid = 0;
    }
    if (palettePreviewPixmap == None)
        return;

    int targetStartY = palettePreviewHeight / 2;
    int dirtyY = 0;
    int dirtyHeight = palettePreviewHeight;
    if (!size_changed && palettePreviewFingerprintValid) {
        if (currentChanged && !targetChanged) {
            dirtyY = 0;
            dirtyHeight = targetStartY;
        } else if (!currentChanged && targetChanged) {
            dirtyY = targetStartY;
            dirtyHeight = palettePreviewHeight - targetStartY;
        }
    }
    if (dirtyHeight <= 0) {
        palettePreviewCurrentFingerprint = currentFingerprint;
        palettePreviewTargetFingerprint = targetFingerprint;
        palettePreviewFingerprintValid = 1;
        return;
    }

    XImage* preview = XCreateImage(xcth_display, visual, planes, ZPixmap, 0, NULL,
        palettePreviewWidth, dirtyHeight, XBitmapPad(xcth_display), 0);
    if (preview == NULL)
        return;

    preview->data = (char*)malloc(preview->bytes_per_line * dirtyHeight);
    if (preview->data == NULL) {
        XDestroyImage(preview);
        return;
    }

    const Palette& currentRaw = currentPalette->raw();
    const Palette& targetRaw = targetPalette->raw();

    for (int y = 0; y < dirtyHeight; y++) {
        int previewY = dirtyY + y;
        const Palette& rowPalette = (previewY < targetStartY) ? currentRaw : targetRaw;
        for (int x = 0; x < palettePreviewWidth; x++) {
            int paletteIndex = (x * 256) / palettePreviewWidth;
            const unsigned char* color = rowPalette[paletteIndex];
            XPutPixel(preview, x, y, palettePreviewPixel(color[0], color[1], color[2]));
        }
    }

    XPutImage(xcth_display, palettePreviewPixmap, gc, preview, 0, 0, 0, dirtyY,
        palettePreviewWidth, dirtyHeight);

    XDestroyImage(preview);
    palettePreviewCurrentFingerprint = currentFingerprint;
    palettePreviewTargetFingerprint = targetFingerprint;
    palettePreviewFingerprintValid = 1;
    drawPalettePreviewRect(0, dirtyY, palettePreviewWidth, dirtyHeight);
}

void DisplayDeviceX11::setPaletteMetadataStatus(const char* status) {
    if (paletteMetadataStatusWidget == NULL)
        return;

    Arg wargs[1];
    xawSetArg(wargs[0], XtNlabel, status ? status : "");
    XtSetValues(paletteMetadataStatusWidget, wargs, 1);
}

void DisplayDeviceX11::updatePaletteMetadataEditor() {
    if ((paletteNameTextWidget == NULL) || (paletteSetTextWidget == NULL)
        || (paletteEnergyTextWidget == NULL))
        return;

    PaletteEntry* paletteEntry = scene.settings().palette;
    if (paletteEntry == NULL)
        return;

    XtVaSetValues(paletteNameTextWidget, XtNstring, paletteEntry->metadataName, NULL);
    XtVaSetValues(paletteSetTextWidget, XtNstring, paletteEntry->metadataSet, NULL);
    XtVaSetValues(paletteEnergyTextWidget, XtNstring, paletteEntry->metadataEnergy, NULL);
    setPaletteMetadataStatus(paletteEntry->sourcePath[0] ? paletteEntry->Name() : "read-only");
}

int DisplayDeviceX11::savePaletteMetadata() {
    if ((paletteNameTextWidget == NULL) || (paletteSetTextWidget == NULL)
        || (paletteEnergyTextWidget == NULL))
        return 0;

    PaletteEntry* paletteEntry = scene.settings().palette;
    if ((paletteEntry == NULL) || (paletteEntry->sourcePath[0] == '\0')) {
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

    PaletteEntry scratch(paletteEntry->Name(), "");
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

    FILE* in = fopen(paletteEntry->sourcePath, "rb");
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
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", paletteEntry->sourcePath);
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
    if (rename(tmp_path, paletteEntry->sourcePath) != 0) {
        unlink(tmp_path);
        setPaletteMetadataStatus("save failed");
        return 0;
    }

    paletteEntry->setMetadataName(scratch.metadataName);
    if (scratch.metadataSet[0]) {
        palette_set_metadata_set(paletteEntry, scratch.metadataSet);
    } else {
        paletteEntry->metadataSet[0] = '\0';
        paletteEntry->metadataSetCount = 0;
    }
    if (scratch.metadataEnergy[0]) {
        palette_set_metadata_energy(paletteEntry, scratch.metadataEnergy);
    } else {
        paletteEntry->metadataEnergy[0] = '\0';
        paletteEntry->metadataEnergyCount = 0;
    }
    XtVaSetValues(paletteNameTextWidget, XtNstring, paletteEntry->metadataName, NULL);
    XtVaSetValues(paletteSetTextWidget, XtNstring, paletteEntry->metadataSet, NULL);
    XtVaSetValues(paletteEnergyTextWidget, XtNstring, paletteEntry->metadataEnergy, NULL);
    setPaletteMetadataStatus("saved");
    return 1;
}

void DisplayDeviceX11::revertPaletteMetadata() {
    updatePaletteMetadataEditor();
    setPaletteMetadataStatus("reverted");
}

void DisplayDeviceX11::nextUntaggedPalette() {
    int n = palette.getNEntries();
    int start = scene.settings().paletteIndex;
    for (int step = 1; step <= n; step++) {
        int candidate = (start + step) % n;
        PaletteEntry* paletteEntry = (PaletteEntry*)palette[candidate];
        if ((paletteEntry != NULL)
            && ((paletteEntry->metadataName[0] == '\0') || (paletteEntry->metadataSetCount == 0)
                || (paletteEntry->metadataEnergyCount == 0))) {
            runtimeCommands.apply(
                RuntimeCommand::activateScene(RuntimeScenePalette, candidate));
            palettePreviewPalette = -1;
            updatePalettePreview();
            setPaletteMetadataStatus("next untagged");
            return;
        }
    }

    setPaletteMetadataStatus("all tagged");
}

void DisplayDeviceX11::savePaletteMetadataCB(
    Widget /*item*/, XtPointer data, XtPointer /*data2*/) {
    DisplayDeviceX11* device = (DisplayDeviceX11*)data;
    if (device != NULL)
        device->runtimeCommands.apply(
            RuntimeCommand::savePaletteMetadata(*device));
}

void DisplayDeviceX11::revertPaletteMetadataCB(
    Widget /*item*/, XtPointer data, XtPointer /*data2*/) {
    DisplayDeviceX11* device = (DisplayDeviceX11*)data;
    if (device != NULL)
        device->runtimeCommands.apply(
            RuntimeCommand::revertPaletteMetadata(*device));
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
        device->palettePreviewFingerprintValid = 0;
        device->updatePalettePreview();
    } else if (event->type == Expose) {
        device->drawPalettePreviewRect(event->xexpose.x, event->xexpose.y,
            event->xexpose.width, event->xexpose.height);
    }
}

void DisplayDeviceX11::panelTextExpose(
    Widget /*item*/, XtPointer data, XEvent* event, Boolean* /*cont*/) {
    DisplayDeviceX11* device = (DisplayDeviceX11*)data;
    if (device == NULL)
        return;

    if (event->type == ConfigureNotify) {
        device->panelTextDirty = 1;
        device->markPanelTextCopyRect(0, 0, text_size.x * fontSize.x,
            text_size.y * fontSize.y, 2);
    } else if (event->type == Expose) {
        device->markPanelTextCopyRect(event->xexpose.x, event->xexpose.y,
            event->xexpose.width, event->xexpose.height, 1);
    }
}

static const char* currentNameOrEmpty(EffectControl& control) {
    const char* name = control.currentName();
    return (name != NULL && strcmp(name, "unknown") != 0) ? name : "";
}

void DisplayDeviceX11::updatePanelSelectionLabels() {
    if (panelMenuButtons[0] == NULL)
        return;

    struct PanelSelection {
        const char* name;
        RuntimeConfigSelectionField field;
        const char* fallback;
    };

    PanelSelection selections[8] = {
        { "Display", RuntimeConfigSelectionDisplay, currentNameOrEmpty(::screen) },
        { "Wave", RuntimeConfigSelectionWave, currentNameOrEmpty(wave) },
        { "Flame", RuntimeConfigSelectionFlame, currentNameOrEmpty(flame) },
        { "Translation", RuntimeConfigSelectionTranslation,
            currentNameOrEmpty(translation) },
        { "Palette", RuntimeConfigSelectionPalette, currentNameOrEmpty(palette) },
        { "Table", RuntimeConfigSelectionTable, currentNameOrEmpty(table) },
        { "Image", RuntimeConfigSelectionImage, currentNameOrEmpty(images) },
        { "Objects", RuntimeConfigSelectionObject, currentNameOrEmpty(object) },
    };

    std::string signature;
    for (int i = 0; i < 8; i++) {
        signature += selections[i].fallback ? selections[i].fallback : "";
        signature.push_back('\n');
    }
    if (signature == panelSelectionSignature)
        return;
    panelSelectionSignature = signature;

    Config config = runtimeConfigRegistry.currentConfig();
    Arg wargs[1];
    for (int i = 0; i < 8; i++) {
        if (panelMenuButtons[i] == NULL)
            continue;

        std::string selected = runtimeConfigSelectionTextOrFallback(config,
            selections[i].field, selections[i].fallback);
        char nextLabel[sizeof(panelMenuLabels[i])];
        snprintf(nextLabel, sizeof(nextLabel), "%s: %s",
            selections[i].name, selected.c_str());
        if (strcmp(panelMenuLabels[i], nextLabel) == 0)
            continue;

        strncpy(panelMenuLabels[i], nextLabel, sizeof(panelMenuLabels[i]));
        panelMenuLabels[i][sizeof(panelMenuLabels[i]) - 1] = '\0';
        xawSetArg(wargs[0], XtNlabel, panelMenuLabels[i]);
        XtSetValues(panelMenuButtons[i], wargs, 1);
    }
}

Widget DisplayDeviceX11::add_menu(
    const char* name, EffectControl* what, Widget parent, Widget under, Widget right) {
    return add_menu_target(name, what, RuntimeSceneFlame, 0, parent, under,
        right);
}

Widget DisplayDeviceX11::add_scene_menu(const char* name, EffectControl* what,
    RuntimeSceneTarget sceneTarget, Widget parent, Widget under,
    Widget right) {
    return add_menu_target(name, what, sceneTarget, 1, parent, under, right);
}

Widget DisplayDeviceX11::add_menu_target(const char* name, EffectControl* what,
    RuntimeSceneTarget sceneTarget, int hasSceneTarget, Widget parent,
    Widget under, Widget right) {
    Arg wargs[3];
    int n;
    Widget button, menu, item;
    int i;

    if (parent == NULL)
        return NULL;

    /* create menu button */
    n = 0;
    xawSetArg(wargs[n], XtNlabel, name);
    n++;
    if (under) {
        xawSetArg(wargs[n], XtNfromVert, under);
        n++;
    }
    if (right) {
        xawSetArg(wargs[n], XtNfromHoriz, right);
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
        xawSetArg(wargs[0], XtNlabel, "none");
        XtCreateManagedWidget("none", smeBSBObjectClass, menu, wargs, 1);
    }

    /* create menu items */
    for (i = 0; i < what->getNEntries(); i++) {
        menu_data_t* md = new menu_data_t;
        const char* label = (*what)[i]->Desc()[0] ? (*what)[i]->Desc() : (*what)[i]->Name();

        md->runtimeCommands = &runtimeCommands;
        md->runtimeCommandRouter = &runtimeCommandRouter;
        md->opt = what;
        md->sceneTarget = sceneTarget;
        md->hasSceneTarget = hasSceneTarget;
        md->pos = i;

        xawSetArg(wargs[0], XtNlabel, label);
        item = XtCreateManagedWidget((*what)[i]->Name(), smeBSBObjectClass, menu, wargs, 1);
        XtAddCallback(item, XtNcallback, menuCB, md);
    }
    return button;
}
void DisplayDeviceX11::xcth_create_panel() {
    Widget panel;
    Widget quit_button, change_button;
    Widget name_label, set_label, energy_label;
    Widget save_metadata_button, revert_metadata_button, next_untagged_button;
    Arg wargs[8];

    /* create the panel formWidget */
    panel = XtCreateManagedWidget("panel", formWidgetClass, xcth_toplevel, NULL, 0);

    /* create the quit button */
    xawSetArg(wargs[0], XtNlabel, "Quit!");
    quit_button = XtCreateManagedWidget("quit", commandWidgetClass, panel, wargs, 1);
    XtAddCallback(quit_button, XtNcallback, (XtCallbackProc)quit, &runtimeCommands);

    /* create the change button */
    xawSetArg(wargs[0], XtNlabel, "Change!");
    xawSetArg(wargs[1], XtNfromHoriz, quit_button);
    change_button = XtCreateManagedWidget("change", commandWidgetClass, panel, wargs, 2);
    XtAddCallback(change_button, XtNcallback, key_button, &changeKeyButtonData);

    /* create the menus */
    panelMenuButtons[0] = add_menu("Display", &::screen, panel, quit_button, NULL);
    panelMenuButtons[1] = add_scene_menu("Wave", &wave, RuntimeSceneWave,
        panel, quit_button, panelMenuButtons[0]);
    panelMenuButtons[2] = add_scene_menu("Flame", &flame, RuntimeSceneFlame,
        panel, quit_button, panelMenuButtons[1]);
    panelMenuButtons[3] = add_scene_menu("Translation", &translation,
        RuntimeSceneTranslation, panel, quit_button, panelMenuButtons[2]);
    panelMenuButtons[4] = add_scene_menu("Palette", &palette,
        RuntimeScenePalette, panel, quit_button, panelMenuButtons[3]);
    panelMenuButtons[5] = add_scene_menu("Table", &table, RuntimeSceneTable,
        panel, quit_button, panelMenuButtons[4]);
    panelMenuButtons[6] = add_scene_menu("Image", &images, RuntimeSceneImage,
        panel, quit_button, panelMenuButtons[5]);
    panelMenuButtons[7] = add_scene_menu("Objects", &object,
        RuntimeSceneObject, panel, quit_button, panelMenuButtons[6]);
    updatePanelSelectionLabels();

    // create the panelText Widget
    text_size.x = 80;
    text_size.y = 40 / fontSize.y;
    if (text_size.y < 1)
        text_size.y = 1;
    xawSetArg(wargs[0], XtNfromVert, panelMenuButtons[0]);
    xawSetArg(wargs[1], XtNwidth, fontSize.x * text_size.x);
    xawSetArg(wargs[2], XtNheight, 40);
    panelTextWidget = XtCreateManagedWidget("panelText", labelWidgetClass, panel, wargs, 3);
    XtAddEventHandler(
        panelTextWidget, ExposureMask | StructureNotifyMask, False, panelTextExpose, this);

    xawSetArg(wargs[0], XtNfromVert, panelTextWidget);
    xawSetArg(wargs[1], XtNwidth, fontSize.x * text_size.x);
    xawSetArg(wargs[2], XtNheight, PalettePreviewStripHeight * 2);
    xawSetArg(wargs[3], XtNright, XawChainRight);
    xawSetArg(wargs[4], XtNborderWidth, 0);
    palettePreviewWidget = XtCreateManagedWidget("palettePreview", simpleWidgetClass, panel, wargs, 5);
    XtAddEventHandler(
        palettePreviewWidget, ExposureMask | StructureNotifyMask, False, palettePreviewExpose, this);

    xawSetArg(wargs[0], XtNlabel, "Name");
    xawSetArg(wargs[1], XtNfromVert, palettePreviewWidget);
    name_label = XtCreateManagedWidget("paletteNameLabel", labelWidgetClass, panel, wargs, 2);

    xawSetArg(wargs[0], XtNfromVert, palettePreviewWidget);
    xawSetArg(wargs[1], XtNfromHoriz, name_label);
    xawSetArg(wargs[2], XtNwidth, fontSize.x * 64);
    xawSetArg(wargs[3], XtNeditType, XawtextEdit);
    xawSetArg(wargs[4], XtNstring, "");
    paletteNameTextWidget
        = XtCreateManagedWidget("paletteNameText", asciiTextWidgetClass, panel, wargs, 5);

    xawSetArg(wargs[0], XtNlabel, "Set");
    xawSetArg(wargs[1], XtNfromVert, name_label);
    set_label = XtCreateManagedWidget("paletteSetLabel", labelWidgetClass, panel, wargs, 2);

    xawSetArg(wargs[0], XtNfromVert, paletteNameTextWidget);
    xawSetArg(wargs[1], XtNfromHoriz, set_label);
    xawSetArg(wargs[2], XtNwidth, fontSize.x * 64);
    xawSetArg(wargs[3], XtNeditType, XawtextEdit);
    xawSetArg(wargs[4], XtNstring, "");
    paletteSetTextWidget
        = XtCreateManagedWidget("paletteSetText", asciiTextWidgetClass, panel, wargs, 5);

    xawSetArg(wargs[0], XtNlabel, "Energy");
    xawSetArg(wargs[1], XtNfromVert, set_label);
    energy_label = XtCreateManagedWidget("paletteEnergyLabel", labelWidgetClass, panel, wargs, 2);

    xawSetArg(wargs[0], XtNfromVert, paletteSetTextWidget);
    xawSetArg(wargs[1], XtNfromHoriz, energy_label);
    xawSetArg(wargs[2], XtNwidth, fontSize.x * 64);
    xawSetArg(wargs[3], XtNeditType, XawtextEdit);
    xawSetArg(wargs[4], XtNstring, "");
    paletteEnergyTextWidget
        = XtCreateManagedWidget("paletteEnergyText", asciiTextWidgetClass, panel, wargs, 5);

    xawSetArg(wargs[0], XtNlabel, "Save Metadata");
    xawSetArg(wargs[1], XtNfromVert, energy_label);
    save_metadata_button
        = XtCreateManagedWidget("savePaletteMetadata", commandWidgetClass, panel, wargs, 2);
    XtAddCallback(save_metadata_button, XtNcallback, savePaletteMetadataCB, this);

    xawSetArg(wargs[0], XtNlabel, "Revert");
    xawSetArg(wargs[1], XtNfromVert, energy_label);
    xawSetArg(wargs[2], XtNfromHoriz, save_metadata_button);
    revert_metadata_button
        = XtCreateManagedWidget("revertPaletteMetadata", commandWidgetClass, panel, wargs, 3);
    XtAddCallback(revert_metadata_button, XtNcallback, revertPaletteMetadataCB, this);

    xawSetArg(wargs[0], XtNlabel, "Next Untagged");
    xawSetArg(wargs[1], XtNfromVert, energy_label);
    xawSetArg(wargs[2], XtNfromHoriz, revert_metadata_button);
    next_untagged_button
        = XtCreateManagedWidget("nextUntaggedPalette", commandWidgetClass, panel, wargs, 3);
    XtAddCallback(next_untagged_button, XtNcallback, nextUntaggedPaletteCB, this);

    xawSetArg(wargs[0], XtNlabel, "");
    xawSetArg(wargs[1], XtNfromVert, energy_label);
    xawSetArg(wargs[2], XtNfromHoriz, next_untagged_button);
    xawSetArg(wargs[3], XtNwidth, fontSize.x * 18);
    xawSetArg(wargs[4], XtNborderWidth, 0);
    xawSetArg(wargs[5], XtNjustify, XtJustifyLeft);
    paletteMetadataStatusWidget
        = XtCreateManagedWidget("paletteMetadataStatus", labelWidgetClass, panel, wargs, 6);

    /* realize window */
    XtRealizeWidget(xcth_toplevel);
    updatePaletteMetadataEditor();
}
