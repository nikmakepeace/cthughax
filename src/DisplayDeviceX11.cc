/** @file
 * X11 display backend.
 *
 * Owns the X Toolkit event loop, window or root-window target, XImage/XPixmap
 * backing storage, optional MIT-SHM acceleration, text overlay rendering, and
 * palette translation for both true-color and indexed-color X visuals.
 */

#include "cthugha.h"
#include "DisplayDevice.h"
#include "ApplicationDisplayFrontend.h"
#include "Configuration.h"
#include "CthughaDisplay.h"
#include "FramePalette.h"
#include "Scene.h"
#include "RuntimeCommandSink.h"
#include "RuntimeConfigRegistry.h"
#include "cthugha.h"
#include "display.h"
#include "disp-sys.h"
#include "imath.h"
#include "xcthugha.h"
#include "InputQueue.h"
#include "Interface.h"
#include "DisplayBackend.h"
#include "DisplayRuntime.h"
#include "DisplaySystem.h"
#include "OverlaySource.h"
#include "PixelTransfer.h"
#include "ProcessServices.h"
#include "ViewportPresentation.h"
#include "xcthugha.h"

#include <unistd.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xatom.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <utility>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "vroot.h"

#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

static void renderOverlayCommands(DisplayDevice& device,
    const OverlayCommands& overlays) {
    device.prePrint();
    for (size_t i = 0; i < overlays.count(); ++i) {
        const OverlayTextCommand& command = overlays.at(i);
        device.print(command.text.c_str(), command.y, command.justification,
            command.color, command.noDarken);
    }
    device.postPrint();
}

class DisplayBackendX11 : public DisplayBackend {
    DisplayDeviceX11& device;

public:
    explicit DisplayBackendX11(DisplayDeviceX11& device_)
        : device(device_) {
    }

    virtual DisplayEventStats processEvents(InputEventSink& input) {
        return device.processEvents(input);
    }

    virtual PixelSize outputSize() const {
        return PixelSize::fromXy(disp_size);
    }

    virtual void present(const DisplayPresentation& presentation) {
        if (presentation.framePalette != NULL)
            device.setFramePalette(presentation.framePalette);
        device.setPresentationViewport(presentation.viewport);
        DisplayDevice& baseDevice = device;
        baseDevice.setGlobalPalette();
        if (presentation.needsFullCopy || presentation.needsBorderClear)
            device.needsFullCopy = 1;

        unsigned char* displayBase = device.preDraw();
        if (displayBase == 0 || !presentation.frame.valid())
            return;

        PixelRect drawRect = ViewportPresentation::drawCopyRect(presentation.viewport);
        if (!drawRect.valid())
            return;

        unsigned char* destination = displayBase
            + ViewportPresentation::drawOffsetBytes(presentation.viewport, bytes_per_line, bypp);
        PixelTransfer::indexedToNative(presentation.frame.pixels(),
            PixelSize(presentation.frame.width(), presentation.frame.height()),
            presentation.frame.pitch(), destination, drawRect.size(),
            bytes_per_line, bypp, bitmap_colors0);

        renderOverlayCommands(device, presentation.overlays);
    }
};

int display_override_redirect = 0; // bypass the window manager
int private_cmap = 0; // allocate a window-private colormap
int display_mit_shm = 0; // use MIT-SHM if possible
int display_on_root = 0; // display on root window
int full_screen = 0;
int window_do_pos = 0;
xy window_pos(0, 0);
int xcth_panel = 0; // use control panel
char xcth_font[256] = "";
static int x11_frame_dump_enabled = 0;
static int x11_frame_dump_directory_ready = 0;
static int x11_frame_dump_frame = 0;
static int x11_frame_dump_dumped = 0;
static int x11_frame_dump_limit = 1;
static int x11_frame_dump_every = 1;
static char x11_frame_dump_directory[PATH_MAX] = "";

void configureDisplayDeviceX11(const X11Config& config) {
    display_override_redirect = config.overrideRedirect;
    private_cmap = config.privateCmap;
    display_mit_shm = config.mitShm;
    display_on_root = config.rootWindow;
    full_screen = config.fullscreen;
    window_do_pos = config.windowPositionEnabled;
    window_pos.x = config.windowPositionX;
    window_pos.y = config.windowPositionY;
    xcth_panel = config.panelEnabled;
    strncpy(xcth_font, config.fontName.c_str(), sizeof(xcth_font));
    xcth_font[sizeof(xcth_font) - 1] = '\0';
    x11_frame_dump_enabled = !config.frameDumpDirectory.empty();
    x11_frame_dump_directory_ready = 0;
    x11_frame_dump_frame = 0;
    x11_frame_dump_dumped = 0;
    x11_frame_dump_limit = max(1, config.frameDumpLimit);
    x11_frame_dump_every = max(1, config.frameDumpEvery);
    strncpy(x11_frame_dump_directory, config.frameDumpDirectory.c_str(),
        sizeof(x11_frame_dump_directory));
    x11_frame_dump_directory[sizeof(x11_frame_dump_directory) - 1] = '\0';
}

Display* xcth_display;

static XtAppContext xcth_app_con;
Widget DisplayDeviceX11::xcth_toplevel;

static int mask_shift(unsigned long mask) {
    int shift = 0;

    if (mask == 0)
        return 0;

    while ((mask & 1) == 0) {
        mask >>= 1;
        shift++;
    }

    return shift;
}

static int mask_bits(unsigned long mask) {
    int bits = 0;

    while (mask) {
        if (mask & 1)
            bits++;
        mask >>= 1;
    }

    return bits;
}

static unsigned char scale_mask_value(unsigned long pixel, unsigned long mask) {
    int shift;
    int bits;
    unsigned long value;
    unsigned long maxValue;

    if (mask == 0)
        return 0;

    shift = mask_shift(mask);
    bits = mask_bits(mask >> shift);
    value = (pixel & mask) >> shift;
    maxValue = (1UL << bits) - 1;

    return (unsigned char)((value * 255UL) / maxValue);
}

static void setIndexedNativePixel(int index, unsigned long pixel) {
    bitmap_colors0[index] = pixel;
    bitmap_colors1[index] = bitmap_colors0[index] << 8;
    bitmap_colors2[index] = bitmap_colors1[index] << 8;
    bitmap_colors3[index] = bitmap_colors2[index] << 8;
}

static void mapIndexedPixelsToColorCells(const XColor* colors) {
    for (int i = 0; i < 256; i++)
        setIndexedNativePixel(i, colors[i].pixel);
}

static void mapIndexedPixelsToPairedColorCells(const XColor* colors) {
    for (int i = 0; i < 256; i++)
        setIndexedNativePixel(i, colors[i / 2].pixel);
}

