#include "cthugha.h"
#include "display.h"
#include "options.h"
#include "Sound.h"
#include "SoundAnalyze.h"
#include "translate.h"
#include "disp-sys.h"
#include "imath.h"
#include "cth_buffer.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"
#include "DisplayDevice.h"

#include <math.h>

/* possible display-function */
int screen_up();
int screen_down();
int screen_2hor();
int screen_r2hor();
int screen_4hor();
int screen_2verd();
int screen_r2verd();
int screen_4kal();
int screen_hfield();
int screen_roll();
int screen_zick();
int screen_bent();
int screen_plate();
int screen_scale2();
int screen_vscale_hmirror();
int screen_hscale_vmirror();

static CoreOptionEntry* _screens[] = { 
    new ScreenEntry(screen_up, "Up", "Up Display", xy(1, 1)),               // 0
    new ScreenEntry(screen_down, "Down", "Upside Down", xy(1, 1)),          // 1
    new ScreenEntry(screen_2hor, "2hor", "Hor. Split out", xy(1, 1)),       // 2
    new ScreenEntry(screen_r2hor, "r2hor", "Hor. Split in", xy(1, 1)),      // 3
    new ScreenEntry(screen_4hor, "4hor", "Kaleidoscope", xy(1, 1)),         // 4
    new ScreenEntry(screen_2verd, "2verd", "90deg rot. mirror", xy(1, 1)),  // 5
    new ScreenEntry(screen_r2verd, "r2verd", "90deg rot. mirror II", xy(1, 1)), // 6
    new ScreenEntry(screen_4kal, "4kal", "90deg Kaleidoscope", xy(1, 1)),   // 7
    new ScreenEntry(screen_hfield, "hfield", "Heightfield", xy(2, 2)),      // 8
    new ScreenEntry(screen_roll, "roll", "Roll around x-axis", xy(1, 1)),   // 9
    new ScreenEntry(screen_zick, "zick", "Zick Zack", xy(1, 1), 0),         // 10
    new ScreenEntry(screen_bent, "bent", "A bending plane", xy(2, 2)),      // 11
    new ScreenEntry(screen_plate, "plate", "A rotating plate", xy(2, 2)),   // 12

    new ScreenEntry(screen_scale2, "scalexy", "Double", xy(2, 2), 0),     // 13
    new ScreenEntry(screen_vscale_hmirror, "scaley", "Scale vertical, mirror horizontal", xy(1, 2)), // 14
    new ScreenEntry(screen_hscale_vmirror, "scalex", "Scale horizontal, mirror vertical", xy(2, 1)), // 15
};
static CoreOptionEntryList screenEntries(_screens, sizeof(_screens) / sizeof(CoreOption*));

CoreOption screen(-1, "display", screenEntries);

char screen_first[256] = ""; /* Start with this scrn-fkt */

/*****************************************************************************
 * Screen-functions
 ****************************************************************************/

/*
 * Draw a permutation of the lines.
 *
 * A lot of the display functions are now based on this.
 */
static unsigned char* perm_lines[MAX_BUFF_HEIGHT];

void screen_perm(void) {
    unsigned char* scrn = cthughaDisplay->buffer;
    unsigned char** perm = perm_lines;
    int i;

    for (i = BUFF_HEIGHT; i != 0; i--) {
        memcpy(scrn, *perm, BUFF_WIDTH);
        scrn += cthughaDisplay->bufferWidth;
        perm++;
    }
}

#define PERM_ADDR(i) (passive_buffer + (i) * BUFF_WIDTH)

int screen_up() {
    int i;
    for (i = 0; i < BUFF_HEIGHT; i++)
        perm_lines[i] = PERM_ADDR(i);

    screen_perm();

    return 0;
}

int screen_down() {
    int i;
    for (i = 0; i < BUFF_HEIGHT; i++)
        perm_lines[i] = PERM_ADDR(BUFF_HEIGHT - i - 1);

    screen_perm();
    return 0;
}

int screen_2hor() {
    int i;
    for (i = 0; i < BUFF_HEIGHT / 2; i++) {
        /* lower half of buffer maps to upper half of screen */
        perm_lines[i] = PERM_ADDR(BUFF_HEIGHT / 2 + i);
        /* lower half of buffer get turned around to lower half of screen*/
        perm_lines[i + BUFF_HEIGHT / 2] = PERM_ADDR(BUFF_HEIGHT - i - 1);
    }

    screen_perm();
    return 0;
}

