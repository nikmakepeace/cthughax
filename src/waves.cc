// Wave option setup, object loading, color tables, and wave renderers.

#include "config.h"
#include "Configuration.h"
#include "EffectChoiceLoader.h"
#include "Interface.h"
#include "information.h"
#include "imath.h"
#include "ProcessServices.h"
#include "waves.h"
#include "disp-sys.h"
#include "FrameStageBuffer.h"
#include "FrameGeneratorContext.h"
#include "WaveObject.h"

#include <math.h>
#include <vector>

static EffectChoiceList waveEntries;
static EffectChoiceList objectEntries;
static EffectChoiceList waveScaleEntries;
static EffectChoiceList tableEntries;

WaveOption wave;
EffectControl waveScale(-1, "wave-scale", waveScaleEntries, EFFECT_CONTROL_AUTO_CHANGE);
EffectControl table(-1, "table", tableEntries, EFFECT_CONTROL_AUTO_CHANGE);
EffectControl object(-1, "object", objectEntries, EFFECT_CONTROL_AUTO_CHANGE);

static EffectChoice* wave_scales[] = { new EffectChoice("scale0", "large"),
    new EffectChoice("scale1", "medium"), new EffectChoice("scale2", "small") };

static EffectChoice* table_entries[] = {
    new EffectChoice("table0", ""),
    new EffectChoice("table1", ""),
    new EffectChoice("table2", ""),
    new EffectChoice("table3", ""),
    new EffectChoice("table4", ""),
    new EffectChoice("table5", ""),
    new EffectChoice("table6", ""),
    new EffectChoice("table7", ""),
    new EffectChoice("table8", ""),
    new EffectChoice("table9", ""),
};

WaveEntry::WaveEntry(Wave& wave_, int inUse)
    : EffectChoice(wave_.name(), wave_.description(), inUse)
    , waveValue(&wave_) { }

Wave& WaveEntry::wave() const {
    return *waveValue;
}

static EffectChoice* _waves[] = {
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
    : EffectControl(-1, "wave", waveEntries, EFFECT_CONTROL_AUTO_CHANGE) { }

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
 * - Entry: Effect choice name followed by description.  The X11 panel menu shows
 *   the description when present, falling back to Name().
 * - Does: visible drawing behavior.
 * - Colours: how the wave turns its own values into palette indices.  tableColor(runtime, )
 *   means the current table is applied first; "raw" means the byte written to
 *   buffer.destinationPixels() is already the palette index and bypasses the table.
 * - Sound: where the wave gets audio information.
 *
 * wave_dotHor
 * - Entry: DotHor (Dots Horizontal)
 * - Does: plots left/right channel dots across the width, with height driven by
 *   each resampled sample value.
 * - Colours: tableColor(runtime, sample), so sample value selects a table entry.
 * - Sound: PreparedWaveSamples(context, buffer.width()) from context processed wave data.
 *
 * wave_dotVert
 * - Entry: DotVert (Dots Vertical)
 * - Does: plots left/right channel dots down the screen, displaced left/right
 *   from the center by sample value.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, BOTTOM) from context processed wave data.
 *
 * wave_lineHor
 * - Entry: LineHor (Lines Horizontal)
 * - Does: draws connected horizontal-scan oscilloscope traces, split into left
 *   and right halves.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, buffer.width()) from context processed wave data.
 *
 * wave_lineVert
 * - Entry: LineVert (Lines Vertical)
 * - Does: draws connected vertical traces, one channel to each side of center.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, BOTTOM) from context processed wave data.
 *
 * wave_spike
 * - Entry: Spike (Spikes)
 * - Does: draws filled vertical bars rising from the bottom, left and right
 *   channels split across the screen.
 * - Colours: tableColor(runtime, height), so colour follows distance up the spike rather
 *   than the original sample value.
 * - Sound: PreparedWaveSamples(context, buffer.width(), 0) from context processed wave data.
 *
 * wave_spikeH
 * - Entry: SpikeH (Spikes Hollow)
 * - Does: draws only the moving outline of spike heights.
 * - Colours: tableColor(runtime, scaled absolute amplitude).
 * - Sound: PreparedWaveSamples(context, buffer.width(), 0) from context processed wave data.
 *
 * wave_buff9
 * - Entry: Walking (Walking)
 * - Does: draws two vertical traces around a horizontally walking center
 *   column.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, BOTTOM) from context processed wave data.
 *
 * wave_buff10
 * - Entry: Falling (Falling)
 * - Does: writes channel sample dots into a row that advances downward.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, 2 * MID_X) from context processed wave data.
 *
 * wave_buff11
 * - Entry: Lissa (Lissa)
 * - Does: draws a Lissajous-style point cloud, using right channel for x and
 *   left channel for y.
 * - Colours: tableColor(runtime, left sample).
 * - Sound: PreparedWaveSamples(context, buffer.width()) from context processed wave data.
 *
 * wave_buff14
 * - Entry: LineX (Line X)
 * - Does: draws two horizontal traces with different center offsets.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, BOTTOM) from context processed wave data.
 *
 * wave_buff15
 * - Entry: Light1 (Lightning 1)
 * - Does: draws jagged lightning paths for each channel.
 * - Colours: raw palette index 255 for every segment.
 * - Sound: PreparedWaveSamples(context, BOTTOM, 0) from context processed wave data.
 *
 * wave_buff16
 * - Entry: Light2 (Lightning 2)
 * - Does: draws a second jagged lightning variant with gentler sample scaling.
 * - Colours: raw palette index 255 for every segment.
 * - Sound: PreparedWaveSamples(context, buffer.width(), 0) from context processed wave data.
 *
 * wave_pete0
 * - Entry: Pete0 (FireFlies)
 * - Does: draws two drifting point clusters whose offsets wander with the
 *   first few samples.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, buffer.width()) from context processed wave data.
 *
 * wave_pete1
 * - Entry: Pete1 (Pete)
 * - Does: draws two sine-shaped rows scaled by average channel energy.
 * - Colours: tableColor(runtime, signed sample).
 * - Sound: PreparedWaveSamples(context, buffer.width(), 0) from context processed wave data.
 *
 * wave_pete2
 * - Entry: Pete2 (Dot VS sine)
 * - Does: plots vertical dots displaced by sample, one channel on each side.
 * - Colours use the runtime-owned legacy sine lookup for the sample.
 * - Sound: PreparedWaveSamples(context, buffer.height()) from context processed wave data.
 *
 * wave_fract1
 * - Entry: Fract1 (Zippy 1)
 * - Does: walks two persistent points around the buffer using half-sized
 *   differences between neighboring samples.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, buffer.width()) from context processed wave data.
 *
 * wave_fract2
 * - Entry: Fract2 (Zippy 2)
 * - Does: like Fract1, but uses full sample differences for a sharper walk.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, buffer.width()) from context processed wave data.
 *
 * wave_test
 * - Entry: Test (Test)
 * - Does: draws sine-shaped rows scaled by average channel energy, similar to
 *   Pete1 but with unsigned colour lookup.
 * - Colours: tableColor(runtime, sample + 128).
 * - Sound: PreparedWaveSamples(context, buffer.width(), 0) from context processed wave data.
 *
 * wave_aaron
 * - Entry: Aaron (Rings of Fire)
 * - Does: draws two moving ring/rosette point sets when the buffer is large
 *   enough, otherwise advances to the next wave.
 * - Colours: tableColor(runtime, sample).
 * - Sound: PreparedWaveSamples(context, buffer.width()) from context processed wave data.
 *
 * wave_wire1
 * - Entry: Wire1 (Wire frame 1)
 * - Does: rotates the selected object; each edge endpoint can scale
 *   independently, giving a fractured audio-reactive wireframe.
 * - Colours: one startup-random value per wave lifetime, drawn as tableColor(runtime, col).
 * - Sound: directly averages slices of context processed wave data per object edge.
 *
 * wave_wire1dot5
 * - Entry: Wire1dot5 (Wire frame 1.5)
 * - Does: rotates the selected object as a rigid model around a startup-random
 *   axis with one frame-wide audio scale.
 * - Colours: one startup-random value per wave lifetime, drawn as tableColor(runtime, col).
 * - Sound: wire_sound_scale() averages all 1024 samples from
 *   context processed wave data.
 *
 * wave_wire1dot55
 * - Entry: Wire1dot55 (Wire frame 1.55)
 * - Does: Wire1dot5 plus a precessing rotation axis.
 * - Colours: one startup-random value per wave lifetime, drawn as tableColor(runtime, col).
 * - Sound: wire_sound_scale() averages all 1024 samples from
 *   context processed wave data.
 *
 * wave_wire1dot6
 * - Entry: Wire1dot6 (Wire frame 1.6)
 * - Does: rotates the selected object while stretching each vertex radially
 *   according to a stable audio slice.
 * - Colours: one startup-random value per wave lifetime, drawn as tableColor(runtime, col).
 * - Sound: vertex_sound_stretch() hashes object-space vertices into small
 *   slices of context processed wave data.
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
 * - Sound: PreparedWaveSamples(context, buffer.width(), 0) from context processed wave data.
 *
 * wave_spiral
 * - Entry: Spiral (Spirograph)
 * - Does: draws a changing spirograph from center using current amplitude.
 * - Colours: one cycling value per frame, drawn as tableColor(runtime, col).
 * - Sound: context audio-analysis amplitude, amplitudeLeft, and amplitudeRight shape
 *   the curve; runtime.fire() counts down to the next random twist count.
 *
 * wave_pyro
 * - Entry: Pyro (Fire works)
 * - Does: launches and animates bouncing firework streaks on fire events.
 * - Colours: one random value per firework, drawn as tableColor(runtime, col).
 * - Sound: runtime.fire() controls launch and vertical velocity.
 *
 * wave_warp
 * - Entry: Warp (Space warp)
 * - Does: launches expanding rotating radial rings on fire events.
 * - Colours: one random value per ring, drawn as tableColor(runtime, col).
 * - Sound: runtime.fire() controls ring speed, trail count, and rotation.
 *
 * wave_laser
 * - Entry: Laser (Laser)
 * - Does: draws beams from the center to moving endpoints driven by adjacent
 *   sample differences.
 * - Colours: table-mapped channel intensity, so louder samples use higher
 *   palette table entries.
 * - Sound: PreparedWaveSamples(context, (buffer.width() / 10) + 1, 0) from context processed wave data.
 *
 * wave_corner
 * - Entry: Corner (Corner)
 * - Does: on fire events, draws a bright corner/axis shape from a moving point.
 * - Colours: raw fading palette indices 255 >> i.
 * - Sound: runtime.fire() controls movement and thickness, then is consumed locally.
 *
 * wave_jump
 * - Entry: Jump (Jumping points)
 * - Does: per-column points jump away from the vertical center with inertia.
 * - Colours: raw palette index 255.
 * - Sound: PreparedWaveSamples(context, buffer.width()) from context processed wave data; left and
 *   right samples are summed per column.
 *
 * wave_sticks
 * - Entry: Sticks (Random sticks)
 * - Does: draws random line segments across the buffer on fire events.
 * - Colours: raw random palette index, bypassing the table.
 * - Sound: runtime.fire() controls how many sticks are drawn.
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