static void dump_x11_frame(XImage* image) {
    if (!x11_frame_dump_enabled || (image == NULL))
        return;

    if (!x11_frame_dump_directory_ready) {
        x11_frame_dump_directory_ready = 1;
        if ((mkdir(x11_frame_dump_directory, 0777) != 0)
            && (errno != EEXIST)) {
            CTH_ERRNO(errno, "Can not create X11 frame dump directory");
            x11_frame_dump_enabled = 0;
            return;
        }
    }

    x11_frame_dump_frame++;
    if ((x11_frame_dump_frame % x11_frame_dump_every) != 0)
        return;
    if (x11_frame_dump_dumped >= x11_frame_dump_limit)
        return;

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/frame-%06d.ppm", x11_frame_dump_directory,
        x11_frame_dump_frame);

    FILE* out = fopen(path, "wb");
    if (out == NULL) {
        CTH_ERRNO(errno, "Can not create X11 frame dump");
        x11_frame_dump_enabled = 0;
        return;
    }

    fprintf(out, "P6\n%d %d\n255\n", image->width, image->height);

    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            unsigned char rgb[3];

            if (image->red_mask || image->green_mask || image->blue_mask) {
                rgb[0] = scale_mask_value(pixel, image->red_mask);
                rgb[1] = scale_mask_value(pixel, image->green_mask);
                rgb[2] = scale_mask_value(pixel, image->blue_mask);
            } else {
                rgb[0] = rgb[1] = rgb[2] = (unsigned char)pixel;
            }

            fwrite(rgb, 1, 3, out);
        }
    }

    fclose(out);
    x11_frame_dump_dumped++;
}

int cth_init(int* argc, char* argv[]) {
    if (xcth_display != NULL)
        return 0;

    // Xt owns command-line option parsing for X resources and creates the
    // application shell used by both the display window and optional panel.
    DisplayDeviceX11::xcth_toplevel
        = XtAppInitialize(&xcth_app_con, "Cthugha", NULL, 0, argc, argv, NULL, NULL, 0);

    xcth_display = XtDisplay(DisplayDeviceX11::xcth_toplevel);

    return 0;
}

int DisplayDeviceX11::getAttributes() {

    XWindowAttributes wa;
    XGetWindowAttributes(xcth_display, window, &wa);

    xcth_root = wa.root;
    visual = wa.visual;
    planes = wa.depth;
    colormap = wa.colormap;

    CTH_DEBUG("    display size       : %dx%d\n", disp_size.x, disp_size.y);
    CTH_DEBUG("    color planes       : %d\n", planes);

    return (wa.map_state == IsViewable);
}

int DisplayDeviceX11::CreateWindow(const char* name, int full_screen) {
    XSetWindowAttributes attr;
    unsigned long mask;
    XSizeHints* sh;
    unsigned long black_pixel = 0;

    if (full_screen) {
        disp_size.x = DisplayWidth(xcth_display, screen);
        disp_size.y = DisplayHeight(xcth_display, screen);
    }

    mask = CWEventMask | CWCursor | CWBackPixel;
    attr.background_pixel = black_pixel;
    attr.event_mask = KeyReleaseMask | StructureNotifyMask | ExposureMask;
    attr.cursor = xcth_cursor();

    if (display_override_redirect) {
        attr.override_redirect = 1;
        mask |= CWOverrideRedirect;
    }

    window = XCreateWindow(xcth_display, DefaultRootWindow(xcth_display),
        window_do_pos ? window_pos.x : 0, window_do_pos ? window_pos.y : 0, disp_size.x,
        disp_size.y, 0, CopyFromParent, InputOutput, CopyFromParent, mask, &attr);

    if (name)
        XStoreName(xcth_display, window, name);

    getAttributes();
    if (allocImage()) // Sets bypp and bits-per-pixel from the server's image format.
        return 1;
    if (initPalette()) // Must be done before XMapWindow when installing a colormap.
        return 1;

    if (full_screen) {
        // Older window managers may still decorate "fullscreen" windows.
        // Request a screen-sized base geometry first, then correct the actual
        // position after the manager has mapped and decorated the window.
        sh = XAllocSizeHints();
        sh->flags = USPosition | PPosition | PBaseSize;
        sh->x = sh->y = 0;
        sh->base_width = disp_size.x;
        sh->base_height = disp_size.y;
        XSetWMNormalHints(xcth_display, window, sh);

        XMapRaised(xcth_display, window);

        XSync(xcth_display, False);

        XWindowAttributes wa;
        XGetWindowAttributes(xcth_display, window, &wa);
        sleep(1);
        XGetWindowAttributes(xcth_display, window, &wa);

        // Move the client window so any manager-added border sits off-screen.
        XMoveWindow(xcth_display, window, -wa.x, -wa.y);

        XSetInputFocus(xcth_display, window, RevertToParent, CurrentTime);
    } else if (window_do_pos) {
        sh = XAllocSizeHints();
        sh->flags = USPosition | PBaseSize | PPosition;
        sh->x = window_pos.x;
        sh->y = window_pos.y;
        sh->base_width = disp_size.x;
        sh->base_height = disp_size.y;
        XSetWMNormalHints(xcth_display, window, sh);

        XMapWindow(xcth_display, window);
    } else
        XMapWindow(xcth_display, window);

    return 0;
}

Cursor DisplayDeviceX11::xcth_cursor() {
    Pixmap blank_pix;
    XColor dummyColor;
    GC bit_1_gc, bit_0_gc;

    // X cursors are pixmaps plus masks. A 1x1 all-zero pixmap produces the
    // invisible cursor wanted for full-screen visual output.
    blank_pix = XCreatePixmap(xcth_display, xcth_root, 1, 1, 1);
    bit_0_gc = XCreateGC(xcth_display, blank_pix, 0, 0);
    XSetForeground(xcth_display, bit_0_gc, 0);

    bit_1_gc = XCreateGC(xcth_display, blank_pix, 0, 0);
    XSetForeground(xcth_display, bit_1_gc, ~0);

    XFillRectangle(xcth_display, blank_pix, bit_0_gc, 0, 0, 1, 1);

    return XCreatePixmapCursor(xcth_display, blank_pix, blank_pix, &dummyColor, &dummyColor, 0, 0);
}

void DisplayDeviceX11::checkDisplaySize(const DisplayConfig& config) {
    if (config.hasCustomDisplaySize) {
        disp_size.x = config.displayWidth;
        disp_size.y = config.displayHeight;
        return;
    }

    int mode = config.displayMode;
    if ((mode >= nScreenSizes) || (mode < 0))
        mode = 0;

    disp_size = screenSizes[mode];
}

int DisplayDeviceX11::loadFont() {
    font = XLoadQueryFont(xcth_display, xcth_font);
    if (font == NULL) {
        CTH_ERROR("Can not load font `%s'. Trying font `fixed'.\n", xcth_font);

        font = XLoadQueryFont(xcth_display, "fixed");
        if (font == NULL) {
            CTH_ERROR("Can not load font fixed.\n");
            return 1;
        }
    }
    XSetFont(xcth_display, gc, font->fid);
    fontSize.y = font->max_bounds.ascent + 1;
    fontSize.x = font->max_bounds.width;
    text_size.x = 0;
    return 0;
}

void DisplayDeviceX11::freeFont() {
    if (font)
        XFreeFont(xcth_display, font);
    font = NULL;
}

