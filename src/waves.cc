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

static CoreOptionEntryList waveEntries;
static CoreOptionEntryList objectEntries;
static CoreOptionEntryList waveScaleEntries;
static CoreOptionEntryList tableEntries;

extern CoreOptionEntry* _objects[];
extern int _nObjects;

WaveOption wave;
CoreOption waveScale(-1, "wave-scale", waveScaleEntries);
CoreOption table(-1, "table", tableEntries);
CoreOption object(-1, "object", objectEntries);

static CoreOptionEntry* wave_scales[] = { new CoreOptionEntry("scale0", "large"),
    new CoreOptionEntry("scale1", "medium"), new CoreOptionEntry("scale2", "small") };

static CoreOptionEntry* table_entries[] = {
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

WaveEntry::WaveEntry(Wave& wave_, int inUse)
    : CoreOptionEntry(wave_.name(), wave_.description(), inUse)
    , waveValue(&wave_) { }

Wave& WaveEntry::wave() const {
    return *waveValue;
}

static CoreOptionEntry* _waves[] = {
    new WaveEntry(waveCatalog[0]), // 0
    new WaveEntry(waveCatalog[1]), // 1
    new WaveEntry(waveCatalog[2]), // 2
    new WaveEntry(waveCatalog[3]), // 3
    new WaveEntry(waveCatalog[4]), // 4
    new WaveEntry(waveCatalog[5]), // 5
    new WaveEntry(waveCatalog[6]), // 6
    new WaveEntry(waveCatalog[7]), // 7
    new WaveEntry(waveCatalog[8]), // 8
    new WaveEntry(waveCatalog[9]), // 9
    new WaveEntry(waveCatalog[10]), // 10
    new WaveEntry(waveCatalog[11]), // 11
    new WaveEntry(waveCatalog[12]), // 12
    new WaveEntry(waveCatalog[13]), // 13
    new WaveEntry(waveCatalog[14]), // 14
    new WaveEntry(waveCatalog[15]), // 15
    new WaveEntry(waveCatalog[16]), // 16
    new WaveEntry(waveCatalog[17]), // 17
    new WaveEntry(waveCatalog[18]), // 18
    new WaveEntry(waveCatalog[19]), // 19
    new WaveEntry(waveCatalog[20]), // 20
    new WaveEntry(waveCatalog[21]), // 21
    new WaveEntry(waveCatalog[22]), // 22
    new WaveEntry(waveCatalog[23]), // 23
    new WaveEntry(waveCatalog[24]), // 24
    new WaveEntry(waveCatalog[25]), // 25
    new WaveEntry(waveCatalog[26]), // 26
    new WaveEntry(waveCatalog[27]), // 27
    new WaveEntry(waveCatalog[28]), // 28
    new WaveEntry(waveCatalog[29]), // 29
    new WaveEntry(waveCatalog[30]), // 30
    new WaveEntry(waveCatalog[31]), // 31
    new WaveEntry(waveCatalog[32]), // 32
    new WaveEntry(waveCatalog[33], 0), // 33
    new WaveEntry(waveCatalog[34], 0), // 34
};
static const int _nWaves = nWaveCatalogEntries;

WaveOption::WaveOption()
    : CoreOption(-1, "wave", waveEntries) { }

Wave* WaveOption::currentWave() {
    WaveEntry* entry = dynamic_cast<WaveEntry*>(current());
    return (entry != 0) ? &entry->wave() : 0;
}

static void init_wave_options() {
    wave.add(_waves, _nWaves);
    object.add(_objects, _nObjects);
    waveScale.add(wave_scales, 3);
    table.add(table_entries, 10);
}

/*
 * Wave behavior catalog
 *
 * Common fields:
 * - Entry: CoreOption name followed by description.  The X11 panel menu shows
 *   the description when present, falling back to Name(); ncurses/list-style
 *   text displays both.
 * - Does: visible drawing behavior.
 * - Colours: how the wave turns its own values into palette indices.  tableColor(runtime, )
 *   means the current table is applied first; "raw" means the byte written to
 *   buffer.activePixels() is already the palette index and bypasses the table.
 * - Sound: where the wave gets audio information.
 *
 * wave_dotHor
 * - Entry: DotHor (Dots Horizontal)
 * - Does: plots left/right channel dots across the width, with height driven by
 *   each resampled sample value.
 * - Colours: tableColor(runtime, sample), so sample value selects a table entry.
 * - Sound: prepareSoundData(buffer.width()) from audioFrameProcessedData().
 *
 * wave_dotVert
 * - Entry: DotVert (Dots Vertical)
 * - Does: plots left/right channel dots down the screen, displaced left/right
 *   from the center by sample value.
 * - Colours: tableColor(runtime, sample).
 * - Sound: prepareSoundData(BOTTOM) from audioFrameProcessedData().
 *
 * wave_lineHor
 * - Entry: LineHor (Lines Horizontal)
 * - Does: draws connected horizontal-scan oscilloscope traces, split into left
 *   and right halves.
 * - Colours: tableColor(runtime, sample).
 * - Sound: prepareSoundData(buffer.width()) from audioFrameProcessedData().
 *
 * wave_lineVert
 * - Entry: LineVert (Lines Vertical)
 * - Does: draws connected vertical traces, one channel to each side of center.
 * - Colours: tableColor(runtime, sample).
 * - Sound: prepareSoundData(BOTTOM) from audioFrameProcessedData().
 *
 * wave_spike
 * - Entry: Spike (Spikes)
 * - Does: draws filled vertical bars rising from the bottom, left and right
 *   channels split across the screen.
 * - Colours: tableColor(runtime, height), so colour follows distance up the spike rather
 *   than the original sample value.
 * - Sound: prepareSoundData(buffer.width(), 0) from audioFrameProcessedData().
 *
 * wave_spikeH
 * - Entry: SpikeH (Spikes Hollow)
 * - Does: draws only the moving outline of spike heights.
 * - Colours: tableColor(runtime, scaled absolute amplitude).
 * - Sound: prepareSoundData(buffer.width(), 0) from audioFrameProcessedData().
 *
 * wave_buff9
 * - Entry: Walking (Walking)
 * - Does: draws two vertical traces around a horizontally walking center
 *   column.
 * - Colours: tableColor(runtime, sample).
 * - Sound: prepareSoundData(BOTTOM) from audioFrameProcessedData().
 *
 * wave_buff10
 * - Entry: Falling (Falling)
 * - Does: writes channel sample dots into a row that advances downward.
 * - Colours: tableColor(runtime, sample).
 * - Sound: prepareSoundData(2 * MID_X) from audioFrameProcessedData().
 *
 * wave_buff11
 * - Entry: Lissa (Lissa)
 * - Does: draws a Lissajous-style point cloud, using right channel for x and
 *   left channel for y.
 * - Colours: tableColor(runtime, left sample).
 * - Sound: prepareSoundData(buffer.width()) from audioFrameProcessedData().
 *
 * wave_buff14
 * - Entry: LineX (Line X)
 * - Does: draws two horizontal traces with different center offsets.
 * - Colours: tableColor(runtime, sample).
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
 * - Sound: prepareSoundData(buffer.width(), 0) from audioFrameProcessedData().
 *
 * wave_pete0
 * - Entry: Pete0 (FireFlies)
 * - Does: draws two drifting point clusters whose offsets wander with the
 *   first few samples.
 * - Colours: tableColor(runtime, sample).
 * - Sound: prepareSoundData(buffer.width()) from audioFrameProcessedData().
 *
 * wave_pete1
 * - Entry: Pete1 (Pete)
 * - Does: draws two sine-shaped rows scaled by average channel energy.
 * - Colours: tableColor(runtime, signed sample).
 * - Sound: prepareSoundData(buffer.width(), 0) from audioFrameProcessedData().
 *
 * wave_pete2
 * - Entry: Pete2 (Dot VS sine)
 * - Does: plots vertical dots displaced by sample, one channel on each side.
 * - Colours: tableColor(runtime, sine[sample]), so colour uses a sine lookup of the sample.
 * - Sound: prepareSoundData(buffer.height()) from audioFrameProcessedData().
 *
 * wave_fract1
 * - Entry: Fract1 (Zippy 1)
 * - Does: walks two persistent points around the buffer using half-sized
 *   differences between neighboring samples.
 * - Colours: tableColor(runtime, sample).
 * - Sound: prepareSoundData(buffer.width()) from audioFrameProcessedData().
 *
 * wave_fract2
 * - Entry: Fract2 (Zippy 2)
 * - Does: like Fract1, but uses full sample differences for a sharper walk.
 * - Colours: tableColor(runtime, sample).
 * - Sound: prepareSoundData(buffer.width()) from audioFrameProcessedData().
 *
 * wave_test
 * - Entry: Test (Test)
 * - Does: draws sine-shaped rows scaled by average channel energy, similar to
 *   Pete1 but with unsigned colour lookup.
 * - Colours: tableColor(runtime, sample + 128).
 * - Sound: prepareSoundData(buffer.width(), 0) from audioFrameProcessedData().
 *
 * wave_aaron
 * - Entry: Aaron (Rings of Fire)
 * - Does: draws two moving ring/rosette point sets when the buffer is large
 *   enough, otherwise advances to the next wave.
 * - Colours: tableColor(runtime, sample).
 * - Sound: prepareSoundData(buffer.width()) from audioFrameProcessedData().
 *
 * wave_wire1
 * - Entry: Wire1 (Wire frame 1)
 * - Does: rotates the selected object; each edge endpoint can scale
 *   independently, giving a fractured audio-reactive wireframe.
 * - Colours: one startup-random value per wave lifetime, drawn as tableColor(runtime, col).
 * - Sound: directly averages slices of audioFrameProcessedData() per object edge.
 *
 * wave_wire1dot5
 * - Entry: Wire1dot5 (Wire frame 1.5)
 * - Does: rotates the selected object as a rigid model around a startup-random
 *   axis with one frame-wide audio scale.
 * - Colours: one startup-random value per wave lifetime, drawn as tableColor(runtime, col).
 * - Sound: wire_sound_scale() averages all 1024 samples from
 *   audioFrameProcessedData().
 *
 * wave_wire1dot55
 * - Entry: Wire1dot55 (Wire frame 1.55)
 * - Does: Wire1dot5 plus a precessing rotation axis.
 * - Colours: one startup-random value per wave lifetime, drawn as tableColor(runtime, col).
 * - Sound: wire_sound_scale() averages all 1024 samples from
 *   audioFrameProcessedData().
 *
 * wave_wire1dot6
 * - Entry: Wire1dot6 (Wire frame 1.6)
 * - Does: rotates the selected object while stretching each vertex radially
 *   according to a stable audio slice.
 * - Colours: one startup-random value per wave lifetime, drawn as tableColor(runtime, col).
 * - Sound: vertex_sound_stretch() hashes object-space vertices into small
 *   slices of audioFrameProcessedData().
 *
 * wave_wire2
 * - Entry: Wire2 (Wire frame 2)
 * - Does: draws a swarm of selected-object copies, each with its own position
 *   and local spin.
 * - Colours: one startup-random value per copy, drawn as tableColor(runtime, col[j]).
 * - Sound: no audio source; motion is time/random-state driven.
 *
 * wave_wire2dot1
 * - Entry: Wire2dot1 (Wire frame 2.1)
 * - Does: Wire2 with a different startup-random local rotation axis per copy.
 * - Colours: one startup-random value per copy, drawn as tableColor(runtime, col[j]).
 * - Sound: no audio source; motion is time/random-state driven.
 *
 * wave_lineHLdiff
 * - Entry: LineHLDiff (Difference Hor.)
 * - Does: draws one horizontal trace from the left-minus-right channel
 *   difference.
 * - Colours: tableColor(runtime, left - right + 128).
 * - Sound: prepareSoundData(buffer.width(), 0) from audioFrameProcessedData().
 *
 * wave_spiral
 * - Entry: Spiral (Spirograph)
 * - Does: draws a changing spirograph from center using current amplitude.
 * - Colours: one cycling value per frame, drawn as tableColor(runtime, col).
 * - Sound: audioAnalysis.amplitude, amplitudeLeft, and amplitudeRight shape
 *   the curve; acousticContext.fire() counts down to the next random twist count.
 *
 * wave_pyro
 * - Entry: Pyro (Fire works)
 * - Does: launches and animates bouncing firework streaks on fire events.
 * - Colours: one random value per firework, drawn as tableColor(runtime, col).
 * - Sound: acousticContext.fire() controls launch and vertical velocity.
 *
 * wave_warp
 * - Entry: Warp (Space warp)
 * - Does: launches expanding rotating radial rings on fire events.
 * - Colours: one random value per ring, drawn as tableColor(runtime, col).
 * - Sound: acousticContext.fire() controls ring speed, trail count, and rotation.
 *
 * wave_laser
 * - Entry: Laser (Laser)
 * - Does: draws beams from the center to moving endpoints driven by adjacent
 *   sample differences.
 * - Colours: table-mapped channel intensity, so louder samples use higher
 *   palette table entries.
 * - Sound: prepareSoundData((buffer.width() / 10) + 1, 0) from audioFrameProcessedData().
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
 * - Sound: prepareSoundData(buffer.width()) from audioFrameProcessedData(); left and
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
static void draw_line(CthughaBuffer& buffer, int x1, int y1, int x2, int y2, int c);

OptionOnOff use_objects("use-objects", 1); /* use 3-D objects */
CoreOptionEntry* read_object(FILE* file, const char* name, const char* dir, const char* total_name);

/*
 * Object waves have two kinds of work:
 *
 *  - startup work, such as picking a random rotation axis or placing the
 *    Wire2 swarm;
 *  - frame work, such as advancing angles and drawing the current object.
 *
 * Object waves need configuration when their injected wave configuration has
 * changed and the wave needs to refresh state that depends on the selected
 * model.
 */
static int object_wave_needs_configuration(WaveRuntime& runtime) {
    return runtime.needsConfiguration();
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

WObject* currentWaveObject() {
    ObjectEntry* entry = static_cast<ObjectEntry*>(object.current());
    return (entry != NULL) ? entry->obj : NULL;
}

/*
 * initialize, load objects
 */
int init_wave() {

    init_wave_options();
    init_tables();

    /* load objects from File  */
    if (int(use_objects)) {

        CTH_INFO("  loading 3-D objects...");
        object.load(object_path, "/obj/", ".obj", read_object);
        CTH_INFO("\n  number of 3-D objects: %d\n", object.getNEntries());
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

#define addr(x, y) ((x) + (y) * buffer.width())
#define BOTTOM (buffer.height() - 1)
#define MID_Y (buffer.height() >> 1)
#define MID_X (buffer.width() >> 1)
#define LOW_LINE (buffer.height() - buffer.height() / 10)

static int tableColor(WaveRuntime& runtime, int value) {
    return tables[runtime.table][value];
}

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
static int setup_wire_object(CthughaBuffer& buffer, WaveRuntime& runtime,
    WireObjectFrame& frame) {
    int j;

    frame.obj = runtime.object;
    if (frame.obj == NULL)
        return 0;

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

    frame.screenScale = (double)min(buffer.height(), buffer.width()) * 0.75;

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

static void project_wire_point(const CthughaBuffer& buffer,
    double ax, double ay, double az, double scale, double cameraDistance, int& sx, int& sy) {
    sx = int((double)ax * scale / (az + cameraDistance) + MID_X);
    sy = int((double)ay * scale / (az + cameraDistance) + MID_Y);
}

static void draw_axis_wire_model(CthughaBuffer& buffer, WaveRuntime& runtime,
    const WireObjectFrame& frame, const double axis[3], int theta, double scale, double cameraDistance, int col) {
    int i, x1, y1, x2, y2;
    double x, y, z, ax, ay, az;

    for (i = 0; i < frame.n; i++) {
        wire_point(frame, i, 0, x, y, z);
        rotate_axis(x, y, z, axis, theta, ax, ay, az);
        project_wire_point(buffer, ax, ay, az, scale, cameraDistance, x1, y1);

        wire_point(frame, i, 1, x, y, z);
        rotate_axis(x, y, z, axis, theta, ax, ay, az);
        project_wire_point(buffer, ax, ay, az, scale, cameraDistance, x2, y2);

        draw_line(buffer, x1, y1, x2, y2, tableColor(runtime, col));
    }
}

void putat(CthughaBuffer& buffer, int x, int y, int val) {
    int a = addr(x, y);

    buffer.activePixels()[a] = val;
    buffer.activePixels()[a - 1] = val;
    buffer.activePixels()[a + 1] = val;
    buffer.activePixels()[a + buffer.width()] = val;
    buffer.activePixels()[a - buffer.width()] = val;
}

void putat_cut(CthughaBuffer& buffer, int x, int y, int val) {
    if ((x < 0) || (x >= buffer.width()))
        return;
    if ((y < 0) || (y >= buffer.height()))
        return;
    putat(buffer, x, y, val);
}

void putpixel_cut(CthughaBuffer& buffer, int x, int y, int val) {
    if ((x < 0) || (x >= buffer.width()))
        return;
    if ((y < 0) || (y >= buffer.height()))
        return;
    buffer.activePixels()[addr(x, y)] = val;
}

void draw_line(CthughaBuffer& buffer, int x1, int y1, int x2, int y2, int c) {
    register int lx, ly, dx, dy;
    register int i, j, k;

    if (x1 < 0)
        x1 = 0;
    if (x1 > buffer.width())
        x1 = buffer.width();
    if (x2 < 0)
        x2 = 0;
    if (x2 > buffer.width())
        x2 = buffer.width();
    if (y1 < 0)
        y1 = 0;
    if (y1 > buffer.height())
        y1 = buffer.height();
    if (y2 < 0)
        y2 = 0;
    if (y2 > buffer.height())
        y2 = buffer.height();

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
            buffer.activePixels()[i + j * buffer.width()] = c;
        }
    } else {
        for (i = y1, j = x1, k = 0; i != y2; i += dy, k += lx) {
            if (k >= ly) {
                k -= ly;
                j += dx;
            }
            buffer.activePixels()[j + i * buffer.width()] = c;
        }
    }
}

void do_vwave(CthughaBuffer& buffer, int ystart, int yend, int x, int val) {
    int ys, ye;
    unsigned char* pos;

    if (ystart > yend)
        ys = yend, ye = ystart;
    else
        ys = ystart, ye = yend;

    if (ys < 0)
        ys = 0;
    if (ys >= buffer.height())
        ys = buffer.height() - 1;
    if (ye < 0)
        ye = 0;
    if (ye >= buffer.height())
        ye = buffer.height() - 1;

    pos = buffer.activePixels() + addr(x, ys);
    for (; ys <= ye; ys++) {
        *pos = (char)val;
        pos += buffer.width();
    }
}

void do_hwave(CthughaBuffer& buffer, int xstart, int xend, int y, int val) {
    int xs, xe;
    unsigned char* pos;

    if (xstart > xend)
        xs = xend, xe = xstart;
    else
        xs = xstart, xe = xend;

    while ((xs < 0) && (xe < 0))
        xs += buffer.width(), xe += buffer.width();
    while ((xs >= buffer.width()) && (xe >= buffer.width()))
        xs -= buffer.width(), xe -= buffer.width();

    if (xs < 0)
        xs = 0;
    if (xs >= buffer.width())
        xs = buffer.width() - 1;
    if (xe < 0)
        xe = 0;
    if (xe >= buffer.width())
        xe = buffer.width() - 1;

    pos = buffer.activePixels() + addr(xs, y);
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
void wave_none(CthughaBuffer& buffer, WaveRuntime& runtime) { }

/* Writes fixed raw palette indices: 48, 96, 128, 160, 192, 224, and 255. */
void wave_grid(CthughaBuffer& buffer, WaveRuntime& runtime) {
    for (int y = 0; y < buffer.height(); y++) {
        for (int x = 0; x < buffer.width(); x++) {
            int c = 0;

            if ((x % 10 == 0) || (y % 10 == 0))
                c = 48;
            if ((x % 20 == 0) || (y % 20 == 0))
                c = 96;
            if ((x == MID_X) || (y == MID_Y))
                c = 192;
            if ((x == 0) || (x == buffer.width() - 1) || (y == 0) || (y == buffer.height() - 1))
                c = 224;

            if (c != 0)
                buffer.activePixels()[addr(x, y)] = c;
        }
    }

    for (int i = 0; (i < buffer.width()) && (i < buffer.height()); i++)
        buffer.activePixels()[addr(i, i)] = 255;

    for (int i = 0; (i < buffer.width()) && (i < buffer.height()); i++)
        buffer.activePixels()[addr(buffer.width() - 1 - i, i)] = 160;

    putat_cut(buffer, 5, 5, 255);
    putat_cut(buffer, buffer.width() - 6, 5, 192);
    putat_cut(buffer, 5, buffer.height() - 6, 160);
    putat_cut(buffer, buffer.width() - 6, buffer.height() - 6, 128);
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_dotHor(CthughaBuffer& buffer, WaveRuntime& runtime) { /* dot horizontal */
    int x, tmp;

    prepareSoundData(buffer.width());

    for (x = 0; x < buffer.width(); x++) {
        tmp = data[x][0];
        putpixel_cut(buffer, x >> 1, LOW_LINE - (tmp >> runtime.waveScale), tableColor(runtime, tmp));

        tmp = data[x][1];
        putpixel_cut(buffer, (x + buffer.width()) >> 1, LOW_LINE - (tmp >> runtime.waveScale),
            tableColor(runtime, tmp));
    }
}

/*****************************************************************************
 * Dot vertical
 *****************************************************************************/

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_dotVert(CthughaBuffer& buffer, WaveRuntime& runtime) { /* dot vertical */
    int tmp, x;

    prepareSoundData(BOTTOM);

    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][0];
        putpixel_cut(buffer, MID_X - (tmp >> runtime.waveScale), x, tableColor(runtime, tmp));

        tmp = data[x][1];
        putpixel_cut(buffer, MID_X + (tmp >> runtime.waveScale), x, tableColor(runtime, tmp));
    }
}

/****************************************************************************
 * Line horizontal
 ****************************************************************************/

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_lineHor(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Line horizontal */
    int x, y, tmp;
    struct State {
        int last;
        State()
            : last(0) { }
    };
    State& state = runtime.state<State>();

    prepareSoundData(buffer.width());

    for (y = 0; y < 2; y++)
        for (x = 0; x < buffer.width(); x++) {
            tmp = data[x][y];
            do_vwave(buffer, MID_Y - ((tmp - 128) >> runtime.waveScale),
                MID_Y - ((state.last - 128) >> runtime.waveScale),
                y ? ((x + buffer.width()) >> 1) : (x >> 1), tableColor(runtime, tmp));
            state.last = tmp;
        }
}

/****************************************************************************
 * Line vertical
 ****************************************************************************/

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_lineVert(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Line veritcal short */
    int tmp, x, last1 = 128, last2 = 128;

    prepareSoundData(BOTTOM);

    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][0];
        do_hwave(buffer, MID_X - (tmp >> runtime.waveScale),
            MID_X - (last1 >> runtime.waveScale), x, tableColor(runtime, tmp));
        last1 = tmp;

        tmp = data[x][1];
        do_hwave(buffer, MID_X + (tmp >> runtime.waveScale),
            MID_X + (last2 >> runtime.waveScale), x, tableColor(runtime, tmp));
        last2 = tmp;
    }
}

/****************************************************************************
 * Spikes functions
 ****************************************************************************/

/* Writes table-mapped spike-height indices, tableColor(runtime, 0..BOTTOM-1). */
void wave_spike(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Spike */
    int x, tmp, i;

    prepareSoundData(buffer.width(), 0);

    for (x = 0; x < buffer.width(); x++) {
        tmp = 2 * abs(data[x][0]) >> runtime.waveScale;
        if (tmp >= BOTTOM)
            tmp = BOTTOM - 1;
        for (i = 0; i < tmp; i++)
            putat(buffer, x >> 1, BOTTOM - i, tableColor(runtime, i));

        tmp = 2 * abs(data[x][1]) >> runtime.waveScale;
        if (tmp >= BOTTOM)
            tmp = BOTTOM - 1;
        for (i = 0; i < tmp; i++)
            putat(buffer, (x + buffer.width()) >> 1, BOTTOM - i, tableColor(runtime, i));
    }
}

/* Writes table-mapped scaled-amplitude indices, tableColor(runtime, amplitude). */
void wave_spikeH(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Spike hollow */
    int tmp, x, y, last = 0;

    prepareSoundData(buffer.width(), 0);

    for (y = 0; y < 2; y++) {
        for (x = 0; x < buffer.width(); x++) {
            tmp = 2 * abs(data[x][y]) >> runtime.waveScale;
            do_vwave(buffer, BOTTOM - tmp, BOTTOM - last,
                y ? ((x + buffer.width()) >> 1) : (x >> 1), tableColor(runtime, tmp));
            last = tmp;
        }
    }
}

/*****************************************************************************
 * other wave-functions
 *****************************************************************************/

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_buff9(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Walking */
    int tmp, x, last1 = 128, last2 = 128;
    struct State {
        int col;
        State()
            : col(128) { }
    };
    State& state = runtime.state<State>();

    state.col = (state.col + 1) % buffer.width();

    prepareSoundData(BOTTOM);

    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][0];
        do_hwave(buffer, state.col - (tmp >> runtime.waveScale),
            state.col - (last1 >> runtime.waveScale), x, tableColor(runtime, tmp));
        last1 = tmp;

        tmp = data[x][1];
        do_hwave(buffer, state.col + (tmp >> runtime.waveScale),
            state.col + (last2 >> runtime.waveScale), x, tableColor(runtime, tmp));
        last2 = tmp;
    }
}
/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_buff10(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Falling */
    int i;
    struct State {
        int row;
        State()
            : row(0) { }
    };
    State& state = runtime.state<State>();

    prepareSoundData(2 * MID_X);

    state.row = (state.row + 1) % BOTTOM;
    for (i = 0; i < MID_X; i++) {
        putat(buffer, i, state.row + 1, tableColor(runtime, data[i][0]));
        putat(buffer, i + MID_X, state.row + 1, tableColor(runtime, data[i][1]));
        putat(buffer, i, state.row, tableColor(runtime, data[i + MID_X][0]));
        putat(buffer, i + MID_X, state.row, tableColor(runtime, data[i + MID_X][1]));
    }
}