static void draw_line(FrameStageBuffer& buffer, int x1, int y1, int x2, int y2, int c);

OptionOnOff use_objects("use-objects", 0); /* use 3-D objects */

void configureWaveOptions(const EffectPolicy& config) {
    use_objects.setValue(config.useObjectsEnabled);
}
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

static const char* object_path[] = { "./", "./resources/obj/", CTH_LIBDIR "/obj/", "" };

WObject* currentWaveObject() {
    return waveObjectEntryObject(object.current());
}

/*
 * initialize, load objects
 */
int init_wave(const PathConfig& pathConfig, LogSink& log) {

    init_wave_options();
    init_tables();

    /* load objects from File  */
    if (int(use_objects)) {

        log.info("  loading 3-D objects...");
        loadEffectChoices(object, pathConfig, object_path, "/obj/", ".obj",
            read_object);
        log.info("\n  number of 3-D objects: %d\n", object.getNEntries());
    }

    return 0;
}

/*
 * some helping macros and functions
 */

#define addr(x, y) (buffer.visibleOffset((x), (y)))
#define BOTTOM (buffer.height() - 1)
#define MID_Y (buffer.height() >> 1)
#define MID_X (buffer.width() >> 1)
#define LOW_LINE (buffer.height() - buffer.height() / 10)

static int tableColor(WaveRuntime& runtime, int value) {
    return tables[runtime.table][value];
}

static const char2* processedWaveData(const FrameGeneratorContext& context) {
    if (context.processedWaveData() != 0)
        return context.processedWaveData();

    if (context.audioFrame() != 0)
        return context.audioFrame()->processedWaveData;

    static char2 silentProcessedWaveData[1024];
    return silentProcessedWaveData;
}

static const AudioMetrics& metrics(const FrameGeneratorContext& context) {
    return context.audioAnalysis().metrics();
}

static double frameNow(const FrameGeneratorContext& context) {
    return context.now();
}

static int frameRate(const FrameGeneratorContext& context) {
    if (context.deltaT() > 0.0)
        return max(1, int((1.0 / context.deltaT()) + 0.5));

    return 60;
}

#define PRECESSION_TIME_MIN 4.0
#define PRECESSION_TIME_MAX 12.0

/*
 * Rotate a point around an arbitrary unit axis using Rodrigues' rotation
 * formula.  Most of the old wire code hard-coded the y axis, but this lets
 * the newer object waves keep the same cheap integer-angle timing while
 * choosing a more interesting axis at startup.
 */