DisplayDeviceX11::DisplayDeviceX11(Scene& scene_, ImageOption& images_,
    SceneVisualSelections* sceneVisualSelections_,
    RuntimeCommandSink& runtimeCommands_,
    RuntimeCommandTargetRouter& runtimeCommandRouter_,
    RuntimeConfigRegistry& runtimeConfigRegistry_, const DisplayConfig& config,
    SecondsClock& clock_)
    : DisplayDevice()
    , scene(scene_)
    , images(images_)
    , sceneVisualSelections(sceneVisualSelections_)
    , runtimeCommands(runtimeCommands_)
    , runtimeCommandRouter(runtimeCommandRouter_)
    , runtimeConfigRegistry(runtimeConfigRegistry_)
    , clock(clock_)
    ,

    visual(NULL)
    , screen(0)
    , planes(0)
    , gc(NULL)
    , window(0)
    ,

    colormap(0)
    , rev_byte_order(0)
    , ssWorkaround(0)
    ,

    panelTextWidget(NULL)
    , panelMenuButtons()
    , palettePreviewWidget(NULL)
    , paletteNameTextWidget(NULL)
    , paletteSetTextWidget(NULL)
    , paletteEnergyTextWidget(NULL)
    , paletteMetadataStatusWidget(NULL)
    , textPixmap(None)
    , palettePreviewPixmap(None)
    , palettePreviewPalette(-1)
    , palettePreviewWidth(0)
    , palettePreviewHeight(0)
    , palettePreviewFingerprintValid(0)
    , palettePreviewCurrentFingerprint(0)
    , palettePreviewTargetFingerprint(0)
    , panelMenuLabels()
    , panelPendingTextCommands()
    , panelCommittedTextCommands()
    , panelPendingTextSignature()
    , panelCommittedTextSignature()
    , panelSelectionSignature()
    , panelTextDirty(1)
    , panelTextCopyX(0)
    , panelTextCopyY(0)
    , panelTextCopyWidth(0)
    , panelTextCopyHeight(0)
    , currentInputSink(NULL)
    , changeKeyButtonData()
    , shmAttached(0)
    , shmMarkedForRemoval(0)
    , pixmap(None)
    , image(NULL)
    , copyText(0)
    , paletteInitialized(0)
    , initialized(0)
    , presentationViewport()
    , fallbackIndexedFrameSize(2 * config.bufferWidth,
          2 * config.bufferHeight) {

    CTH_INFO("Initializing X11 display...\n");
    changeKeyButtonData.device = this;
    changeKeyButtonData.keyText = " ";
    memset(&shminfo, 0, sizeof(shminfo));
    shminfo.shmid = -1;

    // The root window and the control panel both require the default colormap.
    if (display_on_root)
        private_cmap = 0;

    if (xcth_panel && private_cmap)
        private_cmap = 0;

    unsigned int byte_order_test = 0x04030201;
    rev_byte_order = (ImageByteOrder(xcth_display) == MSBFirst) ? 1 : 0;
    if (*(char*)&byte_order_test == 4)
        rev_byte_order = !rev_byte_order;

    checkDisplaySize(config);

    xcth_root = DefaultRootWindow(xcth_display);

    if (display_on_root) {
        window = xcth_root;
        getAttributes();

        disp_size.x = DisplayWidth(xcth_display, screen);
        disp_size.y = DisplayHeight(xcth_display, screen);

        if (allocImage() || initPalette())
            return;
    } else {
        if (CreateWindow("Cthugha", full_screen))
            return;
    }

    gc = XCreateGC(xcth_display, window, 0, 0);

    if (loadFont())
        return;

    if (xcth_panel) {
        xcth_create_panel();
        XSelectInput(xcth_display, XtWindow(xcth_toplevel), KeyReleaseMask | StructureNotifyMask);

        // Text for the panel is rendered off-screen, then copied into the
        // widget window. This avoids redrawing the panel one string at a time.
        if ((textPixmap = XCreatePixmap(xcth_display, XtWindow(panelTextWidget),
                 text_size.x * fontSize.x, text_size.y * fontSize.y, planes))
            == 0) {
            CTH_ERROR("Can not create the text pixmap.\n");
            return;
        }
        markPanelTextCopyRect(0, 0, text_size.x * fontSize.x,
            text_size.y * fontSize.y, 2);
    } else {
        copyText = 0;
    }
    initialized = 1;
}

DisplayDeviceX11::~DisplayDeviceX11() {
    if (palettePreviewPixmap != None) {
        XFreePixmap(xcth_display, palettePreviewPixmap);
        palettePreviewPixmap = None;
    }
    if (paletteInitialized)
        freePalette();
    freeFont();
    freeImage();
}

DisplayEventStats DisplayDeviceX11::processEvents(InputEventSink& input) {
    DisplayEventStats stats;
    currentInputSink = &input;

    // Xt queues X events for both the raw display window and the optional
    // Athena-widget panel. Key releases are translated into Cthugha keys;
    // everything else is dispatched back through Xt.
    while (XtAppPending(xcth_app_con)) {
        XEvent event;
        XtAppNextEvent(xcth_app_con, &event);
        stats.eventCount++;
        if (event.type == KeyRelease) {
            char key_buff[256];
            int count;
            KeySym ks;
            XKeyEvent* kevent = (XKeyEvent*)&event;

            count = XLookupString(kevent, key_buff, 256, &ks, NULL);
            key_buff[count] = '\0';
            if (count == 0) {
                char* tmp = XKeysymToString(ks);
                strncpy(key_buff, tmp ? tmp : "", 256);
            }

            input.pushRawKey(key_buff, (kevent->state & ShiftMask) != 0);

        } else if ((event.type == ConfigureNotify)
            && (event.xconfigure.window == window)) {
            stats.resizeEvents++;
            resizeDisplay(event.xconfigure.width, event.xconfigure.height);
        } else if ((event.type == Expose) && (event.xexpose.window == window)) {
            stats.exposeEvents++;
            needsFullCopy = 1;
        } else {
            XtDispatchEvent(&event);
        }
    }

    currentInputSink = NULL;
    return stats;
}

void DisplayDeviceX11::prePrint() {
    if (panelTextWidget) {
        panelPendingTextCommands.clear();
        panelPendingTextSignature.clear();
    }

    // Text is composited through textPixmap when the frame buffer cannot be
    // drawn to directly. For panel text, the pixmap starts empty; for overlay
    // text, it starts as a copy of the current frame so strings can be drawn
    // over it before one final XCopyArea to the window.
    if (copyText && textPixmap) {
        if (!panelTextWidget) {
            switch (shmLevel) {
            case shmPixmap:
                break;
            case shmImage:
                XShmPutImage(
                    xcth_display, textPixmap, gc, image, 0, 0, 0, 0, disp_size.x, disp_size.y, 0);
                break;
            case shmNone:
                XPutImage(
                    xcth_display, textPixmap, gc, image, 0, 0, 0, 0, disp_size.x, disp_size.y);
            }
        }
    }

    textOnScreen = 0;
    darkenPalette = 0;
}

static void appendPanelTextSignature(
    std::string& signature, int x, int y, int color, int noDarken,
    const char* text, int len) {
    char header[96];
    snprintf(header, sizeof(header), "%d,%d,%d,%d,%d:", x, y, color,
        noDarken, len);
    signature.append(header);
    signature.append(text, len);
    signature.push_back('\n');
}

int DisplayDeviceX11::samePanelTextCommand(const PanelTextCommand& a,
    const PanelTextCommand& b) {
    return (a.x == b.x) && (a.y == b.y) && (a.color == b.color)
        && (a.noDarken == b.noDarken) && (a.text == b.text);
}

