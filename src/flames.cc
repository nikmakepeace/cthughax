// Flame option setup and individual feedback kernels.

#include "cthugha.h"
#include "display.h"
#include "Interface.h"
#include "ProcessServices.h"
#include "cth_buffer.h"
#include "FrameStageBuffer.h"
#include "flames.h"

#include <string.h>

static EffectChoiceList flameEntries;
static EffectChoiceList generalFlameEntries;
static const int generalFlameStates = 9 * 9 * 9 * 9 * 9;

FlameOption flame;
GeneralFlameOption flameGeneral;

static int modInt(int value, int modulo) {
    int result = value % modulo;
    return result < 0 ? result + modulo : result;
}

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

void flame_general_subtle_filter(FrameStageBuffer& buffer,
    const FlameLookupTables& tables, const FlameOffsets& offsets);
void flame_general_slow_filter(FrameStageBuffer& buffer,
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
        snprintf(str, sizeof(str), "locked:%d", value);
    else
        snprintf(str, sizeof(str), "%d", value);

    return str;
}

FlameEntry::FlameEntry(const Flame& flame, int inUse)
    : EffectChoice(flame.name(), flame.description(), inUse)
    , flameValue(&flame) { }

const Flame& FlameEntry::flame() const {
    return *flameValue;
}

FlameOption::FlameOption()
    : EffectControl(-1, "flame", flameEntries, EFFECT_CONTROL_AUTO_CHANGE) { }

const Flame* FlameOption::currentFlame() {
    FlameEntry* entry = dynamic_cast<FlameEntry*>(current());
    return (entry != 0) ? &entry->flame() : 0;
}

GeneralFlameOption::GeneralFlameOption()
    : EffectControl(-1, "flame-general", generalFlameEntries, EFFECT_CONTROL_AUTO_CHANGE) { }

void GeneralFlameOption::change(const char* to, int doSave) {
    static CStdRandomSource fallbackRandomSource;
    change(to, fallbackRandomSource, doSave);
}

void GeneralFlameOption::change(const char* to, RandomSource& randomSource,
    int doSave) {
    char* pos;

    if ((to == NULL) || (to[0] == '\0'))
        return;

    to = applyLockPrefix(to, lock);

    if (doSave)
        save();

    int newValue = strtol(to, &pos, 0);
    if (pos == to) {
        CTH_WARN("Unknown entry `%s' for option `%s'\n", to, name());
        changeRandom(randomSource, 0);
        return;
    }

    value = modInt(newValue, generalFlameStates);
}

void GeneralFlameOption::change(int by, int doSave) {
    if (doSave)
        save();

    value = modInt(value + by, generalFlameStates);
}

void GeneralFlameOption::changeRandom(int doSave) {
    static CStdRandomSource fallbackRandomSource;
    changeRandom(fallbackRandomSource, doSave);
}

void GeneralFlameOption::changeRandom(RandomSource& randomSource, int doSave) {
    if (lock)
        return;

    if (doSave)
        save();

    value = randomSource.uniformInt(generalFlameStates);
}

