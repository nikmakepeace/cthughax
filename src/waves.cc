#include "cthugha.h"
#include "Sound.h"
#include "display.h"
#include "Interface.h"
#include "information.h"
#include "imath.h"
#include "waves.h"
#include "disp-sys.h"
#include "cth_buffer.h"
#include "SoundAnalyze.h"
#include "CthughaBuffer.h"
#include "CthughaDisplay.h"

#include <math.h>

/* Wave-display-functions */
void wave_dotHor();
void wave_dotVert();
void wave_lineHor();
void wave_lineVert();
void wave_spike();
void wave_spikeH();

void wave_buff9();
void wave_buff10();
void wave_buff11();
void wave_buff12();
void wave_buff13();
void wave_buff14();
void wave_buff15();
void wave_buff16();
void wave_pete0();
void wave_pete1();
void wave_pete2();
void wave_fract1();
void wave_fract2();
void wave_test();
void wave_aaron();
void wave_wire1();
void wave_lineHLdiff();
void wave_wire2();
void wave_spiral();
void wave_pyro();
void wave_warp();
void wave_laser();
void wave_corner();
void wave_jump();
void wave_sticks();
void wave_grid();
void wave_none();

class WaveEntry : public CoreOptionEntry {
public:
    void (*wave)();

    WaveEntry(void (*f)(), const char* name, const char* desc, int inUse = 1)
        : CoreOptionEntry(name, desc, inUse)
        , wave(f) { }

    int operator()() {
        (*wave)();
        return 0;
    }
};

CoreOptionEntry* _waves[] = {
    new WaveEntry(wave_dotHor, "DotHor", "Dots Horizontal"), // 0
    new WaveEntry(wave_dotVert, "DotVert", "Dots Vertical"), // 1

    new WaveEntry(wave_lineHor, "LineHor", "Lines Horizontal"), // 2
    new WaveEntry(wave_lineVert, "LineVert", "Lines Vertical"), // 3

    new WaveEntry(wave_spike, "Spike", "Spikes"), // 4
    new WaveEntry(wave_spikeH, "SpikeH", "Spikes Hollow"), // 5

    new WaveEntry(wave_buff9, "Walking", "Walking"), // 6
    new WaveEntry(wave_buff10, "Falling", "Falling"), // 7
    new WaveEntry(wave_buff11, "Lissa", "Lissa"), // 8
    new WaveEntry(wave_buff14, "LineX", "Line X"), // 9
    new WaveEntry(wave_buff15, "Light1", "Lightning 1"), // 10
    new WaveEntry(wave_buff16, "Light2", "Lightning 2"), // 11
    new WaveEntry(wave_pete0, "Pete0", "FireFlies"), // 12
    new WaveEntry(wave_pete1, "Pete1", "Pete"), // 13
    new WaveEntry(wave_pete2, "Pete2", "Dot VS sine"), // 14
    new WaveEntry(wave_fract1, "Fract1", "Zippy 1"), // 15
    new WaveEntry(wave_fract2, "Fract2", "Zippy 2"), // 16
    new WaveEntry(wave_test, "Test", "Test"), // 17
    new WaveEntry(wave_aaron, "Aaron", "Rings of Fire"), // 18
    new WaveEntry(wave_wire1, "Wire1", "Wire frame 1"), // 19
    new WaveEntry(wave_wire2, "Wire2", "Wire frame 2"), // 20
    new WaveEntry(wave_lineHLdiff, "LineHLDiff", "Difference Hor."), // 21
    new WaveEntry(wave_spiral, "Spiral", "Spirograph"), // 22
    new WaveEntry(wave_pyro, "Pyro", "Fire works"), // 23
    new WaveEntry(wave_warp, "Warp", "Space warp"), // 24
    new WaveEntry(wave_laser, "Laser", "Laser"), // 25
    new WaveEntry(wave_corner, "Corner", "Corner"), // 26
    new WaveEntry(wave_jump, "Jump", "Jumping points"), // 27
    new WaveEntry(wave_sticks, "Sticks", "Random sticks"), // 28
    new WaveEntry(wave_grid, "Grid", "Diagnostic grid", 0), // 29
    new WaveEntry(wave_none, "None", "No wave drawing", 0), // 30
};
int _nWaves = sizeof(_waves) / sizeof(CoreOptionEntry*);

/*****************************************************************************/
extern int Bsine[MAX_BUFF_WIDTH];
static void draw_line(int x1, int y1, int x2, int y2, int c);

OptionOnOff use_objects("use-objects", 1); /* use 3-D objects */
CoreOptionEntry* read_object(FILE* file, const char* name, const char* dir, const char* total_name);
int obj_change = 1; /* flag to indicate an object change occurred */

static const char* object_path[] = { "./", "./obj/", CTH_LIBDIR "/obj/", "" };

class ObjectEntry : public CoreOptionEntry {
public:
    WObject* obj;

    ObjectEntry(WObject* o, const char* name, const char* desc)
        : CoreOptionEntry(name, desc)
        , obj(o) { }
    ObjectEntry(const char* name, const char* desc)
        : CoreOptionEntry(name, desc)
        , obj(NULL) { }
    ~ObjectEntry() {
        delete obj;
        obj = NULL;
    }
};

/* big K */
WObject bigK[] = {
    /* layer 1 */
    { { 0, 0, 0 }, { 1, 0, 0 } }, { { 1, 0, 0 }, { 1, 3, 0 } }, { { 1, 3, 0 }, { 3, 0, 0 } },
    { { 3, 0, 0 }, { 4, 0, 0 } }, { { 4, 0, 0 }, { 2, 3, 0 } }, { { 2, 3, 0 }, { 4, 7, 0 } },
    { { 4, 7, 0 }, { 3, 7, 0 } }, { { 3, 7, 0 }, { 1, 4, 0 } }, { { 1, 4, 0 }, { 1, 7, 0 } },
    { { 1, 7, 0 }, { 0, 7, 0 } }, { { 0, 7, 0 }, { 0, 0, 0 } },
    /* layer 2 */
    { { 0, 0, 1 }, { 1, 0, 1 } }, { { 1, 0, 1 }, { 1, 3, 1 } }, { { 1, 3, 1 }, { 3, 0, 1 } },
    { { 3, 0, 1 }, { 4, 0, 1 } }, { { 4, 0, 1 }, { 2, 3, 1 } }, { { 2, 3, 1 }, { 4, 7, 1 } },
    { { 4, 7, 1 }, { 3, 7, 1 } }, { { 3, 7, 1 }, { 1, 4, 1 } }, { { 1, 4, 1 }, { 1, 7, 1 } },
    { { 1, 7, 1 }, { 0, 7, 1 } }, { { 0, 7, 1 }, { 0, 0, 1 } },
    /* layer connecting edges */
    { { 0, 0, 1 }, { 0, 0, 0 } }, { { 1, 0, 1 }, { 1, 0, 0 } }, { { 1, 3, 1 }, { 1, 3, 0 } },
    { { 3, 0, 1 }, { 3, 0, 0 } }, { { 4, 0, 1 }, { 4, 0, 0 } }, { { 2, 3, 1 }, { 2, 3, 0 } },
    { { 4, 7, 1 }, { 4, 7, 0 } }, { { 3, 7, 1 }, { 3, 7, 0 } }, { { 1, 4, 1 }, { 1, 4, 0 } },
    { { 1, 7, 1 }, { 1, 7, 0 } }, { { 0, 7, 1 }, { 0, 7, 0 } },

    { { -1, -1, -1 }, { -1, -1, -1 } }
};
/* cube */
WObject cube4[] = {
    { { 0, 0, 0 }, { 1, 0, 0 } },
    { { 1, 0, 0 }, { 1, 1, 0 } },
    { { 1, 1, 0 }, { 0, 1, 0 } },
    { { 0, 1, 0 }, { 0, 0, 0 } },
    { { 0, 0, 0 }, { 0, 0, 1 } },
    { { 1, 0, 0 }, { 1, 0, 1 } },
    { { 1, 1, 0 }, { 1, 1, 1 } },
    { { 0, 1, 0 }, { 0, 1, 1 } },
    { { 0, 0, 1 }, { 1, 0, 1 } },
    { { 1, 0, 1 }, { 1, 1, 1 } },
    { { 1, 1, 1 }, { 0, 1, 1 } },
    { { 0, 1, 1 }, { 0, 0, 1 } },
    { { -1, -1, -1 }, { -1, -1, -1 } },
};