void DisplayDeviceX11::panelTextCommandBounds(
    const PanelTextCommand& command,
    int* x, int* y, int* width, int* height) {
    *x = command.x;
    *y = command.y;
    *width = int(command.text.size()) * fontSize.x;
    if (*width < 1)
        *width = 1;
    *height = fontSize.y + 1;
}

static int rectIntersects(int ax, int ay, int aw, int ah,
    int bx, int by, int bw, int bh) {
    return (aw > 0) && (ah > 0) && (bw > 0) && (bh > 0)
        && (ax < bx + bw) && (bx < ax + aw)
        && (ay < by + bh) && (by < ay + ah);
}

static void includeRect(int* x, int* y, int* width, int* height,
    int nextX, int nextY, int nextWidth, int nextHeight) {
    if ((nextWidth <= 0) || (nextHeight <= 0))
        return;

    if ((*width <= 0) || (*height <= 0)) {
        *x = nextX;
        *y = nextY;
        *width = nextWidth;
        *height = nextHeight;
        return;
    }

    int right = *x + *width;
    int bottom = *y + *height;
    int nextRight = nextX + nextWidth;
    int nextBottom = nextY + nextHeight;

    if (nextX < *x)
        *x = nextX;
    if (nextY < *y)
        *y = nextY;
    if (nextRight > right)
        right = nextRight;
    if (nextBottom > bottom)
        bottom = nextBottom;

    *width = right - *x;
    *height = bottom - *y;
}

void DisplayDeviceX11::markPanelTextCopyRect(
    int x, int y, int width, int height, int copyCount) {
    if (!panelTextWidget || !textPixmap)
        return;

    int panelWidth = text_size.x * fontSize.x;
    int panelHeight = text_size.y * fontSize.y;
    if ((panelWidth <= 0) || (panelHeight <= 0))
        return;

    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (x + width > panelWidth)
        width = panelWidth - x;
    if (y + height > panelHeight)
        height = panelHeight - y;
    if ((width <= 0) || (height <= 0))
        return;

    includeRect(&panelTextCopyX, &panelTextCopyY, &panelTextCopyWidth,
        &panelTextCopyHeight, x, y, width, height);
    if (copyText < copyCount)
        copyText = copyCount;
}

void DisplayDeviceX11::printString(
    int x, int y, const char* text, int color, int len, int noDarken) {

    if (panelTextWidget) {
        PanelTextCommand command;
        command.x = x;
        command.y = y;
        command.color = color;
        command.noDarken = noDarken;
        command.text.assign(text, len);
        panelPendingTextCommands.push_back(command);
        appendPanelTextSignature(panelPendingTextSignature, x, y, color,
            noDarken, text, len);
        return;
    }

    XSetForeground(xcth_display, gc, textColor[color]);

    if (textPixmap) {
        XDrawString(xcth_display, textPixmap, gc, x, y + fontSize.y, text, len);

        copyText = 2;
    } else
        XDrawString(xcth_display, pixmap, gc, x, y + fontSize.y, text, len);

    if (!panelTextWidget) {
        textOnScreen = 1;
        if (!noDarken)
            darkenPalette = 1;
    }
}

void DisplayDeviceX11::postPrint() {
    if (!panelTextWidget || !textPixmap)
        return;

    int textChanged = panelPendingTextSignature != panelCommittedTextSignature;
    if (!panelTextDirty && !textChanged)
        return;

    int dirtyX = 0;
    int dirtyY = 0;
    int dirtyWidth = 0;
    int dirtyHeight = 0;
    int panelWidth = text_size.x * fontSize.x;
    int panelHeight = text_size.y * fontSize.y;

    if (panelTextDirty) {
        dirtyWidth = panelWidth;
        dirtyHeight = panelHeight;
    } else {
        size_t count = panelPendingTextCommands.size();
        if (panelCommittedTextCommands.size() > count)
            count = panelCommittedTextCommands.size();

        for (size_t i = 0; i < count; i++) {
            const PanelTextCommand* pending
                = (i < panelPendingTextCommands.size())
                ? &panelPendingTextCommands[i] : NULL;
            const PanelTextCommand* committed
                = (i < panelCommittedTextCommands.size())
                ? &panelCommittedTextCommands[i] : NULL;
            if ((pending != NULL) && (committed != NULL)
                && samePanelTextCommand(*pending, *committed))
                continue;

            int x;
            int y;
            int width;
            int height;
            if (committed != NULL) {
                panelTextCommandBounds(*committed, &x, &y, &width, &height);
                includeRect(&dirtyX, &dirtyY, &dirtyWidth, &dirtyHeight,
                    x, y, width, height);
            }
            if (pending != NULL) {
                panelTextCommandBounds(*pending, &x, &y, &width, &height);
                includeRect(&dirtyX, &dirtyY, &dirtyWidth, &dirtyHeight,
                    x, y, width, height);
            }
        }
    }

    if ((dirtyWidth <= 0) || (dirtyHeight <= 0)) {
        panelCommittedTextCommands = panelPendingTextCommands;
        panelCommittedTextSignature = panelPendingTextSignature;
        panelTextDirty = 0;
        return;
    }

    XSetForeground(xcth_display, gc, 0);
    XFillRectangle(xcth_display, textPixmap, gc, dirtyX, dirtyY,
        dirtyWidth, dirtyHeight);

    for (size_t i = 0; i < panelPendingTextCommands.size(); i++) {
        const PanelTextCommand& command = panelPendingTextCommands[i];
        int commandX;
        int commandY;
        int commandWidth;
        int commandHeight;
        panelTextCommandBounds(command, &commandX, &commandY, &commandWidth,
            &commandHeight);
        if (!rectIntersects(dirtyX, dirtyY, dirtyWidth, dirtyHeight,
                commandX, commandY, commandWidth, commandHeight))
            continue;

        XSetForeground(xcth_display, gc, textColor[command.color]);
        XDrawString(xcth_display, textPixmap, gc, command.x,
            command.y + fontSize.y, command.text.c_str(),
            int(command.text.size()));
    }

    panelCommittedTextCommands = panelPendingTextCommands;
    panelCommittedTextSignature = panelPendingTextSignature;
    panelTextDirty = 0;
    markPanelTextCopyRect(dirtyX, dirtyY, dirtyWidth, dirtyHeight, 2);
}

