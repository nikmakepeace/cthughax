/*
 * cmd_huricn.c
 *
 * Generate a translation table -> write table to stdout.
 * based on mkhuricn.
 */

/*
   mkhuricn.c

   Changes by Harald Deischinger to compile with cthugha-L under Linux.
   Changes:
       int -> short
       random -> Random

      short -> tint
      unsigned tint -> utint

      const buff

      removed enum
*/

/*
//
// Create a spiral (hurricane) translation table for Cthugha.
//
// Compiled using Borland C 3.1.
//
// By Ofer Faigon, Sep 1994.
//     ofer@brm.co.il
//     (or oferf@itexjct.jct.ac.il)
//
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../src/cthugha.h"
#include "../src/cth_buffer.h"
#include "../src/imath.h"

int BUFF_WIDTH = 0;
int BUFF_HEIGHT = 0;

/*
// Default center is somewhat off-center to get a more pleasent effect.
*/
#define DFLT_X_CENTER (BUFF_WIDTH / 3)
#define DFLT_Y_CENTER (BUFF_HEIGHT / 2 - 10)

#define DEFAULT_SPEED 100
#define DEFAULT_RAND 70

static int usageError(char* progPath) {
    char* p = strrchr(progPath, '\\');

    p = p ? p + 1 : progPath;

    fprintf(stderr, "Hurricane table generator for CTHUGHA.");
    fprintf(stderr, "The generated table looks like a sattelite view of a hurricane or");
    fprintf(stderr, "Jupiter's red spot.");
    fprintf(stderr, " ");
    fprintf(stderr, "Usage:  %s WIDTH HEIGHT [-s[x][y]] [-c#:#] [-r] [speed [Randomness]]", p);
    fprintf(stderr, "  Speed should be 30..300 (default %d)", DEFAULT_SPEED);
    fprintf(stderr, "  Randomness should be 0..100 (default %d)", DEFAULT_RAND);
    fprintf(stderr, "  -sx slows down horizontal movements");
    fprintf(stderr, "  -sy slows down vertical movements");
    fprintf(stderr, "  -sxy slows down all movements");
    fprintf(stderr, "      Default is -sy.");
    fprintf(stderr, "  -c#:# sets the center of the spiral. Should be between 0:0 and");
    fprintf(stderr, "      %d:%d (default -c%d:%d).", BUFF_WIDTH - 1, BUFF_HEIGHT - 1,
        DFLT_X_CENTER, DFLT_Y_CENTER);
    fprintf(stderr, "  -r reverses the direction of rotation.");
    return 1;
}

/*
 * Look for a given command-line switch and remove it from argc/argv if
 * found.  *val is set to point to the string that follows the switch
 * letter, or to NULL if not found.
 * Decrement argc if a switch was found and removed.
 * If the switch appears more than once, then all occurences are removed
 * but only the last is taken into account.
 */
static void eatSwitch(int* argc, char** argv, char switchLetter, char** val) {
    int i, j;

    *val = NULL;
    i = 1;
    while (i < *argc) {
        if (argv[i][0] == '-' && argv[i][1] == switchLetter) {
            *val = &argv[i][2];
            for (j = i + 1; j < *argc; j++)
                argv[j - 1] = argv[j];
            (*argc)--;
        } else
            i++;
    }
}

int main(int argc, char** argv) {
    char* p;
    int xCenter, yCenter;
    int x, y, dx, dy, map_x, map_y;
    int speed, Randomness;
    int slowX = 0;
    int slowY = 1;
    int reverse;
    uint32_t map;
    int i;

    /* get width and height */
    if (argc < 3) {
        return usageError(argv[0]);
    }
    BUFF_WIDTH = atoi(argv[1]);
    BUFF_HEIGHT = atoi(argv[2]);
    for (i = 3; i < argc; i++)
        argv[i - 2] = argv[i];
    argc -= 2;

    eatSwitch(&argc, argv, 's', &p);
    if (p) {
        slowX = (strchr(p, 'x') != NULL);
        slowY = (strchr(p, 'y') != NULL);
    }

    eatSwitch(&argc, argv, 'c', &p);
    xCenter = p ? min(max(atoi(p), 0), BUFF_WIDTH - 1) : DFLT_X_CENTER;
    if (p)
        p = strchr(p, ':');
    if (p)
        p++;
    yCenter = p ? min(max(atoi(p), 0), BUFF_HEIGHT - 1) : DFLT_Y_CENTER;

    eatSwitch(&argc, argv, 'r', &p);
    reverse = (p != NULL);

    if (argc > 3)
        return usageError(argv[0]);

    speed = (argc > 1) ? atoi(argv[1]) : DEFAULT_SPEED;
    Randomness = (argc > 2) ? atoi(argv[2]) : DEFAULT_RAND;

    speed = min(max(speed, 30), 300);
    Randomness = min(max(Randomness, 0), 100);

    /*
     * now really generate
     */
    for (y = 0; y < BUFF_HEIGHT; y++) {

        for (x = 0; x < BUFF_WIDTH; x++) {
            int speedFactor;
            long sp;

            if (Randomness == 0)
                sp = speed;
            else {
                speedFactor = Random(Randomness + 1) - Randomness / 3;
                sp = speed * (100L + speedFactor) / 100L;
            }

            dx = x - xCenter;
            dy = y - yCenter;

            if (slowX || slowY) {
                long dSquared = (long)dx * dx + (long)dy * dy + 1;

                if (slowY)
                    dx = (int)(dx * 2500L / dSquared);
                if (slowX)
                    dy = (int)(dy * 2500L / dSquared);
            }

            if (reverse)
                sp = (-sp);

            map_x = (int)(x + (dy * sp) / 700);
            map_y = (int)(y - (dx * sp) / 700);

            while (map_y < 0)
                map_y += BUFF_HEIGHT;
            while (map_x < 0)
                map_x += BUFF_WIDTH;
            map_y = map_y % BUFF_HEIGHT;
            map_x = map_x % BUFF_WIDTH;

            map = map_y * BUFF_WIDTH + map_x;

            if (fwrite(&map, sizeof(uint32_t), 1, stdout) != 1) {
                fprintf(stderr, "\n*** Error while writing to output\n");
                return 3;
            }
        }
    }

    return 0;
}
