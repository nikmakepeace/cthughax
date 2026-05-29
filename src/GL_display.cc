#include "cthugha.h"
#include "display.h"
#include "disp-sys.h"
#include "cth_buffer.h"
#include "Interface.h"
#include "CthughaDisplay.h"
#include "AudioAnalyzer.h"
#include "pcx.h"
#include "CthughaBuffer.h"
#include "imath.h"
#include "glcthugha.h"

#include <GL/glut.h>

//
// note: lighting is done on a per-vertex basis, so even for
// plane objects (plate) more triangles are needed
//

//
// note: alpha values for paletted textures (as they are used for
// the buffers) are ignored when blending. 3dfx limitation
//

double fogIntensity = 0.4;

//
// Option to control how fine to make the mesh
//
class OptionS : public OptionInt {
    void setH() {
        h = 1.0 / double(value);
        h2 = 2.0 / double(value);
    }

public:
    double h;
    double h2;

    OptionS()
        : OptionInt("mesh-size", 32, 128, 4) {
        setH();
    }

    virtual void change(int by) {
        OptionInt::change(by);
        setH();
    }
    virtual void change(const char* to) {
        OptionInt::change(to);
        setH();
    }
} S;

Option& MeshSize = S;

//
// Option to specify texture quality
//
class OptionTexture : public OptionInt {
public:
    OptionTexture()
        : OptionInt("texture-quality", 1, 3) { }

    virtual const char* text() const {
        switch (value) {
        case 0:
            return "low";
        case 1:
            return "medium";
        case 2:
            return "high";
        }
        return "unknown quality";
    }
    virtual void change(const char* to) {
        if (strncasecmp(to, "low", 3) == 0) {
            value = 0;
        } else if (strncasecmp(to, "medium", 6) == 0) {
            value = 1;
        } else if (strncasecmp(to, "high", 4) == 0) {
            value = 2;
        } else
            OptionInt::change(to);
    }

    int needMipmaps() { return value >= 2; }
    void setTexParameters() {
        switch (value) {
        case 0:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;
        case 1:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        case 2:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            break;
        }
    }
} textureQuality;

Option& TextureQuality = textureQuality;

//
// possible display-function
//

int screen_plate();
int screen_2plates();
int screen_plane();
int screen_2planes();
int screen_wave1();
int screen_wave();
int screen_height();
int screen_cross();
int screen_dplane();
int screen_cwave();
int screen_nearRing();
int screen_tube();
int screen_sheight();
int screen_2dbig();
int screen_2dquad();
int screen_sphere();
int screen_cones();

static CoreOptionEntry* _screens[] = {
    new ScreenEntryGL(screen_plate, "Plate", "Plate", ScreenEntryGL::object),
    new ScreenEntryGL(screen_2plates, "2Plates", "", ScreenEntryGL::object, 2),
    new ScreenEntryGL(screen_plane, "Plane", "", ScreenEntryGL::plane),
    new ScreenEntryGL(screen_dplane, "dPlane", "", ScreenEntryGL::plane),
    new ScreenEntryGL(screen_2planes, "2Planes", "", ScreenEntryGL::plane, 2),
    new ScreenEntryGL(screen_wave1, "Wave1", "Wave 1", ScreenEntryGL::object),
    new ScreenEntryGL(screen_wave, "Wave", "Full Wave", ScreenEntryGL::object),
    new ScreenEntryGL(screen_height, "Height", "Height field", ScreenEntryGL::object),
    new ScreenEntryGL(screen_cross, "Cross", "Cross", ScreenEntryGL::object, 3),
    new ScreenEntryGL(screen_cwave, "CWave", "Centered Wave", ScreenEntryGL::object),
    new ScreenEntryGL(screen_nearRing, "NearRing", "Almost a Ring", ScreenEntryGL::object),
    new ScreenEntryGL(screen_tube, "Tube", "Simple Tube", ScreenEntryGL::plane),
    new ScreenEntryGL(screen_sheight, "SHeight", "Smooth Height", ScreenEntryGL::object),
    new ScreenEntryGL(screen_2dbig, "2D-Big", "2 dimensional", ScreenEntryGL::object),
    new ScreenEntryGL(screen_2dquad, "2D-Quad", "2 dimensional", ScreenEntryGL::object),
    new ScreenEntryGL(screen_sphere, "Sphere", "", ScreenEntryGL::object),
    new ScreenEntryGL(screen_cones, "Cones", "", ScreenEntryGL::object),
};
static CoreOptionEntryList screenEntries(_screens, sizeof(_screens) / sizeof(CoreOptionEntry*));
CoreOption screen(-1, "display", screenEntries);

