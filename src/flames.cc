#include "cthugha.h"
#include "display.h"
#include "Interface.h"
#include "imath.h"
#include "cth_buffer.h"
#include "CthughaBuffer.h"
#include "AutoChanger.h"

CoreOptionEntryList generalFlameEntries;

void flame_clear(CthughaBuffer& buffer);
void flame_upslow(CthughaBuffer& buffer);
void flame_upsubtle(CthughaBuffer& buffer);
void flame_upfast(CthughaBuffer& buffer);
void flame_leftslow(CthughaBuffer& buffer);
void flame_leftsubtle(CthughaBuffer& buffer);
void flame_leftfast(CthughaBuffer& buffer);
void flame_rightslow(CthughaBuffer& buffer);
void flame_rightsubtle(CthughaBuffer& buffer);
void flame_rightfast(CthughaBuffer& buffer);
void flame_water(CthughaBuffer& buffer);
void flame_watersubtle(CthughaBuffer& buffer);
void flame_skyline(CthughaBuffer& buffer);
void flame_weird(CthughaBuffer& buffer);
void flame_zzz(CthughaBuffer& buffer);
void flame_fade(CthughaBuffer& buffer);
void flame_general_subtle(CthughaBuffer& buffer);
void flame_general_slow(CthughaBuffer& buffer);

void flame_general_subtle_filter(CthughaBuffer& buffer);

void flame_general_slow_filter(CthughaBuffer& buffer);

void flame_down(CthughaBuffer& buffer);

const char* OptionGeneralFlame::text() const {
    static char str[32];

    if (lock)
        sprintf(str, "locked:%d", value);
    else
        sprintf(str, "%d", value);

    return str;
}

FlameEntry::FlameEntry(void (*f)(CthughaBuffer& buffer), const char* name, const char* desc, int inUse)
    : CoreOptionEntry(name, desc, inUse)
    , flame(f) { }

void FlameEntry::execute(CthughaBuffer& buffer, const VisualFrameContext& context) {
    (void)context;

    (*flame)(buffer);
}

CoreOptionEntry* _flames[] = {
    new FlameEntry(flame_clear, "Clear", "Blank the buffer", 0),
    new FlameEntry(flame_upslow, "u-Sl", "Up Slow"),
    new FlameEntry(flame_upsubtle, "u-Su", "Up Subtle"),
    new FlameEntry(flame_upfast, "u-Fa", "Up Fast"),
    new FlameEntry(flame_leftslow, "l-Sl", "Left Slow"),
    new FlameEntry(flame_leftsubtle, "l-Su", "Left Subtle"),
    new FlameEntry(flame_leftfast, "l-Fa", "Left Fast"),
    new FlameEntry(flame_rightslow, "r-Sl", "Right Slow"),
    new FlameEntry(flame_rightsubtle, "r-Su", "Right Subtle"),
    new FlameEntry(flame_rightfast, "r-Fa", "Right Fast"),
    new FlameEntry(flame_water, "Water", "Water"),
    new FlameEntry(flame_watersubtle, "Wa-s", "Water Subtle"),
    new FlameEntry(flame_skyline, "Skyline", "Skyline"),
    new FlameEntry(flame_weird, "Weird", "Weird"),
    new FlameEntry(flame_zzz, "Zzz", "Zzz"),
    new FlameEntry(flame_fade, "Fade", "Fade"),
    new FlameEntry(flame_general_subtle, "GenSubt", "General Subtle"),
    new FlameEntry(flame_general_slow, "GenSlow", "General Slow"),
    new FlameEntry(flame_down, "Down", "Falling Down"),
};
int _nFlames = sizeof(_flames) / sizeof(CoreOptionEntry*);

static int flame_offset[4]; /* table for general flames */

/*****************************************************************************/

unsigned char divsub[4 * 256];
unsigned char divsub2[4 * 256];
unsigned int divsub4[256];

unsigned int divsub_s0[4 * 256];
unsigned int divsub_s1[4 * 256];
unsigned int divsub_s2[4 * 256];
unsigned int divsub_s3[4 * 256];

/*
 * Initialize the tables used in the flame-functions
 */
int init_flames() {
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

    return 0;
}

