/*
 * cmd_bighalfwheel.c
 *
 * Generate a translation table -> write table to stdout.
 */

/* minor changes by Harald Deischinger:
   - un-fucked up Little Endian
   - use BUFF_WIDTH and BUFF_HEIGHT
   - fixed problem with ID
   - filename as parameter
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "../src/cthugha.h"
#include "../src/cth_buffer.h"

int BUFF_WIDTH;
int BUFF_HEIGHT;

#define XSIZE BUFF_WIDTH

#define YSIZE BUFF_HEIGHT

#define BSIZE (XSIZE * YSIZE)

typedef uint32_t tType;
#define MS SL

typedef int* trans_map;

#ifdef MAC

/* these are for reading fucked up Little Endian files from fucked up Little Endian machines */
#define SS(s) (((s) & 0xff) << 8 | ((s) & 0xff00) >> 8)
#define SL(l)                                                                                      \
    (((l) & 0xff) << 24 | ((l) & 0xff00) << 8 | ((l) & 0xff0000) >> 8 | ((l) & 0xff000000) >> 24)

#else

#define SS(s) (s)
#define SL(l) (l)
#endif

#ifndef PI
#define PI 3.14159265399
#endif

int main(int argc, char* argv[]) {
    int i, j, dx, dy, cx, cy, dist;
    unsigned int* theTab;
    tType lineOut[65536];
    float q, ang, p;

    /* printf("Making big half wheel file for %dx%d buffer...\n",XSIZE,YSIZE); */

    if (argc < 3) {
        fprintf(stderr, "Missing size parameters.\n");
        return 1;
    }
    BUFF_WIDTH = atoi(argv[1]);
    BUFF_HEIGHT = atoi(argv[2]);

    theTab = malloc(sizeof(int) * BSIZE);
    if (!theTab) {
        fprintf(stderr, "Not enough memory!\n");
        exit(1);
    }

    cx = XSIZE * 0.4;
    cy = 0;
    q = 3.14159265399 / 2;
    p = (float)0 / 180 * PI;

    for (j = 0; j < YSIZE; j++) {

        for (i = 0; i < XSIZE; i++) {

            if (j == 0 || j == YSIZE) {

                dx = (float)(cx - i) * 0.75;
                dy = cy - j;

            } else {

                dist = sqrt((i - cx) * (i - cx) + (j - cy) * (j - cy));

                if (i == cx) {
                    if (j > cx)
                        ang = q;
                    else
                        ang = -q;
                } else
                    ang = atan((float)(j - cy) / (i - cx));

                if (i < cx)
                    ang += PI;
                if (dist < YSIZE) {
                    dx = ceil(-sin(ang - p) * dist / 10.0);
                    dy = ceil(cos(ang - p) * dist / 10.0);
                } else {
#if 0
		    dx = ceil(-sin(ang+q)*YSIZE/20.0);
		    dy = ceil(cos(ang+q)*YSIZE/20.0);
#endif
                    if (i < cx)
                        dx = 3;
                    else
                        dx = -3;
                    dy = 0;
                }

                if (i == 0 || i == XSIZE) {
                    dx = cx - i;
                    dy = (float)(cy - j) * 0.75;
                }
            }

            theTab[i + j * XSIZE] = abs(i + dx + ((j + dy) * XSIZE)) % BSIZE;
        }
        /* printf(".");
           fflush(stdout);
           */
    }

    /*    printf("\nSaving...\n"); */

    for (j = 0; j < YSIZE; j++) {

        for (i = 0; i < XSIZE; i++)
            lineOut[i] = MS(theTab[i + j * XSIZE]);

        fwrite(lineOut, sizeof(tType) * XSIZE, 1, stdout);
    }
    return 0;
}