static void rotate_axis(const WaveRuntime& runtime,
    double x, double y, double z, const double axis[3], int angle,
    double& rx, double& ry, double& rz) {
    double s = runtime.sineDegrees(angle);
    double c = runtime.cosineDegrees(angle);
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
static void random_axis(WaveRuntime& runtime, double axis[3]) {
    double scale;

    do {
        axis[0] = (double)runtime.randomCenteredInt(100);
        axis[1] = (double)runtime.randomCenteredInt(100);
        axis[2] = (double)runtime.randomCenteredInt(100);
        scale = sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    } while (scale == 0.0);

    axis[0] /= scale;
    axis[1] /= scale;
    axis[2] /= scale;
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
static double vertex_sound_stretch(const FrameGeneratorContext& context, int x, int y, int z) {
    int i;
    int sound = 0;
    int slice = mod(x * 73 + y * 151 + z * 251, 1024);
    const int samples = 8;
    const char2* waveData = processedWaveData(context);

    for (i = 0; i < samples; i++) {
        int sample = (slice + i) & 1023;
        sound += audioSampleMagnitude(waveData[sample][0]);
        sound += audioSampleMagnitude(waveData[sample][1]);
    }

    double amp = (double)sound / (double)(samples * 2 * 128);
    if (amp > 1.0)
        amp = 1.0;

    return 1.0 + amp * 0.35;
}

static int random_wire_color(WaveRuntime& runtime) {
    return runtime.randomInt(256);
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
static int setup_wire_object(FrameStageBuffer& buffer, WaveRuntime& runtime,
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

static double wire_sound_scale(const FrameGeneratorContext& context, double screenScale) {
    int i;
    int sound = 0;
    const char2* waveData = processedWaveData(context);

    for (i = 0; i < 1024; i++) {
        sound += audioSampleMagnitude(waveData[i][0]);
        sound += audioSampleMagnitude(waveData[i][1]);
    }

    return screenScale * (0.60 + 1.40 * ((double)sound / (double)(1024 * 2 * 128)));
}

static void wire_point(
    const WireObjectFrame& frame, int segment, int endpoint, double& x, double& y, double& z) {
    x = (frame.obj[segment][endpoint][0] - frame.px) / frame.objectHalf;
    y = (frame.obj[segment][endpoint][1] - frame.py) / frame.objectHalf;
    z = (frame.obj[segment][endpoint][2] - frame.pz) / frame.objectHalf;
}

static void project_wire_point(const FrameStageBuffer& buffer,
    double ax, double ay, double az, double scale, double cameraDistance, int& sx, int& sy) {
    sx = int((double)ax * scale / (az + cameraDistance) + MID_X);
    sy = int((double)ay * scale / (az + cameraDistance) + MID_Y);
}

static void draw_axis_wire_model(FrameStageBuffer& buffer, WaveRuntime& runtime,
    const WireObjectFrame& frame, const double axis[3], int theta, double scale, double cameraDistance, int col) {
    int i, x1, y1, x2, y2;
    double x, y, z, ax, ay, az;

    for (i = 0; i < frame.n; i++) {
        wire_point(frame, i, 0, x, y, z);
        rotate_axis(runtime, x, y, z, axis, theta, ax, ay, az);
        project_wire_point(buffer, ax, ay, az, scale, cameraDistance, x1, y1);

        wire_point(frame, i, 1, x, y, z);
        rotate_axis(runtime, x, y, z, axis, theta, ax, ay, az);
        project_wire_point(buffer, ax, ay, az, scale, cameraDistance, x2, y2);

        draw_line(buffer, x1, y1, x2, y2, tableColor(runtime, col));
    }
}

static int positiveRoundedScale(int value, int referenceValue) {
    if (value <= 0 || referenceValue <= 0)
        return 1;

    int scale = (value + referenceValue / 2) / referenceValue;
    return scale > 0 ? scale : 1;
}

static int scaledReferencePixels(int value, int actualPixels, int referencePixels) {
    if (value < 0)
        return -scaledReferencePixels(-value, actualPixels, referencePixels);
    if (actualPixels <= 0 || referencePixels <= 0)
        return value;

    return (value * actualPixels + referencePixels / 2) / referencePixels;
}

static const int waveReferenceWidth = 320;
static const int waveReferenceHeight = 240;
static const int legacySinePeriod = 320;

struct WaveDrawingScale {
    int bufferWidth;
    int bufferHeight;
    int width;
    int height;

    /* One reference pixel is one baseline pixel of the historical wave frame. */
    explicit WaveDrawingScale(const FrameStageBuffer& buffer)
        : bufferWidth(buffer.width())
        , bufferHeight(buffer.height())
        , width(positiveRoundedScale(buffer.width(), waveReferenceWidth))
        , height(positiveRoundedScale(buffer.height(), waveReferenceHeight)) { }

    int xOffset(int referencePixels) const {
        return scaledReferencePixels(referencePixels, bufferWidth, waveReferenceWidth);
    }

    int yOffset(int referencePixels) const {
        return scaledReferencePixels(referencePixels, bufferHeight, waveReferenceHeight);
    }

    int referenceYDistance(int actualPixels) const {
        if (actualPixels < 0)
            return -referenceYDistance(-actualPixels);
        if (bufferHeight <= 0)
            return actualPixels;

        return (actualPixels * waveReferenceHeight + bufferHeight / 2) / bufferHeight;
    }
};

static void put_sized_brush_cut(FrameStageBuffer& buffer,
    int brushWidth, int brushHeight, int x, int y, int val) {
    if ((x < 0) || (x >= buffer.width()))
        return;
    if ((y < 0) || (y >= buffer.height()))
        return;

    if (brushWidth < 1)
        brushWidth = 1;
    if (brushHeight < 1)
        brushHeight = 1;

    int left = x - (brushWidth - 1) / 2;
    int right = x + brushWidth / 2;
    int top = y - (brushHeight - 1) / 2;
    int bottom = y + brushHeight / 2;

    if (left < 0)
        left = 0;
    if (right >= buffer.width())
        right = buffer.width() - 1;
    if (top < 0)
        top = 0;
    if (bottom >= buffer.height())
        bottom = buffer.height() - 1;

    for (int yy = top; yy <= bottom; yy++) {
        unsigned char* row = buffer.destinationRow(yy);
        for (int xx = left; xx <= right; xx++)
            row[xx] = (unsigned char)val;
    }
}

static void put_brush_cut(FrameStageBuffer& buffer,
    const WaveDrawingScale& scale, int x, int y, int val) {
    put_sized_brush_cut(buffer, scale.width, scale.height, x, y, val);
}

static void fill_horizontal_span_cut(FrameStageBuffer& buffer,
    int left, int right, int y, int val) {
    if (y < 0 || y >= buffer.height())
        return;

    if (left > right) {
        int temp = left;
        left = right;
        right = temp;
    }

    if (right < 0 || left >= buffer.width())
        return;
    if (left < 0)
        left = 0;
    if (right >= buffer.width())
        right = buffer.width() - 1;

    unsigned char* row = buffer.destinationRow(y);
    for (int x = left; x <= right; x++)
        row[x] = (unsigned char)val;
}

static void put_brush_cut(FrameStageBuffer& buffer, int x, int y, int val) {
    WaveDrawingScale scale(buffer);
    put_brush_cut(buffer, scale, x, y, val);
}

static void put_reference_cross_cut(FrameStageBuffer& buffer,
    const WaveDrawingScale& scale, int x, int y, int val) {
    put_brush_cut(buffer, scale, x, y, val);
    put_brush_cut(buffer, scale, x - scale.xOffset(1), y, val);
    put_brush_cut(buffer, scale, x + scale.xOffset(1), y, val);
    put_brush_cut(buffer, scale, x, y - scale.yOffset(1), val);
    put_brush_cut(buffer, scale, x, y + scale.yOffset(1), val);
}

/* Stable per-point jitter breaks up repeated stamps without temporal noise. */
static unsigned int particle_hash(int x, int y, int val, int spark) {
    unsigned int h = 2166136261u;

    h = (h ^ (unsigned int)x) * 16777619u;
    h = (h ^ (unsigned int)y) * 16777619u;
    h = (h ^ (unsigned int)val) * 16777619u;
    h = (h ^ (unsigned int)spark) * 16777619u;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;

    return h;
}

static int particle_offset(unsigned int value, int radius) {
    if (radius <= 0)
        return 0;

    return int(value % (unsigned int)(radius * 2 + 1)) - radius;
}

static void put_particle_cut(FrameStageBuffer& buffer,
    const WaveDrawingScale& scale, int x, int y, int val) {
    int centerWidth = max(1, (scale.width + 1) / 2);
    int centerHeight = max(1, (scale.height + 1) / 2);
    int sparkWidth = max(1, (centerWidth + 1) / 2);
    int sparkHeight = max(1, (centerHeight + 1) / 2);
    int xRadius = max(1, scale.xOffset(1));
    int yRadius = max(1, scale.yOffset(1));
    unsigned int seed = particle_hash(x, y, val, 0);
    int sparks = 2 + int(seed % 5u);

    put_sized_brush_cut(buffer, centerWidth, centerHeight, x, y, val);

    for (int i = 0; i < sparks; i++) {
        unsigned int h = particle_hash(x, y, val, i + 1);
        int dx = particle_offset(h, xRadius);
        int dy = particle_offset(h >> 8, yRadius);

        if (dx == 0 && dy == 0) {
            if (xRadius >= yRadius)
                dx = (h & 0x10000u) ? xRadius : -xRadius;
            else
                dy = (h & 0x10000u) ? yRadius : -yRadius;
        }

        put_sized_brush_cut(buffer, sparkWidth, sparkHeight,
            x + dx, y + dy, val);
    }
}

void put_particle_cut(FrameStageBuffer& buffer, int x, int y, int val) {
    WaveDrawingScale scale(buffer);
    put_particle_cut(buffer, scale, x, y, val);
}

void putat(FrameStageBuffer& buffer, int x, int y, int val) {
    WaveDrawingScale scale(buffer);
    put_reference_cross_cut(buffer, scale, x, y, val);
}

void putat_cut(FrameStageBuffer& buffer, int x, int y, int val) {
    putat(buffer, x, y, val);
}

void putpixel_cut(FrameStageBuffer& buffer, int x, int y, int val) {
    put_brush_cut(buffer, x, y, val);
}

void draw_line(FrameStageBuffer& buffer, int x1, int y1, int x2, int y2, int c) {
    int lx, ly, dx, dy;
    int i, j, k;
    WaveDrawingScale scale(buffer);

    if (x1 < 0)
        x1 = 0;
    if (x1 >= buffer.width())
        x1 = buffer.width() - 1;
    if (x2 < 0)
        x2 = 0;
    if (x2 >= buffer.width())
        x2 = buffer.width() - 1;
    if (y1 < 0)
        y1 = 0;
    if (y1 >= buffer.height())
        y1 = buffer.height() - 1;
    if (y2 < 0)
        y2 = 0;
    if (y2 >= buffer.height())
        y2 = buffer.height() - 1;

    lx = abs(x1 - x2);
    ly = abs(y1 - y2);
    dx = (x1 > x2) ? -1 : 1;
    dy = (y1 > y2) ? -1 : 1;

    if (lx == 0 && ly == 0) {
        put_brush_cut(buffer, scale, x1, y1, c);
        return;
    }

    if (lx > ly) {
        for (i = x1, j = y1, k = 0; i != x2; i += dx, k += ly) {
            if (k >= lx) {
                k -= lx;
                j += dy;
            }
            put_brush_cut(buffer, scale, i, j, c);
        }
    } else {
        for (i = y1, j = x1, k = 0; i != y2; i += dy, k += lx) {
            if (k >= ly) {
                k -= ly;
                j += dx;
            }
            put_brush_cut(buffer, scale, j, i, c);
        }
    }
}

void do_vwave(FrameStageBuffer& buffer, int ystart, int yend, int x, int val) {
    int ys, ye;
    WaveDrawingScale scale(buffer);
    int left = x - (scale.width - 1) / 2;
    int right = x + scale.width / 2;

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

    for (; ys <= ye; ys++)
        fill_horizontal_span_cut(buffer, left, right, ys, val);
}

void do_hwave(FrameStageBuffer& buffer, int xstart, int xend, int y, int val) {
    int xs, xe;
    WaveDrawingScale scale(buffer);
    int top = y - (scale.height - 1) / 2;
    int bottom = y + scale.height / 2;

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

    if (top < 0)
        top = 0;
    if (bottom >= buffer.height())
        bottom = buffer.height() - 1;

    for (; top <= bottom; top++)
        fill_horizontal_span_cut(buffer, xs, xe, top, val);
}

class PreparedWaveSamples {
    const char2* waveData;
    int step;
    int addValue;

public:
    PreparedWaveSamples(const FrameGeneratorContext& context, int n, int add = 128)
        : waveData(processedWaveData(context))
        , step(n > 0 ? (1024 << 16) / n : 0)
        , addValue(add) { }

    int sample(int index, int channel) const {
        int source = (index * step) >> 16;

        if (source < 0)
            source = 0;
        if (source > 1023)
            source = 1023;

        return waveData[source][channel] + addValue;
    }
};

class WaveRenderer {
public:
    virtual ~WaveRenderer() { }
    virtual void execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) const = 0;
};

class ProcessedWaveDataRenderer : public WaveRenderer {
protected:
    static int color(WaveRuntime& runtime, int value) {
        return tableColor(runtime, value);
    }

    static int scaledSample(int value, int waveScale) {
        return value >> waveScale;
    }

    static int scaledXSample(const WaveDrawingScale& scale, int value,
        int waveScale) {
        return scale.xOffset(scaledSample(value, waveScale));
    }

    static int scaledYSample(const WaveDrawingScale& scale, int value,
        int waveScale) {
        return scale.yOffset(scaledSample(value, waveScale));
    }
};

class DotWaveRenderer : public ProcessedWaveDataRenderer {
public:
    enum Orientation {
        Horizontal,
        Vertical
    };

private:
    Orientation orientation;

public:
    DotWaveRenderer(Orientation orientation_)
        : orientation(orientation_) { }

    void execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) const {
        int x, sample;
        WaveDrawingScale scale(buffer);

        if (orientation == Horizontal) {
            PreparedWaveSamples sound(context, buffer.width());

            for (x = 0; x < buffer.width(); x++) {
                sample = sound.sample(x, 0);
                putpixel_cut(buffer, x >> 1,
                    LOW_LINE - scaledYSample(scale, sample, runtime.waveScale),
                    color(runtime, sample));

                sample = sound.sample(x, 1);
                putpixel_cut(buffer, (x + buffer.width()) >> 1,
                    LOW_LINE - scaledYSample(scale, sample, runtime.waveScale),
                    color(runtime, sample));
            }
            return;
        }

        PreparedWaveSamples sound(context, BOTTOM);

        for (x = 0; x < BOTTOM; x++) {
            sample = sound.sample(x, 0);
            putpixel_cut(buffer,
                MID_X - scaledXSample(scale, sample, runtime.waveScale), x,
                color(runtime, sample));

            sample = sound.sample(x, 1);
            putpixel_cut(buffer,
                MID_X + scaledXSample(scale, sample, runtime.waveScale), x,
                color(runtime, sample));
        }
    }
};

class LineWaveRenderer : public ProcessedWaveDataRenderer {
public:
    enum Variant {
        Horizontal,
        Vertical,
        Walking,
        OffsetPair
    };

private:
    struct HorizontalState {
        int last;

        HorizontalState()
            : last(0) { }
    };

    struct WalkingState {
        int column;

        WalkingState()
            : column(128) { }
    };

    Variant variant;

public:
    LineWaveRenderer(Variant variant_)
        : variant(variant_) { }

    void execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) const {
        switch (variant) {
        case Horizontal:
            executeHorizontal(buffer, context, runtime);
            break;
        case Vertical:
            executeVertical(buffer, context, runtime);
            break;
        case Walking:
            executeWalking(buffer, context, runtime);
            break;
        case OffsetPair:
            executeOffsetPair(buffer, context, runtime);
            break;
        }
    }

private:
    void executeHorizontal(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
        WaveRuntime& runtime) const {
        int x, y, sample;
        HorizontalState& state = runtime.state<HorizontalState>();
        WaveDrawingScale scale(buffer);

        PreparedWaveSamples sound(context, buffer.width());

        for (y = 0; y < 2; y++) {
            for (x = 0; x < buffer.width(); x++) {
                sample = sound.sample(x, y);
                do_vwave(buffer,
                    MID_Y - scaledYSample(scale, sample - 128, runtime.waveScale),
                    MID_Y - scaledYSample(scale, state.last - 128, runtime.waveScale),
                    y ? ((x + buffer.width()) >> 1) : (x >> 1),
                    color(runtime, sample));
                state.last = sample;
            }
        }
    }

    void executeVertical(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
        WaveRuntime& runtime) const {
        int x, sample;
        int last1 = 128;
        int last2 = 128;
        WaveDrawingScale scale(buffer);

        PreparedWaveSamples sound(context, BOTTOM);

        for (x = 0; x < BOTTOM; x++) {
            sample = sound.sample(x, 0);
            do_hwave(buffer,
                MID_X - scaledXSample(scale, sample, runtime.waveScale),
                MID_X - scaledXSample(scale, last1, runtime.waveScale), x,
                color(runtime, sample));
            last1 = sample;

            sample = sound.sample(x, 1);
            do_hwave(buffer,
                MID_X + scaledXSample(scale, sample, runtime.waveScale),
                MID_X + scaledXSample(scale, last2, runtime.waveScale), x,
                color(runtime, sample));
            last2 = sample;
        }
    }

    void executeWalking(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
        WaveRuntime& runtime) const {
        int x, sample;
        int last1 = 128;
        int last2 = 128;
        WalkingState& state = runtime.state<WalkingState>();
        WaveDrawingScale scale(buffer);

        state.column = (state.column + 1) % buffer.width();

        PreparedWaveSamples sound(context, BOTTOM);

        for (x = 0; x < BOTTOM; x++) {
            sample = sound.sample(x, 0);
            do_hwave(buffer,
                state.column - scaledXSample(scale, sample, runtime.waveScale),
                state.column - scaledXSample(scale, last1, runtime.waveScale), x,
                color(runtime, sample));
            last1 = sample;

            sample = sound.sample(x, 1);
            do_hwave(buffer,
                state.column + scaledXSample(scale, sample, runtime.waveScale),
                state.column + scaledXSample(scale, last2, runtime.waveScale), x,
                color(runtime, sample));
            last2 = sample;
        }
    }

    void executeOffsetPair(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
        WaveRuntime& runtime) const {
        int x, sample;
        int last = 128;
        WaveDrawingScale scale(buffer);

        PreparedWaveSamples sound(context, BOTTOM);

        for (x = 0; x < BOTTOM; x++) {
            sample = sound.sample(x, 0);
            do_hwave(buffer,
                MID_X - scaledXSample(scale, sample, runtime.waveScale),
                MID_X - scaledXSample(scale, last, runtime.waveScale), x,
                color(runtime, sample));
            last = sample;
        }
        for (x = 0; x < BOTTOM; x++) {
            sample = sound.sample(x, 1);
            do_hwave(buffer,
                MID_X - scale.xOffset(40) + scaledXSample(scale, sample, runtime.waveScale),
                MID_X - scale.xOffset(40) + scaledXSample(scale, last, runtime.waveScale), x,
                color(runtime, sample));
            last = sample;
        }
    }
};

class SpikeWaveRenderer : public ProcessedWaveDataRenderer {
public:
    enum Style {
        Filled,
        Hollow
    };

private:
    Style style;

public:
    SpikeWaveRenderer(Style style_)
        : style(style_) { }

    void execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) const {
        if (style == Filled)
            executeFilled(buffer, context, runtime);
        else
            executeHollow(buffer, context, runtime);
    }

private:
    static void drawFilledColumn(FrameStageBuffer& buffer,
        const WaveDrawingScale& scale, WaveRuntime& runtime, int x,
        int height) {
        if (height <= 0)
            return;
        if (height >= BOTTOM)
            height = BOTTOM - 1;

        int left = x - (scale.width - 1) / 2;
        int right = x + scale.width / 2;

        for (int y = 0; y < height; y++) {
            int referenceY = scale.referenceYDistance(y);
            if (referenceY > 255)
                referenceY = 255;
            fill_horizontal_span_cut(buffer, left, right, BOTTOM - y,
                color(runtime, referenceY));
        }
    }

    void executeFilled(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
        WaveRuntime& runtime) const {
        int x, sample;
        WaveDrawingScale scale(buffer);

        PreparedWaveSamples sound(context, buffer.width(), 0);

        for (x = 0; x < buffer.width(); x++) {
            sample = scale.yOffset((2 * abs(sound.sample(x, 0))) >> runtime.waveScale);
            drawFilledColumn(buffer, scale, runtime, x >> 1, sample);

            sample = scale.yOffset((2 * abs(sound.sample(x, 1))) >> runtime.waveScale);
            drawFilledColumn(buffer, scale, runtime,
                (x + buffer.width()) >> 1, sample);
        }
    }

    void executeHollow(FrameStageBuffer& buffer, const FrameGeneratorContext& context,
        WaveRuntime& runtime) const {
        int channel, x, sample;
        int last = 0;
        WaveDrawingScale scale(buffer);

        PreparedWaveSamples sound(context, buffer.width(), 0);

        for (channel = 0; channel < 2; channel++) {
            for (x = 0; x < buffer.width(); x++) {
                sample = scale.yOffset((2 * abs(sound.sample(x, channel))) >> runtime.waveScale);
                do_vwave(buffer, BOTTOM - sample, BOTTOM - last,
                    channel ? ((x + buffer.width()) >> 1) : (x >> 1),
                    color(runtime, sample));
                last = sample;
            }
        }
    }
};

