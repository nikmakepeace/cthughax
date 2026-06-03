// X11 display backend.
// Owns the X Toolkit event loop, window or root-window target, XImage/XPixmap
// backing storage, optional MIT-SHM acceleration, text overlay rendering, and
// palette translation for both true-color and indexed-color X visuals.

#include "cthugha.h"
#include "DisplayDevice.h"
#include "FramePalette.h"
#include "Scene.h"
#include "cthugha.h"
#include "defaults.h"
#include "display.h"
#include "disp-sys.h"
#include "imath.h"
#include "xcthugha.h"
#include "keys.h"
#include "Interface.h"
#include "cth_buffer.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "DisplayBackend.h"
#include "DisplayRuntime.h"
#include "OverlaySource.h"
#include "PixelTransfer.h"
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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "vroot.h"

#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

static int indexedDisplayWidth() {
    // Normal presentation has already prepared CthughaDisplay's output frame.
    // Startup and legacy display calls can still fall back to the engine buffer.
    if (cthughaDisplay != NULL)
        return cthughaDisplay->displayFrameWidth();

    return 2 * CthughaBuffer::current->width();
}

static int indexedDisplayHeight() {
    if (cthughaDisplay != NULL)
        return cthughaDisplay->displayFrameHeight();

    return 2 * CthughaBuffer::current->height();
}

static int centeredOffset(int windowPixels, int drawPixels) {
    int offset = (windowPixels - drawPixels) / 2;
    return offset > 0 ? offset : 0;
}

static DisplayViewport currentDisplayViewport() {
    if ((cthughaDisplay != NULL) && cthughaDisplay->displayViewport().valid())
        return cthughaDisplay->displayViewport();

    DisplayViewport viewport;
    viewport.frameSize = PixelSize(indexedDisplayWidth(), indexedDisplayHeight());
    viewport.windowSize = PixelSize::fromXy(disp_size);
    viewport.drawSize = PixelSize::fromXy(draw_size);
    viewport.destination = PixelRect(
        centeredOffset(disp_size.x, draw_size.x),
        centeredOffset(disp_size.y, draw_size.y),
        draw_size.x, draw_size.y);
    return viewport;
}

static const unsigned long* nativePixelsForCurrentDrawMode() {
    if (draw_mode == DM_direct)
        return 0;

    return bitmap_colors0;
}

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

    virtual DisplayEventStats processEvents() {
        return device.processEvents();
    }

    virtual PixelSize outputSize() const {
        return PixelSize::fromXy(disp_size);
    }

    virtual void present(const DisplayPresentation& presentation) {
        if (presentation.framePalette != NULL)
            device.setFramePalette(presentation.framePalette);
        DisplayDevice& displayDeviceBase = device;
        displayDeviceBase.setGlobalPalette();
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
            bytes_per_line, bypp, nativePixelsForCurrentDrawMode());

        renderOverlayCommands(device, presentation.overlays);
    }
};

xy screenSizes[]
    = { xy(320, 200), xy(640, 480), xy(800, 600), xy(1024, 768), xy(1152, 864), xy(1280, 1024) };
int nScreenSizes = sizeof(screenSizes) / sizeof(xy);
xy bufferSizes[]
    = { xy(160, 100), xy(320, 240), xy(400, 300), xy(512, 384), xy(576, 450), xy(600, 512) };
int nBufferSizes = sizeof(bufferSizes) / sizeof(xy);

int display_override_redirect = DEFAULT_X11_OVERRIDE_REDIRECT; // bypass the window manager
int private_cmap = DEFAULT_X11_PRIVATE_CMAP; // allocate a window-private colormap
int display_mit_shm = DEFAULT_X11_MIT_SHM; // use MIT-SHM if possible
int display_on_root = DEFAULT_X11_ROOT_WINDOW; // display on root window
int full_screen = DEFAULT_X11_FULLSCREEN;
int window_do_pos = DEFAULT_X11_WINDOW_POSITION_ENABLED;
xy window_pos(DEFAULT_X11_WINDOW_POSITION_X, DEFAULT_X11_WINDOW_POSITION_Y);
int xcth_panel = DEFAULT_X11_PANEL_ENABLED; // use control panel
char xcth_font[256] = DEFAULT_X11_FONT_NAME;

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

