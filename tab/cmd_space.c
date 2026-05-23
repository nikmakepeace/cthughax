/*
 * cmd_space.c
 *
 * Generate a translation table -> write table to stdout.
 * based on mkspace.
 */

/*
   mkspace.c

   Changes by Harald Deischinger to compile with cthugha-L under Linux.
   Changes:
       int -> short
       random

       short -> tint
       unsigned tint -> utint

       const buff

       removed enum bool
*/

/*
// Create a "space flight" translation table for Cthugha.
// The effect is that of a fast forward motion. A small region at the center
// of the screen is Randomized to make the screen less empty and more
// tinteresting. A backward flight option is also available.
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
#include <math.h>
#include <stdint.h>

#include "../src/cthugha.h"
#include "../src/cth_buffer.h"
#include "../src/imath.h"

int BUFF_WIDTH;
int BUFF_HEIGHT;

#define Y_CENTER (BUFF_HEIGHT / 2)
#define X_CENTER (BUFF_WIDTH / 2)

#define DEFAULT_SPEED 100
#define DEFAULT_RAND 70

int main(int argc, char** argv) {
    int x, y, dx, dy, map_x, map_y;
    int speed, Randomness;
    int reverse;
    uint32_t map;

    if (argc < 3 || argc > 6) {
        char* p = strrchr(argv[0], '\\');
        p = p ? p + 1 : argv[0];
        fprintf(stderr, "\"Space flight\" table generator for CTHUGHA.\n");
        fprintf(stderr, "The generated table creates an effect of flying forward through a\n");
        fprintf(stderr, "3-D space.\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "Usage:  %s WIDTH HEIGHT [speed [Randomness ['r']]]\n", p);
        fprintf(stderr, "  Speed should be 30..300 (default %d)\n", DEFAULT_SPEED);
        fprintf(stderr, "  Randomness should be 0..100 (default %d)\n", DEFAULT_RAND);
        fprintf(stderr, "  Adding a fourth argument will create a backward movement.\n");
        return 1;
    }

    BUFF_WIDTH = atoi(argv[1]);
    BUFF_HEIGHT = atoi(argv[2]);
    speed = (argc > 3) ? atoi(argv[3]) : DEFAULT_SPEED;
    Randomness = (argc > 4) ? atoi(argv[4]) : DEFAULT_RAND;
    reverse = (argc > 5);

    speed = min(max(speed, 30), 300);
    Randomness = min(max(Randomness, 0), 100);

    /*    printf ("Generating table %s with %sspeed=%d and Randomness=%d\n",
                 fileName, reverse ? "reverse " : "", speed, Randomness); */

    for (y = 0; y < BUFF_HEIGHT; y++) {

        for (x = 0; x < BUFF_WIDTH; x++) {
            dx = x - X_CENTER;
            dy = y - Y_CENTER;

            if (!reverse && abs(dx) < 30 && abs(dy) < 20 && Random(abs(dx) + abs(dy)) < 4) {

                map_x = Random(BUFF_WIDTH);
                map_y = Random(BUFF_HEIGHT);
            } else {
                int speedFactor;
                long sp;

                if (Randomness == 0)
                    sp = speed;
                else {
                    speedFactor = Random(Randomness + 1) - Randomness / 3;
                    sp = speed * (100L + speedFactor) / 100L;
                }

                if (reverse)
                    sp = (-sp);

                map_x = (int)(x - (dx * sp) / 700);
                map_y = (int)(y - (dy * sp) / 700);
            }

            if (map_y >= BUFF_HEIGHT || map_y < 0 || map_x >= BUFF_WIDTH || map_x < 0) {
                map_x = 0;
                map_y = 0;
            }

            map = map_y * BUFF_WIDTH + map_x;

            if (fwrite(&map, sizeof(uint32_t), 1, stdout) != 1) {
                fprintf(stderr, "\n*** Error while writing to output file (disk full?)\n");
                return 3;
            }
        }
    }

    return 0;
}