class LightningWaveRenderer : public WaveRenderer {
public:
    enum SamplePreparation {
        HeightSamples,
        WidthSamples
    };

private:
    SamplePreparation samplePreparation;
    int sampleShift;

public:
    LightningWaveRenderer(SamplePreparation samplePreparation_, int sampleShift_)
        : samplePreparation(samplePreparation_)
        , sampleShift(sampleShift_) { }

    void execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) const {
        (void)runtime;

        PreparedWaveSamples sound(context, samplePreparation == HeightSamples ? BOTTOM : buffer.width(), 0);
        drawChannel(buffer, sound, 0, buffer.width() / 3);
        drawChannel(buffer, sound, 1, 2 * buffer.width() / 3);
    }

private:
    void drawChannel(FrameStageBuffer& buffer, const PreparedWaveSamples& sound,
        int channel, int startX) const {
        int lastX = startX;
        int lastY = 0;

        for (int y = 0; y < BOTTOM; y++) {
            int x = (sound.sample(y, channel) >> sampleShift) + lastX;

            if (x >= buffer.width())
                x = buffer.width() - 1;
            if (x < 0)
                x = 0;

            draw_line(buffer, lastX, lastY, x, y, 255);
            lastX = x;
            lastY = y;
        }
    }
};

static const DotWaveRenderer dotHorizontalWave(DotWaveRenderer::Horizontal);
static const DotWaveRenderer dotVerticalWave(DotWaveRenderer::Vertical);
static const LineWaveRenderer lineHorizontalWave(LineWaveRenderer::Horizontal);
static const LineWaveRenderer lineVerticalWave(LineWaveRenderer::Vertical);
static const LineWaveRenderer walkingLineWave(LineWaveRenderer::Walking);
static const LineWaveRenderer offsetPairLineWave(LineWaveRenderer::OffsetPair);
static const SpikeWaveRenderer filledSpikeWave(SpikeWaveRenderer::Filled);
static const SpikeWaveRenderer hollowSpikeWave(SpikeWaveRenderer::Hollow);
static const LightningWaveRenderer lightningHeightWave(LightningWaveRenderer::HeightSamples, 4);
static const LightningWaveRenderer lightningWidthWave(LightningWaveRenderer::WidthSamples, 5);

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
void wave_none(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { }

/* Writes fixed raw palette indices: 48, 96, 128, 160, 192, 224, and 255. */
void wave_grid(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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
                buffer.destinationPixels()[addr(x, y)] = c;
        }
    }

    for (int i = 0; (i < buffer.width()) && (i < buffer.height()); i++)
        buffer.destinationPixels()[addr(i, i)] = 255;

    for (int i = 0; (i < buffer.width()) && (i < buffer.height()); i++)
        buffer.destinationPixels()[addr(buffer.width() - 1 - i, i)] = 160;

    putat_cut(buffer, 5, 5, 255);
    putat_cut(buffer, buffer.width() - 6, 5, 192);
    putat_cut(buffer, 5, buffer.height() - 6, 160);
    putat_cut(buffer, buffer.width() - 6, buffer.height() - 6, 128);
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_dotHor(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* dot horizontal */
    dotHorizontalWave.execute(buffer, context, runtime);
}