/* Writes table-mapped left-channel sample indices, tableColor(runtime, sample), across 0..255. */
void wave_buff11(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Lissa */
    int tmp, x, tmp2;

    prepareSoundData(buffer.width());

    for (x = 0; x < buffer.width(); x++) {
        tmp = data[x][0];
        tmp2 = data[x][1];

        putat(buffer, (tmp2 + 32) % buffer.width(), (tmp + 200 - 28) % BOTTOM, tableColor(runtime, tmp));
    }
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_buff14(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Line X */
    int tmp, x, last = 128;

    prepareSoundData(BOTTOM);

    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][0];
        do_hwave(buffer, MID_X - (tmp >> runtime.waveScale),
            MID_X - (last >> runtime.waveScale), x, tableColor(runtime, tmp));
        last = tmp;
    }
    for (x = 0; x < BOTTOM; x++) {
        tmp = data[x][1];
        do_hwave(buffer, MID_X - 40 + (tmp >> runtime.waveScale),
            MID_X - 40 + (last >> runtime.waveScale), x, tableColor(runtime, tmp));
        last = tmp;
    }
}

/* Writes fixed raw palette index 255. */
void wave_buff15(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Lightning 1 */
    int tmp, x, last = buffer.width() / 3;

    prepareSoundData(BOTTOM, 0);

    for (x = 0; x < BOTTOM; x++) {

        tmp = (data[x][0] >> 4) + last;

        if (tmp >= buffer.width())
            tmp = buffer.width() - 1;
        if (tmp < 0)
            tmp = 0;

        do_hwave(buffer, tmp, last, x, 255);
        last = tmp;
    }

    last = 2 * buffer.width() / 3;
    for (x = 0; x < BOTTOM; x++) {

        tmp = (data[x][1] >> 4) + last;

        if (tmp >= buffer.width())
            tmp = buffer.width() - 1;
        if (tmp < 0)
            tmp = 0;

        do_hwave(buffer, tmp, last, x, 255);
        last = tmp;
    }
}
/* Writes fixed raw palette index 255. */
void wave_buff16(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Lightning 2 */
    int tmp, x, last = buffer.width() / 3;

    prepareSoundData(buffer.width(), 0);

    for (x = 0; x < BOTTOM; x++) {

        tmp = (data[x][0] >> 5) + last;

        if (tmp >= buffer.width())
            tmp = buffer.width() - 1;
        if (tmp < 0)
            tmp = 0;

        do_hwave(buffer, tmp, last, x, 255);
        last = tmp;
    }

    last = 2 * buffer.width() / 3;
    for (x = 0; x < BOTTOM; x++) {

        tmp = (data[x][1] >> 5) + last;

        if (tmp >= buffer.width())
            tmp = buffer.width() - 1;
        if (tmp < 0)
            tmp = 0;

        do_hwave(buffer, tmp, last, x, 255);
        last = tmp;
    }
}