/* another cube */
WObject cube1[] = {
    { { 0, 0, 0 }, { 1, 0, 0 } },
    { { 1, 0, 0 }, { 1, 1, 0 } },
    { { 1, 1, 0 }, { 2, 1, 0 } },
    { { 2, 1, 0 }, { 2, 0, 0 } },
    { { 2, 0, 0 }, { 3, 0, 0 } },
    { { 3, 0, 0 }, { 3, 3, 0 } },
    { { 3, 3, 0 }, { 2, 3, 0 } },
    { { 2, 3, 0 }, { 2, 2, 0 } },
    { { 2, 2, 0 }, { 1, 2, 0 } },
    { { 1, 2, 0 }, { 1, 3, 0 } },
    { { 1, 3, 0 }, { 0, 3, 0 } },
    { { 0, 3, 0 }, { 0, 0, 0 } },
    { { -1, -1, -1 }, { -1, -1, -1 } },
};

CoreOptionEntry* _objects[] = { new ObjectEntry(bigK, "bigK", ""),
    new ObjectEntry(cube1, "bigH", ""), new ObjectEntry(cube4, "Cube4", "") };
int _nObjects = sizeof(_objects) / sizeof(CoreOptionEntry*);

/*
 * initialize, load objects
 */
int init_wave() {

    init_tables();

    /* load objects from File  */
    if (int(use_objects)) {

        CTH_INFO("  loading 3-D objects...");
        CthughaBuffer::current->object.load(object_path, "/obj/", ".obj", read_object);
        CTH_INFO("\n  number of 3-D objects: %d\n", CthughaBuffer::current->object.getNEntries());
    }

    return 0;
}

CoreOptionEntry* read_object(
    FILE* file, const char* name, const char* /* dir */, const char* /*total_name*/) {
    char dummy[256];
    int i, j, nlines, x1, y1, z1, x2, y2, z2, mx, my, mz;

    ObjectEntry* new_obj = new ObjectEntry(name, "");

    /* count relevant lines, discarding comment lines and empty lines */
    nlines = 0;
    while (!feof(file)) {
        fgets(dummy, 255, file);
        if (dummy[0] != 0 && dummy[0] != '#') /* if this is not a comment line */
            nlines++; /* or an empty line, then count it */
    }

    new_obj->obj = new WObject[nlines + 1];

    rewind(file);
    i = 1;
    j = 0;
    mx = my = mz = 0x7fffffff;

    /* now read in the data */
    while (!feof(file)) {

        fgets(dummy, 255, file);

        if (dummy[0] != 0 && dummy[0] != '#') /* if this looks like a legit line */

            if (sscanf(dummy, "%d,%d,%d - %d,%d,%d", &x1, &y1, &z1, &x2, &y2, &z2) < 6) {
                CTH_WARN("\n    Can't read at line: %d (%s)", i, name);
                if (i == 1) { /*  nothing read  */
                    CTH_WARN(" ... skipping file");
                    delete new_obj;
                    return NULL;
                }
            } else {
                if (j >= nlines) {
                    CTH_ERROR("Error reading object file %s", name);
                    delete new_obj;
                    return NULL;
                }

                if (x1 < mx)
                    mx = x1;
                if (x2 < mx)
                    mx = x2;
                if (y1 < my)
                    my = y1;
                if (y2 < my)
                    my = y2;
                if (z1 < mz)
                    mz = z1;
                if (z2 < mz)
                    mz = z2;

                new_obj->obj[j][0][0] = x1;
                new_obj->obj[j][0][1] = y1;
                new_obj->obj[j][0][2] = z1;

                new_obj->obj[j][1][0] = x2;
                new_obj->obj[j][1][1] = y2;
                new_obj->obj[j][1][2] = z2;
                j++;
            }

        i++;
    }

    /* align the object up against the axes */
    for (i = 0; i < j; i++) {
        new_obj->obj[i][0][0] -= mx;
        new_obj->obj[i][0][1] -= my;
        new_obj->obj[i][0][2] -= mz;
        new_obj->obj[i][1][0] -= mx;
        new_obj->obj[i][1][1] -= my;
        new_obj->obj[i][1][2] -= mz;
    }

    /* terminate the line list with -1 coordinates */
    new_obj->obj[j][0][0] = new_obj->obj[j][0][1] = new_obj->obj[j][0][2] = new_obj->obj[j][1][0]
        = new_obj->obj[j][1][1] = new_obj->obj[j][1][2] = -1;

    return new_obj;
}

/*
 * some helping macros and functions
 */

#define addr(x, y) ((x) + (y) * BUFF_WIDTH)
#define BOTTOM (BUFF_HEIGHT - 1)
#define MID_Y (BUFF_HEIGHT >> 1)
#define MID_X (BUFF_WIDTH >> 1)
#define LOW_LINE (BUFF_HEIGHT - BUFF_HEIGHT / 10)

#define tcolor(x) (tables[int(CthughaBuffer::current->table)][(x)])

static void rotate_axis(
    double x, double y, double z, const double axis[3], int angle, double& rx, double& ry, double& rz) {
    double s = isin(angle);
    double c = icos(angle);
    double inv_c = 1.0 - c;
    double dot = axis[0] * x + axis[1] * y + axis[2] * z;

    rx = x * c + (axis[1] * z - axis[2] * y) * s + axis[0] * dot * inv_c;
    ry = y * c + (axis[2] * x - axis[0] * z) * s + axis[1] * dot * inv_c;
    rz = z * c + (axis[0] * y - axis[1] * x) * s + axis[2] * dot * inv_c;
}

// #define putat(x,y,val)	active_buffer[ addr( (x) , (y) ) ] = val
void putat(int x, int y, int val) {
    int a = addr(x, y);

#if 0
    if( (a < 0) || (a >= BUFF_SIZE)) {
	CTH_ERROR("Penguin alert! %d,%d", x,y);
	exit(1);
    }
#endif

    active_buffer[a] = val;
    active_buffer[a - 1] = val;
    active_buffer[a + 1] = val;
    active_buffer[a + BUFF_WIDTH] = val;
    active_buffer[a - BUFF_WIDTH] = val;
}

