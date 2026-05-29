// -*- c++ -*-

#ifndef __CTHUGHA_BUFFER_H
#define __CTHUGHA_BUFFER_H

#include "cthugha.h"
#include "CoreOption.h"

#include "flames.h"
#include "display.h"
#include "pcx.h"
#include "translate.h"

class CthughaBuffer {
public:
    int palChanged;

    CoreOption flame;
    CoreOption palette;
    OptionPCX pcx;
    TranslateOption translate;
    CoreOption wave;
    CoreOption object;
    OptionGeneralFlame flameGeneral;
    CoreOption waveScale;
    CoreOption table;

    Palette currentPalette;
    int lastPalette;

    CthughaBuffer();

    void setPalette(const Palette pal);
    void swapBuffers();
    unsigned char* activePixels();
    unsigned char* passivePixels();
    const unsigned char* activePixels() const;
    const unsigned char* passivePixels() const;

private:
    unsigned char* activeBuffer; /* buffer next on screen */
    unsigned char* passiveBuffer; /* buffer current on screen */

public:
    static int maxNBuffers; // max. buffers in use
    static int nBuffers; // nr. of currently running buffers
    static CthughaBuffer buffers[];
    static CthughaBuffer* current;
    static OptionInt nCurrent;
    static int nInit;

    void init();
    static void initAll();

    static const Palette& getPalette(int i) {
        return ((PaletteEntry*)(buffers[0].palette[i]))->pal;
    }
};

#endif
