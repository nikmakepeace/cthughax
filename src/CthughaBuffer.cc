#include "cthugha.h"
#include "CthughaBuffer.h"
#include "translate.h"
#include "waves.h"
#include "display.h"
#include "CthughaDisplay.h"
#include "flames.h"
#include "imath.h"

CthughaBuffer CthughaBuffer::buffer;

CthughaBuffer* CthughaBuffer::current = &CthughaBuffer::buffer;

CthughaBuffer::CthughaBuffer()
    : activeBuffer(0)
    , passiveBuffer(0)
    , widthValue(160)
    , heightValue(100) { }

int CthughaBuffer::width() const {
    return widthValue;
}

int CthughaBuffer::height() const {
    return heightValue;
}

int CthughaBuffer::pitch() const {
    return widthValue;
}

int CthughaBuffer::size() const {
    return widthValue * heightValue;
}

int CthughaBuffer::bottom() const {
    return heightValue - 1;
}

int CthughaBuffer::maxDimension() const {
    return max(widthValue, heightValue);
}

void CthughaBuffer::setDimensions(int width_, int height_) {
    widthValue = width_;
    heightValue = height_;
}

void CthughaBuffer::init() {

    /* The buffer has a border of 3 lines on top on bottom. These
       lines are set to some boundary value before the 'flame' function is called.
       The border is not displayed to the screen
       */
    activeBuffer = new unsigned char[size() + 6 * pitch()] + 3 * pitch();
    passiveBuffer = new unsigned char[size() + 6 * pitch()] + 3 * pitch();

    /* clear buffers */
    memset(activeBuffer, 0, size());
    memset(passiveBuffer, 0, size());
}

int CthughaBuffer::initAll() {

    current = &buffer;

    //
    // add the default entries
    //
    flame.add(_flames, _nFlames);

    if (init_flames())
        return 1;

    if (init_translate(buffer))
        return 1;

    if (init_wave())
        return 1;

    if (load_palettes())
        return 1;

    // allocate memory for the buffers
    buffer.init();
    return 0;
}

void CthughaBuffer::swapBuffers() {
    unsigned char* t = activeBuffer;
    activeBuffer = passiveBuffer;
    passiveBuffer = t;
}

unsigned char* CthughaBuffer::activePixels() {
    return activeBuffer;
}

unsigned char* CthughaBuffer::passivePixels() {
    return passiveBuffer;
}

const unsigned char* CthughaBuffer::activePixels() const {
    return activeBuffer;
}

const unsigned char* CthughaBuffer::passivePixels() const {
    return passiveBuffer;
}
