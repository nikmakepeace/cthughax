#include "cthugha.h"
#include "display.h"
#include "AudioFrame.h"
#include "AudioAnalyzer.h"
#include "TranslationOptions.h"
#include "imath.h"
#include "cth_buffer.h"
#include "Screen.h"
#include "ScreenRenderContext.h"

#include <math.h>

char screen_first[256] = ""; /* Start with this scrn-fkt */

class VisualFrameView {
    ScreenRenderContext& context;

public:
    explicit VisualFrameView(ScreenRenderContext& context_)
        : context(context_) {
    }

    int width() const {
        return context.sourceWidth();
    }

    int height() const {
        return context.sourceHeight();
    }

    int pitch() const {
        return context.sourcePitch();
    }

    int size() const {
        return width() * height();
    }
};

static VisualFrameView visualBuffer(ScreenRenderContext& context) {
    return VisualFrameView(context);
}

static const unsigned char* sourcePixels(ScreenRenderContext& context) {
    return context.sourcePixels();
}

static const unsigned char* sourceLine(ScreenRenderContext& context, int line) {
    return context.sourceLine(line);
}

static unsigned char* destinationPixels(ScreenRenderContext& context) {
    return context.destinationPixels();
}

static int destinationPitch(ScreenRenderContext& context) {
    return context.destinationPitch();
}

static double visualFramesPerSecond(ScreenRenderContext& context) {
    return context.framesPerSecond();
}

static double visualDeltaTime(ScreenRenderContext& context) {
    return context.deltaTimeSeconds();
}

/*****************************************************************************
 * Screen-functions
 ****************************************************************************/

/*
 * Draw a permutation of the lines.
 *
 * A lot of the display functions are now based on this.
 */
static const unsigned char* perm_lines[MAX_BUFF_HEIGHT];

static void screen_perm(ScreenRenderContext& context) {
    unsigned char* scrn = destinationPixels(context);
    const unsigned char** perm = perm_lines;
    int i;

    for (i = visualBuffer(context).height(); i != 0; i--) {
        memcpy(scrn, *perm, visualBuffer(context).width());
        scrn += destinationPitch(context);
        perm++;
    }
}

static const unsigned char* perm_addr(ScreenRenderContext& context, int i) {
    return sourceLine(context, i);
}

int screen_up(ScreenRenderContext& context) {
    int i;
    for (i = 0; i < visualBuffer(context).height(); i++)
        perm_lines[i] = perm_addr(context, i);

    screen_perm(context);

    return 0;
}

int screen_source(ScreenRenderContext& context) {
    return screen_up(context);
}

int screen_down(ScreenRenderContext& context) {
    int i;
    for (i = 0; i < visualBuffer(context).height(); i++)
        perm_lines[i] = perm_addr(context, visualBuffer(context).height() - i - 1);

    screen_perm(context);
    return 0;
}

int screen_2hor(ScreenRenderContext& context) {
    int i;
    for (i = 0; i < visualBuffer(context).height() / 2; i++) {
        /* lower half of buffer maps to upper half of screen */
        perm_lines[i] = perm_addr(context, visualBuffer(context).height() / 2 + i);
        /* lower half of buffer get turned around to lower half of screen*/
        perm_lines[i + visualBuffer(context).height() / 2] = perm_addr(context, visualBuffer(context).height() - i - 1);
    }

    screen_perm(context);
    return 0;
}

int screen_r2hor(ScreenRenderContext& context) {
    int i;
    for (i = 0; i < visualBuffer(context).height() / 2; i++) {
        /* lower half of buffer get turned around to upper half of screen*/
        perm_lines[i] = perm_addr(context, visualBuffer(context).height() - i - 1);
        /* lower half of buffer maps to lower half of screen */
        perm_lines[i + visualBuffer(context).height() / 2] = perm_addr(context, visualBuffer(context).height() / 2 + i);
    }

    screen_perm(context);
    return 0;
}

