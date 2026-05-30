#include "cthugha.h"
#include "CthughaBuffer.h"
#include "cth_buffer.h"
#include "translate.h"
#include "waves.h"
#include "display.h"
#include "CthughaDisplay.h"
#include "flames.h"
#include "imath.h"

int BUFF_WIDTH = 160;
int BUFF_HEIGHT = 100;

int CthughaBuffer::nInit = 0;
CthughaBuffer CthughaBuffer::buffer;

CthughaBuffer* CthughaBuffer::current = &CthughaBuffer::buffer;

CthughaBuffer::CthughaBuffer()
    : translate(CthughaBuffer::nInit, "translate") {
    nInit++;
}

void CthughaBuffer::init() {

    /* The buffer has a border of 3 lines on top on bottom. These
       lines are set to some boundary value before the 'flame' function is called.
       The border is not displayed to the screen
       */
    activeBuffer = new unsigned char[BUFF_SIZE + 6 * BUFF_WIDTH] + 3 * BUFF_WIDTH;
    passiveBuffer = new unsigned char[BUFF_SIZE + 6 * BUFF_WIDTH] + 3 * BUFF_WIDTH;

    /* clear buffers */
    memset(activeBuffer, 0, BUFF_SIZE);
    memset(passiveBuffer, 0, BUFF_SIZE);
}

void CthughaBuffer::initAll() {

    current = &buffer;

    //
    // add the default entries
    //
    flame.add(_flames, _nFlames);

    if (init_flames())
        exit(0);

    if (init_translate())
        exit(0);

    if (init_wave())
        exit(0);

    if (load_palettes())
        exit(0);

    // allocate memory for the buffers
    buffer.init();
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
