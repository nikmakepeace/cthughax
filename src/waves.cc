#include "cthugha.h"
#include "AudioFrame.h"
#include "display.h"
#include "Interface.h"
#include "information.h"
#include "imath.h"
#include "waves.h"
#include "disp-sys.h"
#include "cth_buffer.h"
#include "AudioAnalyzer.h"
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
void wave_wire1dot5();
void wave_wire1dot55();
void wave_wire1dot6();
void wave_lineHLdiff();
void wave_wire2();
void wave_wire2dot1();
void wave_spiral();
void wave_pyro();
void wave_warp();
void wave_laser();
void wave_corner();
void wave_jump();
void wave_sticks();
void wave_grid();
void wave_none();

static void (*last_wave_function)() = NULL;
static int wave_just_started = 1;

class WaveEntry : public CoreOptionEntry {
public:
    void (*wave)();

    WaveEntry(void (*f)(), const char* name, const char* desc, int inUse = 1)
        : CoreOptionEntry(name, desc, inUse)
        , wave(f) { }

    int operator()() {
        wave_just_started = (last_wave_function != wave);
        last_wave_function = wave;
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
    new WaveEntry(wave_wire1dot5, "Wire1dot5", "Wire frame 1.5"), // 20
    new WaveEntry(wave_wire1dot55, "Wire1dot55", "Wire frame 1.55"), // 21
    new WaveEntry(wave_wire1dot6, "Wire1dot6", "Wire frame 1.6"), // 22
    new WaveEntry(wave_wire2, "Wire2", "Wire frame 2"), // 23
    new WaveEntry(wave_wire2dot1, "Wire2dot1", "Wire frame 2.1"), // 24
    new WaveEntry(wave_lineHLdiff, "LineHLDiff", "Difference Hor."), // 25
    new WaveEntry(wave_spiral, "Spiral", "Spirograph"), // 26
    new WaveEntry(wave_pyro, "Pyro", "Fire works"), // 27
    new WaveEntry(wave_warp, "Warp", "Space warp"), // 28
    new WaveEntry(wave_laser, "Laser", "Laser"), // 29
    new WaveEntry(wave_corner, "Corner", "Corner"), // 30
    new WaveEntry(wave_jump, "Jump", "Jumping points"), // 31
    new WaveEntry(wave_sticks, "Sticks", "Random sticks"), // 32
    new WaveEntry(wave_grid, "Grid", "Diagnostic grid", 0), // 33
    new WaveEntry(wave_none, "None", "No wave drawing", 0), // 34
};
int _nWaves = sizeof(_waves) / sizeof(CoreOptionEntry*);

/*
 * Wave behavior catalog
 *
 * Common fields:
 * - Entry: CoreOption name followed by description.  The X11 panel menu shows
 *   the description when present, falling back to Name(); ncurses/list-style
 *   text displays both.
 * - Does: visible drawing behavior.
 * - Colours: how the wave turns its own values into palette indices.  tcolor()
 *   means the current table is applied first; "raw" means the byte written to
 *   active_buffer is already the palette index and bypasses the table.
 * - Sound: where the wave gets audio information.
 *
 * wave_dotHor
 * - Entry: DotHor (Dots Horizontal)
 * - Does: plots left/right channel dots across the width, with height driven by
 *   each resampled sample value.
 * - Colours: tcolor(sample), so sample value selects a table entry.
 * - Sound: prepareSoundData(BUFF_WIDTH) from audioFrameProcessedData().
 *
 * wave_dotVert
 * - Entry: DotVert (Dots Vertical)
 * - Does: plots left/right channel dots down the screen, displaced left/right
 *   from the center by sample value.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(BOTTOM) from audioFrameProcessedData().
 *
 * wave_lineHor
 * - Entry: LineHor (Lines Horizontal)
 * - Does: draws connected horizontal-scan oscilloscope traces, split into left
 *   and right halves.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(BUFF_WIDTH) from audioFrameProcessedData().
 *
 * wave_lineVert
 * - Entry: LineVert (Lines Vertical)
 * - Does: draws connected vertical traces, one channel to each side of center.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(BOTTOM) from audioFrameProcessedData().
 *
 * wave_spike
 * - Entry: Spike (Spikes)
 * - Does: draws filled vertical bars rising from the bottom, left and right
 *   channels split across the screen.
 * - Colours: tcolor(height), so colour follows distance up the spike rather
 *   than the original sample value.
 * - Sound: prepareSoundData(BUFF_WIDTH, 0) from audioFrameProcessedData().
 *
 * wave_spikeH
 * - Entry: SpikeH (Spikes Hollow)
 * - Does: draws only the moving outline of spike heights.
 * - Colours: tcolor(scaled absolute amplitude).
 * - Sound: prepareSoundData(BUFF_WIDTH, 0) from audioFrameProcessedData().
 *
 * wave_buff9
 * - Entry: Walking (Walking)
 * - Does: draws two vertical traces around a horizontally walking center
 *   column.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(BOTTOM) from audioFrameProcessedData().
 *
 * wave_buff10
 * - Entry: Falling (Falling)
 * - Does: writes channel sample dots into a row that advances downward.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(2 * MID_X) from audioFrameProcessedData().
 *
 * wave_buff11
 * - Entry: Lissa (Lissa)
 * - Does: draws a Lissajous-style point cloud, using right channel for x and
 *   left channel for y.
 * - Colours: tcolor(left sample).
 * - Sound: prepareSoundData(BUFF_WIDTH) from audioFrameProcessedData().
 *
 * wave_buff14
 * - Entry: LineX (Line X)
 * - Does: draws two horizontal traces with different center offsets.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(BOTTOM) from audioFrameProcessedData().
 *
 * wave_buff15
 * - Entry: Light1 (Lightning 1)
 * - Does: draws jagged lightning paths for each channel.
 * - Colours: raw palette index 255 for every segment.
 * - Sound: prepareSoundData(BOTTOM, 0) from audioFrameProcessedData().
 *
 * wave_buff16
 * - Entry: Light2 (Lightning 2)
 * - Does: draws a second jagged lightning variant with gentler sample scaling.
 * - Colours: raw palette index 255 for every segment.
 * - Sound: prepareSoundData(BUFF_WIDTH, 0) from audioFrameProcessedData().
 *
 * wave_pete0
 * - Entry: Pete0 (FireFlies)
 * - Does: draws two drifting point clusters whose offsets wander with the
 *   first few samples.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(BUFF_WIDTH) from audioFrameProcessedData().
 *
 * wave_pete1
 * - Entry: Pete1 (Pete)
 * - Does: draws two sine-shaped rows scaled by average channel energy.
 * - Colours: tcolor(signed sample).
 * - Sound: prepareSoundData(BUFF_WIDTH, 0) from audioFrameProcessedData().
 *
 * wave_pete2
 * - Entry: Pete2 (Dot VS sine)
 * - Does: plots vertical dots displaced by sample, one channel on each side.
 * - Colours: tcolor(sine[sample]), so colour uses a sine lookup of the sample.
 * - Sound: prepareSoundData(BUFF_HEIGHT) from audioFrameProcessedData().
 *
 * wave_fract1
 * - Entry: Fract1 (Zippy 1)
 * - Does: walks two persistent points around the buffer using half-sized
 *   differences between neighboring samples.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(BUFF_WIDTH) from audioFrameProcessedData().
 *
 * wave_fract2
 * - Entry: Fract2 (Zippy 2)
 * - Does: like Fract1, but uses full sample differences for a sharper walk.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(BUFF_WIDTH) from audioFrameProcessedData().
 *
 * wave_test
 * - Entry: Test (Test)
 * - Does: draws sine-shaped rows scaled by average channel energy, similar to
 *   Pete1 but with unsigned colour lookup.
 * - Colours: tcolor(sample + 128).
 * - Sound: prepareSoundData(BUFF_WIDTH, 0) from audioFrameProcessedData().
 *
 * wave_aaron
 * - Entry: Aaron (Rings of Fire)
 * - Does: draws two moving ring/rosette point sets when the buffer is large
 *   enough, otherwise advances to the next wave.
 * - Colours: tcolor(sample).
 * - Sound: prepareSoundData(BUFF_WIDTH) from audioFrameProcessedData().
 *
 * wave_wire1
 * - Entry: Wire1 (Wire frame 1)
 * - Does: rotates the selected object; each edge endpoint can scale
 *   independently, giving a fractured audio-reactive wireframe.
 * - Colours: one startup-random value per wave lifetime, drawn as tcolor(col).
 * - Sound: directly averages slices of audioFrameProcessedData() per object edge.
 *
 * wave_wire1dot5
 * - Entry: Wire1dot5 (Wire frame 1.5)
 * - Does: rotates the selected object as a rigid model around a startup-random
 *   axis with one frame-wide audio scale.
 * - Colours: one startup-random value per wave lifetime, drawn as tcolor(col).
 * - Sound: wire_sound_scale() averages all 1024 samples from
 *   audioFrameProcessedData().
 *
 * wave_wire1dot55
 * - Entry: Wire1dot55 (Wire frame 1.55)
 * - Does: Wire1dot5 plus a precessing rotation axis.
 * - Colours: one startup-random value per wave lifetime, drawn as tcolor(col).
 * - Sound: wire_sound_scale() averages all 1024 samples from
 *   audioFrameProcessedData().
 *
 * wave_wire1dot6
 * - Entry: Wire1dot6 (Wire frame 1.6)
 * - Does: rotates the selected object while stretching each vertex radially
 *   according to a stable audio slice.
 * - Colours: one startup-random value per wave lifetime, drawn as tcolor(col).
 * - Sound: vertex_sound_stretch() hashes object-space vertices into small
 *   slices of audioFrameProcessedData().
 *
 * wave_wire2
 * - Entry: Wire2 (Wire frame 2)
 * - Does: draws a swarm of selected-object copies, each with its own position
 *   and local spin.
 * - Colours: one startup-random value per copy, drawn as tcolor(col[j]).
 * - Sound: no audio source; motion is time/random-state driven.
 *
 * wave_wire2dot1
 * - Entry: Wire2dot1 (Wire frame 2.1)
 * - Does: Wire2 with a different startup-random local rotation axis per copy.
 * - Colours: one startup-random value per copy, drawn as tcolor(col[j]).
 * - Sound: no audio source; motion is time/random-state driven.
 *
 * wave_lineHLdiff
 * - Entry: LineHLDiff (Difference Hor.)
 * - Does: draws one horizontal trace from the left-minus-right channel
 *   difference.
 * - Colours: tcolor(left - right + 128).
 * - Sound: prepareSoundData(BUFF_WIDTH, 0) from audioFrameProcessedData().
 *
 * wave_spiral
 * - Entry: Spiral (Spirograph)
 * - Does: draws a changing spirograph from center using current amplitude.
 * - Colours: one cycling value per frame, drawn as tcolor(col).
 * - Sound: audioAnalysis.amplitude, amplitudeLeft, and amplitudeRight shape
 *   the curve; acousticContext.fire() counts down to the next random twist count.
 *
 * wave_pyro
 * - Entry: Pyro (Fire works)
 * - Does: launches and animates bouncing firework streaks on fire events.
 * - Colours: one random value per firework, drawn as tcolor(col).
 * - Sound: acousticContext.fire() controls launch and vertical velocity.
 *
 * wave_warp
 * - Entry: Warp (Space warp)
 * - Does: launches expanding rotating radial rings on fire events.
 * - Colours: one random value per ring, drawn as tcolor(col).
 * - Sound: acousticContext.fire() controls ring speed, trail count, and rotation.
 *
 * wave_laser
 * - Entry: Laser (Laser)
 * - Does: draws beams from the center to moving endpoints driven by adjacent
 *   sample differences.
 * - Colours: table-mapped channel intensity, so louder samples use higher
 *   palette table entries.
 * - Sound: prepareSoundData((BUFF_WIDTH / 10) + 1, 0) from audioFrameProcessedData().
 *
 * wave_corner
 * - Entry: Corner (Corner)
 * - Does: on fire events, draws a bright corner/axis shape from a moving point.
 * - Colours: raw fading palette indices 255 >> i.
 * - Sound: acousticContext.fire() controls movement and thickness, then is cleared.
 *
 * wave_jump
 * - Entry: Jump (Jumping points)
 * - Does: per-column points jump away from the vertical center with inertia.
 * - Colours: raw palette index 255.
 * - Sound: prepareSoundData(BUFF_WIDTH) from audioFrameProcessedData(); left and
 *   right samples are summed per column.
 *
 * wave_sticks
 * - Entry: Sticks (Random sticks)
 * - Does: draws random line segments across the buffer on fire events.
 * - Colours: raw random palette index Random(256), bypassing the table.
 * - Sound: acousticContext.fire() controls how many sticks are drawn.
 *
 * wave_grid
 * - Entry: Grid (Diagnostic grid)
 * - Does: fills the buffer with diagnostic bands, diagonals, and corner marks.
 * - Colours: fixed raw palette indices.
 * - Sound: none.
 *
 * wave_none
 * - Entry: None (No wave drawing)
 * - Does: draws nothing.
 * - Colours: none.
 * - Sound: none.
 */

/*****************************************************************************/
extern int Bsine[MAX_BUFF_WIDTH];
static void draw_line(int x1, int y1, int x2, int y2, int c);

OptionOnOff use_objects("use-objects", 1); /* use 3-D objects */
CoreOptionEntry* read_object(FILE* file, const char* name, const char* dir, const char* total_name);
int obj_change = 1; /* flag to indicate an object change occurred */

/*
 * Object waves have two kinds of work:
 *
 *  - startup work, such as picking a random rotation axis or placing the
 *    Wire2 swarm;
 *  - frame work, such as advancing angles and drawing the current object.
 *
 * A wave is "starting" when it has just become the active wave, or when the
 * object option has changed and the wave needs to refresh state that depends
 * on the selected model.  The flag is consumed here so callers get one clean
 * startup event, not a repeated event every frame.
 */
static int object_wave_starting() {
    int starting = wave_just_started || obj_change;
    obj_change = 0;
    return starting;
}

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

CoreOptionEntry* _objects[] = { new ObjectEntry(cube1, "bigH", "Big H") };
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

#define PRECESSION_TIME_MIN 4.0
#define PRECESSION_TIME_MAX 12.0

/*
 * Rotate a point around an arbitrary unit axis using Rodrigues' rotation
 * formula.  Most of the old wire code hard-coded the y axis, but this lets
 * the newer object waves keep the same cheap integer-angle timing while
 * choosing a more interesting axis at startup.
 */
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

/*
 * Pick a non-zero random vector and normalize it for rotate_axis().
 * The range is intentionally small because only the direction survives
 * normalization; large random coordinates buy nothing here.
 */
static void random_axis(double axis[3]) {
    double scale;

    do {
        axis[0] = (double)(rand() % 201 - 100);
        axis[1] = (double)(rand() % 201 - 100);
        axis[2] = (double)(rand() % 201 - 100);
        scale = sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    } while (scale == 0.0);

    axis[0] /= scale;
    axis[1] /= scale;
    axis[2] /= scale;
}

static double random_unit() {
    return (double)rand() / (double)RAND_MAX;
}

/*
 * Precess an axis around its original direction.
 *
 * baseAxis is the fixed startup axis A.  coneAngle is the angle away from A.
 * phase is the current position around the cone.  The result is another unit
 * axis suitable for rotate_axis().
 */
static void precess_axis(const double baseAxis[3], double coneAngle, double phase, double axis[3]) {
    double side0[3];
    double side1[3];
    double ref[3] = { 0.0, 1.0, 0.0 };
    double scale;

    if (fabs(baseAxis[1]) > 0.90) {
        ref[0] = 1.0;
        ref[1] = 0.0;
    }

    side0[0] = baseAxis[1] * ref[2] - baseAxis[2] * ref[1];
    side0[1] = baseAxis[2] * ref[0] - baseAxis[0] * ref[2];
    side0[2] = baseAxis[0] * ref[1] - baseAxis[1] * ref[0];

    scale = sqrt(side0[0] * side0[0] + side0[1] * side0[1] + side0[2] * side0[2]);
    if (scale == 0.0) {
        axis[0] = baseAxis[0];
        axis[1] = baseAxis[1];
        axis[2] = baseAxis[2];
        return;
    }

    side0[0] /= scale;
    side0[1] /= scale;
    side0[2] /= scale;

    side1[0] = baseAxis[1] * side0[2] - baseAxis[2] * side0[1];
    side1[1] = baseAxis[2] * side0[0] - baseAxis[0] * side0[2];
    side1[2] = baseAxis[0] * side0[1] - baseAxis[1] * side0[0];

    double ccone = cos(coneAngle);
    double scone = sin(coneAngle);
    double cphase = cos(phase);
    double sphase = sin(phase);

    axis[0] = baseAxis[0] * ccone + (side0[0] * cphase + side1[0] * sphase) * scone;
    axis[1] = baseAxis[1] * ccone + (side0[1] * cphase + side1[1] * sphase) * scone;
    axis[2] = baseAxis[2] * ccone + (side0[2] * cphase + side1[2] * sphase) * scone;
}

/*
 * Map a vertex to a stable little slice of the audio buffer.
 *
 * Wire1 scales each edge independently, which can pull shared vertices apart.
 * Wire1dot6 instead hashes the original object-space vertex coordinates into
 * the 1024-sample sound buffer, averages a few stereo samples from that point,
 * and returns a modest radial stretch.  The same vertex coordinate therefore
 * gets the same stretch wherever it appears in the line list.
 */
static double vertex_sound_stretch(int x, int y, int z) {
    int i;
    int sound = 0;
    int slice = mod(x * 73 + y * 151 + z * 251, 1024);
    const int samples = 8;

    for (i = 0; i < samples; i++) {
        int sample = (slice + i) & 1023;
        sound += abs(audioFrameProcessedData()[sample][0]);
        sound += abs(audioFrameProcessedData()[sample][1]);
    }

    double amp = (double)sound / (double)(samples * 2 * 128);
    if (amp > 1.0)
        amp = 1.0;

    return 1.0 + amp * 0.35;
}

static int random_wire_color() {
    return abs(rand() % 256);
}

struct WireObjectFrame {
    WObject* obj;
    int n;
    int mx, my, mz, m;
    double px, py, pz;
    double objectHalf;
    double screenScale;
};

/*
 * Gather the common geometry facts needed by the single-object wire waves.
 * Object loading has already shifted coordinates so the lower corner is at
 * the origin; the upper corner therefore gives both the midpoint and the
 * largest extent used for normalization.
 */
static int setup_wire_object(WireObjectFrame& frame) {
    int j;

    ObjectEntry* objE = (ObjectEntry*)CthughaBuffer::current->object.current();
    if (objE == NULL) {
        CthughaBuffer::current->wave.change(+1, 0);
        return 0;
    }

    frame.obj = objE->obj;
    if (frame.obj == NULL) {
        CthughaBuffer::current->wave.change(+1, 0);
        return 0;
    }

    frame.mx = frame.my = frame.mz = 0;

    for (frame.n = 0; frame.obj[frame.n][0][0] != -1; frame.n++)
        for (j = 0; j < 2; j++) {
            if (frame.obj[frame.n][j][0] > frame.mx)
                frame.mx = frame.obj[frame.n][j][0];

            if (frame.obj[frame.n][j][1] > frame.my)
                frame.my = frame.obj[frame.n][j][1];

            if (frame.obj[frame.n][j][2] > frame.mz)
                frame.mz = frame.obj[frame.n][j][2];
        }

    frame.m = (frame.mx > frame.my) ? frame.mx : frame.my;
    frame.m = (frame.m > frame.mz) ? frame.m : frame.mz;
    if (frame.m <= 0)
        frame.m = 1;

    frame.px = (double)frame.mx / 2;
    frame.py = (double)frame.my / 2;
    frame.pz = (double)frame.mz / 2;
    frame.objectHalf = (double)frame.m / 2.0;
    if (frame.objectHalf < 1.0)
        frame.objectHalf = 1.0;

    frame.screenScale = (double)min(BUFF_HEIGHT, BUFF_WIDTH) * 0.75;

    return 1;
}

static double wire_sound_scale(double screenScale) {
    int i;
    int sound = 0;

    for (i = 0; i < 1024; i++) {
        sound += abs(audioFrameProcessedData()[i][0]);
        sound += abs(audioFrameProcessedData()[i][1]);
    }

    return screenScale * (0.60 + 1.40 * ((double)sound / (double)(1024 * 2 * 128)));
}

static void wire_point(
    const WireObjectFrame& frame, int segment, int endpoint, double& x, double& y, double& z) {
    x = (frame.obj[segment][endpoint][0] - frame.px) / frame.objectHalf;
    y = (frame.obj[segment][endpoint][1] - frame.py) / frame.objectHalf;
    z = (frame.obj[segment][endpoint][2] - frame.pz) / frame.objectHalf;
}

static void project_wire_point(
    double ax, double ay, double az, double scale, double cameraDistance, int& sx, int& sy) {
    sx = int((double)ax * scale / (az + cameraDistance) + MID_X);
    sy = int((double)ay * scale / (az + cameraDistance) + MID_Y);
}

static void draw_axis_wire_model(
    const WireObjectFrame& frame, const double axis[3], int theta, double scale, double cameraDistance, int col) {
    int i, x1, y1, x2, y2;
    double x, y, z, ax, ay, az;

    for (i = 0; i < frame.n; i++) {
        wire_point(frame, i, 0, x, y, z);
        rotate_axis(x, y, z, axis, theta, ax, ay, az);
        project_wire_point(ax, ay, az, scale, cameraDistance, x1, y1);

        wire_point(frame, i, 1, x, y, z);
        rotate_axis(x, y, z, axis, theta, ax, ay, az);
        project_wire_point(ax, ay, az, scale, cameraDistance, x2, y2);

        draw_line(x1, y1, x2, y2, tcolor(col));
    }
}

// #define putat(x,y,val)	active_buffer[ addr( (x) , (y) ) ] = val
void putat(int x, int y, int val) {
    int a = addr(x, y);

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

void putpixel_cut(int x, int y, int val) {
    if ((x < 0) || (x >= BUFF_WIDTH))
        return;
    if ((y < 0) || (y >= BUFF_HEIGHT))
        return;
    active_buffer[addr(x, y)] = val;
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
        data[i][0] = audioFrameProcessedData()[p >> 16][0] + add;
        data[i][1] = audioFrameProcessedData()[p >> 16][1] + add;

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

/* Writes no pixels. */
void wave_none() { }

/* Writes fixed raw palette indices: 48, 96, 128, 160, 192, 224, and 255. */
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

/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
void wave_dotHor() { /* dot horizontal */
    int x, tmp;

    prepareSoundData(BUFF_WIDTH);

    for (x = 0; x < BUFF_WIDTH; x++) {
        tmp = data[x][0];
        putpixel_cut(x >> 1, LOW_LINE - (tmp >> int(CthughaBuffer::current->waveScale)), tcolor(tmp));

        tmp = data[x][1];
        putpixel_cut((x + BUFF_WIDTH) >> 1, LOW_LINE - (tmp >> int(CthughaBuffer::current->waveScale)),
            tcolor(tmp));
    }
}

/*****************************************************************************
 * Dot vertical
 *****************************************************************************/

/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
void wave_dotVert() { /* dot vertical */
    int tmp, x;

    prepareSoundData(BOTTOM);

    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][0];
        putpixel_cut(MID_X - (tmp >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));

        tmp = data[x][1];
        putpixel_cut(MID_X + (tmp >> int(CthughaBuffer::current->waveScale)), x, tcolor(tmp));
    }
}

/****************************************************************************
 * Line horizontal
 ****************************************************************************/

/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
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

/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
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

/* Writes table-mapped spike-height indices, tcolor(0..BOTTOM-1). */
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

/* Writes table-mapped scaled-amplitude indices, tcolor(amplitude). */
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

/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
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
/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
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

/* Writes table-mapped left-channel sample indices, tcolor(sample), across 0..255. */
void wave_buff11() { /* Lissa */
    int tmp, x, tmp2;

    prepareSoundData(BUFF_WIDTH);

    for (x = 0; x < BUFF_WIDTH; x++) {
        tmp = data[x][0];
        tmp2 = data[x][1];

        putat((tmp2 + 32) % BUFF_WIDTH, (tmp + 200 - 28) % BOTTOM, tcolor(tmp));
    }
}

/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
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

/* Writes fixed raw palette index 255. */
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
/* Writes fixed raw palette index 255. */
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

/* Writes table-mapped sound sample indices, mostly tcolor(sample) across 0..255. */
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

/* Writes table-mapped signed-amplitude indices, tcolor(sample). */
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

/* Writes table-mapped sine lookup indices, tcolor(sine[sample]). */
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

/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
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

/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
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

/* Writes table-mapped sound sample indices, tcolor(sample + 128), across 0..255. */
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
/* Writes table-mapped sound sample indices, tcolor(sample), across 0..255. */
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
/* Writes table-mapped stereo-difference indices, tcolor(left - right + 128). */
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

/*
 * Wire1 is the original single-object wire wave.  It intentionally keeps its
 * old behavior: every edge samples a different slice of the sound buffer, and
 * the two endpoints use separate channel-derived scales.  That makes the
 * object fracture under sound, because a shared vertex can be projected at
 * different radii for different edges.
 */
/* Writes one startup-random table-mapped wire index, tcolor(random 0..255). */
void wave_wire1() {
    static double theta = 0;
    static int col = 255;
    double st, ct, ax, ay, az, x, y, z;
    const double cameraDistance = 3.0;
    WireObjectFrame frame;

    if (!setup_wire_object(frame))
        return;

    int i, j, x1, y1, x2, y2;

    if (object_wave_starting())
        col = random_wire_color();

    theta += M_PI / 45.0;

    st = sin(theta);
    ct = cos(theta);

    for (i = 0; i < frame.n; i++) {
        int s[2];
        int sampleCount;
        double scale0, scale1;

        s[0] = s[1] = 0;
        sampleCount = max(1024 / frame.n, 1);
        for (j = 0; j < sampleCount; j++) {
            int sample = min(i * sampleCount + j, 1023);
            s[0] += abs(audioFrameProcessedData()[sample][0]);
            s[1] += abs(audioFrameProcessedData()[sample][1]);
        }

        scale0 = frame.screenScale * (0.60 + 1.40 * ((double)s[0] / (double)(sampleCount * 128)));
        scale1 = frame.screenScale * (0.60 + 1.40 * ((double)s[1] / (double)(sampleCount * 128)));

        wire_point(frame, i, 0, x, y, z);

        ax = x * ct + z * st;
        ay = y;
        az = z * ct - x * st;

        project_wire_point(ax, ay, az, scale0, cameraDistance, x1, y1);

        wire_point(frame, i, 1, x, y, z);

        ax = x * ct + z * st;
        ay = y;
        az = z * ct - x * st;

        project_wire_point(ax, ay, az, scale1, cameraDistance, x2, y2);

        draw_line(x1, y1, x2, y2, tcolor(col));
    }
}

/*
 * Wire1dot5 keeps Wire1's single rotating object, but treats it as a rigid
 * model.  The whole audio buffer contributes to one frame-wide scale factor,
 * so all vertices move together and the wireframe remains coherent.
 */
/* Writes one startup-random table-mapped wire index, tcolor(random 0..255). */
void wave_wire1dot5() {
    static int theta = 0;
    static int col = 255;
    static double axis[3] = { 0.0, 1.0, 0.0 };
    double scale;
    const double cameraDistance = 3.0;
    WireObjectFrame frame;

    if (!setup_wire_object(frame))
        return;

    if (object_wave_starting()) {
        random_axis(axis);
        col = random_wire_color();
    }

    theta += 2;
    scale = wire_sound_scale(frame.screenScale);

    draw_axis_wire_model(frame, axis, theta, scale, cameraDistance, col);
}

/*
 * Wire1dot55 adds axial precession to Wire1dot5.  At startup it chooses an
 * original axis A, a cone angle away from A, and a precession period.  Each
 * frame the actual rotation axis moves around that cone, while the model still
 * keeps Wire1dot5's coherent frame-wide audio scale.
 */
/* Writes one startup-random table-mapped wire index, tcolor(random 0..255). */
void wave_wire1dot55() {
    static int theta = 0;
    static int col = 255;
    static double baseAxis[3] = { 0.0, 1.0, 0.0 };
    static double coneAngle = 0.0;
    static double precessionTime = PRECESSION_TIME_MIN;
    static double precessionStart = 0.0;
    double axis[3];
    double scale;
    const double cameraDistance = 3.0;
    WireObjectFrame frame;

    if (!setup_wire_object(frame))
        return;

    if (object_wave_starting()) {
        random_axis(baseAxis);
        coneAngle = random_unit() * M_PI;
        precessionTime = PRECESSION_TIME_MIN
            + random_unit() * (PRECESSION_TIME_MAX - PRECESSION_TIME_MIN);
        precessionStart = now;
        col = random_wire_color();
    }

    precess_axis(baseAxis, coneAngle, M_PI2 * (now - precessionStart) / precessionTime, axis);

    theta += 2;
    scale = wire_sound_scale(frame.screenScale);

    draw_axis_wire_model(frame, axis, theta, scale, cameraDistance, col);
}

/*
 * Wire1dot6 is the elastic variant.  It still rotates around one startup axis,
 * but each vertex is pushed radially outward according to its own stable audio
 * slice.  The stretch happens before rotation and perspective projection.
 */
/* Writes one startup-random table-mapped wire index, tcolor(random 0..255). */
void wave_wire1dot6() {
    static int theta = 0;
    static int col = 255;
    static double axis[3] = { 0.0, 1.0, 0.0 };
    double ax, ay, az, x, y, z;
    double stretch;
    const double cameraDistance = 4.0;
    WireObjectFrame frame;

    if (!setup_wire_object(frame))
        return;

    int i, x1, y1, x2, y2;

    if (object_wave_starting()) {
        random_axis(axis);
        col = random_wire_color();
    }

    theta += 2;

    for (i = 0; i < frame.n; i++) {
        /*
         * Stretch all three local axes by the same per-vertex amount.  This is
         * radial in object space, so it changes depth as well as silhouette.
         * The camera is therefore a little farther back than Wire1dot5: close
         * enough to show movement, but not so close that near-side vertices
         * explode as az approaches -cameraDistance.
         */
        stretch = vertex_sound_stretch(frame.obj[i][0][0], frame.obj[i][0][1], frame.obj[i][0][2]);
        wire_point(frame, i, 0, x, y, z);
        x *= stretch;
        y *= stretch;
        z *= stretch;

        rotate_axis(x, y, z, axis, theta, ax, ay, az);

        project_wire_point(ax, ay, az, frame.screenScale, cameraDistance, x1, y1);

        stretch = vertex_sound_stretch(frame.obj[i][1][0], frame.obj[i][1][1], frame.obj[i][1][2]);
        wire_point(frame, i, 1, x, y, z);
        x *= stretch;
        y *= stretch;
        z *= stretch;

        rotate_axis(x, y, z, axis, theta, ax, ay, az);

        project_wire_point(ax, ay, az, frame.screenScale, cameraDistance, x2, y2);

        draw_line(x1, y1, x2, y2, tcolor(col));
    }
}

#define nobj 10
#define whirlyRadius 45

static void init_wire2_copy(int loc[3], int& psi, int& rate, int& col) {
    int j, k;

    loc[1] = rand() % (whirlyRadius * 2) - whirlyRadius;
    j = rand() % 320;
    k = 1 + rand() % (whirlyRadius - 1);
    loc[0] = int(isin(j) * k);
    loc[2] = int(icos(j) * k);

    rate = 1 + rand() % 7;
    if (rand() % 2)
        rate *= -1;
    psi = rand() % 320;
    col = random_wire_color();
}

/*
 * Wire2 draws a little swarm of copies of the selected object.  The object
 * itself is not reloaded or copied here; every frame reads the currently
 * selected WObject and draws it at each saved swarm location.
 */
/* Writes per-copy startup-random table-mapped wire indices, tcolor(random 0..255). */
void wave_wire2() {
    /*
     * Persistent swarm state.  Each copy has a position in the blob, its own
     * local spin angle/rate, and a color.  This state belongs to the wave, not
     * to any particular object, so changing the object swaps the geometry into
     * the existing animated pattern.
     */
    static int theta = 0, psi[nobj], rate[nobj], col[nobj];
    static int loc[nobj][3];
    static double blobAxis[3] = { 0.0, 1.0, 0.0 };

    /* Scratch vertex in object-local or blob-local coordinates. */
    double x, y, z;

    /* Accumulated 3D point after blob rotation plus local object spin. */
    double ax, ay, az;

    /* Projection scale and screen origin. */
    double scl;

    /* Sine/cosine for each object's local spin. */
    double sto, cto;

    register int i, j, x1, y1, x2, y2;
    register int mx, my, mz, m;

    /*
     * Object midpoint and normalization divisor in source coordinate space.
     * The legacy formula deliberately uses the y extent, so flat/wide models
     * keep the same tall, spindly Wire2 character as the built-in glyphs.
     */
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
     * Startup state is refreshed when Wire2 first starts, or when callers
     * explicitly mark the object set as changed. Between those moments the
     * frame loop only advances theta/psi, so the blob axis and swarm layout do
     * not jump every frame.
     */
    if (object_wave_starting()) {
        /* Rotate the whole blob around a random axis for this swarm lifetime. */
        random_axis(blobAxis);

        /*
         * Pick a shell position and local spin for each copy. Avoid zero
         * radius and zero spin so no model sits fixed in the center.
         */
        for (i = 0; i < nobj; i++) {
            init_wire2_copy(loc[i], psi[i], rate[i], col[i]);
            CTH_DEBUG("model %d: rate %d, psi %d, col %d\n", i, rate[i], psi[i], col[i]);

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

    /* Screen-space projection scale.  Wire2 is intentionally height-led. */
    scl = (double)BUFF_HEIGHT * 0.80;
    if (!scl)
        scl = 1;

    ox = BUFF_WIDTH / 2 - mx / 2;
    oy = BUFF_HEIGHT / 2 - my / 2;

    for (j = 0; j < nobj; j++) {

        /* Each copy still spins around its own local y axis. */
        sto = isin(psi[j]);
        cto = icos(psi[j]);

        psi[j] += rate[j];

        for (i = 0; theObj[i][0][0] != -1; i++) {

            /* Rotate this copy's blob position around the swarm axis. */
            x = loc[j][0];
            y = loc[j][1];
            z = loc[j][2];

            rotate_axis(x, y, z, blobAxis, theta, ax, ay, az);

            /* Normalize the first endpoint and spin it around local y. */
            x = (theObj[i][0][0] - omx) / om;
            y = (theObj[i][0][1] - omy) / om;
            z = (theObj[i][0][2] - omz) / om;

            ax += x * cto + z * sto;
            ay += y;
            az += z * cto - x * sto;

            /* Project the first endpoint to the 2D Cthugha buffer. */
            x1 = int((double)ax * scl / (az + m) + ox);
            y1 = int((double)ay * scl / (az + m) + oy);

            /* Repeat for the second endpoint of the same wire segment. */
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

/*
 * Wire2dot1 is Wire2 with one extra degree of freedom: each copy has its own
 * local rotation axis.  The blob still rotates around one shared axis, but the
 * individual models no longer all tumble around their local y axes.
 */
/* Writes per-copy startup-random table-mapped wire indices, tcolor(random 0..255). */
void wave_wire2dot1() {
    /*
     * This mirrors Wire2's persistent swarm state, with modelAxis storing one
     * normalized local spin axis per copy.  The axes are chosen during startup
     * and then reused frame after frame, so the motion is varied but stable.
     */
    static int theta = 0, psi[nobj], rate[nobj], col[nobj];
    static int loc[nobj][3];
    static double blobAxis[3] = { 0.0, 1.0, 0.0 };
    static double modelAxis[nobj][3];

    double x, y, z;
    double ax, ay, az;
    double lx, ly, lz;
    double scl;

    register int i, j, x1, y1, x2, y2;
    register int mx, my, mz, m;
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

    if (object_wave_starting()) {
        random_axis(blobAxis);

        for (i = 0; i < nobj; i++) {
            init_wire2_copy(loc[i], psi[i], rate[i], col[i]);
            CTH_DEBUG("model %d: rate %d, psi %d, col %d\n", i, rate[i], psi[i], col[i]);

            random_axis(modelAxis[i]);
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
    m = whirlyRadius * 3 / 2;

    scl = (double)BUFF_HEIGHT * 0.80;
    if (!scl)
        scl = 1;

    ox = BUFF_WIDTH / 2 - mx / 2;
    oy = BUFF_HEIGHT / 2 - my / 2;

    for (j = 0; j < nobj; j++) {

        psi[j] += rate[j];

        for (i = 0; theObj[i][0][0] != -1; i++) {

            x = loc[j][0];
            y = loc[j][1];
            z = loc[j][2];

            rotate_axis(x, y, z, blobAxis, theta, ax, ay, az);

            /*
             * Unlike Wire2, rotate the endpoint around this copy's own random
             * axis.  Every copy has a different axis, but both endpoints of a
             * line segment use the same axis for that copy.
             */
            x = (theObj[i][0][0] - omx) / om;
            y = (theObj[i][0][1] - omy) / om;
            z = (theObj[i][0][2] - omz) / om;

            rotate_axis(x, y, z, modelAxis[j], psi[j], lx, ly, lz);

            ax += lx;
            ay += ly;
            az += lz;

            x1 = int((double)ax * scl / (az + m) + ox);
            y1 = int((double)ay * scl / (az + m) + oy);

            x = loc[j][0];
            y = loc[j][1];
            z = loc[j][2];

            rotate_axis(x, y, z, blobAxis, theta, ax, ay, az);

            x = (theObj[i][1][0] - omx) / om;
            y = (theObj[i][1][1] - omy) / om;
            z = (theObj[i][1][2] - omz) / om;

            rotate_axis(x, y, z, modelAxis[j], psi[j], lx, ly, lz);

            ax += lx;
            ay += ly;
            az += lz;

            x2 = int((double)ax * scl / (az + m) + ox);
            y2 = int((double)ay * scl / (az + m) + oy);

            draw_line(x1, y1, x2, y2, tcolor(col[j]));
        }
    }
}

/* by Russ */
/* Writes one cycling table-mapped index per frame, tcolor(col 0..255). */
void wave_spiral(void) {
    int i, amp, mx, cx, cy;
    double x, y, ox, oy, a, la, ra;
    static int ofs = 0, col = 0, loops, loopcount = 0;

    col++;
    col %= 256;
    ofs++;
    ofs %= 320;

    if (loopcount <= 0) {
        loopcount = 1 + abs(rand() % 32);
        loops = 2 + abs(rand() % 8);
    }

    if (acousticContext.fire())
        loopcount--;


    mx = min(BUFF_WIDTH, BUFF_HEIGHT);
    cx = BUFF_WIDTH / 2;
    cy = BUFF_HEIGHT / 2;

    amp = audioAnalysis.amplitude;
    int al = audioAnalysis.amplitudeLeft;
    int ar = audioAnalysis.amplitudeRight;

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
/* Writes per-firework random table-mapped indices, tcolor(random 0..255). */
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

        } else if (acousticContext.fire()) {
            int fire = acousticContext.fire();

            /* maintain a maximum attack value for scaling purposes */
            if (fire * 4 > maxA)
                maxA = fire * 4;

            /* slowly reduce max over time to keep it in line with the average levels */
            if (maxA > 30)
                maxA--;

            /* fire off a new firework */
            theWorks[i].dur = 0;
            theWorks[i].oxp = theWorks[i].xp = rand() % BUFF_WIDTH;
            theWorks[i].oyp = theWorks[i].yp = BUFF_HEIGHT - 4;
            theWorks[i].xv = (rand() % 20) - 10;
            theWorks[i].yv = -(fire * maxV / (maxA / 4));
            theWorks[i].col = rand() % 256;
            acousticContext.setFire(fire * 2 / 3);
        }

    /* test rocket exploded, reset */
}

#define maxWarps 15
#define maxWarpTrails 20

typedef struct {
    int r, s, theta, omg, trails, col, rgrav;
} WarpRing;

/* Writes per-ring random table-mapped indices, tcolor(random 0..255). */
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

        } else if (acousticContext.fire()) {
            int fire = acousticContext.fire();

            /* maintain a maximum attack value for scaling purposes*/
            if (fire * 4 > maxA)
                maxA = fire * 4;

            /* slowly reduce max over time to keep it in line with the average levels */
            if (maxA > 30)
                maxA--;

            /* fire off a new warp ring */
            theWarps[i].r = 0;
            theWarps[i].s = 3 + fire * 4 * 20 / maxA;
            theWarps[i].trails = 1 + fire * 4 * maxWarpTrails / maxA;
            theWarps[i].theta = rand() % 360;
            theWarps[i].omg = (rand() % 16 - 8) * fire * 4 / maxA;
            theWarps[i].col = rand() % 256;
            theWarps[i].rgrav = rand() % 2;
            acousticContext.resetFire();
            /*				acousticContext.setFire(fire * 2 / 3); */
        }
}

/* by Deischi */
static int laser_intensity_index(int sample) {
    return min(abs(sample) << 1, 255);
}

/* Writes table-mapped channel intensity. */
void wave_laser() {
    static int xl, xr;
    static int y = 0;
    int x, samples;

    samples = BUFF_WIDTH / 10;
    prepareSoundData(samples + 1, 0);

    y = (y + 2) % BUFF_HEIGHT;
    //y = BUFF_HEIGHT / 10;

    for (x = 0; x < samples; x++) {
        draw_line(BUFF_WIDTH / 2, BUFF_HEIGHT / 2, xl, y,
            tcolor(laser_intensity_index(data[x][0])));
        xl = (xl + abs(data[x][0] - data[x + 1][0])) % BUFF_WIDTH;

        draw_line(BUFF_WIDTH / 2, BUFF_HEIGHT / 2, xr, BUFF_HEIGHT - y,
            tcolor(laser_intensity_index(data[x][1])));
        xr = (xr + abs(data[x][1] - data[x + 1][1])) % BUFF_WIDTH;
    }
}

/* by Deischi (inspired by RTL2) */
/* Writes raw fading indices 255, 127, 63, 31, 15, 7, 3, and 1. */
void wave_corner() {

    if (acousticContext.fire()) {
        static int x = 0, y = 0;
        int i, j, t;
        int fire = acousticContext.fire();

        x = (x + (rand() % fire)) % (BUFF_WIDTH - 16) + 8;
        y = (y + (rand() % fire)) % (BUFF_HEIGHT - 16) + 8;

        t = min(fire >> 2, 8);

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

    acousticContext.resetFire();
}

// by Deischi
/* Writes fixed raw palette index 255. */
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
/* Writes raw random palette indices, Random(256). */
void wave_sticks() {

    int n = acousticContext.fire() >> int(CthughaBuffer::current->waveScale);
    for (int i = 0; i < n; i++) {
        draw_line(Random(BUFF_WIDTH), Random(BUFF_HEIGHT), Random(BUFF_WIDTH), Random(BUFF_HEIGHT),
            Random(256));
    }
}
