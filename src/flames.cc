#include "cthugha.h"
#include "display.h"
#include "Interface.h"
#include "imath.h"
#include "cth_buffer.h"
#include "CthughaBuffer.h"
#include "AutoChanger.h"
#include "flames.h"

static CoreOptionEntryList flameEntries;
static CoreOptionEntryList generalFlameEntries;
static const int generalFlameStates = 9 * 9 * 9 * 9 * 9;

FlameOption flame;
GeneralFlameOption flameGeneral;

struct FlameOffsets {
    int value[4];

    FlameOffsets() {
        value[0] = 0;
        value[1] = 0;
        value[2] = 0;
        value[3] = 0;
    }

    FlameOffsets(int offset0, int offset1, int offset2, int offset3) {
        value[0] = offset0;
        value[1] = offset1;
        value[2] = offset2;
        value[3] = offset3;
    }
};

void flame_general_subtle_filter(CthughaBuffer& buffer,
    const FlameLookupTables& tables, const FlameOffsets& offsets);
void flame_general_slow_filter(CthughaBuffer& buffer,
    const FlameLookupTables& tables, const FlameOffsets& offsets);

static const char* applyLockPrefix(const char* to, OptionOnOff& lock) {
    static const char* lockOn[] = { "l:", "lock:", "locked:" };
    static const char* lockOff[] = { "no-l:", "no-lock:", "no-locked:", "nol:", "nolock:",
        "nolocked:", "non-l:", "non-lock:", "non-locked:", "nonl:", "nonlock:",
        "nonlocked:" };

    for (unsigned int i = 0; i < sizeof(lockOn) / sizeof(lockOn[0]); i++) {
        if (strncasecmp(to, lockOn[i], strlen(lockOn[i])) == 0) {
            lock.change("on");
            return to + strlen(lockOn[i]);
        }
    }

    for (unsigned int i = 0; i < sizeof(lockOff) / sizeof(lockOff[0]); i++) {
        if (strncasecmp(to, lockOff[i], strlen(lockOff[i])) == 0) {
            lock.change("off");
            return to + strlen(lockOff[i]);
        }
    }

    return to;
}

const char* GeneralFlameOption::text() const {
    static char str[32];

    if (lock)
        sprintf(str, "locked:%d", value);
    else
        sprintf(str, "%d", value);

    return str;
}

FlameEntry::FlameEntry(const Flame& flame, int inUse)
    : CoreOptionEntry(flame.name(), flame.description(), inUse)
    , flameValue(&flame) { }

const Flame& FlameEntry::flame() const {
    return *flameValue;
}

FlameOption::FlameOption()
    : CoreOption(-1, "flame", flameEntries) { }

const Flame* FlameOption::currentFlame() {
    FlameEntry* entry = dynamic_cast<FlameEntry*>(current());
    return (entry != 0) ? &entry->flame() : 0;
}

GeneralFlameOption::GeneralFlameOption()
    : CoreOption(-1, "flame-general", generalFlameEntries) { }

void GeneralFlameOption::change(const char* to, int doSave) {
    char* pos;

    if ((to == NULL) || (to[0] == '\0'))
        return;

    to = applyLockPrefix(to, lock);

    if (doSave)
        save();

    int newValue = strtol(to, &pos, 0);
    if (pos == to) {
        CTH_WARN("Unknown entry `%s' for option `%s'\n", to, name());
        changeRandom(0);
        return;
    }

    value = mod(newValue, generalFlameStates);
}

void GeneralFlameOption::change(int by, int doSave) {
    if (doSave)
        save();

    value = mod(value + by, generalFlameStates);
}

void GeneralFlameOption::changeRandom(int doSave) {
    if (lock)
        return;

    if (doSave)
        save();

    value = Random(generalFlameStates);
}

