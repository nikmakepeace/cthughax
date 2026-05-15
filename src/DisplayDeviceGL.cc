#include "cthugha.h"
#include "DisplayDevice.h"
#include "SoundAnalyze.h"
#include "AutoChanger.h"
#include "cth_buffer.h"
#include "CthughaBuffer.h"
#include "Interface.h"
#include "keys.h"
#include "SoundServer.h"
#include "CDPlayer.h"
#include "CthughaDisplay.h"
#include "display.h"
#include "joystick.h"

#include <signal.h>

#include <GL/glut.h>
#if HAVE_GL_XMESA_H
#include <GL/xmesa.h>
#endif

#ifndef GL_EXT_paletted_texture
#error GL_EXT_paletted_texture not available.
#endif

int fullScreen = -1; // don't mess around by default

xy screenSizes[]
    = { xy(320, 200), xy(640, 480), xy(800, 600), xy(1024, 768), xy(1152, 900), xy(1200, 1024) };
int nScreenSizes = sizeof(screenSizes) / sizeof(xy);
xy bufferSizes[]
    = { xy(64, 64), xy(128, 128), xy(256, 256), xy(256, 256), xy(512, 512), xy(512, 512) };
int nBufferSizes = sizeof(bufferSizes) / sizeof(xy);

int GLkey = 0;

int cth_init(int* argc, char* argv[]) {
    glutInit(argc, argv);

    ncurses_use = DisplayDevice::text_on_term;

    CthughaBuffer::maxNBuffers = 3;

    return 0;
}

class GLHints : public OptionOnOff {
public:
    GLHints()
        : OptionOnOff("Hints", 1) { }

    virtual void change(int by) {
        OptionOnOff::change(by);

        if (value) {
            glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
            glHint(GL_FOG_HINT, GL_NICEST);
        } else {
            glDisable(GL_DITHER);
            glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
            glHint(GL_FOG_HINT, GL_FASTEST);
        }
    }
    virtual void change(const char* to) {
        if ((strcasecmp("nice", to) == 0) || (strcasecmp("nicest", to) == 0))
            value = 1;
        else if ((strcasecmp("fast", to) == 0) || (strcasecmp("fastest", to) == 0))
            value = 0;
        else
            OptionOnOff::change(to);
    }

    virtual const char* text() const { return value ? "nicest" : "fastest"; }
} glHints;
Option& Hints = glHints;

class GLDither : public OptionOnOff {
public:
    GLDither()
        : OptionOnOff("Dither", 1) { }

    virtual void change(int by) {
        OptionOnOff::change(by);

        if (value)
            glEnable(GL_DITHER);
        else
            glDisable(GL_DITHER);
    }
    virtual void change(const char* to) { OptionOnOff::change(to); }
} glDither;
Option& Dither = glDither;

class GLShade : public OptionOnOff {
public:
    GLShade()
        : OptionOnOff("Shade", 1) { }

    virtual void change(int by) {
        OptionOnOff::change(by);

        glShadeModel(value ? GL_SMOOTH : GL_FLAT);
    }
    virtual void change(const char* to) {
        if (strncasecmp(to, "flat", 4) == 0)
            value = 0;
        else if (strncasecmp(to, "smooth", 6) == 0)
            value = 1;
        else
            OptionOnOff::change(to);
    }
    virtual const char* text() const { return value ? "smooth" : "flat"; }
} glShade;
Option& Shade = glShade;

//
// display using OpenGL
//
extern GLuint* pcxTextures;

class DisplayDeviceGL : public DisplayDevice {
protected:
    GLuint Window;

    int setGlobalPalette() { return DisplayDevice::setGlobalPalette(); }

