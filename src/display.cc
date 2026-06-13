#include "cthugha.h"
#include "display.h"
#include "AudioAnalyzer.h"
#include "TranslationOptions.h"
#include "imath.h"
#include "Screen.h"
#include "ScreenRenderContext.h"

#include <math.h>
#include <vector>

char screen_first[256] = ""; /* Start with this scrn-fkt */

static const double display3DReferenceWidth = 640.0;

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

static int audioAmplitude(ScreenRenderContext& context) {
    const AudioMetrics* metrics = context.audioMetrics();
    return metrics != 0 ? metrics->amplitude : 0;
}

static int processedWaveSample(ScreenRenderContext& context, int sampleIndex) {
    const char2* processedWaveData = context.processedWaveData();
    if (processedWaveData == 0 || sampleIndex < 0 || sampleIndex >= 1024)
        return 0;

    return processedWaveData[sampleIndex][0];
}

static double acousticIntensity(ScreenRenderContext& context) {
    const AcousticContext* acousticContext = context.acousticContext();
    return acousticContext != 0 ? acousticContext->intensity() : 0.0;
}

/*****************************************************************************
 * Screen-functions
 ****************************************************************************/

/*
 * Draw a permutation of the lines.
 *
 * A lot of the display functions are now based on this.
 */
static std::vector<const unsigned char*> perm_lines;

static const unsigned char* perm_addr(ScreenRenderContext& context, int i);

static void prepare_perm_lines(ScreenRenderContext& context) {
    perm_lines.assign(visualBuffer(context).height(), 0);
}

static void screen_perm(ScreenRenderContext& context) {
    unsigned char* scrn = destinationPixels(context);
    int height = visualBuffer(context).height();
    if (int(perm_lines.size()) < height)
        perm_lines.resize(height, 0);

    for (int i = 0; i < height; i++) {
        const unsigned char* line = perm_lines[i] != 0
            ? perm_lines[i]
            : perm_addr(context, i);
        memcpy(scrn, line, visualBuffer(context).width());
        scrn += destinationPitch(context);
    }
}

static const unsigned char* perm_addr(ScreenRenderContext& context, int i) {
    return sourceLine(context, i);
}

int screen_up(ScreenRenderContext& context) {
    int i;
    prepare_perm_lines(context);
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
    prepare_perm_lines(context);
    for (i = 0; i < visualBuffer(context).height(); i++)
        perm_lines[i] = perm_addr(context, visualBuffer(context).height() - i - 1);

    screen_perm(context);
    return 0;
}