CoreOptionEntry* _flames[] = {
    new FlameEntry(flameCatalog[0], 0),
    new FlameEntry(flameCatalog[1]),
    new FlameEntry(flameCatalog[2]),
    new FlameEntry(flameCatalog[3]),
    new FlameEntry(flameCatalog[4]),
    new FlameEntry(flameCatalog[5]),
    new FlameEntry(flameCatalog[6]),
    new FlameEntry(flameCatalog[7]),
    new FlameEntry(flameCatalog[8]),
    new FlameEntry(flameCatalog[9]),
    new FlameEntry(flameCatalog[10]),
    new FlameEntry(flameCatalog[11]),
    new FlameEntry(flameCatalog[12]),
    new FlameEntry(flameCatalog[13]),
    new FlameEntry(flameCatalog[14]),
    new FlameEntry(flameCatalog[15]),
    new FlameEntry(flameCatalog[16]),
    new FlameEntry(flameCatalog[17]),
    new FlameEntry(flameCatalog[18]),
};
int _nFlames = nFlameCatalogEntries;

/*****************************************************************************/

FlameLookupTables::FlameLookupTables() {
    int i;

    for (i = 0; i < 4 * 256; i++) {
        divsub[i] = i >> 2 ? (i >> 2) - 1 : 0;
        divsub2[i] = i >> 1 ? (i >> 1) - 1 : 0;

#if (__BYTE_ORDER == __BIG_ENDIAN)
        divsub_s3[i] = (unsigned int)divsub[i];
        divsub_s2[i] = (unsigned int)divsub[i] << 8;
        divsub_s1[i] = (unsigned int)divsub[i] << 16;
        divsub_s0[i] = (unsigned int)divsub[i] << 24;
#elif (__BYTE_ORDER == __LITTLE_ENDIAN)
        divsub_s0[i] = (unsigned int)divsub[i];
        divsub_s1[i] = (unsigned int)divsub[i] << 8;
        divsub_s2[i] = (unsigned int)divsub[i] << 16;
        divsub_s3[i] = (unsigned int)divsub[i] << 24;
// #elif
// #error unknown endianess
#endif
    }
    for (i = 0; i < 256; i++)
        divsub4[i] = (i <= 2 ? 0 : i - 2);
}

/*
 * Flame tables are owned by FlameStageModule.  This startup hook remains while
 * the surrounding initialization sequence still calls it.
 */
int init_flames() {
    return 0;
}

static FlameOffsets general_offsets(int generalFlame, int width) {
    int i;
    FlameOffsets offsets;
    /* offset to the neighbors */
    int position[9] = { -width - 1, -width, -width + 1, -1, 0, 1, +width - 1,
        +width, +width + 1 };

    int gen = generalFlame;
    int shift = gen % 9;
    gen = gen / 9;
    /* generate offset-table */
    for (i = 0; i < 4; i++) {
        int p = gen % 9;
        gen = gen / 9;
        offsets.value[i] = position[p] + position[shift];
    }

    return offsets;
}

/*
 * UI: Clear (Blank the buffer)
 * Does: clears every visible buffer pixel to palette index 0 before translate
 * and wave drawing.
 * How: direct memset of buffer.activePixels(); it does not read buffer.passivePixels() or the
 * border rows.
 * Sound/border: ignores sound and border input.
 */
void flame_clear(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) { memset(buffer.activePixels(), 0, buffer.size()); }

/*****************************************************************************
 *  FLAME-UP
 *****************************************************************************/

/*
 * UI: u-Sl (Up Slow)
 * Does: diffuses the previous frame upward, making bright pixels drift toward
 * the top with a slow, smooth trail.
 * How: swaps active/passive buffers, then for each destination pixel averages
 * three nearby horizontal samples plus the pixel below and applies divsub,
 * which divides by four and subtracts one.
 * Sound/border: reads from the hidden bottom border through the below-neighbor
 * samples, so border modes can feed or damp the upward flame.
 */
void flame_upslow(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    unsigned int tmp;
    unsigned int tmp2;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + buffer.width();

    ptr++;
    tmp = (unsigned int)(*(ptr - 2 - 1)) + (unsigned int)(*(ptr - 1 - 1))
        + (unsigned int)(*(ptr - 1));
    for (i = buffer.size(); i != 0; i--) {
        tmp = tmp - (unsigned int)(*(ptr - 2 - 1)) + (unsigned int)(*(ptr + 1 - 1));
        tmp2 = tmp + (unsigned int)(*(ptr + buffer.width() - 1));
        *(ptr - buffer.width() - 1) = runtime.lookupTables.divsub[tmp2];
        ptr++;
    }
}

