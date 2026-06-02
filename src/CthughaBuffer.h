// -*- c++ -*-

#ifndef __CTHUGHA_BUFFER_H
#define __CTHUGHA_BUFFER_H

#include "cthugha.h"

class CthughaBuffer {
public:
    CthughaBuffer();

    int width() const;
    int height() const;
    int pitch() const;
    int size() const;
    int bottom() const;
    int maxDimension() const;
    void setDimensions(int width_, int height_);

    void swapBuffers();
    unsigned char* activePixels();
    unsigned char* passivePixels();
    const unsigned char* activePixels() const;
    const unsigned char* passivePixels() const;

private:
    unsigned char* activeBuffer; /* buffer next on screen */
    unsigned char* passiveBuffer; /* buffer current on screen */
    int widthValue;
    int heightValue;

public:
    static CthughaBuffer buffer;
    static CthughaBuffer* current;

    void init();
    static int initAll();

};

#endif
