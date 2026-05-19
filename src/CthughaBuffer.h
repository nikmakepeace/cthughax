// -*- c++ -*-

#ifndef __CTHUGHA_BUFFER_H
#define __CTHUGHA_BUFFER_H

#include "cthugha.h"
#include "CoreOption.h"

#include "flames.h"
#include "display.h"
#include "pcx.h"
#include "flames.h"
#include "translate.h"

extern double paletteSmoothingChance;

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
    CoreOption border;
    CoreOption soundProcess;
    CoreOption flashlight;

    Palette currentPalette;
    int lastPalette;

    int done_translate;

    CthughaBuffer();

    static void run();

    void smoothPalette();
    void setPalette(const Palette pal);

    unsigned char* activeBuffer; /* buffer next on screen */
    unsigned char* passiveBuffer; /* buffer current on screen */

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

#define active_buffer (CthughaBuffer::current->activeBuffer)
#define passive_buffer (CthughaBuffer::current->passiveBuffer)

#endif