/*
 * UI: u-Su (Up Subtle)
 * Does: a subtler upward flame using the general four-sample filter.
 * How: sets general offsets to lower-left, lower, lower-right, and two rows
 * down, then runs the general subtle implementation.  Translation runs later
 * as its own pipeline stage.
 * Sound/border: bottom border rows affect the lower-neighbor samples.
 */
void flame_upsubtle(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets(-1 + buffer.width(), 0 + buffer.width(),
        1 + buffer.width(), buffer.width() + buffer.width());

    flame_general_subtle_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * UI: u-Fa (Up Fast)
 * Does: a faster upward flame with a stronger upward smear.
 * How: swaps active/passive buffers and scans backward, replacing each pixel
 * from itself plus three lower neighbors, then applying divsub.
 * Sound/border: bottom border rows can inject energy into the upward motion.
 */
void flame_upfast(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + buffer.size();

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)(*ptr) + (int)(*(ptr + buffer.width() - 1)) + (int)(*(ptr + buffer.width() + 1))
            + (int)(*(ptr + buffer.width()));
        *ptr = runtime.lookupTables.divsub[tmp];
        ptr--;
    }
}

/*****************************************************************************
 *  FLAME-LEFT
 *****************************************************************************/

/*
 * UI: l-Sl (Left Slow)
 * Does: diffuses the previous frame up and left, producing a slow leftward
 * flame drift.
 * How: swaps active/passive buffers and averages upper-right, current, right,
 * and lower samples into the pixel one row above.
 * Sound/border: vertical neighbor reads can pick up top/bottom border rows.
 */
void flame_leftslow(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + buffer.width();

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)(*(ptr - buffer.width() + 1)) + (int)(*ptr) + (int)(*(ptr + 1))
            + (int)(*(ptr + buffer.width()));
        *(ptr - buffer.width()) = runtime.lookupTables.divsub[tmp];
        ptr++;
    }
}

/*
 * UI: l-Su (Left Subtle)
 * Does: a subtler leftward/downwind flame using the general filter.
 * How: sets general offsets to right, below, below-right, and two rows down,
 * then runs the general subtle implementation.  Translation runs later as its
 * own pipeline stage.
 * Sound/border: bottom border rows influence the lower offsets.
 */
void flame_leftsubtle(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets(+1, +buffer.width(),
        1 + buffer.width(), buffer.width() + buffer.width());

    flame_general_subtle_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * UI: l-Fa (Left Fast)
 * Does: a faster leftward flame/smear.
 * How: swaps active/passive buffers, then averages current, lower-right twice,
 * and lower into each destination pixel through divsub.
 * Sound/border: bottom border rows influence the lower neighbor reads.
 */
void flame_leftfast(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + buffer.size();

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)(*ptr) + (int)(*(ptr + buffer.width() + 1)) + (int)(*(ptr + buffer.width() + 1))
            + (int)(*(ptr + buffer.width()));
        *ptr = runtime.lookupTables.divsub[tmp];
        ptr--;
    }
}

/*****************************************************************************
 *  FLAME-RIGHT
 *****************************************************************************/

/*
 * UI: r-Sl (Right Slow)
 * Does: diffuses the previous frame up and right, producing a slow rightward
 * flame drift.
 * How: reads buffer.passivePixels() without swapping, averaging upper-left, current,
 * left, and lower samples into buffer.activePixels().
 * Sound/border: vertical neighbor reads can pick up top/bottom border rows.
 */
void flame_rightslow(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    unsigned char* src = buffer.passivePixels() + buffer.width() + 1;
    unsigned char* dst = buffer.activePixels() + 1;

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)(*(src - buffer.width() - 1)) + (int)(*src) + (int)(*(src - 1))
            + (int)(*(src + buffer.width()));
        *dst = runtime.lookupTables.divsub[tmp];
        dst++;
        src++;
    }
}

