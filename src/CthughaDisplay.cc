#include "cthugha.h"
#include "CthughaDisplay.h"
#include "CthughaBuffer.h"
#include "display.h"
#include "DisplayDevice.h"
#include "cth_buffer.h"
#include "disp-sys.h"
#include "imath.h"
#include "Interface.h"
#include "IndexedFrame.h"

#include <stdint.h>
#include <unistd.h>

// The active display coordinator.  The selected frontend supplies
// newCthughaDisplay(), so this points at the X11 display subclass.
CthughaDisplay* cthughaDisplay = NULL;

CthughaDisplay::~CthughaDisplay() { }

OptionInt maxFramesPerSecond("maxFPS", DEFAULT_MAX_FRAMES_PER_SECOND);

OptionInt zoom("zoom", DEFAULT_ZOOM_MODE, ZOOM_MODE_MAX_EXCLUSIVE);
xy draw_size(0, 0); /* size of the drawn image (including zoom) */

double displayStart;

// Frame clock shared by animation, sound processing, and display effects.
// nextFrame() updates these before the rest of the frame runs.
double now = 0;
double deltaT = 0;

class VisualFrameView {
public:
    int width() const {
        return cthughaDisplay->sourceWidth();
    }

    int height() const {
        return cthughaDisplay->sourceHeight();
    }

    int size() const {
        return width() * height();
    }

    int displayWidth() const {
        return cthughaDisplay->displayFrameWidth();
    }

    int displayHeight() const {
        return cthughaDisplay->displayFrameHeight();
    }
};

static VisualFrameView visualBuffer() {
    return VisualFrameView();
}

CthughaDisplay::CthughaDisplay()
    : sourceFrame(0)
    , indexedDisplayFrameValue()
    , buffer0(0)
    , displayStart(0)
    , frames(0)
    , visualLatencyEstimate(0)
    , buffer(0)
    , bufferWidth(0)
    , expandedBuffer(0)
    , expandedBufferWidth(0)
    , needsClear(1)
    , fps(0) {

    displayStart = getTime();
}

void CthughaDisplay::present(const IndexedFrame& frame) {
    if (!frame.valid()) {
        sourceFrame = NULL;
        return;
    }

    sourceFrame = &frame;
    if (displayDevice != NULL && frame.framePalette != NULL)
        displayDevice->setFramePalette(frame.framePalette);
    (*this)();
}

const unsigned char* CthughaDisplay::sourcePixels() const {
    if (sourceFrame != NULL && sourceFrame->valid())
        return sourceFrame->pixels;

    // Legacy fallback for code paths that still call operator() without first
    // publishing an IndexedFrame through present().
    return CthughaBuffer::current->passivePixels();
}

int CthughaDisplay::sourceWidth() const {
    if (sourceFrame != NULL && sourceFrame->valid())
        return sourceFrame->width;

    return CthughaBuffer::current->width();
}

int CthughaDisplay::sourceHeight() const {
    if (sourceFrame != NULL && sourceFrame->valid())
        return sourceFrame->height;

    return CthughaBuffer::current->height();
}

int CthughaDisplay::sourcePitch() const {
    if (sourceFrame != NULL && sourceFrame->valid())
        return sourceFrame->pitch;

    return CthughaBuffer::current->width();
}

int CthughaDisplay::sourceSize() const {
    return sourceWidth() * sourceHeight();
}

const IndexedDisplayFrame& CthughaDisplay::indexedDisplayFrame() const {
    return indexedDisplayFrameValue;
}

int CthughaDisplay::displayFrameWidth() const {
    if (indexedDisplayFrameValue.valid())
        return indexedDisplayFrameValue.width();

    return 2 * sourceWidth();
}

int CthughaDisplay::displayFrameHeight() const {
    if (indexedDisplayFrameValue.valid())
        return indexedDisplayFrameValue.height();

    return 2 * sourceHeight();
}