static void dump_x11_frame(XImage* image) {
    static int initialized = 0;
    static int enabled = 0;
    static int frame = 0;
    static int dumped = 0;
    static int limit = 1;
    static int every = 1;
    static char directory[PATH_MAX];

    const char* env;

    if (!initialized) {
        initialized = 1;
        env = getenv("CTHUGHA_DUMP_X11_FRAMES");
        if (env && env[0]) {
            strncpy(directory, env, PATH_MAX - 1);
            directory[PATH_MAX - 1] = '\0';
            enabled = 1;

            env = getenv("CTHUGHA_DUMP_X11_FRAME_LIMIT");
            if (env && env[0])
                limit = max(1, atoi(env));

            env = getenv("CTHUGHA_DUMP_X11_FRAME_EVERY");
            if (env && env[0])
                every = max(1, atoi(env));

            if ((mkdir(directory, 0777) != 0) && (errno != EEXIST)) {
                CTH_ERRNO(errno, "Can not create X11 frame dump directory");
                enabled = 0;
            }
        }
    }

    if (!enabled || (image == NULL))
        return;

    frame++;
    if ((frame % every) != 0)
        return;
    if (dumped >= limit)
        return;

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/frame-%06d.ppm", directory, frame);

    FILE* out = fopen(path, "wb");
    if (out == NULL) {
        CTH_ERRNO(errno, "Can not create X11 frame dump");
        enabled = 0;
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
    dumped++;
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

void DisplayDeviceX11::checkDisplaySize() {

    if (display_mode != -1) {
        if ((display_mode >= nScreenSizes) || (display_mode < 0))
            display_mode = 0;

        disp_size = screenSizes[display_mode];
    }
}

int DisplayDeviceX11::loadFont() {
    if (!text_on_term) {
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
    } else
        font = NULL;
    return 0;
}

void DisplayDeviceX11::freeFont() {
    if (font)
        XFreeFont(xcth_display, font);
    font = NULL;
}

DisplayDeviceX11::DisplayDeviceX11(Scene& scene_, SceneCommands& sceneCommands_)
    : DisplayDevice()
    , scene(scene_)
    , sceneCommands(sceneCommands_)
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
    , shmAttached(0)
    , shmMarkedForRemoval(0)
    , pixmap(None)
    , image(NULL)
    , copyText(0)
    , paletteInitialized(0)
    , initialized(0) {

    CTH_INFO("Initializing X11 display...\n");
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

    checkDisplaySize();

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
        copyText = 2;
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

DisplayEventStats DisplayDeviceX11::processEvents() {
    DisplayEventStats stats;

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

            keys_x11(key_buff, kevent->state);

        } else if ((event.type == ConfigureNotify)
            && (event.xconfigure.window == window)) {
            stats.resizeEvents++;
            resizeDisplay(event.xconfigure.width, event.xconfigure.height);
        } else if ((event.type == Expose) && (event.xexpose.window == window)) {
            stats.exposeEvents++;
            if (cthughaDisplay != NULL)
                cthughaDisplay->needsClear = 1;
        } else {
            XtDispatchEvent(&event);
        }
    }

    return stats;
}

void DisplayDeviceX11::prePrint() {

    // Text is composited through textPixmap when the frame buffer cannot be
    // drawn to directly. For panel text, the pixmap starts empty; for overlay
    // text, it starts as a copy of the current frame so strings can be drawn
    // over it before one final XCopyArea to the window.
    if (copyText && textPixmap) {
        if (panelTextWidget) {
            XSetForeground(xcth_display, gc, 0);
            XFillRectangle(xcth_display, textPixmap, gc, 0, 0, text_size.x * fontSize.x,
                text_size.y * fontSize.y);
        } else {
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

void DisplayDeviceX11::printString(
    int x, int y, const char* text, int color, int len, int noDarken) {

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

    if ((shmLevel != shmPixmap) && !text_on_term && !xcth_panel) {
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

    if (textPixmap && (shmLevel != shmPixmap) && !text_on_term && !xcth_panel) {
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

void DisplayDeviceX11::resizeDisplay(int new_width, int new_height) {
    if ((new_width == disp_size.x) && (new_height == disp_size.y))
        return;

    disp_size.x = max(new_width, indexedDisplayWidth());
    disp_size.y = max(new_height, indexedDisplayHeight());

    if (!text_on_term && !panelTextWidget) {
        text_size.x = disp_size.x / fontSize.x;
        text_size.y = disp_size.y / fontSize.y;
    }

    freeImage();
    allocImage();

    // After resizing, the border around the image must be redrawn.
    if (cthughaDisplay != NULL)
        cthughaDisplay->needsClear = 1;
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
    double postStart = traceDisplayTiming ? getTime() : 0.0;
    double paletteMs = 0.0;
    double dumpMs = 0.0;
    double previewMs = 0.0;
    double putMs = 0.0;
    double copyMs = 0.0;
    int fullCopy = needsFullCopy;
    int copyTextFrame = copyText ? 1 : 0;
    double stepStart = 0.0;
    DisplayViewport viewport = currentDisplayViewport();
    PixelRect fullRect = ViewportPresentation::fullCopyRect(viewport);
    PixelRect drawRect = ViewportPresentation::drawCopyRect(viewport);

    if (traceDisplayTiming)
        stepStart = getTime();
    this->setGlobalPalette();
    if (traceDisplayTiming)
        paletteMs = (getTime() - stepStart) * 1000.0;

    if (traceDisplayTiming)
        stepStart = getTime();
    dump_x11_frame(image);
    if (traceDisplayTiming)
        dumpMs = (getTime() - stepStart) * 1000.0;

    if (traceDisplayTiming)
        stepStart = getTime();
    if (palettePreviewWidget)
        updatePalettePreview();
    if (traceDisplayTiming)
        previewMs = (getTime() - stepStart) * 1000.0;

    if (copyText) {

        if (panelTextWidget) {
            copyText--;
            if (traceDisplayTiming)
                stepStart = getTime();
            XCopyArea(xcth_display, textPixmap, XtWindow(panelTextWidget), gc, 0, 0,
                text_size.x * fontSize.x, text_size.y * fontSize.y, 0, 0);
            if (traceDisplayTiming)
                copyMs += (getTime() - stepStart) * 1000.0;
        } else {
            switch (shmLevel) {
            case shmPixmap:
                if (traceDisplayTiming)
                    stepStart = getTime();
                XCopyArea(xcth_display, pixmap, window, gc, fullRect.x, fullRect.y,
                    fullRect.width, fullRect.height,
                    fullRect.x, fullRect.y);
                if (traceDisplayTiming)
                    copyMs += (getTime() - stepStart) * 1000.0;

                break;
            case shmImage:
            case shmNone:
                if (traceDisplayTiming)
                    stepStart = getTime();
                XCopyArea(xcth_display, textPixmap, window, gc, fullRect.x, fullRect.y,
                    fullRect.width, fullRect.height,
                    fullRect.x, fullRect.y);
                if (traceDisplayTiming)
                    copyMs += (getTime() - stepStart) * 1000.0;
            }
            {
                double flushStart = getTime();
                XFlush(xcth_display);
                double flushMs = (getTime() - flushStart) * 1000.0;
                CTH_TRACE("post-draw-ms=%.3f palette-ms=%.3f dump-ms=%.3f preview-ms=%.3f put-ms=%.3f copy-ms=%.3f flush-ms=%.3f copy-text=1 full-copy=%d shm-level=%d draw=%dx%d\n",
                    "display timing", (getTime() - postStart) * 1000.0,
                    paletteMs, dumpMs, previewMs, putMs, copyMs, flushMs,
                    fullCopy, shmLevel, draw_size.x, draw_size.y);
            }
            return;
        }
    }

    if (needsFullCopy)
        switch (shmLevel) {
        case shmPixmap:
            if (traceDisplayTiming)
                stepStart = getTime();
            XCopyArea(xcth_display, pixmap, window, gc, fullRect.x, fullRect.y,
                fullRect.width, fullRect.height,
                fullRect.x, fullRect.y);
            if (traceDisplayTiming)
                copyMs += (getTime() - stepStart) * 1000.0;

            break;
        case shmImage:
            if (traceDisplayTiming)
                stepStart = getTime();
            XShmPutImage(xcth_display, window, gc, image, fullRect.x, fullRect.y,
                fullRect.x, fullRect.y, fullRect.width, fullRect.height, 0);
            if (traceDisplayTiming)
                putMs += (getTime() - stepStart) * 1000.0;
            break;
        case shmNone:
            if (traceDisplayTiming)
                stepStart = getTime();
            XPutImage(xcth_display, pixmap, gc, image, fullRect.x, fullRect.y,
                fullRect.x, fullRect.y, fullRect.width, fullRect.height);
            if (traceDisplayTiming)
                putMs += (getTime() - stepStart) * 1000.0;
            if (traceDisplayTiming)
                stepStart = getTime();
            XCopyArea(xcth_display, pixmap, window, gc, fullRect.x, fullRect.y,
                fullRect.width, fullRect.height,
                fullRect.x, fullRect.y);
            if (traceDisplayTiming)
                copyMs += (getTime() - stepStart) * 1000.0;
            break;
        }
    else
        switch (shmLevel) {
        case shmPixmap:
            if (traceDisplayTiming)
                stepStart = getTime();
            XCopyArea(xcth_display, pixmap, window, gc, drawRect.x,
                drawRect.y,
                drawRect.width, drawRect.height,
                drawRect.x,
                drawRect.y);
            if (traceDisplayTiming)
                copyMs += (getTime() - stepStart) * 1000.0;
            break;
        case shmImage:
            if (traceDisplayTiming)
                stepStart = getTime();
            XShmPutImage(xcth_display, window, gc, image, drawRect.x, drawRect.y,
                drawRect.x, drawRect.y, drawRect.width, drawRect.height, 0);
            if (traceDisplayTiming)
                putMs += (getTime() - stepStart) * 1000.0;
            break;
        case shmNone:
            if (traceDisplayTiming)
                stepStart = getTime();
            XPutImage(xcth_display, pixmap, gc, image, drawRect.x, drawRect.y,
                drawRect.x, drawRect.y, drawRect.width, drawRect.height);
            if (traceDisplayTiming)
                putMs += (getTime() - stepStart) * 1000.0;
            if (traceDisplayTiming)
                stepStart = getTime();
            XCopyArea(xcth_display, pixmap, window, gc, drawRect.x, drawRect.y,
                drawRect.width, drawRect.height,
                drawRect.x, drawRect.y);
            if (traceDisplayTiming)
                copyMs += (getTime() - stepStart) * 1000.0;
            break;
        }

    double flushStart = getTime();
    XFlush(xcth_display);
    double flushMs = (getTime() - flushStart) * 1000.0;
    CTH_TRACE("post-draw-ms=%.3f palette-ms=%.3f dump-ms=%.3f preview-ms=%.3f put-ms=%.3f copy-ms=%.3f flush-ms=%.3f copy-text=%d full-copy=%d shm-level=%d draw=%dx%d\n",
        "display timing", (getTime() - postStart) * 1000.0,
        paletteMs, dumpMs, previewMs, putMs, copyMs, flushMs, copyTextFrame,
        fullCopy, shmLevel, draw_size.x, draw_size.y);
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

        switch (bypp) {
        case 1:
            draw_mode = DM_mapped1;
            break;
        case 2:
            draw_mode = DM_mapped2;
            break;
        case 3:
            draw_mode = DM_mapped3;
            break;
        case 4:
            draw_mode = DM_mapped4;
            break;
        default:
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
            // First try to reserve nearly the whole default colormap so the
            // frame buffer bytes can be used directly as palette indexes.
            if (XAllocColorCells(xcth_display, colormap, 1, NULL, 0, pixels, 255)
                != 0) {
                CTH_INFO("Could allocate 255 color cells.\n");

                draw_mode = DM_direct;
                colormapped = 1;

                for (int i = 0; i < 255; i++) {
                    // Historical workaround: older builds reportedly crashed
                    // when assigning pixels[i] directly.
                    int a = 0;
                    colors[i].pixel = pixels[i] + a;
                }
                colors[255].pixel = colors[254].pixel;

                for (int i = 0; i < textColors; i++) {
                    textColor[i] = 128 + i;
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

                for (int i = 0; i < 256; i++) {
                    bitmap_colors0[i] = int(colors[i / 2].pixel);
                    bitmap_colors1[i] = bitmap_colors0[i] << 8;
                    bitmap_colors2[i] = bitmap_colors1[i] << 8;
                    bitmap_colors3[i] = bitmap_colors2[i] << 8;
                }
            }
        } else {
            // A private colormap gives this window all 256 palette entries,
            // at the cost of color flashing when focus enters/leaves it on
            // old indexed-color X displays.
            draw_mode = DM_direct;
            colormap = XCreateColormap(xcth_display, window, visual, AllocAll);
            XSetWindowColormap(xcth_display, window, colormap);
            colormapped = 1;

            for (int i = 0; i < 256; i++)
                for (int j = 0; j < 3; j++)
                    textPalette[i][j] = 0;

            for (int i = 0; i < textColors; i++) {
                textColor[i] = 255 - i;

                // Historical workaround: keep these assignments explicit.
                textPalette[255 - i][0] = textColorRGB[i][0];
                textPalette[255 - i][1] = textColorRGB[i][1];
                textPalette[255 - i][2] = textColorRGB[i][2];
            }

            for (int i = 0; i < 256; i++) {
                colors[i].pixel = i;
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

            if (draw_mode == DM_direct) {
                draw_mode = DM_tmp_mapped;

                for (i = 0; i < 256; i++) {
                    bitmap_colors0[i] = i / 2;
                    bitmap_colors1[i] = bitmap_colors0[i] << 8;
                    bitmap_colors2[i] = bitmap_colors1[i] << 8;
                    bitmap_colors3[i] = bitmap_colors2[i] << 8;
                }
            }

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

        if (draw_mode == DM_tmp_mapped) {
            draw_mode = DM_direct;

            for (int i = 0; i < 256; i++) {
                bitmap_colors0[i] = i;
                bitmap_colors1[i] = bitmap_colors0[i] << 8;
                bitmap_colors2[i] = bitmap_colors1[i] << 8;
                bitmap_colors3[i] = bitmap_colors2[i] << 8;
            }
        }

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

int newDisplayDevice(Scene& scene, SceneCommands& sceneCommands) {
    DisplayDeviceX11* device = new DisplayDeviceX11(scene, sceneCommands);
    if (!device->isInitialized()) {
        delete device;
        return 1;
    }

    displayDevice = device;
    displayBackend = new DisplayBackendX11(*device);
    displayRuntime = new DisplayRuntime(*displayBackend);
    return 0;
}

#if HAVE_XPM_H
#include <xpm.h>
#else
#if HAVE_X11_XPM_H
#include <X11/xpm.h>
#else
#undef USE_XPM
#endif
#endif

#if USE_XPM == 1

int DisplayDeviceX11::printScreen() {
    switch (shmLevel) {
    case shmPixmap:
        XpmWriteFileFromPixmap(xcth_display, prtFileName("xpm"), pixmap, 0, NULL);
        break;
    case shmImage:
    case shmNone:
        XpmWriteFileFromImage(xcth_display, prtFileName("xpm"), image, NULL, NULL);
    }
    return 0;
}

#else

int DisplayDeviceX11::printScreen() {
    CTH_ERROR("Print screen is not available. You need the Xpm library.\n");
    return 1;
}

#endif
