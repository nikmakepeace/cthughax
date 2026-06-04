// -*- C++ -*-

#ifndef __XCTHUGHA_H
#define __XCTHUGHA_H

#include "cthugha.h"
#include <X11/Intrinsic.h>
#include <X11/extensions/XShm.h>

extern Display* xcth_display;

extern int display_mit_shm; /* use MIT-SHM if possible */
extern int display_on_root; /* display on root window */
extern int display_override_redirect; /* set override-redirect (no decoration) */
extern int private_cmap;
extern int xcth_panel;
extern xy window_pos;
extern int window_do_pos;
extern int full_screen;
extern char xcth_font[];

struct DisplayConfig;
struct X11Config;
void configureDisplayDeviceX11(const X11Config& config);

#include "DisplayDevice.h"
#include "DisplayGeometry.h"

class Scene;
class SceneCommands;

class DisplayDeviceX11 : public DisplayDevice {
    Scene& scene;
    SceneCommands& sceneCommands;

protected:
    Visual* visual;
    int screen;
    int planes;
    GC gc;
    Window window;

    // palette stuff
protected:
    Colormap colormap;
    int rev_byte_order;
    int ssWorkaround;

    int red_mask, green_mask, blue_mask;
    int red_shift, green_shift, blue_shift;

    XColor colors[256];

    Palette textPalette;

    int initPalette();
    int setGlobalPalette();
    void freePalette();

public:
    void setPalette(const Palette pal);

    // image alloc/free
    virtual int allocImage();
    virtual void freeImage();

    // font stuff
    XFontStruct* font;
    int loadFont();
    void freeFont();

    void resizeDisplay(int new_width, int new_height);
    int indexedDisplayWidth() const;
    int indexedDisplayHeight() const;
    void setPresentationViewport(const DisplayViewport& viewport);
    const DisplayViewport& currentPresentationViewport() const;

    // display operations
    virtual unsigned char* preDraw();
    virtual void copyBox(int, int, int, int, int, int);
    virtual void clearBox(int, int, int, int);
    virtual void postDraw();

    // print screen
    virtual int printScreen();

    // text output stuff
    virtual void prePrint();
    void writeCharacter(int x, int y, int text, int color);
    void printString(int x, int y, const char* text, int color, int len, int noDarken);

    Widget panelTextWidget;
    Widget palettePreviewWidget;
    Widget paletteNameTextWidget;
    Widget paletteSetTextWidget;
    Widget paletteEnergyTextWidget;
    Widget paletteMetadataStatusWidget;
    Pixmap textPixmap;
    Pixmap palettePreviewPixmap;
    int palettePreviewPalette;
    int palettePreviewWidth;
    int palettePreviewHeight;

    enum { shmNone, shmImage, shmPixmap } shmLevel;
    XShmSegmentInfo shminfo;
    int shmAttached;
    int shmMarkedForRemoval;
    Pixmap pixmap;
    XImage* image;
    int copyText;
    int paletteInitialized;
    int initialized;
    DisplayViewport presentationViewport;

protected:
    static Widget xcth_toplevel;
    Window xcth_root;

    // window creation stuff
    int getAttributes();
    int CreateWindow(const char* name, int full_screen);
    Cursor xcth_cursor();
    void checkDisplaySize(const DisplayConfig& config);
    void markSharedMemoryForRemoval();
    int allocNonSharedImage();
    int fallbackToNonSharedImage(const char* reason, int errnum, size_t requestedBytes);

    static void quit(int /*dummy*/) {
        cthugha_close++;
    }

    // control panel stuff
    typedef struct {
        SceneCommands* sceneCommands;
        EffectControl* opt;
        int pos;
    } menu_data_t;
    static void key_button(Widget w, XtPointer data, XtPointer data2);
    static void menuCB(Widget item, XtPointer data, XtPointer data2);
    static void savePaletteMetadataCB(Widget item, XtPointer data, XtPointer data2);
    static void revertPaletteMetadataCB(Widget item, XtPointer data, XtPointer data2);
    static void nextUntaggedPaletteCB(Widget item, XtPointer data, XtPointer data2);
    static void palettePreviewExpose(Widget item, XtPointer data, XEvent* event, Boolean* cont);
    Widget add_menu(const char* name, EffectControl* what, Widget parent, Widget under, Widget right);
    unsigned long palettePreviewPixel(unsigned char r, unsigned char g, unsigned char b);
    void updatePalettePreview();
    void drawPalettePreview();
    void updatePaletteMetadataEditor();
    void setPaletteMetadataStatus(const char* status);
    int savePaletteMetadata();
    void nextUntaggedPalette();
    void xcth_create_panel();

public:
    DisplayDeviceX11(Scene& scene_, SceneCommands& sceneCommands_,
        const DisplayConfig& config);
    virtual ~DisplayDeviceX11();

    int isInitialized() const { return initialized; }
    virtual DisplayEventStats processEvents();

    friend int cth_init(int* argc, char* argv[]);
};

#endif