    void initImages() {
        pcxTextures = new GLuint[pcxEntries.n()];
        glGenTextures(pcxEntries.n(), pcxTextures);

        for (int i = 1; i < pcxEntries.n(); i++) {

            PCXEntry* pcxE = (PCXEntry*)pcxEntries[i];

            if ((pcxE == NULL) || (pcxE->data == NULL)) { // no PCX, use buffer instead
                continue;
            }

            //
            // convert from palette to RGBA format,
            // alpha value is ignored for palette textures
            //
            PaletteEntry* palE = (PaletteEntry*)paletteEntries[pcxE->pal];
            if ((palE == NULL) || (palE->pal == NULL))
                return;

            unsigned char RGBA[pcxE->height][pcxE->width][4];
            unsigned char* img = pcxE->data;
            Palette& pal = palE->pal;

            for (int y = 0; y < pcxE->height; y++)
                for (int x = 0; x < pcxE->width; x++, img++) {
                    RGBA[y][x][0] = pal[*img][0];
                    RGBA[y][x][1] = pal[*img][1];
                    RGBA[y][x][2] = pal[*img][2];
                    RGBA[y][x][3] = 128; // alpha value
                }

            //
            // build mipmaps
            //
            glEnable(GL_TEXTURE_2D);

            glBindTexture(GL_TEXTURE_2D, pcxTextures[i]);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            gluBuild2DMipmaps(
                GL_TEXTURE_2D, GL_RGB, pcxE->width, pcxE->height, GL_RGBA, GL_UNSIGNED_BYTE, RGBA);
            //
            // NOTE: In the current implementation (Mesa 3.0) RGBA textures
            // are stored in 4444 format, while RGB textures use 565 weight.
            // So RGBA should be avaoided for images - the color reduction is
            // too much.
            //
        }
        glBindTexture(GL_TEXTURE_2D, pcxTextures[0]);
    }

    static void keyCB(unsigned char k, int, int) {
        switch (k) {
        case 8:
            GLkey = CK_BACK;
            break;
        case 27:
            GLkey = CK_ESC;
            break;
        case 10:
        case 13:
            GLkey = CK_ENTER;
            break;
        default:
            GLkey = k;
        }
    }
    static void specialKeyCB(int k, int, int) {
        switch (k) {
        case 27:
            GLkey = CK_ESC;
            break;
        case GLUT_KEY_F1:
            GLkey = CK_FKT(1);
            break;
        case GLUT_KEY_F2:
            GLkey = CK_FKT(2);
            break;
        case GLUT_KEY_F3:
            GLkey = CK_FKT(3);
            break;
        case GLUT_KEY_F4:
            GLkey = CK_FKT(4);
            break;
        case GLUT_KEY_F5:
            GLkey = CK_FKT(5);
            break;
        case GLUT_KEY_F6:
            GLkey = CK_FKT(6);
            break;
        case GLUT_KEY_F7:
            GLkey = CK_FKT(7);
            break;
        case GLUT_KEY_F8:
            GLkey = CK_FKT(8);
            break;
        case GLUT_KEY_F9:
            GLkey = CK_FKT(9);
            break;
        case GLUT_KEY_F10:
            GLkey = CK_FKT(10);
            break;
        case GLUT_KEY_F11:
            GLkey = CK_FKT(11);
            break;
        case GLUT_KEY_F12:
            GLkey = CK_FKT(12);
            break;

        case GLUT_KEY_LEFT:
            GLkey = CK_LEFT;
            break;
        case GLUT_KEY_UP:
            GLkey = CK_UP;
            break;
        case GLUT_KEY_RIGHT:
            GLkey = CK_RIGHT;
            break;
        case GLUT_KEY_DOWN:
            GLkey = CK_DOWN;
            break;
        case GLUT_KEY_PAGE_UP:
            GLkey = CK_PGUP;
            break;
        case GLUT_KEY_PAGE_DOWN:
            GLkey = CK_PGDN;
            break;
        case GLUT_KEY_HOME:
            GLkey = CK_HOME;
            break;
        case GLUT_KEY_END:
            GLkey = CK_END;
            break;

        default:
            GLkey = CK_NONE;
        }
    }