/*
 * UI: r-Su (Right Subtle)
 * Does: a subtler rightward/downwind flame using the general filter.
 * How: sets general offsets to left, below-left, below, and two rows down,
 * then runs the general subtle implementation.  Translation runs later as its
 * own pipeline stage.
 * Sound/border: bottom border rows influence the lower offsets.
 */
void flame_rightsubtle(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets(-1, buffer.width() - 1,
        buffer.width(), buffer.width() + buffer.width());

    flame_general_subtle_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * UI: r-Fa (Right Fast)
 * Does: a faster rightward flame/smear.
 * How: swaps active/passive buffers, then averages current, lower-left twice,
 * and lower into each destination pixel through divsub.
 * Sound/border: bottom border rows influence the lower neighbor reads.
 */
void flame_rightfast(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + buffer.size();

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)(*ptr) + (int)(*(ptr + buffer.width() - 1)) + (int)(*(ptr + buffer.width() - 1))
            + (int)(*(ptr + buffer.width()));
        *ptr = runtime.lookupTables.divsub[tmp];
        ptr--;
    }
}

/****************************************************************************
 *  FLAME-WATER
 *****************************************************************************/

/*
 * UI: Water (Water)
 * Does: pulls the image in from both vertical directions, creating a meeting
 * ripple/waterfall effect around the middle of the buffer.
 * How: top half is filtered forward from left/current/right/lower samples;
 * bottom half is filtered backward from upper/right/current samples.  Both
 * halves use divsub for averaging and decay.
 * Sound/border: both top and bottom border rows can affect the two halves.
 */
void flame_water(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    unsigned char* src = buffer.passivePixels() + buffer.width();
    unsigned char* dst = buffer.activePixels();

    for (i = buffer.size() / 2 + buffer.width(); i != 0; i--) {
        tmp = (int)(*(src - 1)) + (int)(*src) + (int)(*(src + 1)) + (int)(*(src + buffer.width()));
        *dst = runtime.lookupTables.divsub[tmp];
        dst++;
        src++;
    }

    src = buffer.passivePixels() + buffer.width() * (buffer.height() - 1);
    dst = buffer.activePixels() + buffer.width() * (buffer.height() - 0);
    for (i = buffer.size() / 2; i != 0; i--) {
        tmp = (int)(*(src - buffer.width() + 1)) + (int)(*src) + (int)(*(src + 1))
            + (int)(*(src - buffer.width()));
        *dst = runtime.lookupTables.divsub[tmp];
        dst--;
        src--;
    }
}

/*
 * UI: Wa-s (Water Subtle)
 * Does: a lower-precision/subtler water variant.
 * How: same two-half neighbor pattern as Water, but uses signed char
 * temporaries, preserving the old compact arithmetic behavior.
 * Sound/border: both top and bottom border rows can affect the two halves.
 */
void flame_watersubtle(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    unsigned char tmp;
    char* src = (char*)(buffer.passivePixels() + buffer.width());
    char* dst = (char*)buffer.activePixels();

    for (i = buffer.size() / 2 + buffer.width(); i != 0; i--) {
        tmp = (int)(*(src - 1)) + (int)(*src) + (int)(*(src + 1)) + (int)(*(src + buffer.width()));
        *dst = runtime.lookupTables.divsub[tmp];
        dst++;
        src++;
    }

    src = (char*)buffer.passivePixels() + buffer.width() * (buffer.height() - 1);
    dst = (char*)buffer.activePixels() + buffer.width() * (buffer.height() - 0);
    for (i = buffer.size() / 2; i != 0; i--) {
        tmp = (int)(*(src - buffer.width() + 1)) + (int)(*src) + (int)(*(src + 1))
            + (int)(*(src - buffer.width()));
        *dst = runtime.lookupTables.divsub[tmp];
        dst--;
        src--;
    }
}

/****************************************************************************
 *  FLAME-others
 *****************************************************************************/

/*
 * UI: Skyline (Skyline)
 * Does: smears each row from side neighbors and itself, giving a flatter,
 * horizon-like diffusion rather than a directional flame.
 * How: averages left, current, right, and current again through divsub.
 * Sound/border: does not intentionally sample vertical border rows.
 */
