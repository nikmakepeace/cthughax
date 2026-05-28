#include "cthugha.h"
#include "CthughaBuffer.h"
#include "cth_buffer.h"
#include "translate.h"
#include "waves.h"
#include "display.h"
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

static CoreOptionEntryList flameEntries;
static CoreOptionEntryList waveEntries;
static CoreOptionEntryList objectEntries;
static CoreOptionEntryList waveScaleEntries;
static CoreOptionEntryList tableEntries;

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

    // allocate memory for the buffers
    for (int i = 0; i < maxNBuffers; i++)
        buffers[i].init();
}

void CthughaBuffer::run() {
    static int debugReports = 0;

    for (int j = 0; j < nBuffers; j++) {
        current = buffers + j;

        current->done_translate = 0;

        current->flame();
        current->translate();
        current->wave();

        if (CTH_LOG_ENABLED(CTH_LOG_DEBUG) && (debugReports < 16)) {
            int nonzero = 0;
            int peak = 0;
            for (int i = 0; i < BUFF_SIZE; i++) {
                int value = current->activeBuffer[i];
                if (value != 0)
                    nonzero++;
                if (value > peak)
                    peak = value;
            }
            debugReports++;
            CTH_DEBUG("visual buffer: buffer=%d wave=%s wave-scale=%s flame=%s table=%s nonzero-pixels=%d peak-pixel=%d size=%d\n",
                j, current->wave.currentName(), current->waveScale.currentName(),
                current->flame.currentName(), current->table.currentName(),
                nonzero, peak, BUFF_SIZE);
        }

        unsigned char* t = current->activeBuffer;
        current->activeBuffer = current->passiveBuffer;
        current->passiveBuffer = t;
    }
    current = buffers + nCurrent;
}

void CthughaBuffer::setPalette(const Palette pal) {
    memcpy(currentPalette, pal, sizeof(Palette));
    palChanged = 1;
}
