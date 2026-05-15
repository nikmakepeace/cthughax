#include "cthugha.h"
#include "DisplayDevice.h"
#include "keys.h"
#include "display.h"
#include "translate.h"
#include "Interface.h"
#include "CthughaBuffer.h"
#include "disp-sys.h"
#include "cth_buffer.h"
#include "imath.h"
#include "CthughaDisplay.h"

#define Palette Palette_gl
#include <vga.h>
#include <vgagl.h>
#undef Palette

#include <unistd.h>

int cth_init(int* /* argc*/, char** /*argv*/) {

    seteuid(0); // the program might be setuid something else
    vga_init(); /* initialize svgalib */

    DisplayDevice::text_on_term = 0;

    return 0;
}

int screen_modes[] = {
    /* allowed modes */
    G320x200x256, /* 0 */
    G320x240x256, /* 1 */
    G320x400x256, /* 2 */
    G360x480x256, /* 3 */
    G640x480x256, /* 4 */
    G800x600x256, /* 5  here it starts getting tiny */
#ifdef G1024x768x256
    G1024x768x256, /* 6 */
#endif
#ifdef G1280x1024x256
    G1280x1024x256, /* 7 */
#endif
#ifdef G1600x1200x256
    G1600x1200x256, /* 8  only for completeness */
#endif
};
int nr_screen_modes = sizeof(screen_modes) / sizeof(int);

xy screenSizes[] = { xy(320, 200), xy(320, 240), xy(320, 400), xy(360, 480), xy(640, 480),
    xy(800, 600), xy(1024, 768), xy(1200, 1024) };
int nScreenSizes = sizeof(screenSizes) / sizeof(xy);
xy bufferSizes[] = { xy(160, 100), xy(160, 120), xy(160, 200), xy(180, 240), xy(320, 240),
    xy(400, 300), xy(512, 384), xy(600, 512) };
int nBufferSizes = sizeof(bufferSizes) / sizeof(xy);

static GraphicsContext display_screen_context;
static GraphicsContext display_virtual_context;

static int display_copybox = 0; /* use copybox. Not available
                                   at some screen-modes */

char display_font[256 * 8 * 8];

class DisplayDeviceSvga : public DisplayDevice {
protected:
    Palette currentPalette; // the current Palette
    Palette textPalette; // the palette set with svgalib

