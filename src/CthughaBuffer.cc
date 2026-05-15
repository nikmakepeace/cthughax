#include "cthugha.h"
#include "CthughaBuffer.h"
#include "cth_buffer.h"
#include "translate.h"
#include "waves.h"
#include "display.h"
#include "SoundDevice.h"
#include "SoundAnalyze.h"
#include "SoundProcess.h"
#include "CthughaDisplay.h"
#include "imath.h"

int BUFF_WIDTH = 160;
int BUFF_HEIGHT = 100;

#define MAX_BUFFERS 3

int CthughaBuffer::maxNBuffers = 1;
int CthughaBuffer::nBuffers = 1;
int CthughaBuffer::nInit = 0;

CthughaBuffer CthughaBuffer::buffers[MAX_BUFFERS];

CthughaBuffer* CthughaBuffer::current = buffers;
OptionInt CthughaBuffer::nCurrent("", 0, MAX_BUFFERS);

extern CoreOptionEntry* _waves[];
extern int _nWaves;
extern CoreOptionEntry* _objects[];
extern int _nObjects;

CoreOptionEntry* wave_scales[] = { new CoreOptionEntry("scale0", "large"),
    new CoreOptionEntry("scale1", "medium"), new CoreOptionEntry("scale2", "small") };

CoreOptionEntry* table_entries[] = {
    new CoreOptionEntry("table0", ""),
    new CoreOptionEntry("table1", ""),
    new CoreOptionEntry("table2", ""),
    new CoreOptionEntry("table3", ""),
    new CoreOptionEntry("table4", ""),
    new CoreOptionEntry("table5", ""),
    new CoreOptionEntry("table6", ""),
    new CoreOptionEntry("table7", ""),
    new CoreOptionEntry("table8", ""),
    new CoreOptionEntry("table9", ""),
};

CoreOptionEntry* border_entries[]
    = { new CoreOptionEntry("border0", ""), new CoreOptionEntry("border1", ""),
          new CoreOptionEntry("border2", ""), new CoreOptionEntry("border3", "") };

extern CoreOptionEntry* flashlight_entries[];

static CoreOptionEntryList flameEntries;
static CoreOptionEntryList waveEntries;
static CoreOptionEntryList objectEntries;
static CoreOptionEntryList waveScaleEntries;
static CoreOptionEntryList tableEntries;
static CoreOptionEntryList borderEntries;
static CoreOptionEntryList flashlightEntries;

CthughaBuffer::CthughaBuffer()
    : palChanged(1)
    ,

    //           buffer nr             name
    flame(CthughaBuffer::nInit, "flame", flameEntries)
    , palette(CthughaBuffer::nInit, "palette", paletteEntries)
    , pcx(CthughaBuffer::nInit, "pcx")
    , translate(CthughaBuffer::nInit, "translate")
    , wave(CthughaBuffer::nInit, "wave", waveEntries)
    , object(CthughaBuffer::nInit, "object", objectEntries)
    , flameGeneral(CthughaBuffer::nInit)
    , waveScale(CthughaBuffer::nInit, "wave-scale", waveScaleEntries)
    , table(CthughaBuffer::nInit, "table", tableEntries)
    , border(CthughaBuffer::nInit, "border", borderEntries)
    , soundProcess(CthughaBuffer::nInit, "sound-process", soundProcessEntries)
    , flashlight(CthughaBuffer::nInit, "flashlight", flashlightEntries)
    ,

    lastPalette(-1) {
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

    nCurrent.setMaxValue(maxNBuffers);

    nCurrent.value = 0;
    current = buffers;

    //
    // add the default entries
    //
    current->flame.add(_flames, _nFlames);
    current->wave.add(_waves, _nWaves);
    current->object.add(_objects, _nObjects);
    current->waveScale.add(wave_scales, 3);
    current->table.add(table_entries, 10);
    current->border.add(border_entries, 4);
    //  current->soundProcess.add(soundProcessEntries, 4);
    current->flashlight.add(flashlight_entries, 2);

    if (init_flames())
        exit(0);

    if (init_translate())
        exit(0);

    if (init_wave())
        exit(0);

    if (load_palettes())
        exit(0);

    if (init_pcx())
        exit(0);

    /* check if any palettes in use */
    if (CthughaBuffer::current->palette.getNEntries() == 0) {
        CTH_ERROR("No palettes specified. enabling inbuilt palettes\n");
        display_internal_pal = 1;
        if (load_palettes())
            exit(0);
    }

    // allocate memory for the buffers
    for (int i = 0; i < maxNBuffers; i++)
        buffers[i].init();
}

void CthughaBuffer::run() {

    for (int j = 0; j < nBuffers; j++) {
        current = buffers + j;

        current->soundProcess();

        current->flashlight();

        switch (int(current->border)) {
        case 0:
            memset(active_buffer + BUFF_SIZE, 0, 3 * BUFF_WIDTH);
            memset(active_buffer - 3 * BUFF_WIDTH, 0, 3 * BUFF_WIDTH);
            break;
        case 1:
            for (int i = 0; i < 3; i++) {
                memcpy(active_buffer - (i + 1) * BUFF_WIDTH, soundDevice->data, BUFF_WIDTH);
                memcpy(active_buffer + BUFF_SIZE + i * BUFF_WIDTH, soundDevice->data, BUFF_WIDTH);
            }
            break;
        case 2:
            memset(active_buffer + BUFF_SIZE, soundAnalyze.amplitude, 3 * BUFF_WIDTH);
            memset(active_buffer - 3 * BUFF_WIDTH, soundAnalyze.amplitude, 3 * BUFF_WIDTH);
            break;
        case 3:
            memset(active_buffer + BUFF_SIZE, 255, 3 * BUFF_WIDTH);
            memset(active_buffer - 3 * BUFF_WIDTH, 255, 3 * BUFF_WIDTH);
            break;
        }
        current->done_translate = 0;

        current->flame();
        current->translate();
        current->wave();

        current->smoothPalette();

        unsigned char* t = current->activeBuffer;
        current->activeBuffer = current->passiveBuffer;
        current->passiveBuffer = t;
    }
    current = buffers + nCurrent;
}

//
// brings the current palette nearer to the desired palette (palette of buffer 0)
// smoothing one palette to the next needs at most 1 second.
//
//
// original smooth_palette by "Stanislav V. Voronyi" <stas@use.kharkov.ua>
//
void CthughaBuffer::smoothPalette() {
    int i, j;
    int max_chg;

    if ((lastPalette == palette.currentN()) && (palChanged == 0))
        return;
    lastPalette = palette.currentN();

    // this is the palette that should be displayed
    Palette* desiredPal = &(((PaletteEntry*)palette.current())->pal);

    // changing should take at most 2 second
    static int oldMC = 1;
    max_chg = (cthughaDisplay->fps > 0) ? max(int(128.0 / cthughaDisplay->fps), 1) : oldMC;
    oldMC = max_chg;

    // count if anything changed
    palChanged = 256 * 3; // so many value can change
    for (i = 0; i < 256; i++)
        for (j = 0; j < 3; j++) {
            int d = (*desiredPal)[i][j] - currentPalette[i][j];
            if (d == 0)
                palChanged--;
            else {
                if (d < -max_chg)
                    d = -max_chg;
                else if (d > max_chg)
                    d = max_chg;
                currentPalette[i][j] += d;
            }
        }
}

void CthughaBuffer::setPalette(const Palette pal) {
    memcpy(currentPalette, pal, sizeof(Palette));
    palChanged = 1;
}