void CthughaDisplay::prepareIndexedDisplayFrame(int width, int height) {
    if (width <= 0 || height <= 0)
        return;

    unsigned char* oldPixels = indexedDisplayFrameValue.pixels();
    int oldWidth = indexedDisplayFrameValue.width();
    int oldHeight = indexedDisplayFrameValue.height();
    int oldPitch = indexedDisplayFrameValue.pitch();
    int requiredByteCount = width * height;
    int willMove = requiredByteCount > indexedDisplayFrameValue.capacityByteCount();

    if (willMove)
        indexedBufferWillChange(oldPixels);
    indexedDisplayFrameValue.resize(width, height);
    indexedDisplayFrameValue.setFramePalette(sourceFrame != NULL ? sourceFrame->framePalette : NULL);
    buffer0 = indexedDisplayFrameValue.pixels();

    if (indexedDisplayFrameValue.pixels() != oldPixels || indexedDisplayFrameValue.width() != oldWidth
        || indexedDisplayFrameValue.height() != oldHeight || indexedDisplayFrameValue.pitch() != oldPitch)
        needsClear = 1;
}

/*
 * do the horizontal mirroring
 * this is done in the buffer
 */
void CthughaDisplay::mirrorHorizontally(int height) {
    unsigned char* src = buffer;
    unsigned char* dst = buffer + (visualBuffer().displayWidth() - 1);

    for (int i = height; i != 0; i--) {
        for (int j = visualBuffer().width(); j != 0; j--) {
            *dst = *src;
            src++;
            dst--;
        }
        src += bufferWidth - visualBuffer().width();
        dst += bufferWidth + visualBuffer().width();
    }
}

/*
 * do the vertical mirroring
 * this is done in the expandedBuffer
 */
void CthughaDisplay::mirrorVertically() {
    unsigned char* src = expandedBuffer;
    unsigned char* dst = expandedBuffer + (visualBuffer().displayHeight() - 1) * expandedBufferWidth;

    for (int i = visualBuffer().height(); i != 0; i--) {
        memcpy(dst, src, visualBuffer().displayWidth() * bypp);
        src += expandedBufferWidth;
        dst -= expandedBufferWidth;
    }
}

/*
 * clear the border around the image on the screen
 */
int CthughaDisplay::clearBorder() {

    if (displayDevice->textOnScreen || needsClear) {
        int bwidth = SCREEN_OFFSET_X; /* width of left/right border */
        int bheight = SCREEN_OFFSET_Y; /* height of upper/lower border */

        /* upper border */
        displayDevice->clearBox(0, 0, disp_size.x, bheight);
        /* left border */
        displayDevice->clearBox(0, bheight, bwidth, draw_size.y);
        /* right border */
        displayDevice->clearBox(disp_size.x - bwidth, bheight, bwidth, draw_size.y);
        /* lower border */
        displayDevice->clearBox(0, disp_size.y - bheight, disp_size.x, bheight);

        /* if text is on screen, we have to clean in the next iteration,
           because the text may be removed then */
        needsClear = (displayDevice->textOnScreen) ? 1 : 0;

        // Border clearing touches pixels outside the normal image rectangle,
        // so partial-copy display devices must refresh the full frame.
        displayDevice->needsFullCopy = 1;
    }
    return 0;
}

/*
 * copy the expandedBuffer to the screen
 */