void flame_skyline(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    unsigned char* src = buffer.passivePixels() + buffer.width() + 1;
    unsigned char* dst = buffer.activePixels();

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)(*(src - 1)) + (int)(*src) + (int)(*(src + 1)) + (int)(*(src));
        *dst = runtime.lookupTables.divsub[tmp];
        dst++;
        src++;
    }
}

/*
 * UI: Weird (Weird)
 * Does: creates a blockier, high-contrast propagation effect.
 * How: ORs left, current, right, and lower neighbors, then passes that value
 * through divsub.  The OR keeps bits alive differently from arithmetic
 * averaging, which gives this flame its sharper texture.
 * Sound/border: lower neighbor reads can pick up bottom border rows.
 */
void flame_weird(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    unsigned char tmp;
    char* src = (char*)buffer.passivePixels() + buffer.width() + 1;
    char* dst = (char*)buffer.activePixels() + 1;

    for (i = buffer.size(); i != 0; i--) {
        tmp = (*(src - 1)) | (*src) | (*(src + 1)) | (*(src + buffer.width()));
        *dst = runtime.lookupTables.divsub[tmp];
        dst++;
        src++;
    }
}

/*****************************************************************************
 *  new from cthugha 5.3
 *****************************************************************************/

/*
 * UI: Zzz (Zzz)
 * Does: a sparse upward drift/decay effect.
 * How: swaps active/passive buffers, sums left and lower neighbors only, then
 * uses divsub2, which divides by two and subtracts one.
 * Sound/border: lower neighbor reads can pick up bottom border rows.
 */
void flame_zzz(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    unsigned char tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + buffer.width();

    for (i = buffer.size(); i != 0; i--) {
        tmp = (*(ptr - 1)) + (*(ptr + buffer.width()));
        *(ptr - buffer.width()) = runtime.lookupTables.divsub2[tmp];
        ptr++;
    }
}

/*
 * UI: Fade (Fade)
 * Does: uniformly darkens the previous frame without moving it.
 * How: swaps active/passive buffers and subtracts two from each byte, clamped
 * at zero, four pixels at a time through divsub4.
 * Sound/border: ignores sound and border input.
 */
void flame_fade(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    unsigned int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels();

    for (i = buffer.size() / 4; i != 0; i--) {
        tmp = (*(unsigned int*)ptr);
        *(unsigned int*)ptr = runtime.lookupTables.divsub4[(tmp) & 0xff] + (runtime.lookupTables.divsub4[(tmp >> 8) & 0xff] << 8)
            + (runtime.lookupTables.divsub4[(tmp >> 16) & 0xff] << 16) + (runtime.lookupTables.divsub4[(tmp >> 24) & 0xff] << 24);
        ptr += 4;
    }
}

/*****************************************************************************
 * general flame functions
 *
 * this functions can replace most of the functions above.
 *****************************************************************************/

/*****************************************************************************
 * general subtle flame
 *****************************************************************************/
/*
 * UI: GenSubt (General Subtle)
 * Does: configurable subtle four-neighbor flame.
 * How: decodes flameGeneral into four neighbor offsets plus a shared shift,
 * then runs the packed four-pixel filter.  Translation runs later as its own
 * pipeline stage.
 * Sound/border: depends on the selected offsets; any offset crossing top or
 * bottom can use the hidden border rows.
 */