int screen_4hor(ScreenRenderContext& context) {
    const unsigned char* tmp;
    unsigned char* scrn;
    int x, y;

    /* upper half of screen */
    tmp = sourceLine(context, visualBuffer(context).height() / 2);
    scrn = destinationPixels(context);
    for (y = visualBuffer(context).height() / 2; y != 0; y--) {

        /* left half */
        memcpy(scrn, tmp, visualBuffer(context).width() / 2);
        scrn += visualBuffer(context).width() / 2;
        tmp += visualBuffer(context).width() / 2;

        /* right half */
        for (x = visualBuffer(context).width() / 2; x != 0; x--) {
            *scrn = *tmp;
            scrn++;
            tmp--;
        }
        tmp += visualBuffer(context).pitch();
        scrn += destinationPitch(context) - visualBuffer(context).width();
    }

    /* lower half of screen */
    tmp = sourceLine(context, visualBuffer(context).height() - 1);
    scrn = destinationPixels(context) + destinationPitch(context) * visualBuffer(context).height() / 2;
    for (y = visualBuffer(context).height() / 2; y != 0; y--) {

        /* left half */
        memcpy(scrn, tmp, visualBuffer(context).width() / 2);
        scrn += visualBuffer(context).width() / 2;
        tmp += visualBuffer(context).width() / 2;

        /* right half */
        for (x = visualBuffer(context).width() / 2; x != 0; x--) {
            *scrn = *tmp;
            scrn++;
            tmp--;
        }
        tmp -= visualBuffer(context).pitch();
        scrn += destinationPitch(context) - visualBuffer(context).width();
    }

    return 0;
}

int screen_2verd(ScreenRenderContext& context) {
    if (visualBuffer(context).width() / 2 <= visualBuffer(context).height()) {
        int x, y;
        const unsigned char* tmp = sourcePixels(context);
        unsigned char* scrn = destinationPixels(context);
        for (y = visualBuffer(context).height(); y != 0; y--) {
            for (x = visualBuffer(context).width() / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn++;
                tmp += visualBuffer(context).pitch();
            }
            for (x = visualBuffer(context).width() / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn++;
                tmp -= visualBuffer(context).pitch();
            }
            tmp++;
            scrn += destinationPitch(context) - visualBuffer(context).width();
        }

    } else {
        return 1;
    }
    return 0;
}

int screen_r2verd(ScreenRenderContext& context) {
    if (visualBuffer(context).width() / 2 <= visualBuffer(context).height()) {
        int x, y;
        const unsigned char* tmp = sourcePixels(context);
        unsigned char* scrn = destinationPixels(context)
            + destinationPitch(context) * (visualBuffer(context).height() - 1) + (visualBuffer(context).width() - 1);
        for (y = visualBuffer(context).height(); y != 0; y--) {
            for (x = visualBuffer(context).width() / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn--;
                tmp += visualBuffer(context).pitch();
            }
            for (x = visualBuffer(context).width() / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn--;
                tmp -= visualBuffer(context).pitch();
            }
            tmp++;
            scrn -= destinationPitch(context) - visualBuffer(context).width();
        }
    } else {
        return 1;
    }
    return 0;
}

int screen_4kal(ScreenRenderContext& context) {
    if (visualBuffer(context).width() / 2 <= visualBuffer(context).height()) {
        const unsigned char* tmp;
        unsigned char* scrn;
        int x, y;

        tmp = sourcePixels(context);
        scrn = destinationPixels(context);
        /* upper half */
        for (y = visualBuffer(context).height() / 2; y != 0; y--) {
            for (x = visualBuffer(context).width() / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn++;
                tmp += visualBuffer(context).pitch();
            }
            for (x = visualBuffer(context).width() / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn++;
                tmp -= visualBuffer(context).pitch();
            }
            tmp++;
            scrn += destinationPitch(context) - visualBuffer(context).width();
        }

        tmp = sourcePixels(context);
        scrn = destinationPixels(context) + destinationPitch(context) * (visualBuffer(context).height() - 1) + visualBuffer(context).width()
            - 1;
        /* lower half */
        for (y = visualBuffer(context).height() / 2; y != 0; y--) {
            for (x = visualBuffer(context).width() / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn--;
                tmp += visualBuffer(context).pitch();
            }
            for (x = visualBuffer(context).width() / 2; x != 0; x--) {
                *scrn = *tmp;
                scrn--;
                tmp -= visualBuffer(context).pitch();
            }
            tmp++;
            scrn -= destinationPitch(context) - visualBuffer(context).width();
        }
    } else {
        return 1;
    }
    return 0;
}

int screen_vscale_hmirror(ScreenRenderContext& context) {
    return screen_up(context);
}