void putat_cut(int x, int y, int val) {
    if ((x < 0) || (x >= BUFF_WIDTH))
        return;
    if ((y < 0) || (y >= BUFF_HEIGHT))
        return;
    putat(x, y, val);
}

void draw_line(int x1, int y1, int x2, int y2, int c) {
    register int lx, ly, dx, dy;
    register int i, j, k;

    if (x1 < 0)
        x1 = 0;
    if (x1 > BUFF_WIDTH)
        x1 = BUFF_WIDTH;
    if (x2 < 0)
        x2 = 0;
    if (x2 > BUFF_WIDTH)
        x2 = BUFF_WIDTH;
    if (y1 < 0)
        y1 = 0;
    if (y1 > BUFF_HEIGHT)
        y1 = BUFF_HEIGHT;
    if (y2 < 0)
        y2 = 0;
    if (y2 > BUFF_HEIGHT)
        y2 = BUFF_HEIGHT;

    lx = abs(x1 - x2);
    ly = abs(y1 - y2);
    dx = (x1 > x2) ? -1 : 1;
    dy = (y1 > y2) ? -1 : 1;

    if (lx > ly) {
        for (i = x1, j = y1, k = 0; i != x2; i += dx, k += ly) {
            if (k >= lx) {
                k -= lx;
                j += dy;
            }
            active_buffer[i + j * BUFF_WIDTH] = c;
        }
    } else {
        for (i = y1, j = x1, k = 0; i != y2; i += dy, k += lx) {
            if (k >= ly) {
                k -= ly;
                j += dx;
            }
            active_buffer[j + i * BUFF_WIDTH] = c;
        }
    }
}

void do_vwave(int ystart, int yend, int x, int val) {
    int ys, ye;
    unsigned char* pos;

    if (ystart > yend)
        ys = yend, ye = ystart;
    else
        ys = ystart, ye = yend;

    if (ys < 0)
        ys = 0;
    if (ys >= BUFF_HEIGHT)
        ys = BUFF_HEIGHT - 1;
    if (ye < 0)
        ye = 0;
    if (ye >= BUFF_HEIGHT)
        ye = BUFF_HEIGHT - 1;

    pos = active_buffer + addr(x, ys);
    for (; ys <= ye; ys++) {
        *pos = (char)val;
        pos += BUFF_WIDTH;
    }
}

void do_hwave(int xstart, int xend, int y, int val) {
    int xs, xe;
    unsigned char* pos;

    if (xstart > xend)
        xs = xend, xe = xstart;
    else
        xs = xstart, xe = xend;

    while ((xs < 0) && (xe < 0))
        xs += BUFF_WIDTH, xe += BUFF_WIDTH;
    while ((xs >= BUFF_WIDTH) && (xe >= BUFF_WIDTH))
        xs -= BUFF_WIDTH, xe -= BUFF_WIDTH;

    if (xs < 0)
        xs = 0;
    if (xs >= BUFF_WIDTH)
        xs = BUFF_WIDTH - 1;
    if (xe < 0)
        xe = 0;
    if (xe >= BUFF_WIDTH)
        xe = BUFF_WIDTH - 1;

    pos = active_buffer + addr(xs, y);
    for (; xs <= xe; xs++) {
        *pos = (char)val;
        pos++;
    }
}

static int data[MAX_BUFF_WIDTH][2];

//
// scale the sound data to the number of samples needed
//
void prepareSoundData(int n, int add = 128) {
    int p = 0;
    int s = (1024 << 16) / n;

    for (int i = 0; i < n; i++) {
        data[i][0] = soundDevice->dataProc[p >> 16][0] + add;
        data[i][1] = soundDevice->dataProc[p >> 16][1] + add;

        p += s;
    }
}

/*****************************************************************************
 *
 * Some functions require a minimum resolutions. They will be disabled if
 * the buffer is too small. If such a wave-function get selected it will
 * automatically change to the next wave-function.
 *
 *****************************************************************************/

/*****************************************************************************
 * Dot horizontal
 *****************************************************************************/

void wave_none() { }

void wave_grid() {
    for (int y = 0; y < BUFF_HEIGHT; y++) {
        for (int x = 0; x < BUFF_WIDTH; x++) {
            int c = 0;

            if ((x % 10 == 0) || (y % 10 == 0))
                c = 48;
            if ((x % 20 == 0) || (y % 20 == 0))
                c = 96;
            if ((x == MID_X) || (y == MID_Y))
                c = 192;
            if ((x == 0) || (x == BUFF_WIDTH - 1) || (y == 0) || (y == BUFF_HEIGHT - 1))
                c = 224;

            if (c != 0)
                active_buffer[addr(x, y)] = c;
        }
    }

    for (int i = 0; (i < BUFF_WIDTH) && (i < BUFF_HEIGHT); i++)
        active_buffer[addr(i, i)] = 255;

    for (int i = 0; (i < BUFF_WIDTH) && (i < BUFF_HEIGHT); i++)
        active_buffer[addr(BUFF_WIDTH - 1 - i, i)] = 160;

    putat_cut(5, 5, 255);
    putat_cut(BUFF_WIDTH - 6, 5, 192);
    putat_cut(5, BUFF_HEIGHT - 6, 160);
    putat_cut(BUFF_WIDTH - 6, BUFF_HEIGHT - 6, 128);
}

void wave_dotHor() { /* dot horizontal */
    int x, tmp;

    prepareSoundData(BUFF_WIDTH);

    for (x = 0; x < BUFF_WIDTH; x++) {
        tmp = data[x][0];
        putat_cut(x >> 1, LOW_LINE - (tmp >> int(CthughaBuffer::current->waveScale)), tcolor(tmp));

        tmp = data[x][1];
        putat_cut((x + BUFF_WIDTH) >> 1, LOW_LINE - (tmp >> int(CthughaBuffer::current->waveScale)),
            tcolor(tmp));
    }
}

/*****************************************************************************
 * Dot vertical
 *****************************************************************************/

void wave_dotVert() { /* dot vertical */
    int tmp, x;

    prepareSoundData(BOTTOM);

    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][0];
        putat_cut(MID_X - (tmp >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));

        tmp = data[x][1];
        putat_cut(MID_X + (tmp >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));
    }
}

/****************************************************************************
 * Line horizontal
 ****************************************************************************/

void wave_lineHor() { /* Line horizontal */
    int x, y, tmp;
    static int last = 0;

    prepareSoundData(BUFF_WIDTH);

    for (y = 0; y < 2; y++)
        for (x = 0; x < BUFF_WIDTH; x++) {
            tmp = data[x][y];
            do_vwave(MID_Y - ((tmp - 128) >> int(CthughaBuffer::current->waveScale)),
                MID_Y - ((last - 128) >> int(CthughaBuffer::current->waveScale)),
                y ? ((x + BUFF_WIDTH) >> 1) : (x >> 1), tcolor(tmp));
            last = tmp;
        }
}

/****************************************************************************
 * Line vertical
 ****************************************************************************/