void CthughaDisplay::zoom2Screen(unsigned char* scrn, int bytesPerLine) {
    unsigned char* b1 = expandedBuffer;
    unsigned short* b2 = (unsigned short*)(expandedBuffer);
    uint32_t* b4 = (uint32_t*)(expandedBuffer);

    unsigned char* s1 = (unsigned char*)(scrn);
    unsigned short* s2 = (unsigned short*)(scrn);
    uint32_t* s4 = (uint32_t*)(scrn);

    int bpl2 = bytesPerLine / 2;
    int bpl4 = bytesPerLine / 4;

    if ((zoom != 1) && (disp_size.x == visualBuffer().displayWidth()) && (disp_size.y == visualBuffer().displayHeight())) {
        CTH_INFO("Display size matches buffer size. setting zoom to 1\n");
        zoom.setValue(1);
    }

    if ((zoom != 1) && (bypp != 1) && (bypp != 2) && (bypp != 4)) {
        CTH_ERROR("Sorry zooming is only implements for 1,2 and 4 bytes/pixel displays.");
        zoom.setValue(1);
    }

    switch (zoom) {
    case 0: {
        //
        // zoom 0: zoom, so that the image fits the screen
        //
        int dx = int(65536.0 * double(visualBuffer().displayWidth()) / double(disp_size.x));
        double dy = double(visualBuffer().displayHeight()) / double(disp_size.y);

        switch (bypp) {
        case 1:
            for (int i = 0; i < disp_size.y; i++) {
                b1 = (unsigned char*)(expandedBuffer + int(dy * double(i)) * expandedBufferWidth);
                int x = 0;

                for (int j = disp_size.x; j != 0; j--) {
                    *s1 = *(b1 + (x >> 16));
                    s1++;
                    x += dx;
                }
                s1 = s1 + bytesPerLine - disp_size.x;
            }
            break;
        case 2:
            for (int i = 0; i < disp_size.y; i++) {
                b2 = (unsigned short*)(expandedBuffer + int(dy * double(i)) * expandedBufferWidth);
                int x = 0;

                for (int j = disp_size.x; j != 0; j--) {
                    *s2 = *(b2 + (x >> 16));
                    s2++;
                    x += dx;
                }
                s2 = s2 + bpl2 - disp_size.x;
            }
            break;
        case 4:
            for (int i = 0; i < disp_size.y; i++) {
                b4 = (uint32_t*)(expandedBuffer + int(dy * double(i)) * expandedBufferWidth);
                int x = 0;

                for (int j = disp_size.x; j != 0; j--) {
                    *s4 = *(b4 + (x >> 16));
                    s4++;
                    x += dx;
                }
                s4 = s4 + bpl4 - disp_size.x;
            }
            break;
        }
        break;
    }
    case 1:
        //
        // no zooming, just copy
        // this works for all bypp
        //
        for (int i = visualBuffer().displayHeight(); i != 0; i--) {
            memcpy(scrn, b1, visualBuffer().displayWidth() * bypp);
            scrn += bytesPerLine;
            b1 += expandedBufferWidth;
        }
        break;
    case 2:
        //
        // zoom by a factor of 2
        //
        switch (bypp) {
        case 1:
            for (int i = visualBuffer().displayHeight(); i != 0; i--) {
                for (int j = visualBuffer().displayWidth(); j != 0; j--) {
                    unsigned short s = (*b1) | ((unsigned short)(*b1) << 8);
                    *s2 = s;
                    *(s2 + bpl2) = s;
                    s2++;
                    b1++;
                }
                s2 += bpl2 + bpl2 - visualBuffer().displayWidth();
                b1 += expandedBufferWidth - visualBuffer().displayWidth();
            }
            break;
        case 2:
            for (int i = visualBuffer().displayHeight(); i != 0; i--) {
                for (int j = visualBuffer().displayWidth(); j != 0; j--) {
                    uint32_t s = (*b2) | ((uint32_t)(*b2) << 16);
                    *s4 = s;
                    *(s4 + bpl4) = s;
                    s4++;
                    b2++;
                }
                s4 += bpl4 + bpl4 - visualBuffer().displayWidth();
                b2 += expandedBufferWidth / 2 - visualBuffer().displayWidth();
            }
            break;
        case 4:
            for (int i = visualBuffer().displayHeight(); i != 0; i--) {
                for (int j = visualBuffer().displayWidth(); j != 0; j--) {
                    uint32_t s = *b4;
                    *s4 = s;
                    *(s4 + 1) = s;
                    *(s4 + bpl4) = s;
                    *(s4 + bpl4 + 1) = s;
                    s4 += 2;
                    b4++;
                }
                s4 += bpl4 + bpl4 - 2 * visualBuffer().displayWidth();
                b4 += expandedBufferWidth / 4 - visualBuffer().displayWidth();
            }
            break;
        }
        break;
    }
}