EffectChoice* _flames[] = {
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
 * Flame tables are owned by FlameFilter.  This startup hook remains while
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

static unsigned char destinationLinearPixel(FrameStageBuffer& buffer, int offset) {
    return buffer.destinationPixels()[buffer.visibleLinearOffset(offset)];
}

static unsigned char sourceLinearPixel(FrameStageBuffer& buffer, int offset) {
    return buffer.sourcePixels()[buffer.visibleLinearOffset(offset)];
}

static int signedByteFromUnsigned(unsigned char value) {
    return (value < 128) ? int(value) : int(value) - 256;
}

static int sourceSignedLinearPixel(FrameStageBuffer& buffer, int offset) {
    return signedByteFromUnsigned(sourceLinearPixel(buffer, offset));
}

static int signedPackedPixel(const unsigned char* pixels, int offset) {
    return signedByteFromUnsigned(pixels[offset]);
}

static void setDestinationLinearPixel(FrameStageBuffer& buffer, int offset,
    unsigned char value) {
    buffer.destinationPixels()[buffer.visibleLinearOffset(offset)] = value;
}

static void clearDestinationVisiblePixels(FrameStageBuffer& buffer) {
    for (int y = 0; y < buffer.height(); y++)
        memset(buffer.destinationRow(y), 0, buffer.width());
}

/*
 * UI: Clear (Blank the buffer)
 * Does: clears every visible buffer pixel to palette index 0 before translate
 * and wave drawing.
 * How: clears each visible row through the render-target pitch; it does not
 * read source pixels or the border rows.
 * Sound/border: ignores sound and border input.
 */
void flame_clear(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    clearDestinationVisiblePixels(buffer);
}

/*****************************************************************************
 *  FLAME-UP
 *****************************************************************************/

/*
 * UI: u-Sl (Up Slow)
 * Does: diffuses the previous frame upward, making bright pixels drift toward
 * the top with a slow, smooth trail.
 * How: the framework initializes destination from immutable source;
 * this averages three nearby horizontal samples plus the pixel below and
 * applies divsub, which divides by four and subtracts one.
 * Sound/border: reads from the hidden bottom border through the below-neighbor
 * samples, so border modes can feed or damp the upward flame.
 */
void flame_upslow(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    unsigned int tmp;
    unsigned int tmp2;
    int width = buffer.width();
    int ptr = width;

    if (buffer.visibleRowsArePacked()) {
        unsigned char* pixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        ptr++;
        tmp = (unsigned int)pixels[ptr - 2 - 1]
            + (unsigned int)pixels[ptr - 1 - 1]
            + (unsigned int)pixels[ptr - 1];
        for (i = buffer.size(); i != 0; i--) {
            tmp = tmp - (unsigned int)pixels[ptr - 2 - 1]
                + (unsigned int)pixels[ptr + 1 - 1];
            tmp2 = tmp + (unsigned int)pixels[ptr + width - 1];
            pixels[ptr - width - 1] = divsub[tmp2];
            ptr++;
        }
        return;
    }

    ptr++;
    tmp = (unsigned int)destinationLinearPixel(buffer, ptr - 2 - 1)
        + (unsigned int)destinationLinearPixel(buffer, ptr - 1 - 1)
        + (unsigned int)destinationLinearPixel(buffer, ptr - 1);
    for (i = buffer.size(); i != 0; i--) {
        tmp = tmp - (unsigned int)destinationLinearPixel(buffer, ptr - 2 - 1)
            + (unsigned int)destinationLinearPixel(buffer, ptr + 1 - 1);
        tmp2 = tmp + (unsigned int)destinationLinearPixel(buffer, ptr + width - 1);
        setDestinationLinearPixel(buffer, ptr - width - 1,
            runtime.lookupTables.divsub[tmp2]);
        ptr++;
    }
}

/*
 * UI: u-Su (Up Subtle)
 * Does: a subtler upward flame using the general four-sample filter.
 * How: sets general offsets to lower-left, lower, lower-right, and two rows
 * down, then runs the general subtle implementation.  Translation runs later
 * as its own filterchain stage.
 * Sound/border: bottom border rows affect the lower-neighbor samples.
 */
void flame_upsubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets(-1 + buffer.width(), 0 + buffer.width(),
        1 + buffer.width(), buffer.width() + buffer.width());

    flame_general_subtle_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * UI: u-Fa (Up Fast)
 * Does: a faster upward flame with a stronger upward smear.
 * How: the framework initializes destination from immutable source;
 * this scans backward, replacing each pixel from itself plus three lower
 * neighbors, then applying divsub.
 * Sound/border: bottom border rows can inject energy into the upward motion.
 */
void flame_upfast(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    int width = buffer.width();
    int ptr = buffer.size();

    if (buffer.visibleRowsArePacked()) {
        unsigned char* pixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        for (i = buffer.size(); i != 0; i--) {
            tmp = (int)pixels[ptr]
                + (int)pixels[ptr + width - 1]
                + (int)pixels[ptr + width + 1]
                + (int)pixels[ptr + width];
            pixels[ptr] = divsub[tmp];
            ptr--;
        }
        return;
    }

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)destinationLinearPixel(buffer, ptr)
            + (int)destinationLinearPixel(buffer, ptr + width - 1)
            + (int)destinationLinearPixel(buffer, ptr + width + 1)
            + (int)destinationLinearPixel(buffer, ptr + width);
        setDestinationLinearPixel(buffer, ptr, runtime.lookupTables.divsub[tmp]);
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
 * How: the framework initializes destination from immutable source;
 * this averages upper-right, current, right, and lower samples into the pixel
 * one row above.
 * Sound/border: vertical neighbor reads can pick up top/bottom border rows.
 */
void flame_leftslow(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    int width = buffer.width();
    int ptr = width;

    if (buffer.visibleRowsArePacked()) {
        unsigned char* pixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        for (i = buffer.size(); i != 0; i--) {
            tmp = (int)pixels[ptr - width + 1]
                + (int)pixels[ptr]
                + (int)pixels[ptr + 1]
                + (int)pixels[ptr + width];
            pixels[ptr - width] = divsub[tmp];
            ptr++;
        }
        return;
    }

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)destinationLinearPixel(buffer, ptr - width + 1)
            + (int)destinationLinearPixel(buffer, ptr)
            + (int)destinationLinearPixel(buffer, ptr + 1)
            + (int)destinationLinearPixel(buffer, ptr + width);
        setDestinationLinearPixel(buffer, ptr - width,
            runtime.lookupTables.divsub[tmp]);
        ptr++;
    }
}