/*****************************************************************************
 * Dot vertical
 *****************************************************************************/

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_dotVert(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* dot vertical */
    dotVerticalWave.execute(buffer, context, runtime);
}

/****************************************************************************
 * Line horizontal
 ****************************************************************************/

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_lineHor(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Line horizontal */
    lineHorizontalWave.execute(buffer, context, runtime);
}

/****************************************************************************
 * Line vertical
 ****************************************************************************/

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_lineVert(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Line veritcal short */
    lineVerticalWave.execute(buffer, context, runtime);
}

/****************************************************************************
 * Spikes functions
 ****************************************************************************/

/* Writes table-mapped spike-height indices, tableColor(runtime, 0..BOTTOM-1). */
void wave_spike(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Spike */
    filledSpikeWave.execute(buffer, context, runtime);
}

/* Writes table-mapped scaled-amplitude indices, tableColor(runtime, amplitude). */
void wave_spikeH(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Spike hollow */
    hollowSpikeWave.execute(buffer, context, runtime);
}

/*****************************************************************************
 * other wave-functions
 *****************************************************************************/

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_buff9(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Walking */
    walkingLineWave.execute(buffer, context, runtime);
}
/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_buff10(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Falling */
    int i;
    struct State {
        int row;
        State()
            : row(0) { }
    };
    State& state = runtime.state<State>();
    WaveDrawingScale scale(buffer);

    PreparedWaveSamples sound(context, 2 * MID_X);

    state.row = (state.row + 1) % BOTTOM;
    for (i = 0; i < MID_X; i++) {
        put_particle_cut(buffer, scale, i, state.row + 1, tableColor(runtime, sound.sample(i, 0)));
        put_particle_cut(buffer, scale, i + MID_X, state.row + 1, tableColor(runtime, sound.sample(i, 1)));
        put_particle_cut(buffer, scale, i, state.row, tableColor(runtime, sound.sample(i + MID_X, 0)));
        put_particle_cut(buffer, scale, i + MID_X, state.row, tableColor(runtime, sound.sample(i + MID_X, 1)));
    }
}

/* Writes table-mapped left-channel sample indices, tableColor(runtime, sample), across 0..255. */
void wave_buff11(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Lissa */
    int tmp, x, tmp2;
    WaveDrawingScale scale(buffer);

    PreparedWaveSamples sound(context, buffer.width());

    for (x = 0; x < buffer.width(); x++) {
        tmp = sound.sample(x, 0);
        tmp2 = sound.sample(x, 1);

        put_particle_cut(buffer, scale, scale.xOffset(tmp2 + 32) % buffer.width(),
            scale.yOffset(tmp + 200 - 28) % BOTTOM, tableColor(runtime, tmp));
    }
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_buff14(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Line X */
    offsetPairLineWave.execute(buffer, context, runtime);
}

/* Writes fixed raw palette index 255. */
void wave_buff15(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Lightning 1 */
    lightningHeightWave.execute(buffer, context, runtime);
}
/* Writes fixed raw palette index 255. */
void wave_buff16(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Lightning 2 */
    lightningWidthWave.execute(buffer, context, runtime);
}

/* Writes table-mapped sound sample indices, mostly tableColor(runtime, sample) across 0..255. */
void wave_pete0(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* FireFlies */
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
    WaveDrawingScale scale(buffer);

    if (runtime.needsConfiguration()) {
        state.xoff0 = scale.xOffset(160) % buffer.width();
        state.yoff0 = scale.yOffset(100) % buffer.height();
        state.xoff1 = state.xoff0;
        state.yoff1 = state.yoff0;
    }

    PreparedWaveSamples sound(context, buffer.width());

    state.xoff0 += scale.xOffset((sound.sample(0, 0)) % 9 - 4);
    state.yoff0 += scale.yOffset((sound.sample(1, 0)) % 9 - 4);

    state.xoff1 += scale.xOffset((sound.sample(0, 1)) % 9 - 4);
    state.yoff1 += scale.yOffset((sound.sample(1, 1)) % 9 - 4);

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
        temp = sound.sample(x, 0);
        temp2 = sound.sample((x + 80) % buffer.width(), 0);

        put_particle_cut(buffer, scale, (scale.xOffset(temp2 >> 2) + state.xoff0) % buffer.width(),
            (scale.yOffset(temp >> 2) + state.yoff0) % BOTTOM,
            tableColor(runtime, temp));

        temp = sound.sample(x, 1) + 128;
        temp2 = sound.sample((x + 80) % buffer.width(), 1) + 128;

        put_particle_cut(buffer, scale, (scale.xOffset(temp2 >> 2) + state.xoff1) % buffer.width(),
            (scale.yOffset(temp >> 2) + state.yoff1) % BOTTOM,
            tableColor(runtime, temp));
    }
}

/* Writes table-mapped signed-amplitude indices, tableColor(runtime, sample). */
void wave_pete1(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
    int tmp, x, left = 0, right = 0;
    const int* widthSine = runtime.sineForWidth(buffer.width());
    WaveDrawingScale scale(buffer);

    if (widthSine == 0)
        return;

    PreparedWaveSamples sound(context, buffer.width(), 0);

    for (x = 0; x < buffer.width(); x++) {
        left += abs(sound.sample(x, 0));
        right += abs(sound.sample(x, 1));
    }

    left = left / MID_X;
    right = right / MID_X;

    left = min(left, BOTTOM);
    right = min(right, BOTTOM);

    for (x = 0; x < MID_X; x++) {
        tmp = sound.sample(x, 0);
        put_particle_cut(buffer, scale, x,
            BOTTOM - scale.yOffset(abs(left * widthSine[x]) >> 8),
            tableColor(runtime, tmp));
    }
    for (x = MID_X; x < buffer.width(); x++) {
        tmp = sound.sample(x, 1);
        put_particle_cut(buffer, scale, x,
            BOTTOM - scale.yOffset(abs(right * widthSine[x]) >> 8),
            tableColor(runtime, tmp));
    }
}