int screen_2hor(ScreenRenderContext& context) {
    int i;
    prepare_perm_lines(context);
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
    prepare_perm_lines(context);
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
static int splatSampleStep = 1;
static const int maxSplatSize = 4;
static const int highResolutionSplatPixelThreshold = 1280 * 960;
static const int highResolutionMaxSplatSize = 3;

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

struct SplatTarget {
    unsigned char* center;
    int halfWidth;
    int halfHeight;
    int pitch;
    int size;
    int offset;

    SplatTarget(unsigned char* center_, int halfWidth_, int halfHeight_,
        int pitch_, int size_)
        : center(center_)
        , halfWidth(halfWidth_)
        , halfHeight(halfHeight_)
        , pitch(pitch_)
        , size(size_)
        , offset((size_ - 1) / 2) {
    }
};

inline void write_3d_splat_row(unsigned char* dst, int count,
    unsigned char color) {
    switch (count) {
    case 4:
        dst[3] = color;
        /* fall through */
    case 3:
        dst[2] = color;
        /* fall through */
    case 2:
        dst[1] = color;
        /* fall through */
    case 1:
        dst[0] = color;
        break;
    default:
        break;
    }
}

inline void put_3d_splat(const SplatTarget& target, int x,
    int y, unsigned char color) {
    if (target.size == 1) {
        if ((x >= -target.halfWidth) && (x < target.halfWidth)
            && (y >= -target.halfHeight) && (y < target.halfHeight))
            target.center[y * target.pitch + x] = color;
        return;
    }

    int left = x - target.offset;
    int top = y - target.offset;
    int right = left + target.size;
    int bottom = top + target.size;

    if (right <= -target.halfWidth || left >= target.halfWidth
        || bottom <= -target.halfHeight || top >= target.halfHeight)
        return;

    if (left >= -target.halfWidth && right <= target.halfWidth
        && top >= -target.halfHeight && bottom <= target.halfHeight) {
        unsigned char* row = target.center + top * target.pitch + left;
        for (int yy = 0; yy < target.size; yy++) {
            write_3d_splat_row(row, target.size, color);
            row += target.pitch;
        }
        return;
    }

    if (left < -target.halfWidth)
        left = -target.halfWidth;
    if (right > target.halfWidth)
        right = target.halfWidth;
    if (top < -target.halfHeight)
        top = -target.halfHeight;
    if (bottom > target.halfHeight)
        bottom = target.halfHeight;

    int count = right - left;
    unsigned char* row = target.center + top * target.pitch + left;

    for (int yy = top; yy < bottom; yy++) {
        write_3d_splat_row(row, count, color);
        row += target.pitch;
    }
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

    double intensity = acousticIntensity(context);
    scaleFactor = mid - amp * cos(scaleFactorPhase) + (intensityFactor * intensity);
    splatSize = int(scaleFactor + 0.5);
    if (splatSize < 1)
        splatSize = 1;
    if (splatSize > maxSplatSize)
        splatSize = maxSplatSize;

    int sourcePixels = visualBuffer(context).width() * visualBuffer(context).height();
    splatSampleStep = 1;
    if (sourcePixels >= highResolutionSplatPixelThreshold && splatSize >= 3) {
        splatSampleStep = 2;
        if (splatSize > highResolutionMaxSplatSize)
            splatSize = highResolutionMaxSplatSize;
    }

    CTH_TRACE("update_3d_scale_factor: dt=%.3f, phase=%.3f, scaleFactor=%.3f splat=%d step=%d\n", "display",
        dt, scaleFactorPhase,
        scaleFactor, splatSize, splatSampleStep);
    CTH_TRACE("intensity: %.3f\n", "display", intensity);
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
    z = sqr((float)(visualBuffer(context).width()) / display3DReferenceWidth * (float)maxZ);
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
        height_offset[i].x = int(ro[3][0] * i * double(visualBuffer(context).width()) / display3DReferenceWidth * SC);
        height_offset[i].y = int(ro[3][1] * i * double(visualBuffer(context).width()) / display3DReferenceWidth * SC);
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
    int width = visualBuffer(context).width();
    int height = visualBuffer(context).height();
    int pitch = visualBuffer(context).pitch();
    int step;

    prepare_3d(context, 256);

    SplatTarget target(dst, width, height, destinationPitch(context), splatSize);
    step = splatSampleStep;

    xy rowPoint = p;
    for (int y = 0; y < height; y += step) {
        xy point = rowPoint;
        const unsigned char* row = src + y * pitch;
        for (int x = 0; x < width; x += step) {
            unsigned char color = row[x];
            put_3d_splat(target, (point.x + height_offset[color].x) >> 16,
                (point.y + height_offset[color].y) >> 16, color | 128);
            point.x += s1.x * step;
            point.y += s1.y * step;
        }
        rowPoint.x += s2.x * step;
        rowPoint.y += s2.y * step;
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
    int width = visualBuffer(context).width();
    int sourceHeight = visualBuffer(context).height();
    int pitch = visualBuffer(context).pitch();
    int step;
    static std::vector<int> bendHeight;
    static float t = 0;
    static float h = 0.0;
    static float stp = 0.0;

    prepare_3d(context, 256);

    h = h * 0.95 + (min((2 * audioAmplitude(context)), 120)) * 0.05;

    bendHeight.resize(width + 1);
    for (int i = width; i != 0; i--) {
        bendHeight[i] = (int)(h * sin(t) * sin((double)(i) / (double)width * 3.0 * M_PI)) + 128;
    }
    stp = stp * 0.95 + max(int(visualFramesPerSecond(context) * 2), 1) * 0.05;
    t += 4.0 / stp;

    SplatTarget target(dst, width, sourceHeight, destinationPitch(context), splatSize);
    step = splatSampleStep;

    xy rowPoint = p;
    for (int y = 0; y < sourceHeight; y += step) {
        xy point = rowPoint;
        const unsigned char* row = src + y * pitch;
        for (int x = 0; x < width; x += step) {
            int heightIndex = width - x;
            unsigned char color = row[x];
            put_3d_splat(target, (point.x + height_offset[bendHeight[heightIndex]].x) >> 16,
                (point.y + height_offset[bendHeight[heightIndex]].y) >> 16, color | 128);
            point.x += s1.x * step;
            point.y += s1.y * step;
        }
        rowPoint.x += s2.x * step;
        rowPoint.y += s2.y * step;
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
    int width = visualBuffer(context).width();
    int height = visualBuffer(context).height();
    int pitch = visualBuffer(context).pitch();
    int step;

    prepare_3d(context, 1);

    SplatTarget target(dst, width, height, destinationPitch(context), splatSize);
    step = splatSampleStep;

    xy rowPoint = p;
    for (int y = 0; y < height; y += step) {
        xy point = rowPoint;
        const unsigned char* row = src + y * pitch;
        for (int x = 0; x < width; x += step) {
            put_3d_splat(target, point.x >> 16, point.y >> 16, row[x] | 128);
            point.x += s1.x * step;
            point.y += s1.y * step;
        }
        rowPoint.x += s2.x * step;
        rowPoint.y += s2.y * step;
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

    prepare_perm_lines(context);
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
int screen_zick(ScreenRenderContext& context) {
    int i;
    unsigned char* scrn = destinationPixels(context);
    const unsigned char* src = sourcePixels(context);
    static std::vector<int> zicks;

    static int d = 0;

    zicks.assign(visualBuffer(context).height() + 1, 0);

    for (i = visualBuffer(context).height(); i != 0; i--) {
        zicks[i] = (processedWaveSample(context, i) + ZICK_SMOOTH * zicks[i]) / (ZICK_SMOOTH + 1);
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