/* Writes table-mapped sound sample indices, mostly tableColor(runtime, sample) across 0..255. */
void wave_pete0(CthughaBuffer& buffer, WaveRuntime& runtime) { /* FireFlies */
    int temp, temp2, x;
    struct State {
        int xoff0, yoff0;
        int xoff1, yoff1;
        State()
            : xoff0(160)
            , yoff0(100)
            , xoff1(160)
            , yoff1(100) { }
    };
    State& state = runtime.state<State>();

    prepareSoundData(buffer.width());

    state.xoff0 += (data[0][0]) % 9 - 4;
    state.yoff0 += (data[1][0]) % 9 - 4;

    state.xoff1 += (data[0][1]) % 9 - 4;
    state.yoff1 += (data[1][1]) % 9 - 4;

    while (state.xoff0 < 0)
        state.xoff0 += buffer.width();
    while (state.yoff0 < 0)
        state.yoff0 += BOTTOM;

    while (state.xoff1 < 0)
        state.xoff1 += buffer.width();
    while (state.yoff1 < 0)
        state.yoff1 += BOTTOM;

    state.xoff0 = state.xoff0 % buffer.width();
    state.xoff1 = state.xoff1 % buffer.width();

    state.yoff0 = state.yoff0 % buffer.height();
    state.yoff1 = state.yoff1 % buffer.height();

    for (x = 0; x < buffer.width(); x++) {
        temp = data[x][0];
        temp2 = data[(x + 80) % buffer.width()][0];

        putat(buffer, ((temp2 >> 2) + state.xoff0) % buffer.width(), ((temp >> 2) + state.yoff0) % BOTTOM, tableColor(runtime, temp));

        temp = data[x][1] + 128;
        temp2 = data[(x + 80) % buffer.width()][1] + 128;

        putat(buffer, ((temp2 >> 2) + state.xoff1) % buffer.width(), ((temp >> 2) + state.yoff1) % BOTTOM, tableColor(runtime, temp));
    }
}