int screen_r2hor() {
    int i;
    for (i = 0; i < BUFF_HEIGHT / 2; i++) {
        /* lower half of buffer get turned around to upper half of screen*/
        perm_lines[i] = PERM_ADDR(BUFF_HEIGHT - i - 1);
        /* lower half of buffer maps to lower half of screen */
        perm_lines[i + BUFF_HEIGHT / 2] = PERM_ADDR(BUFF_HEIGHT / 2 + i);
    }

    screen_perm();
    return 0;
}

int screen_4hor() {
    unsigned char *tmp, *scrn;
    int x, y;

    /* upper half of screen */
    tmp = passive_buffer + BUFF_SIZE / 2;
    scrn = cthughaDisplay->buffer;
    for (y = BUFF_HEIGHT / 2; y != 0; y--) {

        /* left half */
        memcpy(scrn, tmp, BUFF_WIDTH / 2);
        scrn += BUFF_WIDTH / 2;
        tmp += BUFF_WIDTH / 2;

        /* right half */
        for (x = BUFF_WIDTH / 2; x != 0; x--) {
            *scrn = *tmp;
            scrn++;
            tmp--;
        }
        tmp += BUFF_WIDTH;
        scrn += cthughaDisplay->bufferWidth - BUFF_WIDTH;
    }

    /* lower half of screen */
    tmp = passive_buffer + BUFF_WIDTH * BUFF_HEIGHT - BUFF_WIDTH;
    scrn = cthughaDisplay->buffer + cthughaDisplay->bufferWidth * BUFF_HEIGHT / 2;
    for (y = BUFF_HEIGHT / 2; y != 0; y--) {

        /* left half */
        memcpy(scrn, tmp, BUFF_WIDTH / 2);
        scrn += BUFF_WIDTH / 2;
        tmp += BUFF_WIDTH / 2;

        /* right half */
        for (x = BUFF_WIDTH / 2; x != 0; x--) {
            *scrn = *tmp;
            scrn++;
            tmp--;
        }
        tmp -= BUFF_WIDTH;
        scrn += cthughaDisplay->bufferWidth - BUFF_WIDTH;
    }

    return 0;
}

int screen_2verd() {
    if (BUFF_WIDTH / 2 <= BUFF_HEIGHT) {
        int x, y;
        unsigned char* tmp = passive_buffer;
        unsigned char* scrn = cthughaDisplay->buffer;
        for (y = BUFF_HEIGHT; y != 0; y--) {
            for (x = BUFF_WIDTH / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn++;
                tmp += BUFF_WIDTH;
            }
            for (x = BUFF_WIDTH / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn++;
                tmp -= BUFF_WIDTH;
            }
            tmp++;
            scrn += cthughaDisplay->bufferWidth - BUFF_WIDTH;
        }

    } else {
        screen.change(+1, 0);
        return 1;
    }
    return 0;
}

int screen_r2verd() {
    if (BUFF_WIDTH / 2 <= BUFF_HEIGHT) {
        int x, y;
        unsigned char* tmp = passive_buffer;
        unsigned char* scrn = cthughaDisplay->buffer
            + cthughaDisplay->bufferWidth * (BUFF_HEIGHT - 1) + (BUFF_WIDTH - 1);
        for (y = BUFF_HEIGHT; y != 0; y--) {
            for (x = BUFF_WIDTH / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn--;
                tmp += BUFF_WIDTH;
            }
            for (x = BUFF_WIDTH / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn--;
                tmp -= BUFF_WIDTH;
            }
            tmp++;
            scrn -= cthughaDisplay->bufferWidth - BUFF_WIDTH;
        }
    } else {
        screen.change(+1, 0);
        return 1;
    }
    return 0;
}

int screen_4kal() {
    if (BUFF_WIDTH / 2 <= BUFF_HEIGHT) {
        unsigned char *tmp, *scrn;
        int x, y;

        tmp = passive_buffer;
        scrn = cthughaDisplay->buffer;
        /* upper half */
        for (y = BUFF_HEIGHT / 2; y != 0; y--) {
            for (x = BUFF_WIDTH / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn++;
                tmp += BUFF_WIDTH;
            }
            for (x = BUFF_WIDTH / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn++;
                tmp -= BUFF_WIDTH;
            }
            tmp++;
            scrn += cthughaDisplay->bufferWidth - BUFF_WIDTH;
        }

        tmp = passive_buffer;
        scrn = cthughaDisplay->buffer + cthughaDisplay->bufferWidth * (BUFF_HEIGHT - 1) + BUFF_WIDTH
            - 1;
        /* lower half */
        for (y = BUFF_HEIGHT / 2; y != 0; y--) {
            for (x = BUFF_WIDTH / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn--;
                tmp += BUFF_WIDTH;
            }
            for (x = BUFF_WIDTH / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn--;
                tmp -= BUFF_WIDTH;
            }
            tmp++;
            scrn -= cthughaDisplay->bufferWidth - BUFF_WIDTH;
        }
    } else {
        screen.change(+1, 0);
        return 1;
    }
    return 0;
}