void CthughaDisplay::checkZoom() {
    /*
     * compute the size of the drawing area, and check for sane zoom value
     */
    if (zoom == 0) { /* use optimal size */
        draw_size = disp_size;
    } else { /* use a specified scaling factor */
        static int oldZoom = -1;
        if (zoom != oldZoom) {
            oldZoom = zoom;
            // A changed fixed zoom can expose areas that were image pixels in
            // the previous frame, so clear the border once before drawing.
            needsClear = 1;
        }

        draw_size.x = zoom * visualBuffer().displayWidth();
        draw_size.y = zoom * visualBuffer().displayHeight();
        while ((draw_size.x > disp_size.x) || (draw_size.y > disp_size.y)) {
            CTH_ERROR("Zoom factor is set too high for current display size. reducing.\n");
            zoom.setValue(zoom / 2);
            draw_size.x = zoom * visualBuffer().displayWidth();
            draw_size.y = zoom * visualBuffer().displayHeight();
        }
    }
}

void CthughaDisplay::checkFPS() {
    /*
     * compute frames/second, of at least 3 frames or 0.1 second
     */
    double i = now - displayStart;
    if ((i > 0.1) && (frames > 2))
        fps = double(frames) / i;
    frames++;

    // Enforce maxFPS after measuring deltaT so the rest of the program sees
    // the true time between frame starts.
    if (maxFramesPerSecond) {
        double delta = (1.0 / maxFramesPerSecond) - deltaT;
        double sleepStart = getTime();
        double sleepEnd = sleepStart;
        if (delta > 0) {
            usleep(int(delta * 1e6));
            sleepEnd = getTime();
        }
        CTH_TRACE("checkFPS maxfps=%d deltaT-ms=%.3f requested-sleep-ms=%.3f actual-sleep-ms=%.3f fps=%.3f frames=%d\n",
            "frame pacing", int(maxFramesPerSecond), deltaT * 1000.0,
            (delta > 0 ? delta : 0.0) * 1000.0, (sleepEnd - sleepStart) * 1000.0,
            fps, frames);
    } else {
        CTH_TRACE("checkFPS maxfps=0 deltaT-ms=%.3f requested-sleep-ms=0.000 actual-sleep-ms=0.000 fps=%.3f frames=%d\n",
            "frame pacing", deltaT * 1000.0, fps, frames);
    }
}

void CthughaDisplay::resetFPS() {
    displayStart = getTime(); // restart the averaging window from this instant
    frames = 0;
}

void CthughaDisplay::observeVisualLatency(double seconds) {
    double previous = visualLatencyEstimate;
    double alpha = 0.1;

    if (seconds < 0)
        seconds = 0;

    if (visualLatencyEstimate <= 0)
        visualLatencyEstimate = seconds;
    else
        visualLatencyEstimate = visualLatencyEstimate * (1.0 - alpha) + seconds * alpha;

    CTH_TRACE("visual-latency observed-ms=%.3f previous-ms=%.3f alpha=%.3f estimate-ms=%.3f\n",
        "display timing", seconds * 1000.0, previous * 1000.0, alpha,
        visualLatencyEstimate * 1000.0);
}

double CthughaDisplay::visualLatencySeconds() const {
    return visualLatencyEstimate;
}

// Start a new frame by publishing a stable timestamp for all modules that run
// during this frame, then update FPS accounting and throttling.
void CthughaDisplay::nextFrame() {

    double previousNow = now;
    double nower = getTime();
    deltaT = nower - now;
    now = nower;
    CTH_TRACE("nextFrame previous-now=%.6f sampled-now=%.6f raw-delta-ms=%.3f\n",
        "frame pacing", previousNow, nower, deltaT * 1000.0);

    double checkStart = getTime();
    checkFPS();
    CTH_TRACE("nextFrame checkFPS-ms=%.3f published-now=%.6f published-delta-ms=%.3f\n",
        "frame pacing", (getTime() - checkStart) * 1000.0, now, deltaT * 1000.0);
}

const char* CthughaDisplay::status() {
    static char txt[512];

    sprintf(txt, "fps: %5.2f ", fps);
    return txt;
}