void wave_lineVert() { /* Line veritcal short */
    int tmp, x, last1 = 128, last2 = 128;

    prepareSoundData(BOTTOM);

    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][0];
        do_hwave(MID_X - (tmp >> int(CthughaBuffer::current->waveScale)),
            MID_X - (last1 >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));
        last1 = tmp;

        tmp = data[x][1];
        do_hwave(MID_X + (tmp >> int(CthughaBuffer::current->waveScale)),
            MID_X + (last2 >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));
        last2 = tmp;
    }
}

/****************************************************************************
 * Spikes functions
 ****************************************************************************/

void wave_spike() { /* Spike */
    int x, tmp, i;

    prepareSoundData(BUFF_WIDTH, 0);

    for (x = 0; x < BUFF_WIDTH; x++) {
        tmp = 2 * abs(data[x][0]) >> int(CthughaBuffer::current->waveScale);
        if (tmp >= BOTTOM)
            tmp = BOTTOM - 1;
        for (i = 0; i < tmp; i++)
            putat(x >> 1, BOTTOM - i, tcolor(i));

        tmp = 2 * abs(data[x][1]) >> int(CthughaBuffer::current->waveScale);
        if (tmp >= BOTTOM)
            tmp = BOTTOM - 1;
        for (i = 0; i < tmp; i++)
            putat((x + BUFF_WIDTH) >> 1, BOTTOM - i, tcolor(i));
    }
}

void wave_spikeH() { /* Spike hollow */
    int tmp, x, y, last = 0;

    prepareSoundData(BUFF_WIDTH, 0);

    for (y = 0; y < 2; y++) {
        for (x = 0; x < BUFF_WIDTH; x++) {
            tmp = 2 * abs(data[x][y]) >> int(CthughaBuffer::current->waveScale);
            do_vwave(
                BOTTOM - tmp, BOTTOM - last, y ? ((x + BUFF_WIDTH) >> 1) : (x >> 1), tcolor(tmp));
            last = tmp;
        }
    }
}

/*****************************************************************************
 * other wave-functions
 *****************************************************************************/

void wave_buff9() { /* Walking */
    int tmp, x, last1 = 128, last2 = 128;
    static int col = 128;

    col = (col + 1) % BUFF_WIDTH;

    prepareSoundData(BOTTOM);

    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][0];
        do_hwave(col - (tmp >> int(CthughaBuffer::current->waveScale)),
            col - (last1 >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));
        last1 = tmp;

        tmp = data[x][1];
        do_hwave(col + (tmp >> int(CthughaBuffer::current->waveScale)),
            col + (last2 >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));
        last2 = tmp;
    }
}
void wave_buff10() { /* Falling */
    int i;
    static int row = 0;

    prepareSoundData(2 * MID_X);

    row = (row + 1) % BOTTOM;
    for (i = 0; i < MID_X; i++) {
        putat(i, row + 1, tcolor(data[i][0]));
        putat(i + MID_X, row + 1, tcolor(data[i][1]));
        putat(i, row, tcolor(data[i + MID_X][0]));
        putat(i + MID_X, row, tcolor(data[i + MID_X][1]));
    }
}

void wave_buff11() { /* Lissa */
    int tmp, x, tmp2;

    prepareSoundData(BUFF_WIDTH);

    for (x = 0; x < BUFF_WIDTH; x++) {
        tmp = data[x][0];
        tmp2 = data[x][1];

        putat((tmp2 + 32) % BUFF_WIDTH, (tmp + 200 - 28) % BOTTOM, tcolor(tmp));
    }
}

void wave_buff14() { /* Line X */
    int tmp, x, last = 128;

    prepareSoundData(BOTTOM);

    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][0];
        do_hwave(MID_X - (tmp >> int(CthughaBuffer::current->waveScale)),
            MID_X - (last >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));
        last = tmp;
    }
    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][1];
        do_hwave(MID_X - 40 + (tmp >> int(CthughaBuffer::current->waveScale)),
            MID_X - 40 + (last >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));
        last = tmp;
    }
}

void wave_buff15() { /* Lightning 1 */
    int tmp, x, last = BUFF_WIDTH / 3;

    prepareSoundData(BOTTOM, 0);

    for (x = 0; x < BOTTOM; x++) {

        tmp = (data[x][0] >> 4) + last;

        if (tmp >= BUFF_WIDTH)
            tmp = BUFF_WIDTH - 1;
        if (tmp < 0)
            tmp = 0;

        do_hwave(tmp, last, x, 255);
        last = tmp;
    }

    last = 2 * BUFF_WIDTH / 3;
    for (x = 0; x < BOTTOM; x++) {

        tmp = (data[x][1] >> 4) + last;

        if (tmp >= BUFF_WIDTH)
            tmp = BUFF_WIDTH - 1;
        if (tmp < 0)
            tmp = 0;

        do_hwave(tmp, last, x, 255);
        last = tmp;
    }
}
void wave_buff16() { /* Lightning 2 */
    int tmp, x, last = BUFF_WIDTH / 3;

    prepareSoundData(BUFF_WIDTH, 0);

    for (x = 0; x < BOTTOM; x++) {

        tmp = (data[x][0] >> 5) + last;

        if (tmp >= BUFF_WIDTH)
            tmp = BUFF_WIDTH - 1;
        if (tmp < 0)
            tmp = 0;

        do_hwave(tmp, last, x, 255);
        last = tmp;
    }

    last = 2 * BUFF_WIDTH / 3;
    for (x = 0; x < BOTTOM; x++) {

        tmp = (data[x][1] >> 5) + last;

        if (tmp >= BUFF_WIDTH)
            tmp = BUFF_WIDTH - 1;
        if (tmp < 0)
            tmp = 0;

        do_hwave(tmp, last, x, 255);
        last = tmp;
    }
}

void wave_pete0() { /* FireFlies */
    int temp, temp2, x;
    static int xoff0 = 160, yoff0 = 100;
    static int xoff1 = 160, yoff1 = 100;

    prepareSoundData(BUFF_WIDTH);

    xoff0 += (data[0][0]) % 9 - 4;
    yoff0 += (data[1][0]) % 9 - 4;

    xoff1 += (data[0][1]) % 9 - 4;
    yoff1 += (data[1][1]) % 9 - 4;

    while (xoff0 < 0)
        xoff0 += BUFF_WIDTH;
    while (yoff0 < 0)
        yoff0 += BOTTOM;

    while (xoff1 < 0)
        xoff1 += BUFF_WIDTH;
    while (yoff1 < 0)
        yoff1 += BOTTOM;

    xoff0 = xoff0 % BUFF_WIDTH;
    xoff1 = xoff1 % BUFF_WIDTH;

    yoff0 = yoff0 % BUFF_HEIGHT;
    yoff1 = yoff1 % BUFF_HEIGHT;

    for (x = 0; x < BUFF_WIDTH; x++) {
        temp = data[x][0];
        temp2 = data[(x + 80) % BUFF_WIDTH][0];

        putat(((temp2 >> 2) + xoff0) % BUFF_WIDTH, ((temp >> 2) + yoff0) % BOTTOM, tcolor(temp));

        temp = data[x][1] + 128;
        temp2 = data[(x + 80) % BUFF_WIDTH][1] + 128;

        putat(((temp2 >> 2) + xoff1) % BUFF_WIDTH, ((temp >> 2) + yoff1) % BOTTOM, tcolor(temp));
    }
}