void flame_general_subtle(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets = general_offsets(runtime.generalFlame, buffer.width());

    flame_general_subtle_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * Helper for GenSubt.
 * Does: applies the current four offsets directly from buffer.passivePixels() to
 * buffer.activePixels().
 * How: processes four pixels per loop by packing divsub results into an
 * unsigned int using endian-specific pre-shifted lookup tables.
 */
void flame_general_subtle_filter(CthughaBuffer& buffer,
    const FlameLookupTables& tables, const FlameOffsets& offsets) {
    int i;
    unsigned char tmp;
    unsigned char* ptr = buffer.activePixels();
    int offset1, offset2, offset3, offset4;
    unsigned int t2;

    /* initialize offsets
     *
     *  ptr          -> destination (buffer.activePixels())
     *  ptr + offset -> source (buffer.passivePixels())
     */
    offset1 = offsets.value[0] + (buffer.passivePixels() - buffer.activePixels());
    offset2 = offsets.value[1] + (buffer.passivePixels() - buffer.activePixels());
    offset3 = offsets.value[2] + (buffer.passivePixels() - buffer.activePixels());
    offset4 = offsets.value[3] + (buffer.passivePixels() - buffer.activePixels());

    for (i = buffer.size() / 4; i != 0; i--) {
        tmp = (*(ptr + offset1)) + (*(ptr + offset2)) + (*(ptr + offset3)) + (*(ptr + offset4));
        t2 = tables.divsub_s0[tmp];
        tmp = (*(ptr + offset1 + 1)) + (*(ptr + offset2 + 1)) + (*(ptr + offset3 + 1))
            + (*(ptr + offset4 + 1));
        t2 |= tables.divsub_s1[tmp];

        tmp = (*(ptr + offset1 + 2)) + (*(ptr + offset2 + 2)) + (*(ptr + offset3 + 2))
            + (*(ptr + offset4 + 2));
        t2 |= tables.divsub_s2[tmp];

        tmp = (*(ptr + offset1 + 3)) + (*(ptr + offset2 + 3)) + (*(ptr + offset3 + 3))
            + (*(ptr + offset4 + 3));
        t2 |= tables.divsub_s3[tmp];

        *(unsigned int*)ptr = t2;
        ptr += 4;
    }
}

/*****************************************************************************
 * general slow flame
 *
 * is a bit slower than traditional flame functions
 *****************************************************************************/

/*
 * UI: GenSlow (General Slow)
 * Does: configurable four-neighbor flame, written as the simpler byte-by-byte
 * reference path.
 * How: decodes flameGeneral into offsets and runs the byte-by-byte filter.
 * Translation runs later as its own pipeline stage.
 * Sound/border: depends on the selected offsets; any offset crossing top or
 * bottom can use the hidden border rows.
 */
void flame_general_slow(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets = general_offsets(runtime.generalFlame, buffer.width());

    flame_general_slow_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * Helper for GenSlow.
 * Does: applies the current four offsets directly from buffer.passivePixels() to
 * buffer.activePixels().
 * How: byte-by-byte sum of four neighbors followed by divsub.  Easier to read
 * than GenSubt's packed path, but slower.
 */
void flame_general_slow_filter(CthughaBuffer& buffer,
    const FlameLookupTables& tables, const FlameOffsets& offsets) {
    int i;
    int tmp;
    unsigned char* ptr = buffer.activePixels();
    int offset1, offset2, offset3, offset4;

    /* initialize offsets
     *
     *  ptr          -> destination (buffer.activePixels())
     *  ptr + offset -> source (buffer.passivePixels())
     */
    offset1 = offsets.value[0] + (buffer.passivePixels() - buffer.activePixels());
    offset2 = offsets.value[1] + (buffer.passivePixels() - buffer.activePixels());
    offset3 = offsets.value[2] + (buffer.passivePixels() - buffer.activePixels());
    offset4 = offsets.value[3] + (buffer.passivePixels() - buffer.activePixels());

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)(*(ptr + offset1)) + (int)(*(ptr + offset2)) + (int)(*(ptr + offset3))
            + (int)(*(ptr + offset4));
        *ptr = tables.divsub[tmp];
        ptr++;
    }
}

/*
 * UI: Down (Falling Down)
 * Does: shifts the previous frame downward by one row.
 * How: copies rows from buffer.passivePixels() starting one row above the visible
 * buffer into buffer.activePixels().
 * Sound/border: the top hidden border row becomes the new top visible row, so
 * border mode has a direct visible effect here.
 */
void flame_down(CthughaBuffer& buffer, const VideoFrameContext& context, FlameRuntime& runtime) {
    int i;
    unsigned char* src = buffer.passivePixels() - buffer.width();
    unsigned char* dst = buffer.activePixels();

    for (i = buffer.height(); i != 0; i--) {
        memcpy(dst, src, buffer.width());
        src += buffer.width();
        dst += buffer.width();
    }
}