/*
 * UI: l-Su (Left Subtle)
 * Does: a subtler leftward/downwind flame using the general filter.
 * How: sets general offsets to right, below, below-right, and two rows down,
 * then runs the general subtle implementation.  Translation runs later as its
 * own filterchain stage.
 * Sound/border: bottom border rows influence the lower offsets.
 */
void flame_leftsubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets(+1, +buffer.width(),
        1 + buffer.width(), buffer.width() + buffer.width());

    flame_general_subtle_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * UI: l-Fa (Left Fast)
 * Does: a faster leftward flame/smear.
 * How: the framework initializes destination from immutable source;
 * this averages current, lower-right twice, and lower into each destination
 * pixel through divsub.
 * Sound/border: bottom border rows influence the lower neighbor reads.
 */
void flame_leftfast(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    int width = buffer.width();
    int ptr = buffer.size();

    if (buffer.visibleRowsArePacked()) {
        unsigned char* pixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        for (i = buffer.size(); i != 0; i--) {
            tmp = (int)pixels[ptr]
                + (int)pixels[ptr + width + 1]
                + (int)pixels[ptr + width + 1]
                + (int)pixels[ptr + width];
            pixels[ptr] = divsub[tmp];
            ptr--;
        }
        return;
    }

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)destinationLinearPixel(buffer, ptr)
            + (int)destinationLinearPixel(buffer, ptr + width + 1)
            + (int)destinationLinearPixel(buffer, ptr + width + 1)
            + (int)destinationLinearPixel(buffer, ptr + width);
        setDestinationLinearPixel(buffer, ptr, runtime.lookupTables.divsub[tmp]);
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
 * How: reads immutable source pixels prepared by the framework, averaging
 * upper-left, current, left, and lower samples into destination pixels.
 * Sound/border: vertical neighbor reads can pick up top/bottom border rows.
 */