/* Writes table-mapped sine lookup indices from runtime-owned sine tables. */
void wave_pete2(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Dot VS sine */
    int x, tmp;
    WaveDrawingScale scale(buffer);

    PreparedWaveSamples sound(context, buffer.height());

    for (x = 0; x < buffer.height(); x++) {
        tmp = sound.sample(x, 0);
        put_particle_cut(buffer, scale, MID_X - scale.xOffset(tmp >> runtime.waveScale), x,
            tableColor(runtime, runtime.legacySine(tmp)));
        tmp = sound.sample(x, 1);
        put_particle_cut(buffer, scale, MID_X + scale.xOffset(tmp >> runtime.waveScale), x,
            tableColor(runtime, runtime.legacySine(tmp)));
    }
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_fract1(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Zippy 1*/
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
    WaveDrawingScale scale(buffer);

    PreparedWaveSamples sound(context, buffer.width());

    temp = sound.sample(0, 0);
    for (x = 0; x < buffer.width() - 2; x += 2) {
        state.xoff0 += scale.xOffset((sound.sample(x, 0) - temp) >> 1);
        temp = sound.sample(x, 0);

        while (state.xoff0 < 0)
            state.xoff0 = buffer.width() - 1;

        state.xoff0 = state.xoff0 % buffer.width();

        put_particle_cut(buffer, scale, state.xoff0, state.yoff0, tableColor(runtime, temp));

        state.yoff0 += scale.yOffset((sound.sample(x + 1, 0) - temp) >> 1);
        temp = sound.sample(x + 1, 0);

        while (state.yoff0 < 0)
            state.yoff0 = buffer.height() - 1;

        state.yoff0 = state.yoff0 % buffer.height();

        put_particle_cut(buffer, scale, state.xoff0, state.yoff0, tableColor(runtime, temp));
    }

    temp = sound.sample(0, 1);
    for (x = 0; x < buffer.width() - 2; x += 2) {
        state.xoff1 += scale.xOffset((sound.sample(x, 1) - temp) >> 1);
        temp = sound.sample(x, 1);

        if (state.xoff1 < 0)
            state.xoff1 = buffer.width() - 1;

        state.xoff1 = state.xoff1 % buffer.width();

        put_particle_cut(buffer, scale, state.xoff1, state.yoff1, tableColor(runtime, temp));

        state.yoff1 -= scale.yOffset((sound.sample(x + 1, 1) - temp) >> 1);
        temp = sound.sample(x + 1, 1);

        if (state.yoff1 < 0)
            state.yoff1 = buffer.height() - 1;

        state.yoff1 = state.yoff1 % buffer.height();

        put_particle_cut(buffer, scale, state.xoff1, state.yoff1, tableColor(runtime, temp));
    }
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample), across 0..255. */
void wave_fract2(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Zippy 2 */
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
    WaveDrawingScale scale(buffer);

    PreparedWaveSamples sound(context, buffer.width());

    temp = sound.sample(0, 0);
    for (x = 0; x < buffer.width() - 2; x += 2) {
        state.xoff0 += scale.xOffset(sound.sample(x, 0) - temp);
        temp = sound.sample(x, 0);

        if (state.xoff0 < 0)
            state.xoff0 = buffer.width() - 1;

        state.xoff0 = state.xoff0 % buffer.width();

        put_particle_cut(buffer, scale, state.xoff0, state.yoff0, tableColor(runtime, temp));

        state.yoff0 += scale.yOffset(sound.sample(x + 1, 0) - temp);
        temp = sound.sample(x + 1, 0);

        if (state.yoff0 < 0)
            state.yoff0 = buffer.height() - 1;

        state.yoff0 = state.yoff0 % buffer.height();

        put_particle_cut(buffer, scale, state.xoff0, state.yoff0, tableColor(runtime, temp));
    }

    temp = sound.sample(0, 1);
    for (x = 0; x < buffer.width() - 2; x += 2) {
        state.xoff1 += scale.xOffset(sound.sample(x, 1) - temp);
        temp = sound.sample(x, 1);

        if (state.xoff1 < 0)
            state.xoff1 = buffer.width() - 1;

        state.xoff1 = state.xoff1 % buffer.width();

        put_particle_cut(buffer, scale, state.xoff1, state.yoff1, tableColor(runtime, temp));

        state.yoff1 -= scale.yOffset(sound.sample(x + 1, 1) - temp);
        temp = sound.sample(x + 1, 1);

        if (state.yoff1 < 0)
            state.yoff1 = buffer.height() - 1;

        state.yoff1 = state.yoff1 % buffer.height();

        put_particle_cut(buffer, scale, state.xoff1, state.yoff1, tableColor(runtime, temp));
    }
}

/* Writes table-mapped sound sample indices, tableColor(runtime, sample + 128), across 0..255. */
void wave_test(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) { /* Test */
    int temp, x, left = 0, right = 0;
    const int* widthSine = runtime.sineForWidth(buffer.width());
    WaveDrawingScale scale(buffer);

    if (widthSine == 0)
        return;

    PreparedWaveSamples sound(context, buffer.width(), 0);

    for (x = 0; x < buffer.width(); x++) {
        left += abs(sound.sample(x, 0));
        right += abs(sound.sample(x, 1));
    }

    left = left / (128);
    right = right / (128);

    left = min(left, 199);
    right = min(right, 199);

    for (x = 0; x < MID_X; x++) {
        temp = sound.sample(x, 0) + 128;
        put_particle_cut(buffer, scale, x,
            BOTTOM - scale.yOffset(abs((left)*widthSine[x]) >> 8),
            tableColor(runtime, temp));
    }
    for (x = MID_X; x < buffer.width(); x++) {
        temp = sound.sample(x, 1) + 128;
        put_particle_cut(buffer, scale, x,
            BOTTOM - scale.yOffset(abs((right)*widthSine[x]) >> 8),
            tableColor(runtime, temp));
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
void wave_aaron(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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
    WaveDrawingScale scale(buffer);

    if (state.first || runtime.needsConfiguration()) {
        state.first = 0;
        state.cxl = ((buffer.width() - scale.xOffset(256)) / 2)
            + scale.xOffset(64 + 128);
        state.cxr = ((buffer.width() - scale.xOffset(256)) / 2)
            - scale.xOffset(64) + scale.xOffset(128);
        state.cyl = ((BOTTOM - scale.yOffset(256)) / 2) + scale.yOffset(128);
        state.cyr = state.cyl;
    }

    if ((buffer.height() > 128) && (buffer.width() >= 256)) {
        int tmp, i, sx, sy;

        PreparedWaveSamples sound(context, buffer.width());

        for (i = 0; i < buffer.width(); i++) {
            if (state.y >= legacySinePeriod)
                state.y -= legacySinePeriod;
            if (state.x >= legacySinePeriod)
                state.x -= legacySinePeriod;
            tmp = sound.sample(i, 0);

            sx = scale.xOffset((runtime.legacySine(state.x) * tmp) >> 9);
            txl += sx;
            sy = scale.yOffset((runtime.legacySine(state.y) * tmp) >> 9);
            tyl += sy;

            put_particle_cut(buffer, scale, state.cxl + sx, state.cyl + sy, tableColor(runtime, tmp));

            tmp = sound.sample(i, 1);

            sx = scale.xOffset((runtime.legacySine(state.x) * tmp) >> 9);
            txr += sx;
            sy = scale.yOffset((runtime.legacySine(state.y) * tmp) >> 9);
            tyr += sy;

            put_particle_cut(buffer, scale, state.cxr - sx, state.cyr - sy, tableColor(runtime, tmp));

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
void wave_lineHLdiff(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
    int x, tmp;
    struct State {
        int last;
        State()
            : last(0) { }
    };
    State& state = runtime.state<State>();
    WaveDrawingScale scale(buffer);

    PreparedWaveSamples sound(context, buffer.width(), 0);

    if (buffer.height() < 300) {
        for (x = 0; x < buffer.width(); x++) {
            tmp = sound.sample(x, 0) - sound.sample(x, 1);
            do_vwave(buffer, MID_Y - scale.yOffset(tmp),
                MID_Y - scale.yOffset(state.last), x, tableColor(runtime, tmp + 128));
            state.last = tmp;
        }
    } else {
        for (x = 0; x < buffer.width(); x++) {
            tmp = sound.sample(x, 0) - sound.sample(x, 1);
            do_vwave(buffer, MID_Y - scale.yOffset(tmp),
                MID_Y - scale.yOffset(state.last), x, tableColor(runtime, tmp + 128));
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
void wave_wire1(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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
    const char2* waveData = processedWaveData(context);

    if (object_wave_needs_configuration(runtime))
        state.col = random_wire_color(runtime);

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
            s[0] += audioSampleMagnitude(waveData[sample][0]);
            s[1] += audioSampleMagnitude(waveData[sample][1]);
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
void wave_wire1dot5(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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
        random_axis(runtime, state.axis);
        state.col = random_wire_color(runtime);
    }

    state.theta += 2;
    scale = wire_sound_scale(context, frame.screenScale);

    draw_axis_wire_model(buffer, runtime, frame, state.axis, state.theta, scale, cameraDistance, state.col);
}

/*
 * Wire1dot55 adds axial precession to Wire1dot5.  At startup it chooses an
 * original axis A, a cone angle away from A, and a precession period.  Each
 * frame the actual rotation axis moves around that cone, while the model still
 * keeps Wire1dot5's coherent frame-wide audio scale.
 */
/* Writes one startup-random table-mapped wire index, tableColor(runtime, random 0..255). */
void wave_wire1dot55(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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
        random_axis(runtime, state.baseAxis);
        state.coneAngle = runtime.randomUnit() * M_PI;
        state.precessionTime = PRECESSION_TIME_MIN
            + runtime.randomUnit() * (PRECESSION_TIME_MAX - PRECESSION_TIME_MIN);
        state.precessionStart = frameNow(context);
        state.col = random_wire_color(runtime);
    }

    precess_axis(state.baseAxis, state.coneAngle,
        M_PI2 * (frameNow(context) - state.precessionStart) / state.precessionTime, axis);

    state.theta += 2;
    scale = wire_sound_scale(context, frame.screenScale);

    draw_axis_wire_model(buffer, runtime, frame, axis, state.theta, scale, cameraDistance, state.col);
}

/*
 * Wire1dot6 is the elastic variant.  It still rotates around one startup axis,
 * but each vertex is pushed radially outward according to its own stable audio
 * slice.  The stretch happens before rotation and perspective projection.
 */
/* Writes one startup-random table-mapped wire index, tableColor(runtime, random 0..255). */
void wave_wire1dot6(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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
        random_axis(runtime, state.axis);
        state.col = random_wire_color(runtime);
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
        stretch = vertex_sound_stretch(context,
            frame.obj[i][0][0], frame.obj[i][0][1], frame.obj[i][0][2]);
        wire_point(frame, i, 0, x, y, z);
        x *= stretch;
        y *= stretch;
        z *= stretch;

        rotate_axis(runtime, x, y, z, state.axis, state.theta, ax, ay, az);

        project_wire_point(buffer, ax, ay, az, frame.screenScale, cameraDistance, x1, y1);

        stretch = vertex_sound_stretch(context,
            frame.obj[i][1][0], frame.obj[i][1][1], frame.obj[i][1][2]);
        wire_point(frame, i, 1, x, y, z);
        x *= stretch;
        y *= stretch;
        z *= stretch;

        rotate_axis(runtime, x, y, z, state.axis, state.theta, ax, ay, az);

        project_wire_point(buffer, ax, ay, az, frame.screenScale, cameraDistance, x2, y2);

        draw_line(buffer, x1, y1, x2, y2, tableColor(runtime, state.col));
    }
}

#define nobj 10
#define whirlyRadius 45

static void init_wire2_copy(WaveRuntime& runtime, int loc[3], int& psi, int& rate, int& col) {
    int j, k;

    loc[1] = runtime.randomInt(whirlyRadius * 2) - whirlyRadius;
    j = runtime.randomInt(legacySinePeriod);
    k = 1 + runtime.randomInt(whirlyRadius - 1);
    loc[0] = int(runtime.sineDegrees(j) * k);
    loc[2] = int(runtime.cosineDegrees(j) * k);

    rate = 1 + runtime.randomInt(7);
    if (runtime.randomInt(2))
        rate *= -1;
    psi = runtime.randomInt(legacySinePeriod);
    col = random_wire_color(runtime);
}

class WireSwarmWaveRenderer : public WaveRenderer {
public:
    enum ModelRotation {
        SharedYAxis,
        PerCopyAxis
    };

private:
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

    struct Geometry {
        WObject* obj;
        double objectMidX;
        double objectMidY;
        double objectMidZ;
        double objectScale;
        double screenScale;
        int cameraDistance;
        int originX;
        int originY;
    };

    ModelRotation modelRotation;

public:
    WireSwarmWaveRenderer(ModelRotation modelRotation_)
        : modelRotation(modelRotation_) { }

    void execute(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) const {
        State& state = runtime.state<State>();
        Geometry geometry;

        if (runtime.object == NULL)
            return;

        if (object_wave_needs_configuration(runtime))
            configure(runtime, state);

        if (!prepareGeometry(buffer, runtime, state, geometry))
            return;

        state.theta += 1;

        for (int copy = 0; copy < nobj; copy++) {
            double sto = 0.0;
            double cto = 1.0;

            if (modelRotation == SharedYAxis) {
                sto = runtime.sineDegrees(state.psi[copy]);
                cto = runtime.cosineDegrees(state.psi[copy]);
                state.psi[copy] += state.rate[copy];
            } else {
                state.psi[copy] += state.rate[copy];
            }

            for (int segment = 0; geometry.obj[segment][0][0] != -1; segment++) {
                int x1, y1, x2, y2;

                projectEndpoint(runtime, state, geometry, copy, segment, 0,
                    sto, cto, x1, y1);
                projectEndpoint(runtime, state, geometry, copy, segment, 1,
                    sto, cto, x2, y2);

                draw_line(buffer, x1, y1, x2, y2,
                    tableColor(runtime, state.col[copy]));
            }
        }
    }

private:
    void configure(WaveRuntime& runtime, State& state) const {
        random_axis(runtime, state.blobAxis);

        for (int i = 0; i < nobj; i++) {
            init_wire2_copy(runtime, state.loc[i], state.psi[i], state.rate[i], state.col[i]);
            runtime.log().debug("model %d: rate %d, psi %d, col %d\n",
                i, state.rate[i], state.psi[i], state.col[i]);

            if (modelRotation == PerCopyAxis)
                random_axis(runtime, state.modelAxis[i]);
        }
    }

    int prepareGeometry(FrameStageBuffer& buffer, WaveRuntime& runtime,
        const State& state, Geometry& geometry) const {
        int maxX = 0;
        int maxY = 0;
        int maxZ = 0;
        double objectMaxX = 0.0;
        double objectMaxY = 0.0;
        double objectMaxZ = 0.0;

        geometry.obj = runtime.object;
        if (geometry.obj == NULL)
            return 0;

        for (int segment = 0; geometry.obj[segment][0][0] != -1; segment++) {
            for (int endpoint = 0; endpoint < 2; endpoint++) {
                if (geometry.obj[segment][endpoint][0] > objectMaxX)
                    objectMaxX = geometry.obj[segment][endpoint][0];
                if (geometry.obj[segment][endpoint][1] > objectMaxY)
                    objectMaxY = geometry.obj[segment][endpoint][1];
                if (geometry.obj[segment][endpoint][2] > objectMaxZ)
                    objectMaxZ = geometry.obj[segment][endpoint][2];
            }
        }

        geometry.objectMidX = objectMaxX / 2.0;
        geometry.objectMidY = objectMaxY / 2.0;
        geometry.objectMidZ = objectMaxZ / 2.0;
        geometry.objectScale = geometry.objectMidY / 3.0;

        for (int i = 0; i < nobj; i++) {
            if (state.loc[i][0] > maxX)
                maxX = state.loc[i][0];
            if (state.loc[i][1] > maxY)
                maxY = state.loc[i][1];
            if (state.loc[i][2] > maxZ)
                maxZ = state.loc[i][2];
        }

        geometry.cameraDistance = whirlyRadius * 3 / 2;
        geometry.screenScale = (double)buffer.height() * 0.80;
        if (!geometry.screenScale)
            geometry.screenScale = 1;
        geometry.originX = buffer.width() / 2 - maxX / 2;
        geometry.originY = buffer.height() / 2 - maxY / 2;

        return 1;
    }

    void projectEndpoint(WaveRuntime& runtime, const State& state,
        const Geometry& geometry, int copy, int segment, int endpoint,
        double sto, double cto, int& screenX, int& screenY) const {
        double x = state.loc[copy][0];
        double y = state.loc[copy][1];
        double z = state.loc[copy][2];
        double ax, ay, az;

        rotate_axis(runtime, x, y, z, state.blobAxis, state.theta, ax, ay, az);

        x = (geometry.obj[segment][endpoint][0] - geometry.objectMidX)
            / geometry.objectScale;
        y = (geometry.obj[segment][endpoint][1] - geometry.objectMidY)
            / geometry.objectScale;
        z = (geometry.obj[segment][endpoint][2] - geometry.objectMidZ)
            / geometry.objectScale;

        if (modelRotation == SharedYAxis) {
            ax += x * cto + z * sto;
            ay += y;
            az += z * cto - x * sto;
        } else {
            double lx, ly, lz;
            rotate_axis(runtime, x, y, z, state.modelAxis[copy], state.psi[copy],
                lx, ly, lz);
            ax += lx;
            ay += ly;
            az += lz;
        }

        screenX = int((double)ax * geometry.screenScale
            / (az + geometry.cameraDistance) + geometry.originX);
        screenY = int((double)ay * geometry.screenScale
            / (az + geometry.cameraDistance) + geometry.originY);
    }
};

static const WireSwarmWaveRenderer wireSwarmYAxisWave(WireSwarmWaveRenderer::SharedYAxis);
static const WireSwarmWaveRenderer wireSwarmPerCopyAxisWave(WireSwarmWaveRenderer::PerCopyAxis);

/*
 * Wire2 draws a little swarm of copies of the selected object.  The object
 * itself is not reloaded or copied here; every frame reads the currently
 * selected WObject and draws it at each saved swarm location.
 */
/* Writes per-copy startup-random table-mapped wire indices, tableColor(runtime, random 0..255). */
void wave_wire2(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
    wireSwarmYAxisWave.execute(buffer, context, runtime);
}

/*
 * Wire2dot1 is Wire2 with one extra degree of freedom: each copy has its own
 * local rotation axis.  The blob still rotates around one shared axis, but the
 * individual models no longer all tumble around their local y axes.
 */
/* Writes per-copy startup-random table-mapped wire indices, tableColor(runtime, random 0..255). */
void wave_wire2dot1(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
    wireSwarmPerCopyAxisWave.execute(buffer, context, runtime);
}

/* by Russ */
/* Writes one cycling table-mapped index per frame, tableColor(runtime, col 0..255). */
void wave_spiral(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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
    state.ofs %= legacySinePeriod;

    if (state.loopcount <= 0) {
        state.loopcount = 1 + runtime.randomInt(32);
        state.loops = 2 + runtime.randomInt(8);
    }

    if (runtime.fire())
        state.loopcount--;


    mx = min(buffer.width(), buffer.height());
    cx = buffer.width() / 2;
    cy = buffer.height() / 2;

    const AudioMetrics& audio = metrics(context);
    amp = audio.amplitude;
    int al = audio.amplitudeLeft;
    int ar = audio.amplitudeRight;

    /* convert to float now instead of every time it gets used */
    a = (double)amp * mx / 256.0 / 128.0;
    la = (double)al * mx / 256.0 / 128.0;
    ra = (double)ar * mx / 256.0 / 128.0;

    ox = int(a * runtime.legacySine(state.ofs + 120)
        + la * runtime.legacySine(120) / 2);
    oy = int(a * runtime.legacySine(state.ofs)
        + ra * runtime.legacySine(0) / 2);

    for (i = 1; i < legacySinePeriod; i++) {

        x = a * runtime.legacySine(i + state.ofs + 120)
            + la * runtime.legacySine((i * state.loops) + 120) / 2;
        y = a * runtime.legacySine(i + state.ofs)
            + ra * runtime.legacySine(i * state.loops) / 2;

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
void wave_pyro(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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

        } else if (runtime.fire()) {
            int fire = runtime.fire();

            /* maintain a maximum attack value for scaling purposes */
            if (fire * 4 > state.maxA)
                state.maxA = fire * 4;

            /* slowly reduce max over time to keep it in line with the average levels */
            if (state.maxA > 30)
                state.maxA--;

            /* fire off a new firework */
            state.theWorks[i].dur = 0;
            state.theWorks[i].oxp = state.theWorks[i].xp = runtime.randomInt(buffer.width());
            state.theWorks[i].oyp = state.theWorks[i].yp = buffer.height() - 4;
            state.theWorks[i].xv = runtime.randomInt(20) - 10;
            state.theWorks[i].yv = -(fire * state.maxV / (state.maxA / 4));
            state.theWorks[i].col = runtime.randomInt(256);
            runtime.scaleFire(2, 3);
        }

    /* test rocket exploded, reset */
}

#define maxWarps 15
#define maxWarpTrails 20

typedef struct {
    int r, s, theta, omg, trails, col, rgrav;
} WarpRing;

static int warp_reference_pixels(const FrameStageBuffer& buffer, int referencePixels) {
    int scaled = scaledReferencePixels(referencePixels, buffer.maxDimension(),
        waveReferenceWidth);
    return scaled > 0 ? scaled : 1;
}

static int warp_reference_trails(const FrameStageBuffer& buffer, int referenceTrails) {
    if (referenceTrails <= 0)
        return 1;

    double scale = sqrt((double)buffer.maxDimension() / (double)waveReferenceWidth);
    int scaled = (int)(referenceTrails * scale + 0.5);
    return scaled > 0 ? scaled : 1;
}

/* Writes per-ring random table-mapped indices, tableColor(runtime, random 0..255). */
void wave_warp(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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
                        * runtime.sineDegrees((360 * j) / tr
                            + state.theWarps[i].theta));
                y1 = int(state.cy + ((double)state.theWarps[i].r)
                        * runtime.cosineDegrees((360 * j) / tr
                            + state.theWarps[i].theta));
                x2 = int(state.cx + (double)r2
                    * runtime.sineDegrees(360 * j / tr + t2));
                y2 = int(state.cy + (double)r2
                    * runtime.cosineDegrees(360 * j / tr + t2));
                draw_line(buffer, x1, y1, x2, y2,
                    tableColor(runtime, state.theWarps[i].col));
            }

            /* increment the radius and spiral */
            state.theWarps[i].r += state.theWarps[i].s;
            state.theWarps[i].theta += state.theWarps[i].omg;
            state.theWarps[i].s -= state.theWarps[i].rgrav;

            if (state.theWarps[i].r > state.maxRad || state.theWarps[i].r < 0)
                state.theWarps[i].r = -1;

        } else if (runtime.fire()) {
            int fire = runtime.fire();

            /* maintain a maximum attack value for scaling purposes*/
            if (fire * 4 > state.maxA)
                state.maxA = fire * 4;

            /* slowly reduce max over time to keep it in line with the average levels */
            if (state.maxA > 30)
                state.maxA--;

            /* fire off a new warp ring */
            state.theWarps[i].r = 0;
            state.theWarps[i].s = warp_reference_pixels(buffer,
                3 + fire * 4 * 20 / state.maxA);
            state.theWarps[i].trails = 1 + fire * 4
                * warp_reference_trails(buffer, maxWarpTrails) / state.maxA;
            state.theWarps[i].theta = runtime.randomInt(360);
            state.theWarps[i].omg = (runtime.randomInt(16) - 8) * fire * 4 / state.maxA;
            state.theWarps[i].col = runtime.randomInt(256);
            state.theWarps[i].rgrav = runtime.randomInt(2);
            runtime.consumeFire();
        }
}

/* by Deischi */
static int laser_intensity_index(int sample) {
    return min(abs(sample) << 1, 255);
}

/* Writes table-mapped channel intensity. */
void wave_laser(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
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
    WaveDrawingScale scale(buffer);

    samples = buffer.width() / 10;
    PreparedWaveSamples sound(context, samples + 1, 0);

    state.y = (state.y + scale.yOffset(2)) % buffer.height();
    //y = buffer.height() / 10;

    for (x = 0; x < samples; x++) {
        draw_line(buffer, buffer.width() / 2, buffer.height() / 2, state.xl, state.y,
            tableColor(runtime, laser_intensity_index(sound.sample(x, 0))));
        state.xl = (state.xl
            + scale.xOffset(abs(sound.sample(x, 0) - sound.sample(x + 1, 0))))
            % buffer.width();

        draw_line(buffer, buffer.width() / 2, buffer.height() / 2, state.xr, buffer.height() - state.y,
            tableColor(runtime, laser_intensity_index(sound.sample(x, 1))));
        state.xr = (state.xr
            + scale.xOffset(abs(sound.sample(x, 1) - sound.sample(x + 1, 1))))
            % buffer.width();
    }
}

/* by Deischi (inspired by RTL2) */
/* Writes raw fading indices 255, 127, 63, 31, 15, 7, 3, and 1. */
void wave_corner(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
    struct State {
        int x, y;
        State()
            : x(0)
            , y(0) { }
    };
    State& state = runtime.state<State>();
    WaveDrawingScale scale(buffer);

    if (runtime.fire()) {
        int i, j, t;
        int fire = runtime.fire();
        int marginX = max(1, scale.xOffset(8));
        int marginY = max(1, scale.yOffset(8));
        int rangeX = max(1, buffer.width() - marginX * 2);
        int rangeY = max(1, buffer.height() - marginY * 2);

        state.x = (state.x + runtime.randomInt(fire)) % rangeX + marginX;
        state.y = (state.y + runtime.randomInt(fire)) % rangeY + marginY;

        t = min(fire >> 2, 8);

        if (runtime.randomInt(2)) {
            /* draw corner pointing right down */
            for (i = 0; i < t; i++) {
                int dx = scale.xOffset(i);
                int dy = scale.yOffset(i);
                for (j = 0; j < state.x; j++) {
                    putat(buffer, j, state.y + dy, 255 >> i);
                    putat(buffer, j, state.y - dy, 255 >> i);
                }
                for (j = 0; j < state.y; j++) {
                    putat(buffer, state.x - dx, j, 255 >> i);
                    putat(buffer, state.x + dx, j, 255 >> i);
                }
            }
        } else {
            /* draw corner pointer up left */
            for (i = 0; i < t; i++) {
                int dx = scale.xOffset(i);
                int dy = scale.yOffset(i);
                for (j = state.x; j < buffer.width(); j++) {
                    putat(buffer, j, state.y + dy, 255 >> i);
                    putat(buffer, j, state.y - dy, 255 >> i);
                }
                for (j = state.y; j < buffer.height(); j++) {
                    putat(buffer, state.x - dx, j, 255 >> i);
                    putat(buffer, state.x + dx, j, 255 >> i);
                }
            }
        }
    }

    runtime.consumeFire();
}

// by Deischi
/* Writes fixed raw palette index 255. */
void wave_jump(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {
    struct State {
        std::vector<int> speed;
        std::vector<int> pos;
        std::vector<int> dir;

        void resize(int width) {
            speed.resize(width);
            pos.resize(width);
            dir.resize(width);
        }
    };
    State& state = runtime.state<State>();
    state.resize(buffer.width());
    WaveDrawingScale drawingScale(buffer);

    PreparedWaveSamples sound(context, buffer.width());

    const int sampleScale = 2 + runtime.waveScale;
    for (int i = 0; i < buffer.width(); i++) {
        int e = sound.sample(i, 0) + sound.sample(i, 1);
        if (state.pos[i] < abs(e)) {
            state.speed[i] = abs(e);
            state.dir[i] = e > 0 ? 1 : 0;
        }

        state.pos[i] += state.speed[i];
        if (state.pos[i] < 0) {
            state.pos[i] = 0;
            state.speed[i] = 0;
        } else
            state.speed[i] -= frameRate(context);

        if (state.dir[i] > 0)
            put_particle_cut(buffer, drawingScale, i,
                drawingScale.yOffset(state.pos[i] >> sampleScale) + buffer.height() / 2,
                255);
        else
            put_particle_cut(buffer, drawingScale, i,
                -drawingScale.yOffset(state.pos[i] >> sampleScale) + buffer.height() / 2,
                255);
    }
}

// by Deischi
/* Writes raw random palette indices. */
void wave_sticks(FrameStageBuffer& buffer, const FrameGeneratorContext& context, WaveRuntime& runtime) {

    int n = runtime.fire() >> runtime.waveScale;
    for (int i = 0; i < n; i++) {
        draw_line(buffer, runtime.randomInt(buffer.width()), runtime.randomInt(buffer.height()),
            runtime.randomInt(buffer.width()), runtime.randomInt(buffer.height()),
            runtime.randomInt(256));
    }
}