    int setGlobalPalette() {

        if (DisplayDevice::setGlobalPalette() == 0) // nothing changed
            return 0;

        if (textOnScreen) { // text is on screen
            int i, j;

            if (darkenPalette) {
                // create a dimmed palette.
                // average two palette entries and divide by 2
                for (i = 0; i < 128; i++)
                    for (j = 0; j < 3; j++)
                        textPalette[i][j]
                            = (CthughaBuffer::buffers[0].currentPalette[i * 2][j]
                                  + CthughaBuffer::buffers[0].currentPalette[i * 2 + 1][j])
                            >> 2;
            } else {
                for (i = 0; i < 128; i++)
                    for (j = 0; j < 3; j++)
                        textPalette[i][j]
                            = (CthughaBuffer::buffers[0].currentPalette[i * 2][j]
                                  + CthughaBuffer::buffers[0].currentPalette[i * 2 + 1][j])
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

            memcpy(currentPalette, textPalette, sizeof(Palette));

            // really set now
            vga_waitretrace();
            gl_setpalette(textPalette);

        } else {
            draw_mode = DM_direct;

            memcpy(currentPalette, CthughaBuffer::buffers[0].currentPalette, sizeof(Palette));

            vga_waitretrace();
            gl_setpalette(CthughaBuffer::buffers[0].currentPalette);
        }
        return 0;
    }

public:
    DisplayDeviceSvga() { init_graph_mode(); }

    void mainLoop() {
        do {
            Interface::current->run();
            run(1);
        } while (cthugha_close == 0);
    }

    //
    // save what is on the screen now
    //
    int printScreen() {
        unsigned char* v = preDraw();
        return save_pcx(v, WIDTH, HEIGHT, currentPalette);
    }

    void init_graph_mode() {
        vga_modeinfo* mi;

        if ((display_mode >= nr_screen_modes) || (display_mode < 0))
            display_mode = 0;

        /* create screen-context */
        vga_setmode(screen_modes[display_mode]);
        gl_setcontextvga(screen_modes[display_mode]);

        gl_getcontext(&display_screen_context);
        /*    gl_enablepageflipping( &display_screen_context);  */
        /* direct display only in 320x200 */
        if (display_mode != 0)
            display_direct = 0;

        mi = vga_getmodeinfo(screen_modes[display_mode]);

        /* can't use copybox if screen-mode is planar 256 colors */
        display_copybox = (mi->flags & 4) ? 0 : 1;

        /* prepare font */
        gl_expandfont(8, 8, 255, gl_font8x8, display_font);
        gl_setfont(8, 8, display_font);
        gl_setwritemode(WRITEMODE_MASKED);

        /* create context for drawing in memory */
        gl_setcontextvgavirtual(screen_modes[display_mode]);
        gl_getcontext(&display_virtual_context);
        /*    gl_enablepageflipping( &display_virtual_context);  */

        bypp = 1;
        disp_size.x = WIDTH;
        disp_size.y = HEIGHT;
        bytes_per_line = BYTEWIDTH;
        draw_mode = DM_direct;

        if ((disp_size.x < BUFF_WIDTH) || (disp_size.y < BUFF_HEIGHT)) {
            exit_graph_mode();
            CTH_ERROR("Screen must not be smaller than buffer.");
            exit(0);
        }

        text_size.x = disp_size.x / 8;
        text_size.y = disp_size.y / 8;

        //
        // prepare Palette for Text display
        //
        for (int i = 0; i < textColors; i++) {
            textColor[i] = 255 - i;

            textPalette[255 - i][0] = textColorRGB[i][0];
            textPalette[255 - i][1] = textColorRGB[i][1];
            textPalette[255 - i][2] = textColorRGB[i][2];
        }
    }

    void exit_graph_mode() { vga_setmode(TEXT); }

    void clearBox(int x, int y, int width, int height) { gl_fillbox(x, y, width, height, 0); }

    void copyBox(int x, int y, int width, int height, int destx, int desty) {
        gl_copybox(x, y, width, height, destx, desty);
    }

    unsigned char* preDraw() {
        if (display_direct)
            gl_setcontext(&display_screen_context);
        else
            gl_setcontext(&display_virtual_context);

        return (unsigned char*)VBUF;
    }

    void postDraw() {
        this->setGlobalPalette(); // update the global palette

        if (display_syncwait)
            vga_waitretrace();

        /* copy virtual screen to physical screen */
        if (!display_direct) {
            if (needsFullCopy || !display_copybox) /* draw full screen */
                gl_copyscreen(&display_screen_context);
            else { /* or only a part */
                gl_setcontext(&display_screen_context);
                gl_copyboxfromcontext(&display_virtual_context, SCREEN_OFFSET_X, SCREEN_OFFSET_Y,
                    draw_size.x, draw_size.y, SCREEN_OFFSET_X, SCREEN_OFFSET_Y);
            }
        }

        needsFullCopy = 0;
    }

    void printString(int x, int y, const char* text, int color, int len, int noDarken) {
        static int last_color = -1;

        if (color != last_color)
            gl_colorfont(8, 8, textColor[color], display_font);
        last_color = color;

        if ((y >= 0) && (y <= HEIGHT - 8))
            gl_writen(x, y, min(len, WIDTH / 8), (char*)text);

        textOnScreen = 1;
        if (!noDarken)
            darkenPalette = 1;
    }
};

void newDisplayDevice() { displayDevice = new DisplayDeviceSvga(); }