int DisplayDeviceX11::allocImage() {

    // MIT-SHM lets the client and X server share the frame buffer memory.
    // A shared pixmap is fastest because XCopyArea can present it directly;
    // a shared image still avoids copying data through the X socket.
    if (display_mit_shm) {
        int d1, d2;
        int pix;
        Status mit_shm = XShmQueryVersion(xcth_display, &d1, &d2, &pix);

        if (pix && (XShmPixmapFormat(xcth_display) != ZPixmap))
            pix = 0;

        if (mit_shm && pix)
            shmLevel = shmPixmap;
        else if (mit_shm)
            shmLevel = shmImage;
        else
            shmLevel = shmNone;
    } else
        shmLevel = shmNone;

    switch (shmLevel) {
    case shmPixmap:
        CTH_DEBUG("    using shared pixmap\n");

        // Ask Xlib for the server's exact image layout before allocating the
        // shared segment that will back both the XImage and the shared pixmap.
        if ((image = XShmCreateImage(
                 xcth_display, visual, planes, ZPixmap, NULL, &shminfo, disp_size.x, disp_size.y))
            == NULL) {
            if (fallbackToNonSharedImage("Can not create the shared XImage", 0, 0))
                return 1;
            break;
        }
        bypp = (image->bits_per_pixel + 7) / 8;
        bytes_per_line = image->bytes_per_line;

        if ((shminfo.shmid = shmget(IPC_PRIVATE, bytes_per_line * disp_size.y, IPC_CREAT | 0777))
            == -1) {
            if (fallbackToNonSharedImage(
                    "Can not create shared memory segment", errno, bytes_per_line * disp_size.y))
                return 1;
            break;
        }
        if ((shminfo.shmaddr = image->data = (char*)shmat(shminfo.shmid, 0, 0)) == (void*)-1) {
            int savedErrno = errno;
            image->data = NULL;
            if (fallbackToNonSharedImage(
                    "Can not attach shared memory segment", savedErrno, bytes_per_line * disp_size.y))
                return 1;
            break;
        }
        shminfo.readOnly = False;
        if (XShmAttach(xcth_display, &shminfo) == 0) {
            if (fallbackToNonSharedImage(
                    "Can not X-attach shared memory segment", 0, bytes_per_line * disp_size.y))
                return 1;
            break;
        }
        shmAttached = 1;
        XSync(xcth_display, False);
        markSharedMemoryForRemoval();

        if ((pixmap = XShmCreatePixmap(
                 xcth_display, window, image->data, &shminfo, disp_size.x, disp_size.y, planes))
            == 0) {
            if (fallbackToNonSharedImage(
                    "Can not create the shared XPixmap", 0, bytes_per_line * disp_size.y))
                return 1;
            break;
        }
        break;

    case shmImage:
        CTH_DEBUG("    using shared image\n");

        if ((image = XShmCreateImage(xcth_display, visual, planes, ZPixmap,
                 NULL,
                 &shminfo, disp_size.x, disp_size.y))
            == NULL) {
            if (fallbackToNonSharedImage("Can not create shared XImage", 0, 0))
                return 1;
            break;
        }
        bypp = (image->bits_per_pixel + 7) / 8;
        bytes_per_line = image->bytes_per_line;

        if ((shminfo.shmid = shmget(IPC_PRIVATE, bytes_per_line * disp_size.y, IPC_CREAT | 0777))
            == -1) {
            if (fallbackToNonSharedImage(
                    "Can not create shared memory segment", errno, bytes_per_line * disp_size.y))
                return 1;
            break;
        }
        if ((shminfo.shmaddr = image->data = (char*)shmat(shminfo.shmid, 0, 0)) == (void*)-1) {
            int savedErrno = errno;
            image->data = NULL;
            if (fallbackToNonSharedImage(
                    "Can not attach shared memory segment", savedErrno, bytes_per_line * disp_size.y))
                return 1;
            break;
        }
        shminfo.readOnly = False;

        if (XShmAttach(xcth_display, &shminfo) == 0) {
            if (fallbackToNonSharedImage(
                    "Can not X-attach shared memory segment", 0, bytes_per_line * disp_size.y))
                return 1;
            break;
        }
        shmAttached = 1;
        XSync(xcth_display, False);
        markSharedMemoryForRemoval();
        break;

    case shmNone:
        if (allocNonSharedImage())
            return 1;
        break;
    }

    if ((shmLevel != shmPixmap) && !xcth_panel) {
        textPixmap = XCreatePixmap(xcth_display, window, disp_size.x, disp_size.y, planes);
        text_size.x = disp_size.x / fontSize.x;
        text_size.y = disp_size.y / fontSize.y;
    }

    CTH_DEBUG("    bytes/pixel        : %d\n", bypp);
    CTH_DEBUG("    bytes/line         : %d\n", bytes_per_line);
    return 0;
}

int DisplayDeviceX11::allocNonSharedImage() {
    shmLevel = shmNone;
    CTH_DEBUG("    using no shared image; staging through server pixmap.\n");

    if ((image = XCreateImage(xcth_display, visual, planes, ZPixmap, 0, NULL, disp_size.x,
             disp_size.y, XBitmapPad(xcth_display), 0))
        == NULL) {
        CTH_ERROR("Can not create XImage.\n");
        return 1;
    }
    bypp = (image->bits_per_pixel + 7) / 8;
    bytes_per_line = image->bytes_per_line;

    if ((image->data = (char*)malloc(disp_size.y * bytes_per_line)) == NULL) {
        CTH_ERROR("Can not allocate memory for bitmap.\n");
        XDestroyImage(image);
        image = NULL;
        return 1;
    }

    if ((pixmap = XCreatePixmap(xcth_display, window, disp_size.x, disp_size.y, planes)) == 0) {
        CTH_ERROR("Can not create the staging XPixmap.\n");
        freeImage();
        return 1;
    }

    return 0;
}

int DisplayDeviceX11::fallbackToNonSharedImage(
    const char* reason, int errnum, size_t requestedBytes) {
    if (errnum != 0 && requestedBytes > 0) {
        CTH_WARN("%s (%d - %s); requested %lu bytes, falling back to non-SHM XImage.\n",
            reason, errnum, strerror(errnum), (unsigned long)requestedBytes);
    } else if (errnum != 0) {
        CTH_WARN("%s (%d - %s); falling back to non-SHM XImage.\n",
            reason, errnum, strerror(errnum));
    } else if (requestedBytes > 0) {
        CTH_WARN("%s; requested %lu bytes, falling back to non-SHM XImage.\n",
            reason, (unsigned long)requestedBytes);
    } else {
        CTH_WARN("%s; falling back to non-SHM XImage.\n", reason);
    }

    freeImage();
    return allocNonSharedImage();
}

void DisplayDeviceX11::markSharedMemoryForRemoval() {
    if (shminfo.shmid == -1 || shmMarkedForRemoval)
        return;

    if (shmctl(shminfo.shmid, IPC_RMID, 0) == -1) {
        CTH_ERRNO(errno, "Can not mark shared memory segment for removal");
    } else {
        shmMarkedForRemoval = 1;
    }
}

void DisplayDeviceX11::freeImage() {

    XSync(xcth_display, True);

    if (textPixmap && (shmLevel != shmPixmap) && !xcth_panel) {
        XFreePixmap(xcth_display, textPixmap);
        textPixmap = 0;
    }

    if (image) {
        switch (shmLevel) {
        case shmPixmap:
            if (shmAttached)
                XShmDetach(xcth_display, &shminfo);
            XDestroyImage(image);
            if (pixmap) {
                XFreePixmap(xcth_display, pixmap);
                pixmap = 0;
            }
            if (shminfo.shmaddr != 0 && shminfo.shmaddr != (char*)-1)
                shmdt(shminfo.shmaddr);
            if (shminfo.shmid != -1 && !shmMarkedForRemoval)
                shmctl(shminfo.shmid, IPC_RMID, 0);
            break;
        case shmImage:
            if (shmAttached)
                XShmDetach(xcth_display, &shminfo);
            XDestroyImage(image);
            if (shminfo.shmaddr != 0 && shminfo.shmaddr != (char*)-1)
                shmdt(shminfo.shmaddr);
            if (shminfo.shmid != -1 && !shmMarkedForRemoval)
                shmctl(shminfo.shmid, IPC_RMID, 0);
            break;
        case shmNone:
            if (pixmap) {
                XFreePixmap(xcth_display, pixmap);
                pixmap = 0;
            }
            XDestroyImage(image);
        }
    }
    image = NULL;
    shmAttached = 0;
    shmMarkedForRemoval = 0;
    shminfo.shmid = -1;
    shminfo.shmaddr = 0;
}