int screen_scale2() {
    unsigned char* src = passive_buffer;
    unsigned char* dst = cthughaDisplay->buffer;

    for (int y = BUFF_HEIGHT; y != 0; y--) {
        unsigned char* dst1 = dst;
        unsigned char* dst2 = dst + cthughaDisplay->bufferWidth;

        for (int x = BUFF_WIDTH; x != 0; x--) {
            unsigned char color = *src;
            src++;

            *dst1 = color;
            dst1++;
            *dst1 = color;
            dst1++;

            *dst2 = color;
            dst2++;
            *dst2 = color;
            dst2++;
        }

        dst += 2 * cthughaDisplay->bufferWidth;
    }

    return 0;
}

int screen_vscale_hmirror() {
    unsigned char* src = passive_buffer;
    unsigned char* dst = cthughaDisplay->buffer;

    for (int y = BUFF_HEIGHT; y != 0; y--) {
        memcpy(dst, src, BUFF_WIDTH);
        memcpy(dst + cthughaDisplay->bufferWidth, src, BUFF_WIDTH);

        src += BUFF_WIDTH;
        dst += 2 * cthughaDisplay->bufferWidth;
    }

    return 0;
}

int screen_hscale_vmirror() {
    unsigned char* src = passive_buffer;
    unsigned char* dst = cthughaDisplay->buffer;

    for (int y = BUFF_HEIGHT; y != 0; y--) {
        unsigned char* d = dst;

        for (int x = BUFF_WIDTH; x != 0; x--) {
            unsigned char color = *src;
            src++;

            *d = color;
            d++;
            *d = color;
            d++;
        }

        dst += cthughaDisplay->bufferWidth;
    }

    return 0;
}

/*
 * 3-D display functions
 */
static const float SC = 1 << 16;
static xy height_offset[256];
static xy s1, s2, p;
static float rot[3] = { 0, 0, 0 };
static float P[4][3] = { { -1, -1, 0 }, { 1, -1, 0 }, { 1, 1, 0 }, { 0, 0, 1 } };
static const float rotSpeed[3] = { 0.150, 0.250, 0.100 }; /* radians per second */
static double scaleFactor = 0.8;
static double scaleFactorPhase = 0.0;
static double scaleFactorSpeed = 0.03; /* cycles per second */
static double intensityFactor = 10.0; /* how much the sound intensity affects the scale factor */
static const double minScaleFactor = 0.8;
static const double maxScaleFactor = 2.0;
static int splatSize = 1;
static const int maxSplatSize = 4;

/*
 * rotate vektor p around x/y/z axis (by rot[i])
 * result in vector r
 */
void rotate(float p[3], float rot[3], float r[3]) {
    float t1[3], t2[3];
    /* first around x axis */
    t1[0] = p[0];
    t1[1] = p[1] * cos(rot[0]) + p[2] * sin(rot[0]);
    t1[2] = -p[1] * sin(rot[0]) + p[2] * cos(rot[0]);

    /* around y axis */
    t2[0] = t1[0] * cos(rot[1]) + t1[2] * sin(rot[1]);
    t2[1] = t1[1];
    t2[2] = -t1[0] * sin(rot[1]) + t1[2] * cos(rot[1]);

    /* around z axis */
    r[0] = t2[0] * cos(rot[2]) + t2[1] * sin(rot[2]);
    r[1] = -t2[0] * sin(rot[2]) + t2[1] * cos(rot[2]);
    r[2] = t2[2];
}
inline float sqr(float a) { return a * a; }

inline void put_3d_pixel(unsigned char* dst, int x, int y, unsigned char color) {
    if ((x >= -BUFF_WIDTH) && (x < BUFF_WIDTH) && (y >= -BUFF_HEIGHT) && (y < BUFF_HEIGHT))
        dst[y * cthughaDisplay->bufferWidth + x] = color;
}