void wave_pete1() {
    int tmp, x, left = 0, right = 0;

    prepareSoundData(BUFF_WIDTH, 0);

    for (x = 0; x < BUFF_WIDTH; x++) {
        left += abs(data[x][0]);
        right += abs(data[x][1]);
    }

    left = left / MID_X;
    right = right / MID_X;

    left = min(left, BOTTOM);
    right = min(right, BOTTOM);

    for (x = 0; x < MID_X; x++) {
        tmp = data[x][0];
        putat_cut(x, BOTTOM - (abs(left * Bsine[x]) >> 8), tcolor(tmp));
    }
    for (x = MID_X; x < BUFF_WIDTH; x++) {
        tmp = data[x][1];
        putat_cut(x, BOTTOM - (abs(right * Bsine[x]) >> 8), tcolor(tmp));
    }
}

void wave_pete2() { /* Dot VS sine */
    int x, tmp;

    prepareSoundData(BUFF_HEIGHT);

    for (x = 0; x < BUFF_HEIGHT; x++) {
        tmp = data[x][0];
        putat_cut(MID_X - (tmp >> int(CthughaBuffer::current->waveScale)), x, tcolor(sine[tmp]));
        tmp = data[x][1];
        putat_cut(MID_X + (tmp >> int(CthughaBuffer::current->waveScale)), x, tcolor(sine[tmp]));
    }
}

void wave_fract1() { /* Zippy 1*/
    int temp, x;
    static int xoff0 = 0, yoff0 = 0;
    static int xoff1 = 0, yoff1 = 0;

    prepareSoundData(BUFF_WIDTH);

    temp = data[0][0];
    for (x = 0; x < BUFF_WIDTH - 2; x += 2) {
        xoff0 += (data[x][0] - temp) >> 1;
        temp = data[x][0];

        while (xoff0 < 0)
            xoff0 = BUFF_WIDTH - 1;

        xoff0 = xoff0 % BUFF_WIDTH;

        putat(xoff0, yoff0, tcolor(temp));

        yoff0 += (data[x + 1][0] - temp) >> 1;
        temp = data[x + 1][0];

        while (yoff0 < 0)
            yoff0 = BUFF_HEIGHT - 1;

        yoff0 = yoff0 % BUFF_HEIGHT;

        putat(xoff0, yoff0, tcolor(temp));
    }

    temp = data[0][1];
    for (x = 0; x < BUFF_WIDTH - 2; x += 2) {
        xoff1 += (data[x][1] - temp) >> 1;
        temp = data[x][1];

        if (xoff1 < 0)
            xoff1 = BUFF_WIDTH - 1;

        xoff1 = xoff1 % BUFF_WIDTH;

        putat(xoff1, yoff1, tcolor(temp));

        yoff1 -= (data[x + 1][1] - temp) >> 1;
        temp = data[x + 1][1];

        if (yoff1 < 0)
            yoff1 = BUFF_HEIGHT - 1;

        yoff1 = yoff1 % BUFF_HEIGHT;

        putat(xoff1, yoff1, tcolor(temp));
    }
}

void wave_fract2(void) { /* Zippy 2 */
    int temp, x;
    static int xoff0 = 0, yoff0 = 0;
    static int xoff1 = 0, yoff1 = 0;

    prepareSoundData(BUFF_WIDTH);

    temp = data[0][0];
    for (x = 0; x < BUFF_WIDTH - 2; x += 2) {
        xoff0 += (data[x][0] - temp);
        temp = data[x][0];

        if (xoff0 < 0)
            xoff0 = BUFF_WIDTH - 1;

        xoff0 = xoff0 % BUFF_WIDTH;

        putat(xoff0, yoff0, tcolor(temp));

        yoff0 += (data[x + 1][0] - temp);
        temp = data[x + 1][0];

        if (yoff0 < 0)
            yoff0 = BUFF_HEIGHT - 1;

        yoff0 = yoff0 % BUFF_HEIGHT;

        putat(xoff0, yoff0, tcolor(temp));
    }

    temp = data[0][1];
    for (x = 0; x < BUFF_WIDTH - 2; x += 2) {
        xoff1 += (data[x][1] - temp);
        temp = data[x][1];

        if (xoff1 < 0)
            xoff1 = BUFF_WIDTH - 1;

        xoff1 = xoff1 % BUFF_WIDTH;

        putat(xoff1, yoff1, tcolor(temp));

        yoff1 -= (data[x + 1][1] - temp);
        temp = data[x + 1][1];

        if (yoff1 < 0)
            yoff1 = BUFF_HEIGHT - 1;

        yoff1 = yoff1 % BUFF_HEIGHT;

        putat(xoff1, yoff1, tcolor(temp));
    }
}

void wave_test() { /* Test */
    int temp, x, left = 0, right = 0;

    prepareSoundData(BUFF_WIDTH, 0);

    for (x = 0; x < BUFF_WIDTH; x++) {
        left += abs(data[x][0]);
        right += abs(data[x][1]);
    }

    left = left / (128);
    right = right / (128);

    left = min(left, 199);
    right = min(right, 199);

    for (x = 0; x < MID_X; x++) {
        temp = data[x][0] + 128;
        putat_cut(x, BOTTOM - (abs((left)*Bsine[x]) >> 8), tcolor(temp));
    }
    for (x = MID_X; x < BUFF_WIDTH; x++) {
        temp = data[x][1] + 128;
        putat_cut(x, BOTTOM - (abs((right)*Bsine[x]) >> 8), tcolor(temp));
    }
}

/*
 * From root@hangon.onramp.net Sun Jun 18 04:42:22 1995
 * some changes by Deischinger Harald
 * some more changes by Rus Maxham
 *
 * the rings have a radius of 64.
 */
void wave_aaron() {
    static int x = 40, y = 0;
    static int first = 1;
    static int cxl = -1;
    static int cxr = -1;
    static int cyl = -1;
    static int cyr = -1;
    int txl = 0, tyl = 0, txr = 0, tyr = 0;

    if (first) {
        first = 0;
        cxl = ((BUFF_WIDTH - 256) / 2) + 64 + 128;
        cxr = ((BUFF_WIDTH - 256) / 2) - 64 + 128;
        cyl = ((BOTTOM - 256) / 2) + 128;
        cyr = cyl;
    }

    if ((BUFF_HEIGHT > 128) && (BUFF_WIDTH >= 256)) {
        register int tmp, i, sx, sy;

        prepareSoundData(BUFF_WIDTH);

        for (i = 0; i < BUFF_WIDTH; i++) {
            if (y >= 320)
                y -= 320;
            if (x >= 320)
                x -= 320;
            tmp = data[i][0];

            sx = (sine[x] * tmp) >> 9;
            txl += sx;
            sy = (sine[y] * tmp) >> 9;
            tyl += sy;

            putat_cut(cxl + sx, cyl + sy, tcolor(tmp));

            tmp = data[i][1];

            sx = (sine[x] * tmp) >> 9;
            txr += sx;
            sy = (sine[y] * tmp) >> 9;
            tyr += sy;

            putat_cut(cxr - sx, cyr - sy, tcolor(tmp));

            x++;
            y++;
        }
        cxl += txl * 3 / BUFF_WIDTH + 1;
        cyl += tyl * 3 / BUFF_WIDTH + 1;
        cxr += txr * 3 / BUFF_WIDTH + 1;
        cyr += tyr * 3 / BUFF_WIDTH + 1;
        cxl %= BUFF_WIDTH;
        cxr %= BUFF_WIDTH;
        cyl %= BUFF_HEIGHT;
        cyr %= BUFF_HEIGHT;
        if (cxl < 0)
            cxl += BUFF_WIDTH;
        if (cxr < 0)
            cxr += BUFF_WIDTH;
        if (cyl < 0)
            cyl += BUFF_HEIGHT;
        if (cyr < 0)
            cyr += BUFF_HEIGHT;

    } else {
        CthughaBuffer::current->wave.change(+1, 0);
    }
}