int screen_hscale_vmirror(ScreenRenderContext& context) {
    return screen_up(context);
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

inline void put_3d_pixel(ScreenRenderContext& context, unsigned char* dst, int x,
    int y, unsigned char color) {
    if ((x >= -visualBuffer(context).width()) && (x < visualBuffer(context).width()) && (y >= -visualBuffer(context).height()) && (y < visualBuffer(context).height()))
        dst[y * destinationPitch(context) + x] = color;
}

inline void put_3d_splat(ScreenRenderContext& context, unsigned char* dst, int x,
    int y, unsigned char color) {
    int offset = (splatSize - 1) / 2;

    for (int yy = 0; yy < splatSize; yy++)
        for (int xx = 0; xx < splatSize; xx++)
            put_3d_pixel(context, dst, x + xx - offset, y + yy - offset, color);
}

void update_3d_scale_factor(ScreenRenderContext& context) {
    double dt = visualDeltaTime(context);
    double mid = (minScaleFactor + maxScaleFactor) / 2.0;
    double amp = (maxScaleFactor - minScaleFactor) / 2.0;

    if (dt < 0.0)
        dt = 0.0;
    if (dt > 0.25)
        dt = 0.25;

    scaleFactorPhase += 2.0 * M_PI * scaleFactorSpeed * dt;
    if (scaleFactorPhase >= 2.0 * M_PI)
        scaleFactorPhase = fmod(scaleFactorPhase, 2.0 * M_PI);

    scaleFactor = mid - amp * cos(scaleFactorPhase) + (intensityFactor * acousticContext.intensity());
    splatSize = int(scaleFactor + 0.5);
    if (splatSize < 1)
        splatSize = 1;
    if (splatSize > maxSplatSize)
        splatSize = maxSplatSize;

    CTH_TRACE("update_3d_scale_factor: dt=%.3f, phase=%.3f, scaleFactor=%.3f\n", "display",
        dt, scaleFactorPhase,
        scaleFactor);
    CTH_TRACE("intensity: %.3f\n", "display", acousticContext.intensity());
}

/*
 * preparations for a 3D display
 */
int prepare_3d(ScreenRenderContext& context, int maxZ) {
    int i;
    float x, y, z, l;
    float ro[4][3];
    double scale;

    /* calculate the maximal size of the "sheet" */
    x = sqr(visualBuffer(context).width() / 2);
    y = sqr(visualBuffer(context).height() / 2);
    z = sqr((float)(visualBuffer(context).width()) / 640.0 * (float)maxZ);
    l = sqrt(x + y + z);

    update_3d_scale_factor(context);
    scale = scaleFactor * (float)min(visualBuffer(context).width(), visualBuffer(context).height()) / l;

    /* rotate a little bit */
    /* TODO: use some value from the sound to set speed. */
    double dt = visualDeltaTime(context);
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
        height_offset[i].x = int(ro[3][0] * i * double(visualBuffer(context).width()) / 640.0 * SC);
        height_offset[i].y = int(ro[3][1] * i * double(visualBuffer(context).width()) / 640.0 * SC);
    }

    /* calculate the step sizes */
    s1.x = int((ro[1][0] - ro[0][0]) * scale * SC / 2.0);
    s1.y = int((ro[1][1] - ro[0][1]) * scale * SC / 2.0);

    s2.x = int((ro[2][0] - ro[1][0]) * scale * SC / 2.0);
    s2.y = int((ro[2][1] - ro[1][1]) * scale * SC / 2.0);

    /* starting point */
    p.x = -visualBuffer(context).width() / 2 * s1.x - visualBuffer(context).height() / 2 * s2.x;
    p.y = -visualBuffer(context).width() / 2 * s1.y - visualBuffer(context).height() / 2 * s2.y;

    /* clear screen */
    unsigned char* clr = destinationPixels(context);
    for (i = 2 * visualBuffer(context).height(); i != 0; i--) {
        memset(clr, '\0', 2 * visualBuffer(context).width());
        clr += destinationPitch(context);
    }

    return 0;
}

/*
 * hfield: buffer_value determines height
 */
int screen_hfield(ScreenRenderContext& context) {
    const unsigned char* src = sourcePixels(context);
    unsigned char* dst
        = destinationPixels(context) + visualBuffer(context).width() + destinationPitch(context) * visualBuffer(context).height();
    int i, j;

    prepare_3d(context, 256);

    for (i = visualBuffer(context).height(); i != 0; i--) {
        xy t = p;
        for (j = visualBuffer(context).width(); j != 0; j--) {
            put_3d_splat(context, dst, (p.x + height_offset[*src].x) >> 16,
                (p.y + height_offset[*src].y) >> 16, *src | 128);
            src++;
            p.x += s1.x;
            p.y += s1.y;
        }
        src += visualBuffer(context).pitch() - visualBuffer(context).width();
        p.x = t.x + s2.x;
        p.y = t.y + s2.y;
    }

    return 0;
}

