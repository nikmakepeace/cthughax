// -*- c++ -*-

#ifndef __CTHUGHA_BUFFER_H
#define __CTHUGHA_BUFFER_H

#include "cthugha.h"

class CthughaBuffer {
public:
    CthughaBuffer();
    ~CthughaBuffer();

    int width() const;
    int height() const;
    int size() const;
    int bottom() const;
    int maxDimension() const;

    /**
     * @return Hidden border height on each vertical side, in pixel rows.
     */
    int hiddenBorderRows() const;

    /**
     * @return Hidden border storage on each vertical side, in bytes.
     */
    int hiddenBorderByteCount() const;
    void setDimensions(int width_, int height_);

    void swapBuffers();
    /**
     * Clears active and passive visible plus hidden pixel storage.
     */
    void clear();
    unsigned char* activePixels();
    unsigned char* passivePixels();
    const unsigned char* activePixels() const;
    const unsigned char* passivePixels() const;

    /**
     * @return First hidden row above the active visible image.
     */
    unsigned char* activeTopHiddenRows();

    /**
     * @return First hidden row below the active visible image.
     */
    unsigned char* activeBottomHiddenRows();

private:
    CthughaBuffer(const CthughaBuffer&) = delete;
    CthughaBuffer& operator=(const CthughaBuffer&) = delete;

    unsigned char* activeAllocation;
    unsigned char* passiveAllocation;
    unsigned char* activeBuffer; /* visible pixels next on screen */
    unsigned char* passiveBuffer; /* visible pixels current on screen */
    int widthValue;
    int heightValue;

    int allocationByteCount() const;
    unsigned char* visiblePixels(unsigned char* allocation) const;

public:
    static CthughaBuffer buffer;
    static CthughaBuffer* current;

    /**
     * Allocates active/passive pixel memory for the current dimensions.
     *
     * Must run after option parsing has applied the final buffer width/height.
     * The allocation includes hidden rows above and below the visible pixels so
     * flame feedback can sample outside the displayed image.
     */
    void allocatePixels();

};

#endif