/* Line horizontal long diff */
void wave_lineHLdiff() {
    register int x, tmp;
    static int last = 0;

    prepareSoundData(BUFF_WIDTH, 0);

    if (BUFF_HEIGHT < 300) {
        for (x = 0; x < BUFF_WIDTH; x++) {
            tmp = data[x][0] - data[x][1];
            do_vwave(MID_Y - tmp, MID_Y - last, x, tcolor(tmp + 128));
            last = tmp;
        }
    } else {
        for (x = 0; x < BUFF_WIDTH; x++) {
            tmp = data[x][0] - data[x][1];
            do_vwave(MID_Y - tmp, MID_Y - last, x, tcolor(tmp + 128));
            last = tmp << 1;
        }
    }
}

/*
 * Rotation objects
 */

/* by Russ, changed by Harald */
void wave_wire1() {
    static double theta = 0;
    double st, ct, ax, ay, az, x, y, z, px, py, pz;
    double objectHalf, screenScale;
    const double cameraDistance = 3.0;

    ObjectEntry* objE = (ObjectEntry*)CthughaBuffer::current->object.current();
    if (objE == NULL) {
        CthughaBuffer::current->wave.change(+1, 0);
        return;
    }
    WObject* theObj = objE->obj;
    if (theObj == NULL) {
        CthughaBuffer::current->wave.change(+1, 0);
        return;
    }

    int i, j, x1, y1, x2, y2;
    int mx, my, mz, m;
    int n;

    theta += M_PI / 45.0;
    mx = my = mz = 0;

    /* find size of the cube */
    for (n = 0; theObj[n][0][0] != -1; n++)
        for (j = 0; j < 2; j++) {
            if (theObj[n][j][0] > mx)
                mx = theObj[n][j][0];

            if (theObj[n][j][1] > my)
                my = theObj[n][j][1];

            if (theObj[n][j][2] > mz)
                mz = theObj[n][j][2];
        }
    m = (mx > my) ? mx : my;
    m = (m > mz) ? m : mz;

    if (m <= 0)
        m = 1;

    px = (double)mx / 2;
    py = (double)my / 2;
    pz = (double)mz / 2;
    objectHalf = (double)m / 2.0;
    if (objectHalf < 1.0)
        objectHalf = 1.0;
    screenScale = (double)min(BUFF_HEIGHT, BUFF_WIDTH) * 0.75;

    st = sin(theta);
    ct = cos(theta);

    for (i = 0; i < n; i++) {
        int s[2];
        int sampleCount;
        double scale0, scale1;

        s[0] = s[1] = 0;
        sampleCount = max(1024 / n, 1);
        for (j = 0; j < sampleCount; j++) {
            int sample = min(i * sampleCount + j, 1023);
            s[0] += abs(soundDevice->dataProc[sample][0]);
            s[1] += abs(soundDevice->dataProc[sample][1]);
        }

        scale0 = screenScale * (0.60 + 1.40 * ((double)s[0] / (double)(sampleCount * 128)));
        scale1 = screenScale * (0.60 + 1.40 * ((double)s[1] / (double)(sampleCount * 128)));

        x = (theObj[i][0][0] - px) / objectHalf;
        y = (theObj[i][0][1] - py) / objectHalf;
        z = (theObj[i][0][2] - pz) / objectHalf;

        ax = x * ct + z * st;
        ay = y;
        az = z * ct - x * st;

        x1 = int((double)ax * scale0 / (az + cameraDistance) + MID_X);
        y1 = int((double)ay * scale0 / (az + cameraDistance) + MID_Y);

        x = (theObj[i][1][0] - px) / objectHalf;
        y = (theObj[i][1][1] - py) / objectHalf;
        z = (theObj[i][1][2] - pz) / objectHalf;

        ax = x * ct + z * st;
        ay = y;
        az = z * ct - x * st;

        x2 = int((double)ax * scale1 / (az + cameraDistance) + MID_X);
        y2 = int((double)ay * scale1 / (az + cameraDistance) + MID_Y);

        draw_line(x1, y1, x2, y2, tcolor(255));
    }
}