//
// some helpers
//

void setPalette(Palette pal, int P) {
    //
    // for glide we must use an RGBA palette
    //
    static char rgba_pal[6][256][4];

    if (CthughaBuffer::buffers[P].palChanged) {
        int avg[3] = { 0, 0, 0 };
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 3; j++) {
                rgba_pal[P][i][j] = pal[i][j];
                avg[j] += (int)(pal[i][j]);
            }
            rgba_pal[P][i][3] = 0;
        }
        //	for(int j=0; j < 3; j++)
        //	    rgba_pal[P][0][j] = avg[j] / 256;
        rgba_pal[P][0][0] = 200;
        rgba_pal[P][0][1] = 200;
        rgba_pal[P][0][2] = 200;
    }

#if defined(GL_EXT_shared_texture_palette) && defined(SHARED_PALETTE)
    glColorTableEXT(GL_SHARED_TEXTURE_PALETTE_EXT, /* target */
        GL_RGBA, /* internal format */
        256, /* table size */
        GL_RGBA, /* table format */
        GL_UNSIGNED_BYTE, /* table type */
        rgba_pal[P]);
    glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
#else
    glColorTableEXT(GL_TEXTURE_2D, /* target */
        GL_RGBA, /* internal format */
        256, /* table size */
        GL_RGBA, /* table format */
        GL_UNSIGNED_BYTE, /* table type */
        rgba_pal[P]); /* the color table */
#endif
}

void setTexture(int P) {

    setPalette(CthughaBuffer::buffers[P].currentPalette, P);

    if (textureQuality.needMipmaps()) {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_COLOR_INDEX8_EXT, BUFF_WIDTH, BUFF_HEIGHT,
            GL_COLOR_INDEX, GL_UNSIGNED_BYTE, CthughaBuffer::buffers[P].passivePixels());
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, BUFF_WIDTH, BUFF_HEIGHT, 0,
            GL_COLOR_INDEX, GL_UNSIGNED_BYTE, CthughaBuffer::buffers[P].passivePixels());
    }
    textureQuality.setTexParameters();

    glEnable(GL_TEXTURE_2D);
}

