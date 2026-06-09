// -*- C++ -*-

#ifndef __XCTHUGHA_H
#define __XCTHUGHA_H

#include "cthugha.h"
#include <X11/Intrinsic.h>
#include <X11/extensions/XShm.h>
#include <memory>
#include <string>
#include <vector>

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
#include "RuntimeCommandSink.h"

class DisplayDriverFactory;
class DisplayFrontendInitializer;
class Scene;
class ImageOption;
class RuntimeConfigRegistry;
class RuntimeCommandTargetRouter;
class SecondsClock;
class InputEventSink;
class SceneOptionSelection;
class SceneVisualSelections;

class DisplayDeviceX11 : public DisplayDevice, public RuntimePaletteMetadataTarget {
    Scene& scene;
    ImageOption& images;
    SceneVisualSelections* sceneVisualSelections;
    RuntimeCommandSink& runtimeCommands;
    RuntimeCommandTargetRouter& runtimeCommandRouter;
    RuntimeConfigRegistry& runtimeConfigRegistry;
    SecondsClock& clock;

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

    // text output stuff
    virtual void prePrint();
    virtual void postPrint();
    void writeCharacter(int x, int y, int text, int color);
    void printString(int x, int y, const char* text, int color, int len, int noDarken);

    Widget panelTextWidget;
    Widget panelMenuButtons[8];
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
    int palettePreviewFingerprintValid;
    unsigned long palettePreviewCurrentFingerprint;
    unsigned long palettePreviewTargetFingerprint;
    char panelMenuLabels[8][128];
    struct PanelTextCommand {
        int x;
        int y;
        int color;
        int noDarken;
        std::string text;
    };
    static int samePanelTextCommand(const PanelTextCommand& a,
        const PanelTextCommand& b);
    static void panelTextCommandBounds(const PanelTextCommand& command,
        int* x, int* y, int* width, int* height);
    std::vector<PanelTextCommand> panelPendingTextCommands;
    std::vector<PanelTextCommand> panelCommittedTextCommands;
    std::string panelPendingTextSignature;
    std::string panelCommittedTextSignature;
    std::string panelSelectionSignature;
    int panelTextDirty;
    int panelTextCopyX;
    int panelTextCopyY;
    int panelTextCopyWidth;
    int panelTextCopyHeight;
    InputEventSink* currentInputSink;
    struct KeyButtonData {
        DisplayDeviceX11* device;
        const char* keyText;

        KeyButtonData()
            : device(NULL)
            , keyText(NULL) { }
    } changeKeyButtonData;

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
    PixelSize fallbackIndexedFrameSize;

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

    static void quit(Widget w, XtPointer data, XtPointer data2);

    // control panel stuff
    typedef struct {
        RuntimeCommandSink* runtimeCommands;
        RuntimeCommandTargetRouter* runtimeCommandRouter;
        EffectControl* opt;
        RuntimeSceneTarget sceneTarget;
        int hasSceneTarget;
        int pos;
    } menu_data_t;
    static void key_button(Widget w, XtPointer data, XtPointer data2);
    void enqueuePanelKey(const char* keyText);
    static void menuCB(Widget item, XtPointer data, XtPointer data2);
    static void savePaletteMetadataCB(Widget item, XtPointer data, XtPointer data2);
    static void revertPaletteMetadataCB(Widget item, XtPointer data, XtPointer data2);
    static void nextUntaggedPaletteCB(Widget item, XtPointer data, XtPointer data2);
    static void palettePreviewExpose(Widget item, XtPointer data, XEvent* event, Boolean* cont);
    static void panelTextExpose(Widget item, XtPointer data, XEvent* event, Boolean* cont);
    Widget add_menu(const char* name, EffectControl* what, Widget parent, Widget under, Widget right);
    Widget add_scene_menu(const char* name, RuntimeSceneTarget sceneTarget,
        Widget parent, Widget under, Widget right);
    Widget add_menu_target(const char* name, EffectControl* what,
        RuntimeSceneTarget sceneTarget, int hasSceneTarget, Widget parent,
        Widget under, Widget right);
    SceneOptionSelection* sceneSelection(RuntimeSceneTarget target) const;
    void updatePanelSelectionLabels();
    void markPanelTextCopyRect(int x, int y, int width, int height, int copyCount);
    unsigned long palettePreviewPixel(unsigned char r, unsigned char g, unsigned char b);
    void updatePalettePreview();
    void drawPalettePreview();
    void drawPalettePreviewRect(int x, int y, int width, int height);
    void updatePaletteMetadataEditor();
    void setPaletteMetadataStatus(const char* status);
    virtual int savePaletteMetadata();
    virtual void revertPaletteMetadata();
    void nextUntaggedPalette();
    void xcth_create_panel();

public:
    DisplayDeviceX11(Scene& scene_, ImageOption& images_,
        SceneVisualSelections* sceneVisualSelections_,
        RuntimeCommandSink& runtimeCommands_,
        RuntimeCommandTargetRouter& runtimeCommandRouter_,
        RuntimeConfigRegistry& runtimeConfigRegistry_, const DisplayConfig& config,
        SecondsClock& clock_);
    virtual ~DisplayDeviceX11();

    int isInitialized() const { return initialized; }
    virtual DisplayEventStats processEvents(InputEventSink& input);

    friend int cth_init(int* argc, char* argv[]);
};

/** Creates the X11 display driver factory when X11 is compiled in. */
std::unique_ptr<DisplayDriverFactory> newX11DisplayDriverFactory(
    DisplayFrontendInitializer& frontendInitializer,
    const X11Config& config);

#endif