#define nobj 10
#define whirlyRadius 45
/* by Russ, changed by Harald, then Nik */
void wave_wire2() {
    // Persistent swarm state. Wire2 draws nobj copies of the selected wire
    // object, each with its own position in the blob, local spin, and color.
    static int theta = 0, psi[nobj], rate[nobj], col[nobj];
    static int loc[nobj][3];
    static double blobAxis[3] = { 0.0, 1.0, 0.0 };

    // Scratch vertex in object-local or blob-local coordinates.
    double x, y, z;

    // Accumulated 3D point after blob rotation plus local object spin.
    double ax, ay, az;

    // Projection scale and screen origin.
    double scl;

    // Sine/cosine for each object's local spin.
    double sto, cto;

    register int i, j, k, x1, y1, x2, y2;
    register int mx, my, mz, m;

    // Object midpoint and normalization divisor in source coordinate space.
    // The legacy formula deliberately uses the y extent, so flat/wide models
    // keep the same tall, spindly Wire2 character as the built-in glyphs.
    double omx, omy, omz, om;
    register int ox, oy;

    ObjectEntry* objE = (ObjectEntry*)CthughaBuffer::current->object.current();
    if (objE == NULL) {
        CthughaBuffer::current->wave.change(+1, 0);
        return;
    }
    WObject* theObj = objE->obj;
    if (theObj == NULL) {
        CthughaBuffer::current->wave.change(+1, 0);
        return;
    }

    /*
     * Initialize the swarm when Wire2 first starts, or when callers explicitly
     * mark the object set as changed. The selected model is not baked into this
     * state: changing objects normally replaces the geometry in the existing
     * swarm positions/spins.
     */
    if (obj_change) {
        obj_change = 0;

        // Rotate the whole blob around a random axis for this swarm lifetime.
        do {
            blobAxis[0] = (double)(rand() % 201 - 100);
            blobAxis[1] = (double)(rand() % 201 - 100);
            blobAxis[2] = (double)(rand() % 201 - 100);
            scl = sqrt(blobAxis[0] * blobAxis[0] + blobAxis[1] * blobAxis[1]
                + blobAxis[2] * blobAxis[2]);
        } while (scl == 0.0);
        blobAxis[0] /= scl;
        blobAxis[1] /= scl;
        blobAxis[2] /= scl;

        /*
         * Pick a shell position and local spin for each copy. Avoid zero
         * radius and zero spin so no model sits fixed in the center.
         */
        for (i = 0; i < nobj; i++) {

            loc[i][1] = rand() % (whirlyRadius * 2) - whirlyRadius;
            j = rand() % 320;
            k = 1 + rand() % (whirlyRadius - 1);
            loc[i][0] = int(isin(j) * k);
            loc[i][2] = int(icos(j) * k);

            rate[i] = 1 + rand() % 7;
            if (rand() % 2)
                rate[i] *= -1;
            psi[i] = rand() % 320;
            col[i] = abs(rand() % 256);
        }
    }

    theta += 1;

    mx = my = mz = 0;
    omx = omy = omz = 0;

    for (i = 0; theObj[i][0][0] != -1; i++)
        for (j = 0; j < 2; j++) {

            if (theObj[i][j][0] > omx)
                omx = theObj[i][j][0];

            if (theObj[i][j][1] > omy)
                omy = theObj[i][j][1];

            if (theObj[i][j][2] > omz)
                omz = theObj[i][j][2];
        }

    omx /= 2.0;
    omy /= 2.0;
    omz /= 2.0;
    om = omy;
    om /= 3.0;

    for (i = 0; i < nobj; i++) {
        if (loc[i][0] > mx)
            mx = loc[i][0];

        if (loc[i][1] > my)
            my = loc[i][1];

        if (loc[i][2] > mz)
            mz = loc[i][2];
    }

    m = (mx > my) ? mx : my;
    m = (m > mz) ? m : mz;

    /*
     * The perspective distance is fixed to the blob radius rather than the
     * measured max above. This keeps the swarm compact and centered even when
     * the random shell is uneven.
     */
    m = whirlyRadius * 3 / 2;

    // Screen-space projection scale. Wire2 is intentionally height-led.
    scl = (double)BUFF_HEIGHT * 0.80;
    if (!scl)
        scl = 1;

    ox = BUFF_WIDTH / 2 - mx / 2;
    oy = BUFF_HEIGHT / 2 - my / 2;

    for (j = 0; j < nobj; j++) {

        // Each copy still spins around its own local y axis.
        sto = isin(psi[j]);
        cto = icos(psi[j]);

        psi[j] += rate[j];

        for (i = 0; theObj[i][0][0] != -1; i++) {

            // Rotate this copy's blob position around the swarm axis.
            x = loc[j][0];
            y = loc[j][1];
            z = loc[j][2];

            rotate_axis(x, y, z, blobAxis, theta, ax, ay, az);

            // Normalize the first endpoint and spin it around local y.
            x = (theObj[i][0][0] - omx) / om;
            y = (theObj[i][0][1] - omy) / om;
            z = (theObj[i][0][2] - omz) / om;

            ax += x * cto + z * sto;
            ay += y;
            az += z * cto - x * sto;

            // Project the first endpoint to the 2D Cthugha buffer.
            x1 = int((double)ax * scl / (az + m) + ox);
            y1 = int((double)ay * scl / (az + m) + oy);

            // Repeat for the second endpoint of the same wire segment.
            x = loc[j][0];
            y = loc[j][1];
            z = loc[j][2];

            rotate_axis(x, y, z, blobAxis, theta, ax, ay, az);

            x = (theObj[i][1][0] - omx) / om;
            y = (theObj[i][1][1] - omy) / om;
            z = (theObj[i][1][2] - omz) / om;

            ax += x * cto + z * sto;
            ay += y;
            az += z * cto - x * sto;

            x2 = int((double)ax * scl / (az + m) + ox);
            y2 = int((double)ay * scl / (az + m) + oy);

            draw_line(x1, y1, x2, y2, tcolor(col[j]));
        }
    }
}

/* by Russ */
void wave_spiral(void) {
    int i, amp, mx, cx, cy;
    double x, y, ox, oy, a, la, ra;
    static int ofs = 0, col = 0, loops, loopcount = 0;

    col++;
    col %= 256;
    ofs++;
    ofs %= 320;

    if (loopcount <= 0) {
        loopcount = abs(rand() % 1000);
        loops = 2 + abs(rand() % 8);
    }

    loopcount--;

    mx = min(BUFF_WIDTH, BUFF_HEIGHT);
    cx = BUFF_WIDTH / 2;
    cy = BUFF_HEIGHT / 2;

    amp = soundAnalyze.amplitude;
    int al = soundAnalyze.amplitudeLeft;
    int ar = soundAnalyze.amplitudeRight;

    /* convert to float now instead of every time it gets used */
    a = (double)amp * mx / 256.0 / 128.0;
    la = (double)al * mx / 256.0 / 128.0;
    ra = (double)ar * mx / 256.0 / 128.0;

    ox = int(a * sine[(ofs + 120) % 320] + la * sine[120] / 2);
    oy = int(a * sine[ofs] + ra * sine[0] / 2);

    for (i = 1; i < 320; i++) {

        x = a * sine[(i + ofs + 120) % 320] + la * sine[((i)*loops + 120) % 320] / 2;
        y = a * sine[(i + ofs) % 320] + ra * sine[((i)*loops) % 320] / 2;

        draw_line(int(cx + ox), int(cy + oy), int(cx + x), int(cy + y), tcolor(col));

        ox = x;
        oy = y;
    }
}

#define maxWorks 15
#define maxTrails 5
#define GRAV 1

typedef struct {
    double xv, yv, xp, yp, dur, foo;
} Fexpl;

typedef struct {
    int xv, yv, xp, yp, dur, col, oxp, oyp;
    Fexpl expl[maxTrails];
} Fwork;

/* by Russ */
void wave_pyro(void) {
    int i, x1, y1;
    static int first = 1, maxV, maxA;
    static Fwork theWorks[maxWorks];

    if (first) {
        int fire;

        first = 0;

        /* intialize all the fire works to be inoperative */
        for (i = 0; i < maxWorks; i++)
            theWorks[i].dur = -1;

        /* find the maximum vertical launch velocity to stay on the screen */
        for (fire = 0, maxV = 0; fire < BUFF_HEIGHT; maxV += GRAV, fire += maxV)
            ;

        maxV--;
        maxA = 30;
    }

    for (i = 0; i < maxWorks; i++)

        /* if this work is in flight, process it */
        if (theWorks[i].dur != -1) {

            /* compute next position */
            x1 = theWorks[i].xp + theWorks[i].xv;
            y1 = theWorks[i].yp + theWorks[i].yv;

            /* draw the work */
            draw_line(theWorks[i].oxp, theWorks[i].oyp, x1, y1, tcolor(theWorks[i].col));
            draw_line(theWorks[i].oxp + 1, theWorks[i].oyp, x1 + 1, y1, tcolor(theWorks[i].col));

            /* bounce off walls */
            if (x1 < 0 || x1 > BUFF_WIDTH)
                theWorks[i].xv *= -1;

            /* gravity and the passage of time, aren't they cute */
            theWorks[i].yv += GRAV;
            theWorks[i].dur++;
            theWorks[i].oxp = theWorks[i].xp;
            theWorks[i].oyp = theWorks[i].yp;
            theWorks[i].xp = x1;
            theWorks[i].yp = y1;

            /* time to explode or they go off the screen */
            if (theWorks[i].yv > 5 || theWorks[i].yp < 0) {
                theWorks[i].dur = -1;
            }

        } else if (soundAnalyze.fire) {

            /* maintain a maximum attack value for scaling purposes */
            if (soundAnalyze.fire * 4 > maxA)
                maxA = soundAnalyze.fire * 4;

            /* slowly reduce max over time to keep it in line with the average levels */
            if (maxA > 30)
                maxA--;

            /* fire off a new firework */
            theWorks[i].dur = 0;
            theWorks[i].oxp = theWorks[i].xp = rand() % BUFF_WIDTH;
            theWorks[i].oyp = theWorks[i].yp = BUFF_HEIGHT - 4;
            theWorks[i].xv = (rand() % 20) - 10;
            theWorks[i].yv = -(soundAnalyze.fire * maxV / (maxA / 4));
            theWorks[i].col = rand() % 256;
            soundAnalyze.fire = soundAnalyze.fire * 2 / 3;
        }

    /* test rocket exploded, reset */
}