void flame_rightslow(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    int width = buffer.width();
    int src = width + 1;
    int dst = 1;

    if (buffer.visibleRowsArePacked()) {
        const unsigned char* srcPixels = buffer.sourcePixels();
        unsigned char* dstPixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        for (i = buffer.size(); i != 0; i--) {
            tmp = (int)srcPixels[src - width - 1]
                + (int)srcPixels[src]
                + (int)srcPixels[src - 1]
                + (int)srcPixels[src + width];
            dstPixels[dst] = divsub[tmp];
            dst++;
            src++;
        }
        return;
    }

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)sourceLinearPixel(buffer, src - width - 1)
            + (int)sourceLinearPixel(buffer, src)
            + (int)sourceLinearPixel(buffer, src - 1)
            + (int)sourceLinearPixel(buffer, src + width);
        setDestinationLinearPixel(buffer, dst, runtime.lookupTables.divsub[tmp]);
        dst++;
        src++;
    }
}

/*
 * UI: r-Su (Right Subtle)
 * Does: a subtler rightward/downwind flame using the general filter.
 * How: sets general offsets to left, below-left, below, and two rows down,
 * then runs the general subtle implementation.  Translation runs later as its
 * own filterchain stage.
 * Sound/border: bottom border rows influence the lower offsets.
 */
void flame_rightsubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets(-1, buffer.width() - 1,
        buffer.width(), buffer.width() + buffer.width());

    flame_general_subtle_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * UI: r-Fa (Right Fast)
 * Does: a faster rightward flame/smear.
 * How: the framework initializes destination from immutable source;
 * this averages current, lower-left twice, and lower into each destination
 * pixel through divsub.
 * Sound/border: bottom border rows influence the lower neighbor reads.
 */