inline void put_3d_splat(unsigned char* dst, int x, int y, unsigned char color) {
    int offset = (splatSize - 1) / 2;

    for (int yy = 0; yy < splatSize; yy++)
        for (int xx = 0; xx < splatSize; xx++)
            put_3d_pixel(dst, x + xx - offset, y + yy - offset, color);
}

void update_3d_scale_factor() {
    double dt = deltaT;
    double mid = (minScaleFactor + maxScaleFactor) / 2.0;
    double amp = (maxScaleFactor - minScaleFactor) / 2.0;

    if (dt < 0.0)
        dt = 0.0;
    if (dt > 0.25)
        dt = 0.25;

    scaleFactorPhase += 2.0 * M_PI * scaleFactorSpeed * dt;
    if (scaleFactorPhase >= 2.0 * M_PI)
        scaleFactorPhase = fmod(scaleFactorPhase, 2.0 * M_PI);

    scaleFactor = mid - amp * cos(scaleFactorPhase) + (intensityFactor * soundAnalyze.intensity);
    splatSize = int(scaleFactor + 0.5);
    if (splatSize < 1)
        splatSize = 1;
    if (splatSize > maxSplatSize)
        splatSize = maxSplatSize;

    CTH_TRACE("update_3d_scale_factor: dt=%.3f, phase=%.3f, scaleFactor=%.3f\n", dt, scaleFactorPhase,
        scaleFactor);
    CTH_TRACE("intensity: %.3f\n", soundAnalyze.intensity);
}

/*
 * preparations for a 3D display
 */
int prepare_3d(int maxZ) {
    int i;
    float x, y, z, l;
    float ro[4][3];
    double scale;

    /* calculate the maximal size of the "sheet" */
    x = sqr(BUFF_WIDTH / 2);
    y = sqr(BUFF_HEIGHT / 2);
    z = sqr((float)(BUFF_WIDTH) / 640.0 * (float)maxZ);
    l = sqrt(x + y + z);

    update_3d_scale_factor();
    scale = scaleFactor * (float)min(BUFF_WIDTH, BUFF_HEIGHT) / l;

    /* rotate a little bit */
    /* TODO: use some value from the sound to set speed,
       external value (joystick) to set rotation */
    double dt = deltaT;
    if (dt < 0.0)
        dt = 0.0;
    if (dt > 0.25)
        dt = 0.25;
    rot[0] += rotSpeed[0] * dt;
    rot[1] += rotSpeed[1] * dt;
    rot[2] += rotSpeed[2] * dt;
    for (i = 0; i < 4; i++)
        rotate(P[i], rot, ro[i]);

    for (i = 0; i < 256; i++) {
        height_offset[i].x = int(ro[3][0] * i * double(BUFF_WIDTH) / 640.0 * SC);
        height_offset[i].y = int(ro[3][1] * i * double(BUFF_WIDTH) / 640.0 * SC);
    }

    /* calculate the step sizes */
    s1.x = int((ro[1][0] - ro[0][0]) * scale * SC / 2.0);
    s1.y = int((ro[1][1] - ro[0][1]) * scale * SC / 2.0);

    s2.x = int((ro[2][0] - ro[1][0]) * scale * SC / 2.0);
    s2.y = int((ro[2][1] - ro[1][1]) * scale * SC / 2.0);

    /* starting point */
    p.x = -BUFF_WIDTH / 2 * s1.x - BUFF_HEIGHT / 2 * s2.x;
    p.y = -BUFF_WIDTH / 2 * s1.y - BUFF_HEIGHT / 2 * s2.y;

    /* clear screen */
    unsigned char* clr = cthughaDisplay->buffer;
    for (i = 2 * BUFF_HEIGHT; i != 0; i--) {
        memset(clr, '\0', 2 * BUFF_WIDTH);
        clr += cthughaDisplay->bufferWidth;
    }

    return 0;
}

/*
 * hfield: buffer_value determines height
 */
int screen_hfield() {
    unsigned char* src = passive_buffer;
    unsigned char* dst
        = cthughaDisplay->buffer + BUFF_WIDTH + cthughaDisplay->bufferWidth * BUFF_HEIGHT;
    int i, j;

    prepare_3d(256);

    for (i = BUFF_HEIGHT; i != 0; i--) {
        xy t = p;
        for (j = BUFF_WIDTH; j != 0; j--) {
            put_3d_splat(dst, (p.x + height_offset[*src].x) >> 16,
                (p.y + height_offset[*src].y) >> 16, *src | 128);
            src++;
            p.x += s1.x;
            p.y += s1.y;
        }
        p.x = t.x + s2.x;
        p.y = t.y + s2.y;
    }

    return 0;
}