/*
 * bent: height is a sine-wave along x axis
 */
int screen_bent(ScreenRenderContext& context) {
    const unsigned char* src = sourcePixels(context);
    unsigned char* dst
        = destinationPixels(context) + visualBuffer(context).width() + destinationPitch(context) * visualBuffer(context).height();
    int i, j;
    static int height[MAX_BUFF_WIDTH];
    static float t = 0;
    static float h = 0.0;
    static float stp = 0.0;

    prepare_3d(context, 256);

    h = h * 0.95 + (min((2 * audioFrameMetrics().amplitude), 120)) * 0.05;

    for (i = visualBuffer(context).width(); i != 0; i--) {
        height[i] = (int)(h * sin(t) * sin((double)(i) / (double)visualBuffer(context).width() * 3.0 * M_PI)) + 128;
    }
    stp = stp * 0.95 + max(int(visualFramesPerSecond(context) * 2), 1) * 0.05;
    t += 4.0 / stp;

    for (i = visualBuffer(context).height(); i != 0; i--) {
        xy t = p;
        for (j = visualBuffer(context).width(); j != 0; j--) {
            put_3d_splat(context, dst, (p.x + height_offset[height[j]].x) >> 16,
                (p.y + height_offset[height[j]].y) >> 16, *src | 128);
            src++;
            p.x += s1.x;
            p.y += s1.y;
        }
        src += visualBuffer(context).pitch() - visualBuffer(context).width();
        p.x = t.x + s2.x;
        p.y = t.y + s2.y;
    }

    return 0;
}

/*
 * plate: height contant to 0
 */
int screen_plate(ScreenRenderContext& context) {
    const unsigned char* src = sourcePixels(context);
    unsigned char* dst
        = destinationPixels(context) + visualBuffer(context).width() + destinationPitch(context) * visualBuffer(context).height();
    int i, j;

    prepare_3d(context, 1);

    for (i = visualBuffer(context).height(); i != 0; i--) {
        xy t = p;
        for (j = visualBuffer(context).width(); j != 0; j--) {
            put_3d_splat(context, dst, p.x >> 16, p.y >> 16, *src | 128);
            src++;
            p.x += s1.x;
            p.y += s1.y;
        }
        src += visualBuffer(context).pitch() - visualBuffer(context).width();
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
int screen_roll(ScreenRenderContext& context) {
    int i;
    static double theta = 0; /* rotation angle */

    for (i = 0; i < visualBuffer(context).height(); i++) {
        int p = i - visualBuffer(context).height() / 2;
        double phi = acos((double)p / (double)(visualBuffer(context).height() / 2));
        int b = (int)((theta + phi) / M_PI * (double)visualBuffer(context).height());
        b %= 2 * visualBuffer(context).height();
        if (b < 0)
            b += 2 * visualBuffer(context).height();
        if (b >= visualBuffer(context).height())
            b = 2 * visualBuffer(context).height() - b - 1;

        perm_lines[i] = perm_addr(context, b);
    }
    theta += M_PI / 40.0; /* roate a little bit */
    if (theta > 2.0 * M_PI)
        theta -= 2.0 * M_PI;

    screen_perm(context);

    return 0;
}

/*
 * this is not looking good
 */
#define ZICK_SMOOTH 10
static int zicks[MAX_BUFF_HEIGHT];
int screen_zick(ScreenRenderContext& context) {
    int i;
    unsigned char* scrn = destinationPixels(context);
    const unsigned char* src = sourcePixels(context);
    static int first = 1;

    static int d = 0;

    if (first) {
        for (i = 0; i < visualBuffer(context).height() + 1; i++)
            zicks[i] = 0;
    }

    for (i = visualBuffer(context).height(); i != 0; i--) {
        zicks[i] = ((audioFrameProcessedWaveData()[i][0]) + ZICK_SMOOTH * zicks[i]) / (ZICK_SMOOTH + 1);
        d = (d + zicks[i]) / 2;
        if (d == 0) {
            memcpy(scrn, src, visualBuffer(context).width());
        } else if (d > 0) {
            memset(scrn, 0, d);
            memcpy(scrn + d, src, visualBuffer(context).width() - d);
        } else {
            memcpy(scrn, src - d, visualBuffer(context).width() + d);
            memset(scrn + visualBuffer(context).width() + d, 0, -d);
        }
        src += visualBuffer(context).pitch();
        scrn += destinationPitch(context);
    }

    return 0;
}