int DisplayDeviceX11::indexedDisplayWidth() const {
    if (presentationViewport.valid())
        return presentationViewport.frameSize.width;

    return fallbackIndexedFrameSize.width;
}

int DisplayDeviceX11::indexedDisplayHeight() const {
    if (presentationViewport.valid())
        return presentationViewport.frameSize.height;

    return fallbackIndexedFrameSize.height;
}

void DisplayDeviceX11::setPresentationViewport(const DisplayViewport& viewport) {
    presentationViewport = viewport;
}

const DisplayViewport& DisplayDeviceX11::currentPresentationViewport() const {
    return presentationViewport;
}

void DisplayDeviceX11::resizeDisplay(int new_width, int new_height) {
    if ((new_width == disp_size.x) && (new_height == disp_size.y))
        return;

    disp_size.x = max(new_width, indexedDisplayWidth());
    disp_size.y = max(new_height, indexedDisplayHeight());

    if (!panelTextWidget) {
        text_size.x = disp_size.x / fontSize.x;
        text_size.y = disp_size.y / fontSize.y;
    }

    freeImage();
    allocImage();

}

unsigned char* DisplayDeviceX11::preDraw() {
    if ((disp_size.x < indexedDisplayWidth())
        || (disp_size.y < indexedDisplayHeight())) {
        resizeDisplay(indexedDisplayWidth(), indexedDisplayHeight());
    }

    return (unsigned char*)image->data;
}
void DisplayDeviceX11::copyBox(int x, int y, int width, int height, int destx, int desty) {
    char* src = image->data + y * bytes_per_line + x * bypp;
    char* dst = image->data + desty * bytes_per_line + destx * bypp;

    for (int i = 0; i < height; i++, src += bytes_per_line, dst += bytes_per_line)
        memcpy(dst, src, width * bypp);
}

void DisplayDeviceX11::clearBox(int x, int y, int width, int height) {
    char* dst = image->data + y * bytes_per_line + x * bypp;

    for (int i = 0; i < height; i++, dst += bytes_per_line)
        memset(dst, 0, bypp * width);
}

void DisplayDeviceX11::postDraw() {
    int traceDisplayTiming = CTH_LOG_ENABLED(CTH_LOG_TRACE);
    double postStart = traceDisplayTiming ? clock.nowSeconds() : 0.0;
    double paletteMs = 0.0;
    double dumpMs = 0.0;
    double previewMs = 0.0;
    double labelMs = 0.0;
    double putMs = 0.0;
    double copyMs = 0.0;
    int fullCopy = needsFullCopy;
    int copyTextFrame = copyText ? 1 : 0;
    double stepStart = 0.0;
    DisplayViewport viewport = currentPresentationViewport();
    PixelRect fullRect = ViewportPresentation::fullCopyRect(viewport);
    PixelRect drawRect = ViewportPresentation::drawCopyRect(viewport);

    if (traceDisplayTiming)
        stepStart = clock.nowSeconds();
    this->setGlobalPalette();
    if (traceDisplayTiming)
        paletteMs = (clock.nowSeconds() - stepStart) * 1000.0;

    if (traceDisplayTiming)
        stepStart = clock.nowSeconds();
    dump_x11_frame(image);
    if (traceDisplayTiming)
        dumpMs = (clock.nowSeconds() - stepStart) * 1000.0;

    if (traceDisplayTiming)
        stepStart = clock.nowSeconds();
    if (palettePreviewWidget)
        updatePalettePreview();
    if (traceDisplayTiming)
        previewMs = (clock.nowSeconds() - stepStart) * 1000.0;
    if (traceDisplayTiming)
        stepStart = clock.nowSeconds();
    if (panelTextWidget)
        updatePanelSelectionLabels();
    if (traceDisplayTiming)
        labelMs = (clock.nowSeconds() - stepStart) * 1000.0;

    if (copyText) {

        if (panelTextWidget) {
            if ((panelTextCopyWidth > 0) && (panelTextCopyHeight > 0)) {
                if (traceDisplayTiming)
                    stepStart = clock.nowSeconds();
                XCopyArea(xcth_display, textPixmap, XtWindow(panelTextWidget), gc,
                    panelTextCopyX, panelTextCopyY, panelTextCopyWidth,
                    panelTextCopyHeight, panelTextCopyX, panelTextCopyY);
                if (traceDisplayTiming)
                    copyMs += (clock.nowSeconds() - stepStart) * 1000.0;
            }
            copyText--;
            if (copyText <= 0) {
                panelTextCopyX = 0;
                panelTextCopyY = 0;
                panelTextCopyWidth = 0;
                panelTextCopyHeight = 0;
            }
        } else {
            switch (shmLevel) {
            case shmPixmap:
                if (traceDisplayTiming)
                    stepStart = clock.nowSeconds();
                XCopyArea(xcth_display, pixmap, window, gc, fullRect.x, fullRect.y,
                    fullRect.width, fullRect.height,
                    fullRect.x, fullRect.y);
                if (traceDisplayTiming)
                    copyMs += (clock.nowSeconds() - stepStart) * 1000.0;

                break;
            case shmImage:
            case shmNone:
                if (traceDisplayTiming)
                    stepStart = clock.nowSeconds();
                XCopyArea(xcth_display, textPixmap, window, gc, fullRect.x, fullRect.y,
                    fullRect.width, fullRect.height,
                    fullRect.x, fullRect.y);
                if (traceDisplayTiming)
                    copyMs += (clock.nowSeconds() - stepStart) * 1000.0;
            }
            {
                double flushStart = clock.nowSeconds();
                XFlush(xcth_display);
                double flushMs = (clock.nowSeconds() - flushStart) * 1000.0;
                CTH_TRACE("post-draw-ms=%.3f palette-ms=%.3f dump-ms=%.3f preview-ms=%.3f label-ms=%.3f put-ms=%.3f copy-ms=%.3f flush-ms=%.3f copy-text=1 full-copy=%d shm-level=%d draw=%dx%d\n",
                    "display timing", (clock.nowSeconds() - postStart) * 1000.0,
                    paletteMs, dumpMs, previewMs, labelMs, putMs, copyMs, flushMs,
                    fullCopy, shmLevel, viewport.drawSize.width,
                    viewport.drawSize.height);
            }
            return;
        }
    }

    if (needsFullCopy)
        switch (shmLevel) {
        case shmPixmap:
            if (traceDisplayTiming)
                stepStart = clock.nowSeconds();
            XCopyArea(xcth_display, pixmap, window, gc, fullRect.x, fullRect.y,
                fullRect.width, fullRect.height,
                fullRect.x, fullRect.y);
            if (traceDisplayTiming)
                copyMs += (clock.nowSeconds() - stepStart) * 1000.0;

            break;
        case shmImage:
            if (traceDisplayTiming)
                stepStart = clock.nowSeconds();
            XShmPutImage(xcth_display, window, gc, image, fullRect.x, fullRect.y,
                fullRect.x, fullRect.y, fullRect.width, fullRect.height, 0);
            if (traceDisplayTiming)
                putMs += (clock.nowSeconds() - stepStart) * 1000.0;
            break;
        case shmNone:
            if (traceDisplayTiming)
                stepStart = clock.nowSeconds();
            XPutImage(xcth_display, pixmap, gc, image, fullRect.x, fullRect.y,
                fullRect.x, fullRect.y, fullRect.width, fullRect.height);
            if (traceDisplayTiming)
                putMs += (clock.nowSeconds() - stepStart) * 1000.0;
            if (traceDisplayTiming)
                stepStart = clock.nowSeconds();
            XCopyArea(xcth_display, pixmap, window, gc, fullRect.x, fullRect.y,
                fullRect.width, fullRect.height,
                fullRect.x, fullRect.y);
            if (traceDisplayTiming)
                copyMs += (clock.nowSeconds() - stepStart) * 1000.0;
            break;
        }
    else
        switch (shmLevel) {
        case shmPixmap:
            if (traceDisplayTiming)
                stepStart = clock.nowSeconds();
            XCopyArea(xcth_display, pixmap, window, gc, drawRect.x,
                drawRect.y,
                drawRect.width, drawRect.height,
                drawRect.x,
                drawRect.y);
            if (traceDisplayTiming)
                copyMs += (clock.nowSeconds() - stepStart) * 1000.0;
            break;
        case shmImage:
            if (traceDisplayTiming)
                stepStart = clock.nowSeconds();
            XShmPutImage(xcth_display, window, gc, image, drawRect.x, drawRect.y,
                drawRect.x, drawRect.y, drawRect.width, drawRect.height, 0);
            if (traceDisplayTiming)
                putMs += (clock.nowSeconds() - stepStart) * 1000.0;
            break;
        case shmNone:
            if (traceDisplayTiming)
                stepStart = clock.nowSeconds();
            XPutImage(xcth_display, pixmap, gc, image, drawRect.x, drawRect.y,
                drawRect.x, drawRect.y, drawRect.width, drawRect.height);
            if (traceDisplayTiming)
                putMs += (clock.nowSeconds() - stepStart) * 1000.0;
            if (traceDisplayTiming)
                stepStart = clock.nowSeconds();
            XCopyArea(xcth_display, pixmap, window, gc, drawRect.x, drawRect.y,
                drawRect.width, drawRect.height,
                drawRect.x, drawRect.y);
            if (traceDisplayTiming)
                copyMs += (clock.nowSeconds() - stepStart) * 1000.0;
            break;
        }

    double flushStart = clock.nowSeconds();
    XFlush(xcth_display);
    double flushMs = (clock.nowSeconds() - flushStart) * 1000.0;
    CTH_TRACE("post-draw-ms=%.3f palette-ms=%.3f dump-ms=%.3f preview-ms=%.3f label-ms=%.3f put-ms=%.3f copy-ms=%.3f flush-ms=%.3f copy-text=%d full-copy=%d shm-level=%d draw=%dx%d\n",
        "display timing", (clock.nowSeconds() - postStart) * 1000.0,
        paletteMs, dumpMs, previewMs, labelMs, putMs, copyMs, flushMs, copyTextFrame,
        fullCopy, shmLevel, viewport.drawSize.width, viewport.drawSize.height);
    needsFullCopy = 0;
}