#define maxWarps 15
#define maxWarpTrails 20

typedef struct {
    int r, s, theta, omg, trails, col, rgrav;
} WarpRing;

void wave_warp(void) {
    int i, x1, y1;
    static int first = 1, cx, cy, maxRad, maxA;
    static WarpRing theWarps[maxWarps];

    if (first) {
        first = 0;

        /* intialize all the fire works to be inoperative */
        for (i = 0; i < maxWarps; i++)
            theWarps[i].r = -1;

        cx = BUFF_WIDTH / 2;
        cy = BUFF_HEIGHT / 2;
        maxRad = (cx > cy) ? cx : cy;
    }

    for (i = 0; i < maxWarps; i++)

        /* if this warp is in flight, process it */
        if (theWarps[i].r != -1) {
            int r2 = theWarps[i].r + theWarps[i].s;
            int t2 = theWarps[i].theta + theWarps[i].omg;
            int j, x2, y2, tr = theWarps[i].trails;

            /* draw the ring of warps */
            for (j = 0; j < tr; j++) {
                x1 = int(cx + ((double)theWarps[i].r) * isin((360 * j) / tr + theWarps[i].theta));
                y1 = int(cy + ((double)theWarps[i].r) * icos((360 * j) / tr + theWarps[i].theta));
                x2 = int(cx + (double)r2 * isin(360 * j / tr + t2));
                y2 = int(cy + (double)r2 * icos(360 * j / tr + t2));
                draw_line(x1, y1, x2, y2, tcolor(theWarps[i].col));
            }

            /* increment the radius and spiral */
            theWarps[i].r += theWarps[i].s;
            theWarps[i].theta += theWarps[i].omg;
            theWarps[i].s -= theWarps[i].rgrav;

            if (theWarps[i].r > maxRad || theWarps[i].r < 0)
                theWarps[i].r = -1;

        } else if (soundAnalyze.fire) {

            /* maintain a maximum attack value for scaling purposes*/
            if (soundAnalyze.fire * 4 > maxA)
                maxA = soundAnalyze.fire * 4;

            /* slowly reduce max over time to keep it in line with the average levels */
            if (maxA > 30)
                maxA--;

            /* fire off a new warp ring */
            theWarps[i].r = 0;
            theWarps[i].s = 3 + soundAnalyze.fire * 4 * 20 / maxA;
            theWarps[i].trails = 1 + soundAnalyze.fire * 4 * maxWarpTrails / maxA;
            theWarps[i].theta = rand() % 360;
            theWarps[i].omg = (rand() % 16 - 8) * soundAnalyze.fire * 4 / maxA;
            theWarps[i].col = rand() % 256;
            theWarps[i].rgrav = rand() % 2;
            soundAnalyze.fire = 0;
            /*				soundAnalyze.fire = soundAnalyze.fire * 2 / 3;*/
        }
}

/* by Deischi */
void wave_laser() {
    static int xl, xr;
    static int y = 0;
    int x;

    prepareSoundData(BUFF_WIDTH / 10, 0);

    /*    y = (y+2) % BUFF_HEIGHT;*/
    y = BUFF_HEIGHT / 10;

    for (x = 0; x < BUFF_WIDTH / 10; x++) {
        draw_line(BUFF_WIDTH / 2, BUFF_HEIGHT / 2, xl, y, tcolor(255));
        xl = (xl + abs(data[x][0] - data[x + 1][0])) % BUFF_WIDTH;

        draw_line(BUFF_WIDTH / 2, BUFF_HEIGHT / 2, xr, BUFF_HEIGHT - y, tcolor(255));
        xr = (xr + abs(data[x][1] - data[x + 1][1])) % BUFF_WIDTH;
    }
}

/* by Deischi (inspired by RTL2) */
void wave_corner() {

    if (soundAnalyze.fire) {
        static int x = 0, y = 0;
        int i, j, t;

        x = (x + (rand() % soundAnalyze.fire)) % (BUFF_WIDTH - 16) + 8;
        y = (y + (rand() % soundAnalyze.fire)) % (BUFF_HEIGHT - 16) + 8;

        t = min(soundAnalyze.fire >> 2, 8);

        if (rand() & 1) {
            /* draw corner pointing right down */
            for (i = 0; i < t; i++) {
                for (j = 0; j < x; j++) {
                    putat(j, y + i, 255 >> i);
                    putat(j, y - i, 255 >> i);
                }
                for (j = 0; j < y; j++) {
                    putat(x - i, j, 255 >> i);
                    putat(x + i, j, 255 >> i);
                }
            }
        } else {
            /* draw corner pointer up left */
            for (i = 0; i < t; i++) {
                for (j = x; j < BUFF_WIDTH; j++) {
                    putat(j, y + i, 255 >> i);
                    putat(j, y - i, 255 >> i);
                }
                for (j = y; j < BUFF_HEIGHT; j++) {
                    putat(x - i, j, 255 >> i);
                    putat(x + i, j, 255 >> i);
                }
            }
        }
    }

    soundAnalyze.fire = 0;
}

// by Deischi
void wave_jump() {
    static int speed[MAX_BUFF_WIDTH];
    static int pos[MAX_BUFF_WIDTH];
    static int dir[MAX_BUFF_WIDTH];
    static int first = 1;

    if (first) {
        for (int i = 0; i < MAX_BUFF_WIDTH; i++)
            speed[i] = pos[i] = dir[i] = 0;
        first = 0;
    }

    prepareSoundData(BUFF_WIDTH);

    const int scale = 2 + int(CthughaBuffer::current->waveScale);
    for (int i = 0; i < BUFF_WIDTH; i++) {
        int e = data[i][0] + data[i][1];
        if (pos[i] < abs(e)) {
            speed[i] = abs(e);
            dir[i] = e > 0 ? 1 : 0;
        }

        pos[i] += speed[i];
        if (pos[i] < 0) {
            pos[i] = 0;
            speed[i] = 0;
        } else
            speed[i] -= int(cthughaDisplay->fps);

        if (dir[i] > 0)
            putat_cut(i, (pos[i] >> scale) + BUFF_HEIGHT / 2, 255);
        else
            putat_cut(i, -(pos[i] >> scale) + BUFF_HEIGHT / 2, 255);
    }
}

// by Deischi
void wave_sticks() {

    int n = soundAnalyze.fire >> int(CthughaBuffer::current->waveScale);
    for (int i = 0; i < n; i++) {
        draw_line(Random(BUFF_WIDTH), Random(BUFF_HEIGHT), Random(BUFF_WIDTH), Random(BUFF_HEIGHT),
            Random(256));
    }
}