GLuint* pcxTextures;
void setPcxTexture(int P) {

    glBindTexture(GL_TEXTURE_2D, pcxTextures[CthughaBuffer::buffers[P].pcx.currentN()]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnable(GL_TEXTURE_2D);
}

#include "joystick.h"

//
// Camera Positions
//

void CameraObject() {

    // distance to object from axis 3 (throttle on Sidewinder)
    const double r = double(Joystick::axis[3]) / 32767.0 + 1.5;

    // update rotation around object
    static double alpha[2] = { 0.4, 0.6 };
    alpha[0] += double(Joystick::stiffAxis[0]) * deltaT;
    alpha[1] += double(Joystick::stiffAxis[1]) * deltaT;

    const double s0 = sin(alpha[0] * 9.588e-5);
    const double s1 = sin(alpha[1] * 9.588e-5);
    const double c0 = cos(alpha[0] * 9.588e-5);
    const double c1 = cos(alpha[1] * 9.588e-5);

    gluLookAt(r * s0 * s1, r * c0 * s1, r * c1, // position
        0, 0, 0, // look at
        s0 * c1, c0 * c1, -s1); // up vector
}

void CameraRotate() {

    static double a = 0;
    a += double(Joystick::stiffAxis[0]) * 1e-4 * deltaT;

    gluLookAt(0, 0, 0, 0, 0, 2, sin(a), cos(a), 0);
}

//
// Map buffer onto an arbitrary 3D-mesh
//
typedef void (*meshFkt)(GLfloat pos[3], GLfloat normal[3], double alpha);

void mesh(meshFkt f, int n = 0, double alpha = 0.0) {

    GLfloat h = (n == 0) ? S.h : 1.0 / n;

    GLfloat pos1[3] = { 0, 0, 0 };
    GLfloat pos2[3] = { 0, 0, 0 };

    GLfloat normal1[3] = { 0, 0, 1 };
    GLfloat normal2[3] = { 0, 0, 1 };

    for (double y = 0; y < 1.0; y += h) {
        glBegin(GL_QUAD_STRIP);

        for (double x = 0; x <= 1.001; x += h) {
            pos1[0] = x;
            pos2[0] = x;
            pos1[1] = y;
            pos2[1] = y + h;

            glTexCoord2fv(pos1);
            f(pos1, normal1, alpha);
            glNormal3fv(normal1);
            glVertex3fv(pos1);

            glTexCoord2fv(pos2);
            f(pos2, normal2, alpha);
            glNormal3fv(normal2);
            glVertex3fv(pos2);
        }
        glEnd();
    }
}

void plate(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[0] = -1.0 + pos[0] * 2.0;
    pos[1] = -1.0 + pos[1] * 2.0;
    pos[2] = 0;
}
int screen_plate() {

    CameraObject();
    setTexture(0);

    mesh(plate, (light.currentN() == 0) ? 1 : S);

    return 0;
}

void plateSmall(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[0] = -0.8 + pos[0] * 1.6;
    pos[1] = -0.8 + pos[1] * 1.6;
    pos[2] = 0.25;
}
int screen_2plates() {

    CameraObject();

    setTexture(0);

    mesh(plate, (light.currentN() == 0) ? 1 : S);

    glPushMatrix();

    setTexture(1);
    double alpha = 520.0 * sin(now * 0.3 + 1.01) + 200.0 * sin(now * 0.5 + 1.10);
    glRotatef(alpha, 0, 0, 1);

    mesh(plateSmall, (light.currentN() == 0) ? 1 : S);

    return 0;
}

void plane(int tex, GLfloat pos) {
    setTexture(tex);

    static double s = 0;
    s -= double(Joystick::stiffAxis[1]) * 1e-4 * deltaT;
    s = fmod(s, 2.0);

    glBegin(GL_POLYGON);
    glTexCoord2f(0.0, 0.0 + s);
    glVertex3f(-3.0, pos, 0.0);
    glTexCoord2f(1.0, 0.0 + s);
    glVertex3f(3.0, pos, 0.0);
    glTexCoord2f(1.0, 3.0 + s);
    glVertex3f(4.0, 0, 5.0);
    glTexCoord2f(0.0, 3.0 + s);
    glVertex3f(-4.0, 0, 5.0);
    glEnd();
}

int screen_plane() {
    glEnable(GL_FOG);
    glFogf(GL_FOG_DENSITY, fogIntensity);

    CameraRotate();

    plane(0, 0.5);

    glDisable(GL_FOG);

    return 0;
}

int screen_dplane() {
    glEnable(GL_FOG);
    glFogf(GL_FOG_DENSITY, fogIntensity);

    CameraRotate();

    plane(0, 0.5);
    plane(0, -0.5);

    glDisable(GL_FOG);
    return 0;
}

int screen_2planes() {
    glEnable(GL_FOG);
    glFogf(GL_FOG_DENSITY, fogIntensity);

    CameraRotate();

    plane(0, 0.5);
    plane(1, -0.5);

    glDisable(GL_FOG);
    return 0;
}

void wave1(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[2] = acousticContext.intensity() * sin(2.0 * M_PI * pos[0] + alpha);
    pos[0] = -1.0 + pos[0] * 2.0;
    pos[1] = -1.0 + pos[1] * 2.0;

    normal[0] = acousticContext.intensity() * cos(2.0 * M_PI * pos[0] + alpha);
}
int screen_wave1() {

    CameraObject();
    double alpha = fmod(now, M_PI2);

    setTexture(0);
    glEnable(GL_NORMALIZE);
    mesh(wave1, S, alpha);
    glDisable(GL_NORMALIZE);

    return 0;
}

void wave(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[2] = acousticContext.intensity() * sin(2.0 * M_PI * pos[0] + alpha)
        * sin(2.0 * M_PI * pos[1] + alpha);
    pos[0] = -1.0 + pos[0] * 2.0;
    pos[1] = -1.0 + pos[1] * 2.0;

    normal[0] = acousticContext.intensity() * cos(2.0 * M_PI * pos[0] + alpha)
        * sin(2.0 * M_PI * pos[1] + alpha);
    normal[1] = acousticContext.intensity() * sin(2.0 * M_PI * pos[0] + alpha)
        * cos(2.0 * M_PI * pos[1] + alpha);
}
int screen_wave() {

    CameraObject();
    double alpha = fmod(now, M_PI2);

    setTexture(0);

    mesh(wave, S, alpha);

    return 0;
}

static double sc_x = 1;
static double sc_y = 1;
void height1(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[2] = 1.0 / 256.0 * GLfloat(CthughaBuffer::current->passivePixels()[int(pos[1] * sc_y + pos[0] * sc_x)]);
    pos[0] = -1.0 + pos[0] * 2.0;
    pos[1] = -1.0 + pos[1] * 2.0;

    // TODO: compute normal vector
}
int screen_height() {
    CameraObject();

    sc_x = BUFF_WIDTH;
    sc_y = BUFF_HEIGHT * BUFF_WIDTH;

    setTexture(0);
    mesh(height1);

    return 0;
}

void cross1(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[0] = -1.0 + pos[0] * 2.0;
    pos[1] = -1.0 + pos[1] * 2.0;
    pos[2] = 0.0; // -1.0;
}
void cross2(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[2] = -1.0 + pos[1] * 2.0;
    pos[1] = -1.0 + pos[0] * 2.0;
    pos[0] = 0.0; // +1.0;

    normal[0] = 1;
    normal[1] = 0;
    normal[2] = 0;
}
void cross3(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[0] = -1.0 + pos[0] * 2.0;
    pos[2] = -1.0 + pos[1] * 2.0;
    pos[1] = 0.0; // -1.0;

    normal[0] = 0;
    normal[1] = 1;
    normal[2] = 0;
}
int screen_cross() {
    CameraObject();

    int n = (light.currentN() == 0) ? 1 : S;

    setTexture(0);
    mesh(cross1, n);
    setTexture(1);
    mesh(cross2, n);
    setTexture(2);
    mesh(cross3, n);

    return 0;
}

void cwave(GLfloat pos[3], GLfloat normal[3], double alpha) {
    const GLfloat x = pos[0] - 0.5;
    const GLfloat y = pos[1] - 0.5;

    const double r = x * x + y * y;
    pos[0] = -1.0 + pos[0] * 2.0;
    pos[1] = -1.0 + pos[1] * 2.0;
    pos[2] = acousticContext.intensity() * 0.2 * sin(25.0 * r - alpha) / (r + 0.5);

    // TODO: compute normal
}
int screen_cwave() {

    CameraObject();
    double alpha = fmod(now, M_PI2);

    setTexture(0);
    mesh(cwave, S, alpha);

    return 0;
}

void nearRing(GLfloat pos[3], GLfloat normal[3], double /* alpha */) {
    double F = 0.25 * (1.0 + pos[0]);
    double alpha = 8.0 * pos[0] + now * 0.5;
    double beta = M_PI2 * pos[1];

    pos[0] = sin(alpha) * (1.0 + F * sin(beta));
    pos[1] = F * cos(beta);
    pos[2] = cos(alpha) * (1.0 + F * sin(beta));

    // TODO: compute normal
}

int screen_nearRing() {
    CameraObject();

    setTexture(0);
    mesh(nearRing);

    return 0;
}

void tube1(GLfloat pos[3], GLfloat normal[3], double alpha) {
    GLfloat p = pos[1];
    pos[2] = 4.0 * pos[0] + alpha;
    pos[0] = sin(p * M_PI);
    pos[1] = cos(p * M_PI);
}
void tube2(GLfloat pos[3], GLfloat normal[3], double alpha) {
    GLfloat p = pos[1];
    pos[2] = 4.0 * pos[0] + alpha;
    pos[0] = -sin(p * M_PI);
    pos[1] = cos(p * M_PI);
}
int screen_tube() {

    glEnable(GL_FOG);
    glFogf(GL_FOG_DENSITY, fogIntensity);

    CameraRotate();

    setTexture(0);

    mesh(tube1, S / 2);
    mesh(tube2, S / 2);
    mesh(tube1, S / 2, 4.0);
    mesh(tube2, S / 2, 4.0);
    mesh(tube1, S / 2, 8.0);
    mesh(tube2, S / 2, 8.0);

    glDisable(GL_FOG);
    return 0;
}

static GLfloat H[256][256];
static GLfloat H2[256][256];
void sheight(GLfloat pos[3], GLfloat normal[3], double alpha) {
    int x = int(pos[0] * S);
    int y = int(pos[1] * S);
    pos[0] = -1.0 + pos[0] * 2;
    pos[1] = -1.0 + pos[1] * 2;
    pos[2] = H2[x][y];

    // TODO: compute normal
}
int screen_sheight() {
    CameraObject();

    int sc_xi = (BUFF_WIDTH / S);
    int sc_yi = (BUFF_HEIGHT / S) * BUFF_WIDTH;

    if (S > 255)
        return 1;

    // take new values from buffer
    for (int i = 0; i < S; i++)
        for (int j = 0; j < S; j++)
            H[i][j] = GLfloat(CthughaBuffer::current->passivePixels()[i * sc_yi + j * sc_xi]) / 512.0;

    // set border to 0
    for (int i = 0; i < S; i++)
        H[i][0] = H[i][S - 1] = H[0][i] = H[S - 1][i] = 0;

    // do some smoothing steps
    for (int l = 0; l < 3; l++) {
        for (int i = 1; i < S; i++)
            for (int j = 1; j < S; j += 2)
                H[i][j] = 0.25 * (H[i - 1][j] + H[i + 1][j] + H[i][j + 1] + H[i][j - 1]);

        for (int i = 1; i < S; i++)
            for (int j = 2; j < S; j += 2)
                H[i][j] = 0.25 * (H[i - 1][j] + H[i + 1][j] + H[i][j + 1] + H[i][j - 1]);
    }

    // average with last height
    for (int i = 0; i < S; i++)
        for (int j = 0; j < S; j++)
            H2[i][j] = 0.7 * H2[i][j] + 0.3 * H[i][j];

    setTexture(0);
    mesh(sheight);

    return 0;
}

int screen_2dbig() {
    setTexture(0);

    // set up the coordinate system
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    mesh(plate, (light.currentN() == 0) ? 1 : S);

    return 0;
}

void plate_Q1(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[0] = -1.0 + pos[0];
    pos[1] = -1.0 + pos[1];
    pos[2] = 0;
}
void plate_Q2(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[0] = 1.0 - pos[0];
    pos[1] = -1.0 + pos[1];
    pos[2] = 0;
}
void plate_Q3(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[0] = 1.0 - pos[0];
    pos[1] = 1.0 - pos[1];
    pos[2] = 0;
}
void plate_Q4(GLfloat pos[3], GLfloat normal[3], double alpha) {
    pos[0] = -1.0 + pos[0];
    pos[1] = 1.0 - pos[1];
    pos[2] = 0;
}
int screen_2dquad() {

    setTexture(0);

    // set up the coordinate system
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    int n = (light.currentN() == 0) ? 1 : S / 2;

    mesh(plate_Q1, n);
    mesh(plate_Q2, n);
    mesh(plate_Q3, n);
    mesh(plate_Q4, n);

    return 0;
}

int screen_sphere() {

    static GLUquadricObj* o = NULL;
    if (o == NULL) {
        o = gluNewQuadric();
        gluQuadricTexture(o, GL_TRUE); // turn on texture
    }

    CameraObject();

    setTexture(0);

    gluSphere(o, 0.5, S, S);

    return 0;
}

int screen_cones() {

    static GLUquadricObj* o = NULL;
    if (o == NULL) {
        o = gluNewQuadric();
        gluQuadricTexture(o, GL_TRUE); // turn on texture
    }

    CameraObject();

    setTexture(0);

    glRotatef(90, 1, 0, 0);

    gluCylinder(o, 0.0, 0.5, 0.5, S, S);
    gluCylinder(o, 0.0, 0.5, -0.5, S, S);

    return 0;
}