void general_offset(CthughaBuffer& buffer) {
    int i;
    /* offset to the neighbors */
    int position[9] = { -BUFF_WIDTH - 1, -BUFF_WIDTH, -BUFF_WIDTH + 1, -1, 0, 1, +BUFF_WIDTH - 1,
        +BUFF_WIDTH, +BUFF_WIDTH + 1 };

    int gen = buffer.flameGeneral;
    int shift = gen % 9;
    gen = gen / 9;
    /* generate offset-table */
    for (i = 0; i < 4; i++) {
        int p = gen % 9;
        gen = gen / 9;
        flame_offset[i] = position[p] + position[shift];
    }
}

/*
 * UI: Clear (Blank the buffer)
 * Does: clears every visible buffer pixel to palette index 0 before translate
 * and wave drawing.
 * How: direct memset of buffer.activePixels(); it does not read buffer.passivePixels() or the
 * border rows.
 * Sound/border: ignores sound and border input.
 */
void flame_clear(CthughaBuffer& buffer) { memset(buffer.activePixels(), 0, BUFF_SIZE); }

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
void flame_upslow(CthughaBuffer& buffer) {
    int i;
    unsigned int tmp;
    unsigned int tmp2;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + BUFF_WIDTH;

    ptr++;
    tmp = (unsigned int)(*(ptr - 2 - 1)) + (unsigned int)(*(ptr - 1 - 1))
        + (unsigned int)(*(ptr - 1));
    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = tmp - (unsigned int)(*(ptr - 2 - 1)) + (unsigned int)(*(ptr + 1 - 1));
        tmp2 = tmp + (unsigned int)(*(ptr + BUFF_WIDTH - 1));
        *(ptr - BUFF_WIDTH - 1) = divsub[tmp2];
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
void flame_upsubtle(CthughaBuffer& buffer) {
    flame_offset[0] = -1 + BUFF_WIDTH;
    flame_offset[1] = 0 + BUFF_WIDTH;
    flame_offset[2] = 1 + BUFF_WIDTH;
    flame_offset[3] = BUFF_WIDTH + BUFF_WIDTH;

    flame_general_subtle_filter(buffer);
}

/*
 * UI: u-Fa (Up Fast)
 * Does: a faster upward flame with a stronger upward smear.
 * How: swaps active/passive buffers and scans backward, replacing each pixel
 * from itself plus three lower neighbors, then applying divsub.
 * Sound/border: bottom border rows can inject energy into the upward motion.
 */
void flame_upfast(CthughaBuffer& buffer) {
    int i;
    int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + BUFF_SIZE;

    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = (int)(*ptr) + (int)(*(ptr + BUFF_WIDTH - 1)) + (int)(*(ptr + BUFF_WIDTH + 1))
            + (int)(*(ptr + BUFF_WIDTH));
        *ptr = divsub[tmp];
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
void flame_leftslow(CthughaBuffer& buffer) {
    int i;
    int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + BUFF_WIDTH;

    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = (int)(*(ptr - BUFF_WIDTH + 1)) + (int)(*ptr) + (int)(*(ptr + 1))
            + (int)(*(ptr + BUFF_WIDTH));
        *(ptr - BUFF_WIDTH) = divsub[tmp];
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
void flame_leftsubtle(CthughaBuffer& buffer) {
    flame_offset[0] = +1;
    flame_offset[1] = +BUFF_WIDTH;
    flame_offset[2] = 1 + BUFF_WIDTH;
    flame_offset[3] = BUFF_WIDTH + BUFF_WIDTH;

    flame_general_subtle_filter(buffer);
}

/*
 * UI: l-Fa (Left Fast)
 * Does: a faster leftward flame/smear.
 * How: swaps active/passive buffers, then averages current, lower-right twice,
 * and lower into each destination pixel through divsub.
 * Sound/border: bottom border rows influence the lower neighbor reads.
 */
void flame_leftfast(CthughaBuffer& buffer) {
    int i;
    int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + BUFF_SIZE;

    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = (int)(*ptr) + (int)(*(ptr + BUFF_WIDTH + 1)) + (int)(*(ptr + BUFF_WIDTH + 1))
            + (int)(*(ptr + BUFF_WIDTH));
        *ptr = divsub[tmp];
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
void flame_rightslow(CthughaBuffer& buffer) {
    int i;
    int tmp;
    unsigned char* src = buffer.passivePixels() + BUFF_WIDTH + 1;
    unsigned char* dst = buffer.activePixels() + 1;

    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = (int)(*(src - BUFF_WIDTH - 1)) + (int)(*src) + (int)(*(src - 1))
            + (int)(*(src + BUFF_WIDTH));
        *dst = divsub[tmp];
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
void flame_rightsubtle(CthughaBuffer& buffer) {
    flame_offset[0] = -1;
    flame_offset[1] = BUFF_WIDTH - 1;
    flame_offset[2] = BUFF_WIDTH;
    flame_offset[3] = BUFF_WIDTH + BUFF_WIDTH;

    flame_general_subtle_filter(buffer);
}

/*
 * UI: r-Fa (Right Fast)
 * Does: a faster rightward flame/smear.
 * How: swaps active/passive buffers, then averages current, lower-left twice,
 * and lower into each destination pixel through divsub.
 * Sound/border: bottom border rows influence the lower neighbor reads.
 */
void flame_rightfast(CthughaBuffer& buffer) {
    int i;
    int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + BUFF_SIZE;

    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = (int)(*ptr) + (int)(*(ptr + BUFF_WIDTH - 1)) + (int)(*(ptr + BUFF_WIDTH - 1))
            + (int)(*(ptr + BUFF_WIDTH));
        *ptr = divsub[tmp];
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
void flame_water(CthughaBuffer& buffer) {
    int i;
    int tmp;
    unsigned char* src = buffer.passivePixels() + BUFF_WIDTH;
    unsigned char* dst = buffer.activePixels();

    for (i = BUFF_SIZE / 2 + BUFF_WIDTH; i != 0; i--) {
        tmp = (int)(*(src - 1)) + (int)(*src) + (int)(*(src + 1)) + (int)(*(src + BUFF_WIDTH));
        *dst = divsub[tmp];
        dst++;
        src++;
    }

    src = buffer.passivePixels() + BUFF_WIDTH * (BUFF_HEIGHT - 1);
    dst = buffer.activePixels() + BUFF_WIDTH * (BUFF_HEIGHT - 0);
    for (i = BUFF_SIZE / 2; i != 0; i--) {
        tmp = (int)(*(src - BUFF_WIDTH + 1)) + (int)(*src) + (int)(*(src + 1))
            + (int)(*(src - BUFF_WIDTH));
        *dst = divsub[tmp];
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
void flame_watersubtle(CthughaBuffer& buffer) {
    int i;
    unsigned char tmp;
    char* src = (char*)(buffer.passivePixels() + BUFF_WIDTH);
    char* dst = (char*)buffer.activePixels();

    for (i = BUFF_SIZE / 2 + BUFF_WIDTH; i != 0; i--) {
        tmp = (int)(*(src - 1)) + (int)(*src) + (int)(*(src + 1)) + (int)(*(src + BUFF_WIDTH));
        *dst = divsub[tmp];
        dst++;
        src++;
    }

    src = (char*)buffer.passivePixels() + BUFF_WIDTH * (BUFF_HEIGHT - 1);
    dst = (char*)buffer.activePixels() + BUFF_WIDTH * (BUFF_HEIGHT - 0);
    for (i = BUFF_SIZE / 2; i != 0; i--) {
        tmp = (int)(*(src - BUFF_WIDTH + 1)) + (int)(*src) + (int)(*(src + 1))
            + (int)(*(src - BUFF_WIDTH));
        *dst = divsub[tmp];
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
void flame_skyline(CthughaBuffer& buffer) {
    int i;
    int tmp;
    unsigned char* src = buffer.passivePixels() + BUFF_WIDTH + 1;
    unsigned char* dst = buffer.activePixels();

    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = (int)(*(src - 1)) + (int)(*src) + (int)(*(src + 1)) + (int)(*(src));
        *dst = divsub[tmp];
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
void flame_weird(CthughaBuffer& buffer) {
    int i;
    unsigned char tmp;
    char* src = (char*)buffer.passivePixels() + BUFF_WIDTH + 1;
    char* dst = (char*)buffer.activePixels() + 1;

    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = (*(src - 1)) | (*src) | (*(src + 1)) | (*(src + BUFF_WIDTH));
        *dst = divsub[tmp];
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
void flame_zzz(CthughaBuffer& buffer) {
    int i;
    unsigned char tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels() + BUFF_WIDTH;

    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = (*(ptr - 1)) + (*(ptr + BUFF_WIDTH));
        *(ptr - BUFF_WIDTH) = divsub2[tmp];
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
void flame_fade(CthughaBuffer& buffer) {
    int i;
    unsigned int tmp;
    buffer.swapBuffers();
    unsigned char* ptr = buffer.activePixels();

    for (i = BUFF_SIZE / 4; i != 0; i--) {
        tmp = (*(unsigned int*)ptr);
        *(unsigned int*)ptr = divsub4[(tmp) & 0xff] + (divsub4[(tmp >> 8) & 0xff] << 8)
            + (divsub4[(tmp >> 16) & 0xff] << 16) + (divsub4[(tmp >> 24) & 0xff] << 24);
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
void flame_general_subtle(CthughaBuffer& buffer) {
    general_offset(buffer);

    flame_general_subtle_filter(buffer);
}

/*
 * Helper for GenSubt.
 * Does: applies the current four offsets directly from buffer.passivePixels() to
 * buffer.activePixels().
 * How: processes four pixels per loop by packing divsub results into an
 * unsigned int using endian-specific pre-shifted lookup tables.
 */
void flame_general_subtle_filter(CthughaBuffer& buffer) {
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
    offset1 = flame_offset[0] + (buffer.passivePixels() - buffer.activePixels());
    offset2 = flame_offset[1] + (buffer.passivePixels() - buffer.activePixels());
    offset3 = flame_offset[2] + (buffer.passivePixels() - buffer.activePixels());
    offset4 = flame_offset[3] + (buffer.passivePixels() - buffer.activePixels());

    for (i = BUFF_SIZE / 4; i != 0; i--) {
        tmp = (*(ptr + offset1)) + (*(ptr + offset2)) + (*(ptr + offset3)) + (*(ptr + offset4));
        t2 = divsub_s0[tmp];
        tmp = (*(ptr + offset1 + 1)) + (*(ptr + offset2 + 1)) + (*(ptr + offset3 + 1))
            + (*(ptr + offset4 + 1));
        t2 |= divsub_s1[tmp];

        tmp = (*(ptr + offset1 + 2)) + (*(ptr + offset2 + 2)) + (*(ptr + offset3 + 2))
            + (*(ptr + offset4 + 2));
        t2 |= divsub_s2[tmp];

        tmp = (*(ptr + offset1 + 3)) + (*(ptr + offset2 + 3)) + (*(ptr + offset3 + 3))
            + (*(ptr + offset4 + 3));
        t2 |= divsub_s3[tmp];

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
void flame_general_slow(CthughaBuffer& buffer) {
    general_offset(buffer);

    flame_general_slow_filter(buffer);
}

/*
 * Helper for GenSlow.
 * Does: applies the current four offsets directly from buffer.passivePixels() to
 * buffer.activePixels().
 * How: byte-by-byte sum of four neighbors followed by divsub.  Easier to read
 * than GenSubt's packed path, but slower.
 */
void flame_general_slow_filter(CthughaBuffer& buffer) {
    int i;
    int tmp;
    unsigned char* ptr = buffer.activePixels();
    int offset1, offset2, offset3, offset4;

    /* initialize offsets
     *
     *  ptr          -> destination (buffer.activePixels())
     *  ptr + offset -> source (buffer.passivePixels())
     */
    offset1 = flame_offset[0] + (buffer.passivePixels() - buffer.activePixels());
    offset2 = flame_offset[1] + (buffer.passivePixels() - buffer.activePixels());
    offset3 = flame_offset[2] + (buffer.passivePixels() - buffer.activePixels());
    offset4 = flame_offset[3] + (buffer.passivePixels() - buffer.activePixels());

    for (i = BUFF_SIZE; i != 0; i--) {
        tmp = (int)(*(ptr + offset1)) + (int)(*(ptr + offset2)) + (int)(*(ptr + offset3))
            + (int)(*(ptr + offset4));
        *ptr = divsub[tmp];
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
void flame_down(CthughaBuffer& buffer) {
    int i;
    unsigned char* src = buffer.passivePixels() - BUFF_WIDTH;
    unsigned char* dst = buffer.activePixels();

    for (i = BUFF_HEIGHT; i != 0; i--) {
        memcpy(dst, src, BUFF_WIDTH);
        src += BUFF_WIDTH;
        dst += BUFF_WIDTH;
    }
}