void flame_rightfast(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    int width = buffer.width();
    int ptr = buffer.size();

    if (buffer.visibleRowsArePacked()) {
        unsigned char* pixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        for (i = buffer.size(); i != 0; i--) {
            tmp = (int)pixels[ptr]
                + (int)pixels[ptr + width - 1]
                + (int)pixels[ptr + width - 1]
                + (int)pixels[ptr + width];
            pixels[ptr] = divsub[tmp];
            ptr--;
        }
        return;
    }

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)destinationLinearPixel(buffer, ptr)
            + (int)destinationLinearPixel(buffer, ptr + width - 1)
            + (int)destinationLinearPixel(buffer, ptr + width - 1)
            + (int)destinationLinearPixel(buffer, ptr + width);
        setDestinationLinearPixel(buffer, ptr, runtime.lookupTables.divsub[tmp]);
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
void flame_water(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    int width = buffer.width();
    int size = buffer.size();
    int src = width;
    int dst = 0;

    if (buffer.visibleRowsArePacked()) {
        const unsigned char* srcPixels = buffer.sourcePixels();
        unsigned char* dstPixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        for (i = size / 2 + width; i != 0; i--) {
            tmp = (int)srcPixels[src - 1]
                + (int)srcPixels[src]
                + (int)srcPixels[src + 1]
                + (int)srcPixels[src + width];
            dstPixels[dst] = divsub[tmp];
            dst++;
            src++;
        }

        src = width * (buffer.height() - 1);
        dst = width * buffer.height();
        for (i = size / 2; i != 0; i--) {
            tmp = (int)srcPixels[src - width + 1]
                + (int)srcPixels[src]
                + (int)srcPixels[src + 1]
                + (int)srcPixels[src - width];
            dstPixels[dst] = divsub[tmp];
            dst--;
            src--;
        }
        return;
    }

    for (i = size / 2 + width; i != 0; i--) {
        tmp = (int)sourceLinearPixel(buffer, src - 1)
            + (int)sourceLinearPixel(buffer, src)
            + (int)sourceLinearPixel(buffer, src + 1)
            + (int)sourceLinearPixel(buffer, src + width);
        setDestinationLinearPixel(buffer, dst, runtime.lookupTables.divsub[tmp]);
        dst++;
        src++;
    }

    src = width * (buffer.height() - 1);
    dst = width * buffer.height();
    for (i = size / 2; i != 0; i--) {
        tmp = (int)sourceLinearPixel(buffer, src - width + 1)
            + (int)sourceLinearPixel(buffer, src)
            + (int)sourceLinearPixel(buffer, src + 1)
            + (int)sourceLinearPixel(buffer, src - width);
        setDestinationLinearPixel(buffer, dst, runtime.lookupTables.divsub[tmp]);
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
void flame_watersubtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    unsigned char tmp;
    int width = buffer.width();
    int size = buffer.size();
    int src = width;
    int dst = 0;

    if (buffer.visibleRowsArePacked()) {
        const unsigned char* srcPixels = buffer.sourcePixels();
        unsigned char* dstPixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        for (i = size / 2 + width; i != 0; i--) {
            tmp = (unsigned char)(signedPackedPixel(srcPixels, src - 1)
                + signedPackedPixel(srcPixels, src)
                + signedPackedPixel(srcPixels, src + 1)
                + signedPackedPixel(srcPixels, src + width));
            dstPixels[dst] = divsub[tmp];
            dst++;
            src++;
        }

        src = width * (buffer.height() - 1);
        dst = width * buffer.height();
        for (i = size / 2; i != 0; i--) {
            tmp = (unsigned char)(signedPackedPixel(srcPixels, src - width + 1)
                + signedPackedPixel(srcPixels, src)
                + signedPackedPixel(srcPixels, src + 1)
                + signedPackedPixel(srcPixels, src - width));
            dstPixels[dst] = divsub[tmp];
            dst--;
            src--;
        }
        return;
    }

    for (i = size / 2 + width; i != 0; i--) {
        tmp = (unsigned char)(sourceSignedLinearPixel(buffer, src - 1)
            + sourceSignedLinearPixel(buffer, src)
            + sourceSignedLinearPixel(buffer, src + 1)
            + sourceSignedLinearPixel(buffer, src + width));
        setDestinationLinearPixel(buffer, dst, runtime.lookupTables.divsub[tmp]);
        dst++;
        src++;
    }

    src = width * (buffer.height() - 1);
    dst = width * buffer.height();
    for (i = size / 2; i != 0; i--) {
        tmp = (unsigned char)(sourceSignedLinearPixel(buffer, src - width + 1)
            + sourceSignedLinearPixel(buffer, src)
            + sourceSignedLinearPixel(buffer, src + 1)
            + sourceSignedLinearPixel(buffer, src - width));
        setDestinationLinearPixel(buffer, dst, runtime.lookupTables.divsub[tmp]);
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
void flame_skyline(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    int tmp;
    int width = buffer.width();
    int src = width + 1;
    int dst = 0;

    if (buffer.visibleRowsArePacked()) {
        const unsigned char* srcPixels = buffer.sourcePixels();
        unsigned char* dstPixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        for (i = buffer.size(); i != 0; i--) {
            tmp = (int)srcPixels[src - 1]
                + (int)srcPixels[src]
                + (int)srcPixels[src + 1]
                + (int)srcPixels[src];
            dstPixels[dst] = divsub[tmp];
            dst++;
            src++;
        }
        return;
    }

    for (i = buffer.size(); i != 0; i--) {
        tmp = (int)sourceLinearPixel(buffer, src - 1)
            + (int)sourceLinearPixel(buffer, src)
            + (int)sourceLinearPixel(buffer, src + 1)
            + (int)sourceLinearPixel(buffer, src);
        setDestinationLinearPixel(buffer, dst, runtime.lookupTables.divsub[tmp]);
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
void flame_weird(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    unsigned char tmp;
    int width = buffer.width();
    int src = width + 1;
    int dst = 1;

    if (buffer.visibleRowsArePacked()) {
        const unsigned char* srcPixels = buffer.sourcePixels();
        unsigned char* dstPixels = buffer.destinationPixels();
        const unsigned char* divsub = runtime.lookupTables.divsub;

        for (i = buffer.size(); i != 0; i--) {
            tmp = (unsigned char)(signedPackedPixel(srcPixels, src - 1)
                | signedPackedPixel(srcPixels, src)
                | signedPackedPixel(srcPixels, src + 1)
                | signedPackedPixel(srcPixels, src + width));
            dstPixels[dst] = divsub[tmp];
            dst++;
            src++;
        }
        return;
    }

    for (i = buffer.size(); i != 0; i--) {
        tmp = (unsigned char)(sourceSignedLinearPixel(buffer, src - 1)
            | sourceSignedLinearPixel(buffer, src)
            | sourceSignedLinearPixel(buffer, src + 1)
            | sourceSignedLinearPixel(buffer, src + width));
        setDestinationLinearPixel(buffer, dst, runtime.lookupTables.divsub[tmp]);
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
 * How: the framework initializes destination from immutable source;
 * this sums left and lower neighbors only, then uses divsub2, which divides by
 * two and subtracts one.
 * Sound/border: lower neighbor reads can pick up bottom border rows.
 */
void flame_zzz(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;
    unsigned char tmp;
    int width = buffer.width();
    int ptr = width;

    if (buffer.visibleRowsArePacked()) {
        unsigned char* pixels = buffer.destinationPixels();
        const unsigned char* divsub2 = runtime.lookupTables.divsub2;

        for (i = buffer.size(); i != 0; i--) {
            tmp = pixels[ptr - 1] + pixels[ptr + width];
            pixels[ptr - width] = divsub2[tmp];
            ptr++;
        }
        return;
    }

    for (i = buffer.size(); i != 0; i--) {
        tmp = destinationLinearPixel(buffer, ptr - 1)
            + destinationLinearPixel(buffer, ptr + width);
        setDestinationLinearPixel(buffer, ptr - width,
            runtime.lookupTables.divsub2[tmp]);
        ptr++;
    }
}

/*
 * UI: Fade (Fade)
 * Does: uniformly darkens the previous frame without moving it.
 * How: the framework initializes destination from immutable source;
 * this subtracts two from each byte, clamped at zero, four pixels at a time
 * through divsub4.
 * Sound/border: ignores sound and border input.
 */
void flame_fade(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    int i;

    if (buffer.visibleRowsArePacked()) {
        unsigned char* pixels = buffer.destinationPixels();
        const unsigned int* divsub4 = runtime.lookupTables.divsub4;
        int limit = (buffer.size() / 4) * 4;

        for (i = 0; i < limit; i++)
            pixels[i] = (unsigned char)divsub4[pixels[i]];
        return;
    }

    for (i = buffer.size() / 4; i != 0; i--) {
        int offset = (buffer.size() / 4 - i) * 4;
        setDestinationLinearPixel(buffer, offset,
            (unsigned char)runtime.lookupTables.divsub4[destinationLinearPixel(buffer, offset)]);
        setDestinationLinearPixel(buffer, offset + 1,
            (unsigned char)runtime.lookupTables.divsub4[destinationLinearPixel(buffer, offset + 1)]);
        setDestinationLinearPixel(buffer, offset + 2,
            (unsigned char)runtime.lookupTables.divsub4[destinationLinearPixel(buffer, offset + 2)]);
        setDestinationLinearPixel(buffer, offset + 3,
            (unsigned char)runtime.lookupTables.divsub4[destinationLinearPixel(buffer, offset + 3)]);
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
 * filterchain stage.
 * Sound/border: depends on the selected offsets; any offset crossing top or
 * bottom can use the hidden border rows.
 */
void flame_general_subtle(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets = general_offsets(runtime.generalFlame, buffer.width());

    flame_general_subtle_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * Helper for GenSubt.
 * Does: applies the current four offsets directly from immutable source pixels
 * to destination pixels.
 * How: processes four pixels per loop by packing divsub results into an
 * unsigned int using endian-specific pre-shifted lookup tables.
 */
void flame_general_subtle_filter(FrameStageBuffer& buffer,
    const FlameLookupTables& tables, const FlameOffsets& offsets) {
    int i;
    unsigned char tmp;

    if (buffer.visibleRowsArePacked()) {
        const unsigned char* src = buffer.sourcePixels();
        unsigned char* dst = buffer.destinationPixels();
        const unsigned char* divsub = tables.divsub;
        const int offset0 = offsets.value[0];
        const int offset1 = offsets.value[1];
        const int offset2 = offsets.value[2];
        const int offset3 = offsets.value[3];

        for (i = 0; i < buffer.size(); i++) {
            tmp = src[i + offset0] + src[i + offset1]
                + src[i + offset2] + src[i + offset3];
            dst[i] = divsub[tmp];
        }
        return;
    }

    for (i = 0; i < buffer.size(); i++) {
        tmp = sourceLinearPixel(buffer, i + offsets.value[0])
            + sourceLinearPixel(buffer, i + offsets.value[1])
            + sourceLinearPixel(buffer, i + offsets.value[2])
            + sourceLinearPixel(buffer, i + offsets.value[3]);
        setDestinationLinearPixel(buffer, i, tables.divsub[tmp]);
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
 * Translation runs later as its own filterchain stage.
 * Sound/border: depends on the selected offsets; any offset crossing top or
 * bottom can use the hidden border rows.
 */
void flame_general_slow(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    FlameOffsets offsets = general_offsets(runtime.generalFlame, buffer.width());

    flame_general_slow_filter(buffer, runtime.lookupTables, offsets);
}

/*
 * Helper for GenSlow.
 * Does: applies the current four offsets directly from immutable source pixels
 * to destination pixels.
 * How: byte-by-byte sum of four neighbors followed by divsub.  Easier to read
 * than GenSubt's packed path, but slower.
 */
void flame_general_slow_filter(FrameStageBuffer& buffer,
    const FlameLookupTables& tables, const FlameOffsets& offsets) {
    int i;
    int tmp;

    if (buffer.visibleRowsArePacked()) {
        const unsigned char* src = buffer.sourcePixels();
        unsigned char* dst = buffer.destinationPixels();
        const unsigned char* divsub = tables.divsub;
        const int offset0 = offsets.value[0];
        const int offset1 = offsets.value[1];
        const int offset2 = offsets.value[2];
        const int offset3 = offsets.value[3];

        for (i = 0; i < buffer.size(); i++) {
            tmp = (int)src[i + offset0] + (int)src[i + offset1]
                + (int)src[i + offset2] + (int)src[i + offset3];
            dst[i] = divsub[tmp];
        }
        return;
    }

    for (i = 0; i < buffer.size(); i++) {
        tmp = (int)sourceLinearPixel(buffer, i + offsets.value[0])
            + (int)sourceLinearPixel(buffer, i + offsets.value[1])
            + (int)sourceLinearPixel(buffer, i + offsets.value[2])
            + (int)sourceLinearPixel(buffer, i + offsets.value[3]);
        setDestinationLinearPixel(buffer, i, tables.divsub[tmp]);
    }
}

/*
 * UI: Down (Falling Down)
 * Does: shifts the previous frame downward by one row.
 * How: copies rows from source pixels starting one row above the visible buffer
 * into destination pixels.
 * Sound/border: the top hidden border row becomes the new top visible row, so
 * border mode has a direct visible effect here.
 */
void flame_down(FrameStageBuffer& buffer, const FrameGeneratorContext& context, FlameRuntime& runtime) {
    if (buffer.visibleRowsArePacked()) {
        memcpy(buffer.destinationPixels(), buffer.sourcePixels() - buffer.width(),
            buffer.size());
        return;
    }

    for (int y = 0; y < buffer.height(); y++) {
        unsigned char* dst = buffer.destinationRow(y);
        int src = -buffer.width() + y * buffer.width();
        for (int x = 0; x < buffer.width(); x++)
            dst[x] = sourceLinearPixel(buffer, src + x);
    }
}
