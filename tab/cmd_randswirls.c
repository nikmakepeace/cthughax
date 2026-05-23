/* minor changes by Harald Deischinger:
   - un-fucked up Little Endian
   - use BUFF_WIDTH and BUFF_HEIGHT
   - filename as parameter
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

typedef struct {
    int x, y;
} ptType;

#ifdef MAC

// these are for reading fucked up Little Endian files from fucked up Little Endian machines
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

#define XP(d, l) ((d) * llen + (l))

int BUFF_WIDTH;
int BUFF_HEIGHT;

#define BSIZE (BUFF_WIDTH * BUFF_HEIGHT)

int main(int argc, char* argv[]) {
    int i, j, k, dx, dy, size, dist, q;
    ptType* nline;
    int *theTab, llen, cl, x, y, u, p, ox, oy, m, n, a, b;
    uint32_t lineOut[65536];
    double ang, cx, cy, s, c, xf, yf, stepInc;

    srand(time(0));

    if (argc < 3) {
        fprintf(stderr, "Missing arguments...\n");
        return 1;
    }

    BUFF_WIDTH = atoi(argv[1]);
    BUFF_HEIGHT = atoi(argv[2]);

    /* for scaling to the appropriate buffer size */
    stepInc = (double)BUFF_WIDTH / 2500;

    theTab = malloc(sizeof(int) * BSIZE);
    if (!theTab) {
        fprintf(stderr, "Not enough memory!\n");
        exit(1);
    }

    llen = (BUFF_WIDTH < BUFF_HEIGHT ? BUFF_WIDTH : BUFF_HEIGHT) * 0.35;
    cl = llen / 2;

    dist = 8 / stepInc;

    nline = malloc(sizeof(ptType) * llen * dist);

    if (!nline) {
        fprintf(stderr, "Not enough memory!\n");
        exit(1);
    }

    /* init the line buffers */
    for (j = 0; j < dist; j++)
        for (k = 0; k < llen; k++) {
            nline[XP(j, k)].x = 0;
            nline[XP(j, k)].y = k;
        }

    /* init the table */
    for (j = 0; j < BUFF_HEIGHT; j++)
        for (i = 0; i < BUFF_WIDTH; i++)
            theTab[i + j * BUFF_WIDTH] = abs(i - 3 + (j * BUFF_WIDTH)) % BSIZE;

    cx = 0;
    cy = cl;
    ang = 0;
    p = rand() % 20;
    q = 0;
    a = 0;
    for (k = 0; k < 100000; k++) {

        /* movement rate */
#if 0
	if (rand() % 4) {
	    u = rand() % 3;
	    if (u == 0)
		dist++;
	    else if (u == 1);
	    dist--;
	}
#endif
        /* angle rate of change */
        if ((rand() % 50) == 1)
            p = (rand() % 20) - 10;

        ang += (float)p / 360 * stepInc;

        s = sin(ang) * stepInc;
        c = cos(ang) * stepInc;
        cx += s;
        cy += c;

        for (i = 0; i < llen; i++) {

            x = nline[XP(a % dist, i)].x = (int)(cx + (cl - i) * c) % BUFF_WIDTH;
            y = nline[XP(a % dist, i)].y = (int)(cy + (cl - i) * s) % BUFF_HEIGHT;

            ox = nline[XP((a + 1) % dist, i)].x;
            oy = nline[XP((a + 1) % dist, i)].y;

            dx = ox - x;
            dy = oy - y;

            theTab[abs(x + y * BUFF_WIDTH) % BSIZE] = abs(x + dx + (y + dy) * BUFF_WIDTH) % BSIZE;

            a++;
        }
    }

    for (j = 0; j < BUFF_HEIGHT; j++) {

        for (i = 0; i < BUFF_WIDTH; i++) {

            if (j == 0 || j == BUFF_HEIGHT) {

                dx = (float)(cx - i) * 0.75;
                dy = cy - j;
                theTab[i + j * BUFF_WIDTH] = abs(i + dx + ((j + dy) * BUFF_WIDTH)) % BSIZE;

            } else {

                if (i == 0 || i == BUFF_WIDTH) {
                    dx = cx - i;
                    dy = (float)(cy - j) * 0.75;
                    theTab[i + j * BUFF_WIDTH] = abs(i + dx + ((j + dy) * BUFF_WIDTH)) % BSIZE;
                }
            }
        }
    }

    /*	printf("\nSaving...\n"); */

    for (j = 0; j < BUFF_HEIGHT; j++) {

        for (i = 0; i < BUFF_WIDTH; i++)
            lineOut[i] = theTab[i + j * BUFF_WIDTH];

        fwrite(lineOut, sizeof(uint32_t) * BUFF_WIDTH, 1, stdout);
    }
    return 0;
}
