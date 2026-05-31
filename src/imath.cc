/*
 * this file should stay a C file
 */
#include "imath.h"
#include "cth_buffer.h"
#if defined(__cplusplus) && !defined(CTH_AUDIO_FRAME_NO_RUNTIME) && !defined(CONST_BUFF)        \
    && !defined(BUFF_WIDTH) && !defined(BUFF_HEIGHT)
#include "CthughaBuffer.h"
#endif

#include <stdlib.h>
#include <math.h>

/*
 * return a random number between 0 and range
 */
int Random(int range) {
    if (range > 0)
        return rand() % range;
    else
        return 0;
}

/*
 * Square root for integers.
 * Returns floor(sqrt(n)).
 */
long longSqrt(long n) {
    long root = 0;
    long bit = 0x00008000;

    while (bit) {
        if ((root | bit) * (root | bit) <= n)
            root |= bit;
        bit >>= 1;
    }
    return root;
}

int ilog2(int n) {
    int r = 0;
    while ((1 << r) < n) {
        r++;
    }
    return r;
}

/*
 * sine
 */
int sine[320]; /* sine in 1/320° */
int Bsine[MAX_BUFF_WIDTH]; /* sine in 1/current buffer width */

double sin360[360];

static int imathBufferWidth() {
#if defined(CONST_BUFF) || defined(BUFF_WIDTH) || defined(BUFF_HEIGHT)
    return BUFF_WIDTH;
#elif defined(CTH_AUDIO_FRAME_NO_RUNTIME)
    return 160;
#elif defined(__cplusplus)
    return CthughaBuffer::current->width();
#else
    extern int BUFF_WIDTH;
    return (BUFF_WIDTH > 0) ? BUFF_WIDTH : 320;
#endif
}

int init_imath() {
    int i;

    for (i = 0; i < 320; i++)
        sine[i] = (int)(128 * sin((double)i * 0.03927));

    for (i = 0; i < 360; i++)
        sin360[i] = sin((double)i * (2.0 * M_PI / 360.0));

    int bufferWidth = imathBufferWidth();
    for (i = 0; i < bufferWidth; i++)
        Bsine[i] = (int)(128 * sin((float)i / ((float)bufferWidth) * 2.0 * M_PI));

    return 0;
}

double isin(int deg) {
    deg %= 360;
    if (deg < 0)
        deg += 360;

    return sin360[deg];
}