/* Writes table-mapped signed-amplitude indices, tableColor(runtime, sample). */
void wave_pete1(CthughaBuffer& buffer, WaveRuntime& runtime) {
    int tmp, x, left = 0, right = 0;

    prepareSoundData(buffer.width(), 0);

    for (x = 0; x < buffer.width(); x++) {
        left += abs(data[x][0]);
        right += abs(data[x][1]);
    }

    left = left / MID_X;
    right = right / MID_X;

    left = min(left, BOTTOM);
    right = min(right, BOTTOM);

    for (x = 0; x < MID_X; x++) {
        tmp = data[x][0];
        putat_cut(buffer, x, BOTTOM - (abs(left * Bsine[x]) >> 8), tableColor(runtime, tmp));
    }
    for (x = MID_X; x < buffer.width(); x++) {
        tmp = data[x][1];
        putat_cut(buffer, x, BOTTOM - (abs(right * Bsine[x]) >> 8), tableColor(runtime, tmp));
    }
}

/* Writes table-mapped sine lookup indices, tableColor(runtime, sine[sample]). */
void wave_pete2(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Dot VS sine */
    int x, tmp;

    prepareSoundData(buffer.height());

    for (x = 0; x < buffer.height(); x++) {
        tmp = data[x][0];
        putat_cut(buffer, MID_X - (tmp >> runtime.waveScale), x, tableColor(runtime, sine[tmp]));
        tmp = data[x][1];
        putat_cut(buffer, MID_X + (tmp >> runtime.waveScale), x, tableColor(runtime, sine[tmp]));
    }
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_fract1(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Zippy 1*/
    int temp, x;
    struct State {
        int xoff0, yoff0;
        int xoff1, yoff1;
        State()
            : xoff0(0)
            , yoff0(0)
            , xoff1(0)
            , yoff1(0) { }
    };
    State& state = runtime.state<State>();

    prepareSoundData(buffer.width());

    temp = data[0][0];
    for (x = 0; x < buffer.width() - 2; x += 2) {
        state.xoff0 += (data[x][0] - temp) >> 1;
        temp = data[x][0];

        while (state.xoff0 < 0)
            state.xoff0 = buffer.width() - 1;

        state.xoff0 = state.xoff0 % buffer.width();

        putat(buffer, state.xoff0, state.yoff0, tableColor(runtime, temp));

        state.yoff0 += (data[x + 1][0] - temp) >> 1;
        temp = data[x + 1][0];

        while (state.yoff0 < 0)
            state.yoff0 = buffer.height() - 1;

        state.yoff0 = state.yoff0 % buffer.height();

        putat(buffer, state.xoff0, state.yoff0, tableColor(runtime, temp));
    }

    temp = data[0][1];
    for (x = 0; x < buffer.width() - 2; x += 2) {
        state.xoff1 += (data[x][1] - temp) >> 1;
        temp = data[x][1];

        if (state.xoff1 < 0)
            state.xoff1 = buffer.width() - 1;

        state.xoff1 = state.xoff1 % buffer.width();

        putat(buffer, state.xoff1, state.yoff1, tableColor(runtime, temp));

        state.yoff1 -= (data[x + 1][1] - temp) >> 1;
        temp = data[x + 1][1];

        if (state.yoff1 < 0)
            state.yoff1 = buffer.height() - 1;

        state.yoff1 = state.yoff1 % buffer.height();

        putat(buffer, state.xoff1, state.yoff1, tableColor(runtime, temp));
    }
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_fract2(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Zippy 2 */
    int temp, x;
    struct State {
        int xoff0, yoff0;
        int xoff1, yoff1;
        State()
            : xoff0(0)
            , yoff0(0)
            , xoff1(0)
            , yoff1(0) { }
    };
    State& state = runtime.state<State>();

    prepareSoundData(buffer.width());

    temp = data[0][0];
    for (x = 0; x < buffer.width() - 2; x += 2) {
        state.xoff0 += (data[x][0] - temp);
        temp = data[x][0];

        if (state.xoff0 < 0)
            state.xoff0 = buffer.width() - 1;

        state.xoff0 = state.xoff0 % buffer.width();

        putat(buffer, state.xoff0, state.yoff0, tableColor(runtime, temp));

        state.yoff0 += (data[x + 1][0] - temp);
        temp = data[x + 1][0];

        if (state.yoff0 < 0)
            state.yoff0 = buffer.height() - 1;

        state.yoff0 = state.yoff0 % buffer.height();

        putat(buffer, state.xoff0, state.yoff0, tableColor(runtime, temp));
    }

    temp = data[0][1];
    for (x = 0; x < buffer.width() - 2; x += 2) {
        state.xoff1 += (data[x][1] - temp);
        temp = data[x][1];

        if (state.xoff1 < 0)
            state.xoff1 = buffer.width() - 1;

        state.xoff1 = state.xoff1 % buffer.width();

        putat(buffer, state.xoff1, state.yoff1, tableColor(runtime, temp));

        state.yoff1 -= (data[x + 1][1] - temp);
        temp = data[x + 1][1];

        if (state.yoff1 < 0)
            state.yoff1 = buffer.height() - 1;

        state.yoff1 = state.yoff1 % buffer.height();

        putat(buffer, state.xoff1, state.yoff1, tableColor(runtime, temp));
    }
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample + 128), across 0..255. */
void wave_test(CthughaBuffer& buffer, WaveRuntime& runtime) { /* Test */
    int temp, x, left = 0, right = 0;

    prepareSoundData(buffer.width(), 0);

    for (x = 0; x < buffer.width(); x++) {
        left += abs(data[x][0]);
        right += abs(data[x][1]);
    }

    left = left / (128);
    right = right / (128);

    left = min(left, 199);
    right = min(right, 199);

    for (x = 0; x < MID_X; x++) {
        temp = data[x][0] + 128;
        putat_cut(buffer, x, BOTTOM - (abs((left)*Bsine[x]) >> 8), tableColor(runtime, temp));
    }
    for (x = MID_X; x < buffer.width(); x++) {
        temp = data[x][1] + 128;
        putat_cut(buffer, x, BOTTOM - (abs((right)*Bsine[x]) >> 8), tableColor(runtime, temp));
    }
}

/*
 * From root@hangon.onramp.net Sun Jun 18 04:42:22 1995
 * some changes by Deischinger Harald
 * some more changes by Rus Maxham
 *
 * the rings have a radius of 64.
 */
/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_aaron(CthughaBuffer& buffer, WaveRuntime& runtime) {
    struct State {
        int x, y;
        int first;
        int cxl, cxr, cyl, cyr;
        State()
            : x(40)
            , y(0)
            , first(1)
            , cxl(-1)
            , cxr(-1)
            , cyl(-1)
            , cyr(-1) { }
    };
    State& state = runtime.state<State>();
    int txl = 0, tyl = 0, txr = 0, tyr = 0;

    if (state.first) {
        state.first = 0;
        state.cxl = ((buffer.width() - 256) / 2) + 64 + 128;
        state.cxr = ((buffer.width() - 256) / 2) - 64 + 128;
        state.cyl = ((BOTTOM - 256) / 2) + 128;
        state.cyr = state.cyl;
    }

    if ((buffer.height() > 128) && (buffer.width() >= 256)) {
        register int tmp, i, sx, sy;

        prepareSoundData(buffer.width());

        for (i = 0; i < buffer.width(); i++) {
            if (state.y >= 320)
                state.y -= 320;
            if (state.x >= 320)
                state.x -= 320;
            tmp = data[i][0];

            sx = (sine[state.x] * tmp) >> 9;
            txl += sx;
            sy = (sine[state.y] * tmp) >> 9;
            tyl += sy;

            putat_cut(buffer, state.cxl + sx, state.cyl + sy, tableColor(runtime, tmp));

            tmp = data[i][1];

            sx = (sine[state.x] * tmp) >> 9;
            txr += sx;
            sy = (sine[state.y] * tmp) >> 9;
            tyr += sy;

            putat_cut(buffer, state.cxr - sx, state.cyr - sy, tableColor(runtime, tmp));

            state.x++;
            state.y++;
        }
        state.cxl += txl * 3 / buffer.width() + 1;
        state.cyl += tyl * 3 / buffer.width() + 1;
        state.cxr += txr * 3 / buffer.width() + 1;
        state.cyr += tyr * 3 / buffer.width() + 1;
        state.cxl %= buffer.width();
        state.cxr %= buffer.width();
        state.cyl %= buffer.height();
        state.cyr %= buffer.height();
        if (state.cxl < 0)
            state.cxl += buffer.width();
        if (state.cxr < 0)
            state.cxr += buffer.width();
        if (state.cyl < 0)
            state.cyl += buffer.height();
        if (state.cyr < 0)
            state.cyr += buffer.height();

    }
}

/* Line horizontal long diff */
/* Writes table-mapped stereo-difference indices, tableColor(runtime, left - right + 128). */
void wave_lineHLdiff(CthughaBuffer& buffer, WaveRuntime& runtime) {
    register int x, tmp;
    struct State {
        int last;
        State()
            : last(0) { }
    };
    State& state = runtime.state<State>();

    prepareSoundData(buffer.width(), 0);

    if (buffer.height() < 300) {
        for (x = 0; x < buffer.width(); x++) {
            tmp = data[x][0] - data[x][1];
            do_vwave(buffer, MID_Y - tmp, MID_Y - state.last, x, tableColor(runtime, tmp + 128));
            state.last = tmp;
        }
    } else {
        for (x = 0; x < buffer.width(); x++) {
            tmp = data[x][0] - data[x][1];
            do_vwave(buffer, MID_Y - tmp, MID_Y - state.last, x, tableColor(runtime, tmp + 128));
            state.last = tmp << 1;
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
/* Writes one startup-random table-mapped wire index, tableColor(runtime, random 0..255). */
void wave_wire1(CthughaBuffer& buffer, WaveRuntime& runtime) {
    struct State {
        double theta;
        int col;
        State()
            : theta(0)
            , col(255) { }
    };
    State& state = runtime.state<State>();
    double st, ct, ax, ay, az, x, y, z;
    const double cameraDistance = 3.0;
    WireObjectFrame frame;

    if (!setup_wire_object(buffer, runtime, frame))
        return;

    int i, j, x1, y1, x2, y2;

    if (object_wave_needs_configuration(runtime))
        state.col = random_wire_color();

    state.theta += M_PI / 45.0;

    st = sin(state.theta);
    ct = cos(state.theta);

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

        project_wire_point(buffer, ax, ay, az, scale0, cameraDistance, x1, y1);

        wire_point(frame, i, 1, x, y, z);

        ax = x * ct + z * st;
        ay = y;
        az = z * ct - x * st;

        project_wire_point(buffer, ax, ay, az, scale1, cameraDistance, x2, y2);

        draw_line(buffer, x1, y1, x2, y2, tableColor(runtime, state.col));
    }
}

/*
 * Wire1dot5 keeps Wire1's single rotating object, but treats it as a rigid
 * model.  The whole audio buffer contributes to one frame-wide scale factor,
 * so all vertices move together and the wireframe remains coherent.
 */
/* Writes one startup-random table-mapped wire index, tableColor(runtime, random 0..255). */
void wave_wire1dot5(CthughaBuffer& buffer, WaveRuntime& runtime) {
    struct State {
        int theta;
        int col;
        double axis[3];
        State()
            : theta(0)
            , col(255) {
            axis[0] = 0.0;
            axis[1] = 1.0;
            axis[2] = 0.0;
        }
    };
    State& state = runtime.state<State>();
    double scale;
    const double cameraDistance = 3.0;
    WireObjectFrame frame;

    if (!setup_wire_object(buffer, runtime, frame))
        return;

    if (object_wave_needs_configuration(runtime)) {
        random_axis(state.axis);
        state.col = random_wire_color();
    }

    state.theta += 2;
    scale = wire_sound_scale(frame.screenScale);

    draw_axis_wire_model(buffer, runtime, frame, state.axis, state.theta, scale, cameraDistance, state.col);
}

/*
 * Wire1dot55 adds axial precession to Wire1dot5.  At startup it chooses an
 * original axis A, a cone angle away from A, and a precession period.  Each
 * frame the actual rotation axis moves around that cone, while the model still
 * keeps Wire1dot5's coherent frame-wide audio scale.
 */
/* Writes one startup-random table-mapped wire index, tableColor(runtime, random 0..255). */
void wave_wire1dot55(CthughaBuffer& buffer, WaveRuntime& runtime) {
    struct State {
        int theta;
        int col;
        double baseAxis[3];
        double coneAngle;
        double precessionTime;
        double precessionStart;
        State()
            : theta(0)
            , col(255)
            , coneAngle(0.0)
            , precessionTime(PRECESSION_TIME_MIN)
            , precessionStart(0.0) {
            baseAxis[0] = 0.0;
            baseAxis[1] = 1.0;
            baseAxis[2] = 0.0;
        }
    };
    State& state = runtime.state<State>();
    double axis[3];
    double scale;
    const double cameraDistance = 3.0;
    WireObjectFrame frame;

    if (!setup_wire_object(buffer, runtime, frame))
        return;

    if (object_wave_needs_configuration(runtime)) {
        random_axis(state.baseAxis);
        state.coneAngle = random_unit() * M_PI;
        state.precessionTime = PRECESSION_TIME_MIN
            + random_unit() * (PRECESSION_TIME_MAX - PRECESSION_TIME_MIN);
        state.precessionStart = now;
        state.col = random_wire_color();
    }

    precess_axis(state.baseAxis, state.coneAngle,
        M_PI2 * (now - state.precessionStart) / state.precessionTime, axis);

    state.theta += 2;
    scale = wire_sound_scale(frame.screenScale);

    draw_axis_wire_model(buffer, runtime, frame, axis, state.theta, scale, cameraDistance, state.col);
}

/*
 * Wire1dot6 is the elastic variant.  It still rotates around one startup axis,
 * but each vertex is pushed radially outward according to its own stable audio
 * slice.  The stretch happens before rotation and perspective projection.
 */
/* Writes one startup-random table-mapped wire index, tableColor(runtime, random 0..255). */
void wave_wire1dot6(CthughaBuffer& buffer, WaveRuntime& runtime) {
    struct State {
        int theta;
        int col;
        double axis[3];
        State()
            : theta(0)
            , col(255) {
            axis[0] = 0.0;
            axis[1] = 1.0;
            axis[2] = 0.0;
        }
    };
    State& state = runtime.state<State>();
    double ax, ay, az, x, y, z;
    double stretch;
    const double cameraDistance = 4.0;
    WireObjectFrame frame;

    if (!setup_wire_object(buffer, runtime, frame))
        return;

    int i, x1, y1, x2, y2;

    if (object_wave_needs_configuration(runtime)) {
        random_axis(state.axis);
        state.col = random_wire_color();
    }

    state.theta += 2;

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

        rotate_axis(x, y, z, state.axis, state.theta, ax, ay, az);

        project_wire_point(buffer, ax, ay, az, frame.screenScale, cameraDistance, x1, y1);

        stretch = vertex_sound_stretch(frame.obj[i][1][0], frame.obj[i][1][1], frame.obj[i][1][2]);
        wire_point(frame, i, 1, x, y, z);
        x *= stretch;
        y *= stretch;
        z *= stretch;

        rotate_axis(x, y, z, state.axis, state.theta, ax, ay, az);

        project_wire_point(buffer, ax, ay, az, frame.screenScale, cameraDistance, x2, y2);

        draw_line(buffer, x1, y1, x2, y2, tableColor(runtime, state.col));
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
/* Writes per-copy startup-random table-mapped wire indices, tableColor(runtime, random 0..255). */
void wave_wire2(CthughaBuffer& buffer, WaveRuntime& runtime) {
    /*
     * Persistent swarm state.  Each copy has a position in the blob, its own
     * local spin angle/rate, and a color.  This state belongs to the wave, not
     * to any particular object, so changing the object swaps the geometry into
     * the existing animated pattern.
     */
    struct State {
        int theta;
        int psi[nobj];
        int rate[nobj];
        int col[nobj];
        int loc[nobj][3];
        double blobAxis[3];
        State()
            : theta(0) {
            memset(psi, 0, sizeof(psi));
            memset(rate, 0, sizeof(rate));
            memset(col, 0, sizeof(col));
            memset(loc, 0, sizeof(loc));
            blobAxis[0] = 0.0;
            blobAxis[1] = 1.0;
            blobAxis[2] = 0.0;
        }
    };
    State& state = runtime.state<State>();

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
     * The classic formula deliberately uses the y extent, so flat/wide models
     * keep the same tall, spindly Wire2 character as the built-in glyphs.
     */
    double omx, omy, omz, om;
    register int ox, oy;

    WObject* theObj = runtime.object;
    if (theObj == NULL)
        return;

    /*
     * Startup state is refreshed when Wire2 first starts, or when callers
     * explicitly mark the object set as changed. Between those moments the
     * frame loop only advances theta/psi, so the blob axis and swarm layout do
     * not jump every frame.
     */
    if (object_wave_needs_configuration(runtime)) {
        /* Rotate the whole blob around a random axis for this swarm lifetime. */
        random_axis(state.blobAxis);

        /*
         * Pick a shell position and local spin for each copy. Avoid zero
         * radius and zero spin so no model sits fixed in the center.
         */
        for (i = 0; i < nobj; i++) {
            init_wire2_copy(state.loc[i], state.psi[i], state.rate[i], state.col[i]);
            CTH_DEBUG("model %d: rate %d, psi %d, col %d\n",
                i, state.rate[i], state.psi[i], state.col[i]);

        }
    }

    state.theta += 1;

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
        if (state.loc[i][0] > mx)
            mx = state.loc[i][0];

        if (state.loc[i][1] > my)
            my = state.loc[i][1];

        if (state.loc[i][2] > mz)
            mz = state.loc[i][2];
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
    scl = (double)buffer.height() * 0.80;
    if (!scl)
        scl = 1;

    ox = buffer.width() / 2 - mx / 2;
    oy = buffer.height() / 2 - my / 2;

    for (j = 0; j < nobj; j++) {

        /* Each copy still spins around its own local y axis. */
        sto = isin(state.psi[j]);
        cto = icos(state.psi[j]);

        state.psi[j] += state.rate[j];

        for (i = 0; theObj[i][0][0] != -1; i++) {

            /* Rotate this copy's blob position around the swarm axis. */
            x = state.loc[j][0];
            y = state.loc[j][1];
            z = state.loc[j][2];

            rotate_axis(x, y, z, state.blobAxis, state.theta, ax, ay, az);

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
            x = state.loc[j][0];
            y = state.loc[j][1];
            z = state.loc[j][2];

            rotate_axis(x, y, z, state.blobAxis, state.theta, ax, ay, az);

            x = (theObj[i][1][0] - omx) / om;
            y = (theObj[i][1][1] - omy) / om;
            z = (theObj[i][1][2] - omz) / om;

            ax += x * cto + z * sto;
            ay += y;
            az += z * cto - x * sto;

            x2 = int((double)ax * scl / (az + m) + ox);
            y2 = int((double)ay * scl / (az + m) + oy);

            draw_line(buffer, x1, y1, x2, y2, tableColor(runtime, state.col[j]));
        }
    }
}

/*
 * Wire2dot1 is Wire2 with one extra degree of freedom: each copy has its own
 * local rotation axis.  The blob still rotates around one shared axis, but the
 * individual models no longer all tumble around their local y axes.
 */
/* Writes per-copy startup-random table-mapped wire indices, tableColor(runtime, random 0..255). */
void wave_wire2dot1(CthughaBuffer& buffer, WaveRuntime& runtime) {
    /*
     * This mirrors Wire2's persistent swarm state, with modelAxis storing one
     * normalized local spin axis per copy.  The axes are chosen during startup
     * and then reused frame after frame, so the motion is varied but stable.
     */
    struct State {
        int theta;
        int psi[nobj];
        int rate[nobj];
        int col[nobj];
        int loc[nobj][3];
        double blobAxis[3];
        double modelAxis[nobj][3];
        State()
            : theta(0) {
            memset(psi, 0, sizeof(psi));
            memset(rate, 0, sizeof(rate));
            memset(col, 0, sizeof(col));
            memset(loc, 0, sizeof(loc));
            memset(modelAxis, 0, sizeof(modelAxis));
            blobAxis[0] = 0.0;
            blobAxis[1] = 1.0;
            blobAxis[2] = 0.0;
        }
    };
    State& state = runtime.state<State>();

    double x, y, z;
    double ax, ay, az;
    double lx, ly, lz;
    double scl;

    register int i, j, x1, y1, x2, y2;
    register int mx, my, mz, m;
    double omx, omy, omz, om;
    register int ox, oy;

    WObject* theObj = runtime.object;
    if (theObj == NULL)
        return;

    if (object_wave_needs_configuration(runtime)) {
        random_axis(state.blobAxis);

        for (i = 0; i < nobj; i++) {
            init_wire2_copy(state.loc[i], state.psi[i], state.rate[i], state.col[i]);
            CTH_DEBUG("model %d: rate %d, psi %d, col %d\n",
                i, state.rate[i], state.psi[i], state.col[i]);

            random_axis(state.modelAxis[i]);
        }
    }

    state.theta += 1;

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
        if (state.loc[i][0] > mx)
            mx = state.loc[i][0];

        if (state.loc[i][1] > my)
            my = state.loc[i][1];

        if (state.loc[i][2] > mz)
            mz = state.loc[i][2];
    }

    m = (mx > my) ? mx : my;
    m = (m > mz) ? m : mz;
    m = whirlyRadius * 3 / 2;

    scl = (double)buffer.height() * 0.80;
    if (!scl)
        scl = 1;

    ox = buffer.width() / 2 - mx / 2;
    oy = buffer.height() / 2 - my / 2;

    for (j = 0; j < nobj; j++) {

        state.psi[j] += state.rate[j];

        for (i = 0; theObj[i][0][0] != -1; i++) {

            x = state.loc[j][0];
            y = state.loc[j][1];
            z = state.loc[j][2];

            rotate_axis(x, y, z, state.blobAxis, state.theta, ax, ay, az);

            /*
             * Unlike Wire2, rotate the endpoint around this copy's own random
             * axis.  Every copy has a different axis, but both endpoints of a
             * line segment use the same axis for that copy.
             */
            x = (theObj[i][0][0] - omx) / om;
            y = (theObj[i][0][1] - omy) / om;
            z = (theObj[i][0][2] - omz) / om;

            rotate_axis(x, y, z, state.modelAxis[j], state.psi[j], lx, ly, lz);

            ax += lx;
            ay += ly;
            az += lz;

            x1 = int((double)ax * scl / (az + m) + ox);
            y1 = int((double)ay * scl / (az + m) + oy);

            x = state.loc[j][0];
            y = state.loc[j][1];
            z = state.loc[j][2];

            rotate_axis(x, y, z, state.blobAxis, state.theta, ax, ay, az);

            x = (theObj[i][1][0] - omx) / om;
            y = (theObj[i][1][1] - omy) / om;
            z = (theObj[i][1][2] - omz) / om;

            rotate_axis(x, y, z, state.modelAxis[j], state.psi[j], lx, ly, lz);

            ax += lx;
            ay += ly;
            az += lz;

            x2 = int((double)ax * scl / (az + m) + ox);
            y2 = int((double)ay * scl / (az + m) + oy);

            draw_line(buffer, x1, y1, x2, y2, tableColor(runtime, state.col[j]));
        }
    }
}

/* by Russ */
/* Writes one cycling table-mapped index per frame, tableColor(runtime, col 0..255). */
void wave_spiral(CthughaBuffer& buffer, WaveRuntime& runtime) {
    int i, amp, mx, cx, cy;
    double x, y, ox, oy, a, la, ra;
    struct State {
        int ofs, col, loops, loopcount;
        State()
            : ofs(0)
            , col(0)
            , loops(0)
            , loopcount(0) { }
    };
    State& state = runtime.state<State>();

    state.col++;
    state.col %= 256;
    state.ofs++;
    state.ofs %= 320;

    if (state.loopcount <= 0) {
        state.loopcount = 1 + abs(rand() % 32);
        state.loops = 2 + abs(rand() % 8);
    }

    if (acousticContext.fire())
        state.loopcount--;


    mx = min(buffer.width(), buffer.height());
    cx = buffer.width() / 2;
    cy = buffer.height() / 2;

    amp = audioAnalysis.amplitude;
    int al = audioAnalysis.amplitudeLeft;
    int ar = audioAnalysis.amplitudeRight;

    /* convert to float now instead of every time it gets used */
    a = (double)amp * mx / 256.0 / 128.0;
    la = (double)al * mx / 256.0 / 128.0;
    ra = (double)ar * mx / 256.0 / 128.0;

    ox = int(a * sine[(state.ofs + 120) % 320] + la * sine[120] / 2);
    oy = int(a * sine[state.ofs] + ra * sine[0] / 2);

    for (i = 1; i < 320; i++) {

        x = a * sine[(i + state.ofs + 120) % 320]
            + la * sine[((i)*state.loops + 120) % 320] / 2;
        y = a * sine[(i + state.ofs) % 320]
            + ra * sine[((i)*state.loops) % 320] / 2;

        draw_line(buffer, int(cx + ox), int(cy + oy), int(cx + x), int(cy + y),
            tableColor(runtime, state.col));

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
/* Writes per-firework random table-mapped indices, tableColor(runtime, random 0..255). */
void wave_pyro(CthughaBuffer& buffer, WaveRuntime& runtime) {
    int i, x1, y1;
    struct State {
        int first;
        int maxV, maxA;
        Fwork theWorks[maxWorks];
        State()
            : first(1)
            , maxV(0)
            , maxA(0) {
            memset(theWorks, 0, sizeof(theWorks));
        }
    };
    State& state = runtime.state<State>();

    if (state.first) {
        int fire;

        state.first = 0;

        /* intialize all the fire works to be inoperative */
        for (i = 0; i < maxWorks; i++)
            state.theWorks[i].dur = -1;

        /* find the maximum vertical launch velocity to stay on the screen */
        for (fire = 0, state.maxV = 0; fire < buffer.height();
             state.maxV += GRAV, fire += state.maxV)
            ;

        state.maxV--;
        state.maxA = 30;
    }

    for (i = 0; i < maxWorks; i++)

        /* if this work is in flight, process it */
        if (state.theWorks[i].dur != -1) {

            /* compute next position */
            x1 = state.theWorks[i].xp + state.theWorks[i].xv;
            y1 = state.theWorks[i].yp + state.theWorks[i].yv;

            /* draw the work */
            draw_line(buffer, state.theWorks[i].oxp, state.theWorks[i].oyp,
                x1, y1, tableColor(runtime, state.theWorks[i].col));
            draw_line(buffer, state.theWorks[i].oxp + 1, state.theWorks[i].oyp,
                x1 + 1, y1, tableColor(runtime, state.theWorks[i].col));

            /* bounce off walls */
            if (x1 < 0 || x1 > buffer.width())
                state.theWorks[i].xv *= -1;

            /* gravity and the passage of time, aren't they cute */
            state.theWorks[i].yv += GRAV;
            state.theWorks[i].dur++;
            state.theWorks[i].oxp = state.theWorks[i].xp;
            state.theWorks[i].oyp = state.theWorks[i].yp;
            state.theWorks[i].xp = x1;
            state.theWorks[i].yp = y1;

            /* time to explode or they go off the screen */
            if (state.theWorks[i].yv > 5 || state.theWorks[i].yp < 0) {
                state.theWorks[i].dur = -1;
            }

        } else if (acousticContext.fire()) {
            int fire = acousticContext.fire();

            /* maintain a maximum attack value for scaling purposes */
            if (fire * 4 > state.maxA)
                state.maxA = fire * 4;

            /* slowly reduce max over time to keep it in line with the average levels */
            if (state.maxA > 30)
                state.maxA--;

            /* fire off a new firework */
            state.theWorks[i].dur = 0;
            state.theWorks[i].oxp = state.theWorks[i].xp = rand() % buffer.width();
            state.theWorks[i].oyp = state.theWorks[i].yp = buffer.height() - 4;
            state.theWorks[i].xv = (rand() % 20) - 10;
            state.theWorks[i].yv = -(fire * state.maxV / (state.maxA / 4));
            state.theWorks[i].col = rand() % 256;
            acousticContext.setFire(fire * 2 / 3);
        }

    /* test rocket exploded, reset */
}

#define maxWarps 15
#define maxWarpTrails 20

typedef struct {
    int r, s, theta, omg, trails, col, rgrav;
} WarpRing;

/* Writes per-ring random table-mapped indices, tableColor(runtime, random 0..255). */
void wave_warp(CthughaBuffer& buffer, WaveRuntime& runtime) {
    int i, x1, y1;
    struct State {
        int first;
        int cx, cy, maxRad, maxA;
        WarpRing theWarps[maxWarps];
        State()
            : first(1)
            , cx(0)
            , cy(0)
            , maxRad(0)
            , maxA(0) {
            memset(theWarps, 0, sizeof(theWarps));
        }
    };
    State& state = runtime.state<State>();

    if (state.first) {
        state.first = 0;

        /* intialize all the fire works to be inoperative */
        for (i = 0; i < maxWarps; i++)
            state.theWarps[i].r = -1;

        state.cx = buffer.width() / 2;
        state.cy = buffer.height() / 2;
        state.maxRad = (state.cx > state.cy) ? state.cx : state.cy;
    }

    for (i = 0; i < maxWarps; i++)

        /* if this warp is in flight, process it */
        if (state.theWarps[i].r != -1) {
            int r2 = state.theWarps[i].r + state.theWarps[i].s;
            int t2 = state.theWarps[i].theta + state.theWarps[i].omg;
            int j, x2, y2, tr = state.theWarps[i].trails;

            /* draw the ring of warps */
            for (j = 0; j < tr; j++) {
                x1 = int(state.cx + ((double)state.theWarps[i].r)
                        * isin((360 * j) / tr + state.theWarps[i].theta));
                y1 = int(state.cy + ((double)state.theWarps[i].r)
                        * icos((360 * j) / tr + state.theWarps[i].theta));
                x2 = int(state.cx + (double)r2 * isin(360 * j / tr + t2));
                y2 = int(state.cy + (double)r2 * icos(360 * j / tr + t2));
                draw_line(buffer, x1, y1, x2, y2,
                    tableColor(runtime, state.theWarps[i].col));
            }

            /* increment the radius and spiral */
            state.theWarps[i].r += state.theWarps[i].s;
            state.theWarps[i].theta += state.theWarps[i].omg;
            state.theWarps[i].s -= state.theWarps[i].rgrav;

            if (state.theWarps[i].r > state.maxRad || state.theWarps[i].r < 0)
                state.theWarps[i].r = -1;

        } else if (acousticContext.fire()) {
            int fire = acousticContext.fire();

            /* maintain a maximum attack value for scaling purposes*/
            if (fire * 4 > state.maxA)
                state.maxA = fire * 4;

            /* slowly reduce max over time to keep it in line with the average levels */
            if (state.maxA > 30)
                state.maxA--;

            /* fire off a new warp ring */
            state.theWarps[i].r = 0;
            state.theWarps[i].s = 3 + fire * 4 * 20 / state.maxA;
            state.theWarps[i].trails = 1 + fire * 4 * maxWarpTrails / state.maxA;
            state.theWarps[i].theta = rand() % 360;
            state.theWarps[i].omg = (rand() % 16 - 8) * fire * 4 / state.maxA;
            state.theWarps[i].col = rand() % 256;
            state.theWarps[i].rgrav = rand() % 2;
            acousticContext.resetFire();
            /*				acousticContext.setFire(fire * 2 / 3); */
        }
}

/* by Deischi */
static int laser_intensity_index(int sample) {
    return min(abs(sample) << 1, 255);
}

/* Writes table-mapped channel intensity. */
void wave_laser(CthughaBuffer& buffer, WaveRuntime& runtime) {
    struct State {
        int xl, xr;
        int y;
        State()
            : xl(0)
            , xr(0)
            , y(0) { }
    };
    State& state = runtime.state<State>();
    int x, samples;

    samples = buffer.width() / 10;
    prepareSoundData(samples + 1, 0);

    state.y = (state.y + 2) % buffer.height();
    //y = buffer.height() / 10;

    for (x = 0; x < samples; x++) {
        draw_line(buffer, buffer.width() / 2, buffer.height() / 2, state.xl, state.y,
            tableColor(runtime, laser_intensity_index(data[x][0])));
        state.xl = (state.xl + abs(data[x][0] - data[x + 1][0])) % buffer.width();

        draw_line(buffer, buffer.width() / 2, buffer.height() / 2, state.xr, buffer.height() - state.y,
            tableColor(runtime, laser_intensity_index(data[x][1])));
        state.xr = (state.xr + abs(data[x][1] - data[x + 1][1])) % buffer.width();
    }
}

/* by Deischi (inspired by RTL2) */
/* Writes raw fading indices 255, 127, 63, 31, 15, 7, 3, and 1. */
void wave_corner(CthughaBuffer& buffer, WaveRuntime& runtime) {
    struct State {
        int x, y;
        State()
            : x(0)
            , y(0) { }
    };
    State& state = runtime.state<State>();

    if (acousticContext.fire()) {
        int i, j, t;
        int fire = acousticContext.fire();

        state.x = (state.x + (rand() % fire)) % (buffer.width() - 16) + 8;
        state.y = (state.y + (rand() % fire)) % (buffer.height() - 16) + 8;

        t = min(fire >> 2, 8);

        if (rand() & 1) {
            /* draw corner pointing right down */
            for (i = 0; i < t; i++) {
                for (j = 0; j < state.x; j++) {
                    putat(buffer, j, state.y + i, 255 >> i);
                    putat(buffer, j, state.y - i, 255 >> i);
                }
                for (j = 0; j < state.y; j++) {
                    putat(buffer, state.x - i, j, 255 >> i);
                    putat(buffer, state.x + i, j, 255 >> i);
                }
            }
        } else {
            /* draw corner pointer up left */
            for (i = 0; i < t; i++) {
                for (j = state.x; j < buffer.width(); j++) {
                    putat(buffer, j, state.y + i, 255 >> i);
                    putat(buffer, j, state.y - i, 255 >> i);
                }
                for (j = state.y; j < buffer.height(); j++) {
                    putat(buffer, state.x - i, j, 255 >> i);
                    putat(buffer, state.x + i, j, 255 >> i);
                }
            }
        }
    }

    acousticContext.resetFire();
}

// by Deischi
/* Writes fixed raw palette index 255. */
void wave_jump(CthughaBuffer& buffer, WaveRuntime& runtime) {
    struct State {
        int speed[MAX_BUFF_WIDTH];
        int pos[MAX_BUFF_WIDTH];
        int dir[MAX_BUFF_WIDTH];
        State() {
            memset(speed, 0, sizeof(speed));
            memset(pos, 0, sizeof(pos));
            memset(dir, 0, sizeof(dir));
        }
    };
    State& state = runtime.state<State>();

    prepareSoundData(buffer.width());

    const int scale = 2 + runtime.waveScale;
    for (int i = 0; i < buffer.width(); i++) {
        int e = data[i][0] + data[i][1];
        if (state.pos[i] < abs(e)) {
            state.speed[i] = abs(e);
            state.dir[i] = e > 0 ? 1 : 0;
        }

        state.pos[i] += state.speed[i];
        if (state.pos[i] < 0) {
            state.pos[i] = 0;
            state.speed[i] = 0;
        } else
            state.speed[i] -= int(cthughaDisplay->fps);

        if (state.dir[i] > 0)
            putat_cut(buffer, i, (state.pos[i] >> scale) + buffer.height() / 2, 255);
        else
            putat_cut(buffer, i, -(state.pos[i] >> scale) + buffer.height() / 2, 255);
    }
}

// by Deischi
/* Writes raw random palette indices, Random(256). */
void wave_sticks(CthughaBuffer& buffer, WaveRuntime& runtime) {

    int n = acousticContext.fire() >> runtime.waveScale;
    for (int i = 0; i < n; i++) {
        draw_line(buffer, Random(buffer.width()), Random(buffer.height()), Random(buffer.width()), Random(buffer.height()),
            Random(256));
    }
}