    static void reshapeCB(int width, int height) {
        disp_size.x = width;
        disp_size.y = height;

        glViewport(0, 0, (GLint)width, (GLint)height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60, double(width) / double(height), 0.1, 40.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    static void idleCB() {
        Joystick::run();

        run(0);
        Interface::current->run();
        if (cthugha_close)
            exit(0);
        glutPostRedisplay();
    }

    static void drawCB() {

        glEnable(GL_TEXTURE_2D);
        //	glEnable(GL_ALPHA_TEST);
        glEnable(GL_DEPTH_TEST);
        //	glEnable(GL_FOG);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        (*cthughaDisplay)();

        // display text
        displayDevice->prePrint();
        Interface::current->display(); // print the text of the current interface
        errors.display(); // and the error messages
        displayDevice->postPrint();

        displayDevice->postDraw();

        glutSwapBuffers();
    }

public:
    DisplayDeviceGL()
        : Window(0) {
        CTH_INFO("Initializing OpenGL display...\n");

        //
        // set up display size
        //
        if (display_mode != -1) {
            /* use one of the default resolutions */
            if ((display_mode >= nScreenSizes) || (display_mode < 0))
                display_mode = 0;

            disp_size = screenSizes[display_mode];
        }

        //
        // make main window
        //
        glutInitWindowPosition(0, 0);
        glutInitWindowSize(disp_size.x, disp_size.y);
        glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

        if ((Window = glutCreateWindow("GLCthugha")) == 0) {
            CTH_ERROR("Can not create GLUT window.\n");
            exit(0);
        }

// it would be nice to set this before glutCreateWindow,
// but it works only after that call
#if HAVE_GL_XMESA_H
        switch (fullScreen) {
        case 0:
            XMesaSetFXmode(XMESA_FX_WINDOW);
            break;
        case 1:
            XMesaSetFXmode(XMESA_FX_FULLSCREEN);
            break;
        }
#endif

        glutSetCursor(GLUT_CURSOR_NONE);

        // check if paletted texture is available
        if (!glutExtensionSupported("GL_EXT_paletted_texture")) {
            CTH_ERROR("Sorry, GL_EXT_paletted_texture is not available.\n");
            exit(0);
        }

        //
        // some GL init
        //
        glDither.change(0);
        glHints.change(0);
        glShade.change(0);

        //
        // prepare textures for images
        //
        initImages();

        //
        // set up glut-callbacks
        //
        glutReshapeFunc(reshapeCB);
        glutIdleFunc(idleCB);
        glutDisplayFunc(drawCB);
        glutKeyboardFunc(keyCB);
        glutSpecialFunc(specialKeyCB);

        fontSize.x = 100;
        fontSize.y = 100;

        text_size.x = 80; // always use 80x40 charachers
        text_size.y = 40;

        FW = glutStrokeWidth(GLUT_STROKE_MONO_ROMAN, 'A');
    }

    int FW;

    virtual void printstring(void* font, const char* string, int len) {
        while ((*string != '\0') && (len > 0)) {
            glutStrokeCharacter(font, *string);
            string++;
            len--;
        }
    }

// this is taken from the glut sources
#define FH (119.048 + 33.3333)
#define FU 33.3333

    virtual void prePrint() {
        DisplayDevice::prePrint();

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_FOG);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, FW * text_size.x, 0.0, FH * text_size.y, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    virtual void printString(int x, int y, const char* text, int color, int len, int noDarken) {

        glPushMatrix();

        glTranslatef(FW * x / 100.0, FH * (39.0 - y / 100.0), 0);

        if (!noDarken) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glColor4f(0.0, 0.0, 0.0, 0.5);

            glBegin(GL_POLYGON);
            glVertex2f(0.0, 0.0);
            glVertex2f(0.0, FH * 1.0);
            glVertex2f(FW * len, FH * 1.0);
            glVertex2f(FW * len, 0.0);
            glEnd();

            glDisable(GL_BLEND);
        }

        glColor4f(double(textColorRGB[color][0]) / 255.0, double(textColorRGB[color][1]) / 255.0,
            double(textColorRGB[color][2]) / 255.0, 0.0);

        glTranslatef(0, FU, 0);
        printstring(GLUT_STROKE_MONO_ROMAN, text, len);

        glPopMatrix();
    }

    virtual void postPrint() { }

    virtual int printScreen() {

        GLvoid* pixels = new char[disp_size.x * disp_size.y * 3];

        glReadPixels(0, 0, disp_size.x, disp_size.y, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        FILE* file;
        if ((file = fopen(prtFileName("tga"), "w")) == NULL) {
            printfee("Can not open print screen file.\n");
            delete pixels;
            return 1;
        }

        short s = 0;

        fputc(0, file); // IDLen
        fputc(0, file); // palType
        fputc(2, file); // Type (uncompressed TrueColor);
        fwrite(&s, 2, 1, file); // FirstPalIndex
        fwrite(&s, 2, 1, file); // PalSize
        fputc(0, file); // PalBits
        fwrite(&s, 2, 1, file); // XNull
        fwrite(&s, 2, 1, file); // YNull
        s = disp_size.x;
        fwrite(&s, 2, 1, file);
        s = disp_size.y;
        fwrite(&s, 2, 1, file);
        fputc(24, file); // PixelBits
        fputc(0, file); // Orientation

        fwrite(pixels, 3, disp_size.x * disp_size.y, file); // write all RGB triples

        fclose(file);

        delete pixels;

        return 0;
    }

    void mainLoop() { glutMainLoop(); }
};

void newDisplayDevice() { displayDevice = new DisplayDeviceGL(); }