int DisplayDeviceX11::initPalette() {

    switch (visual->c_class) {

    case TrueColor:
    case DirectColor: {
        colormapped = 0;

        red_mask = visual->red_mask;
        red_shift = ffs(red_mask & ~(red_mask >> 1)) - 8;
        green_mask = visual->green_mask;
        green_shift = ffs(green_mask & ~(green_mask >> 1)) - 8;
        blue_mask = visual->blue_mask;
        blue_shift = ffs(blue_mask & ~(blue_mask >> 1)) - 8;

        CTH_DEBUG("    red   mask/shift   : 0x%4x/%d\n", red_mask, red_shift);
        CTH_DEBUG("    green mask/shift   : 0x%4x/%d\n", green_mask, green_shift);
        CTH_DEBUG("    blue  mask/shift   : 0x%4x/%d\n", blue_mask, blue_shift);

        if (!mappedDrawModeForBytesPerPixel(bypp, draw_mode)) {
            CTH_ERROR("Unsupported bytes per pixel %d.", bypp);
            return 1;
        }

        for (int i = 0; i < textColors; i++) {
            textColor[i] = (red_mask & shift(textColorRGB[i][0], red_shift))
                | (green_mask & shift(textColorRGB[i][1], green_shift))
                | (blue_mask & shift(textColorRGB[i][2], blue_shift));
        }

        break;
    }
    case StaticGray:
    case StaticColor:
    case GrayScale:

        CTH_ERROR("Cthugha will propably not work on this visual.\n");

        colormapped = 0;
        draw_mode = DM_mapped1;
        red_mask = green_mask = blue_mask = 0xffff;
        red_shift = green_shift = blue_shift = 0;

        for (int i = 0; i < textColors; i++) {
            textColor[i] = 0xffff;
        }
        break;

    case PseudoColor: {
        if (planes != 8) {
            CTH_ERROR("xcthugha needs for PseudoColor 8 bits/pixel - you have %d.\n", planes);
        }

        if (private_cmap == 0) {
            unsigned long pixels[256];

            // In PseudoColor, pixels are indexes into a server-side colormap.
            // First try to reserve nearly the whole default colormap, then map
            // each Cthugha palette index to its allocated X pixel value.
            if (XAllocColorCells(xcth_display, colormap, 1, NULL, 0, pixels, 255)
                != 0) {
                CTH_INFO("Could allocate 255 color cells.\n");

                draw_mode = DM_mapped1;
                colormapped = 1;

                for (int i = 0; i < 255; i++) {
                    // Historical workaround: older builds reportedly crashed
                    // when assigning pixels[i] directly.
                    int a = 0;
                    colors[i].pixel = pixels[i] + a;
                }
                colors[255].pixel = colors[254].pixel;
                mapIndexedPixelsToColorCells(colors);

                for (int i = 0; i < textColors; i++) {
                    textColor[i] = colors[128 + i].pixel;
                    for (int j = 0; j < 3; j++) {
                        textPalette[128 + i][j] = textColorRGB[i][j];
                    }
                }

            } else {
                // Shared default colormaps are often partly occupied by other
                // clients. Fall back to 128 color cells and map pairs of
                // Cthugha palette entries onto each allocated X pixel.
                draw_mode = DM_mapped1;
                colormapped = 2;

                for (int i = 0; i < textColors; i++) {
                    XColor col;
                    col.pixel = i;
                    col.red = textColorRGB[i][0] << 8;
                    col.green = textColorRGB[i][1] << 8;
                    col.blue = textColorRGB[i][2] << 8;
                    col.flags = DoRed | DoGreen | DoBlue;
                    if (XAllocColor(xcth_display, colormap, &col) == 0) {
                        textColor[i] = 0;

                        CTH_ERROR("Could not allocate color for text.\n");
                    } else {
                        textColor[i] = col.pixel;
                    }
                }

                if (XAllocColorCells(xcth_display, colormap, 0, NULL, 0, pixels, 128)
                    == 0) {
                    CTH_ERROR("Could not allocate 128 color cells.\n"
                            "please make more colors available or start with '--install' option.");
                    return 1;
                }

                for (int i = 0; i < 128; i++)
                    colors[i].pixel = pixels[i];

                mapIndexedPixelsToPairedColorCells(colors);
            }
        } else {
            // A private colormap gives this window all 256 palette entries,
            // at the cost of color flashing when focus enters/leaves it on
            // old indexed-color X displays.
            draw_mode = DM_mapped1;
            colormap = XCreateColormap(xcth_display, window, visual, AllocAll);
            XSetWindowColormap(xcth_display, window, colormap);
            colormapped = 1;

            for (int i = 0; i < 256; i++)
                for (int j = 0; j < 3; j++)
                    textPalette[i][j] = 0;

            for (int i = 0; i < 256; i++) {
                colors[i].pixel = i;
            }
            mapIndexedPixelsToColorCells(colors);

            for (int i = 0; i < textColors; i++) {
                textColor[i] = colors[255 - i].pixel;

                // Historical workaround: keep these assignments explicit.
                textPalette[255 - i][0] = textColorRGB[i][0];
                textPalette[255 - i][1] = textColorRGB[i][1];
                textPalette[255 - i][2] = textColorRGB[i][2];
            }
        }
    }
    }
    paletteInitialized = 1;
    return 0;
}