/*
 * bent: height is a sine-wave along x axis
 */
int screen_bent() {
    unsigned char* src = passive_buffer;
    unsigned char* dst
        = cthughaDisplay->buffer + BUFF_WIDTH + cthughaDisplay->bufferWidth * BUFF_HEIGHT;
    int i, j;
    static int height[MAX_BUFF_WIDTH];
    static float t = 0;
    static float h = 0.0;
    static float stp = 0.0;

    prepare_3d(256);

    h = h * 0.95 + (min((2 * soundAnalyze.amplitude), 120)) * 0.05;

    for (i = BUFF_WIDTH; i != 0; i--) {
        height[i] = (int)(h * sin(t) * sin((double)(i) / (double)BUFF_WIDTH * 3.0 * M_PI)) + 128;
    }
    stp = stp * 0.95 + max(int(cthughaDisplay->fps * 2), 1) * 0.05;
    t += 4.0 / stp;

    for (i = BUFF_HEIGHT; i != 0; i--) {
        xy t = p;
        for (j = BUFF_WIDTH; j != 0; j--) {
            put_3d_splat(dst, (p.x + height_offset[height[j]].x) >> 16,
                (p.y + height_offset[height[j]].y) >> 16, *src | 128);
            src++;
            p.x += s1.x;
            p.y += s1.y;
        }
        p.x = t.x + s2.x;
        p.y = t.y + s2.y;
    }

    return 0;
}

/*
 * plate: height contant to 0
 */
int screen_plate() {
    unsigned char* src = passive_buffer;
    unsigned char* dst
        = cthughaDisplay->buffer + BUFF_WIDTH + cthughaDisplay->bufferWidth * BUFF_HEIGHT;
    int i, j;

    prepare_3d(1);

    for (i = BUFF_HEIGHT; i != 0; i--) {
        xy t = p;
        for (j = BUFF_WIDTH; j != 0; j--) {
            put_3d_splat(dst, p.x >> 16, p.y >> 16, *src | 128);
            src++;
            p.x += s1.x;
            p.y += s1.y;
        }
        p.x = t.x + s2.x;
        p.y = t.y + s2.y;
    }

    return 0;
}

/*
 * Map the buffer on a cylinder (around x-axis), and roll it.
 * Buffer is mapped 2 times on the cylinder, so that no discontinuities arise
 *
 * other possible things with this idea: waves, ...
 */
int screen_roll() {
    int i;
    static double theta = 0; /* rotation angle */

    for (i = 0; i < BUFF_HEIGHT; i++) {
        int p = i - BUFF_HEIGHT / 2;
        double phi = acos((double)p / (double)(BUFF_HEIGHT / 2));
        int b = (int)((theta + phi) / M_PI * (double)BUFF_HEIGHT);
        b %= 2 * BUFF_HEIGHT;
        if (b < 0)
            b += 2 * BUFF_HEIGHT;
        if (b >= BUFF_HEIGHT)
            b = 2 * BUFF_HEIGHT - b - 1;

        perm_lines[i] = PERM_ADDR(b);
    }
    theta += M_PI / 40.0; /* roate a little bit */
    if (theta > 2.0 * M_PI)
        theta -= 2.0 * M_PI;

    screen_perm();

    return 0;
}

/*
 * this is not looking good
 */
#define ZICK_SMOOTH 10
static int zicks[MAX_BUFF_HEIGHT];
int screen_zick() {
    int i;
    unsigned char* scrn = cthughaDisplay->buffer;
    unsigned char* src = passive_buffer;
    static int first = 1;

    static int d = 0;

    if (first) {
        for (i = 0; i < BUFF_HEIGHT + 1; i++)
            zicks[i] = 0;
    }

    for (i = BUFF_HEIGHT; i != 0; i--) {
        zicks[i] = ((soundDevice->dataProc[i][0]) + ZICK_SMOOTH * zicks[i]) / (ZICK_SMOOTH + 1);
        d = (d + zicks[i]) / 2;
        if (d == 0) {
            memcpy(scrn, src, BUFF_WIDTH);
        } else if (d > 0) {
            memset(scrn, 0, d);
            memcpy(scrn + d, src, BUFF_WIDTH - d);
        } else {
            memcpy(scrn, src - d, BUFF_WIDTH + d);
            memset(scrn + draw_size.x + d, 0, -d);
        }
        src += BUFF_WIDTH;
        scrn += cthughaDisplay->bufferWidth;
    }

    return 0;
}