void DisplayDeviceX11::freePalette() {
    if (colormapped)
        XFreeColormap(xcth_display, colormap);

    // Default-colormap cells are not owned through the colormap handle itself.
}

int DisplayDeviceX11::setGlobalPalette() {

    if (DisplayDevice::setGlobalPalette() == 0)
        return 0;
    if (framePalette == 0)
        return 0;

    const Palette& currentPalette = framePalette->currentPalette().raw();

    if (textOnScreen) {
        int i, j;

        switch (colormapped) {
        case 1:
            // Reserve half of the palette for text colors while text overlays
            // are visible. The image is remapped to 128 averaged entries so
            // text can keep stable, readable colors.
            if (darkenPalette) {
                for (i = 0; i < 128; i++)
                    for (j = 0; j < 3; j++)
                        textPalette[i][j]
                            = (currentPalette[i * 2][j]
                                  + currentPalette[i * 2 + 1][j])
                            >> 2;
            } else {
                for (i = 0; i < 128; i++)
                    for (j = 0; j < 3; j++)
                        textPalette[i][j]
                            = (currentPalette[i * 2][j]
                                  + currentPalette[i * 2 + 1][j])
                            >> 1;
            }

            mapIndexedPixelsToPairedColorCells(colors);

            setPalette(textPalette);
            break;
        case 2:
        default:

            if (darkenPalette) {
                for (i = 0; i < 256; i++)
                    for (j = 0; j < 3; j++)
                        textPalette[i][j] = currentPalette[i][j] >> 1;

                setPalette(textPalette);

            } else {
                setPalette(currentPalette);
            }
        }
    } else {

        if (colormapped == 1)
            mapIndexedPixelsToColorCells(colors);

        setPalette(currentPalette);
    }

    return 0;
}

void DisplayDeviceX11::setPalette(const Palette pal) {
    int i;

    switch (colormapped) {
    case 1:
        for (i = 0; i < 256; i++) {
            colors[i].red = pal[i][0] << 8;
            colors[i].green = pal[i][1] << 8;
            colors[i].blue = pal[i][2] << 8;
            colors[i].flags = DoRed | DoGreen | DoBlue;
        }
        XStoreColors(xcth_display, colormap, colors, 256);

        break;

    case 2:

        for (i = 0; i < 128; i++) {
            colors[i].red = (pal[2 * i][0] + pal[2 * i + 1][0]) << 7;
            colors[i].green = (pal[2 * i][1] + pal[2 * i + 1][1]) << 7;
            colors[i].blue = (pal[2 * i][2] + pal[2 * i + 1][2]) << 7;
            colors[i].flags = DoRed | DoGreen | DoBlue;
        }
        XStoreColors(xcth_display, colormap, colors, 128);

        break;

    default:
        // TrueColor/DirectColor visuals do not use writable palette entries.
        // Precompute the native pixel value for each Cthugha palette color so
        // the renderer can translate indexed pixels quickly while drawing.
        for (i = 0; i < 256; i++) {
            bitmap_colors0[i] = (red_mask & shift(pal[i][0], red_shift))
                | (green_mask & shift(pal[i][1], green_shift))
                | (blue_mask & shift(pal[i][2], blue_shift));
            if (rev_byte_order)
                bitmap_colors0[i] = switch_byte_order(bitmap_colors0[i]);

            switch (bypp) {
            case 1:
                bitmap_colors1[i] = bitmap_colors0[i] << 8;
                bitmap_colors2[i] = bitmap_colors1[i] << 8;
                bitmap_colors3[i] = bitmap_colors2[i] << 8;
                break;
            case 2:
                bitmap_colors1[i] = bitmap_colors0[i] << 16;
                break;
            }
        }
    }
}

class X11DisplayDriverFactory : public DisplayDriverFactory {
    DisplayFrontendInitializer& frontendInitializer;
    const X11Config& config;

public:
    explicit X11DisplayDriverFactory(
        DisplayFrontendInitializer& frontendInitializer_,
        const X11Config& config_)
        : frontendInitializer(frontendInitializer_)
        , config(config_) {
    }

    virtual DisplayDriverId driverId() const {
        return DisplayDriverX11;
    }

    virtual const char* driverName() const {
        return "x11";
    }

    virtual std::unique_ptr<DisplaySystemComponents> open(
        const DisplayOpenRequest& request) {
        configureDisplayDeviceX11(config);
        if (frontendInitializer.initializeDisplayFrontend(
                request.argc, request.argv))
            return std::unique_ptr<DisplaySystemComponents>();

        std::unique_ptr<DisplayDeviceX11> device(
            new DisplayDeviceX11(request.scene, request.images,
                request.sceneVisualSelections, request.runtimeCommands,
                request.runtimeCommandRouter, request.runtimeConfigRegistry,
                request.config, request.clock));
        if (!device->isInitialized())
            return std::unique_ptr<DisplaySystemComponents>();

        DisplayDeviceX11& deviceRef = *device;
        std::unique_ptr<DisplayBackend> backend(
            new DisplayBackendX11(deviceRef));
        std::unique_ptr<DisplayRuntime> runtime(new DisplayRuntime(*backend));
        std::unique_ptr<CthughaDisplay> coordinator = newCthughaDisplay(
            deviceRef, *runtime, request.clock, request.presentationSettings,
            request.interfaceRuntime, request.errorMessages);

        return std::unique_ptr<DisplaySystemComponents>(
            new DisplaySystemComponents(std::move(device),
                std::move(backend), std::move(runtime),
                std::move(coordinator)));
    }
};

std::unique_ptr<DisplayDriverFactory> newX11DisplayDriverFactory(
    DisplayFrontendInitializer& frontendInitializer,
    const X11Config& config) {
    return std::unique_ptr<DisplayDriverFactory>(
        new X11DisplayDriverFactory(frontendInitializer, config));
}
