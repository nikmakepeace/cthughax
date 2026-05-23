/*
 * cmd_smoke.c
 *
 * Generate a translation table -> write table to stdout.
 * based on mksmoke
 */
/*
   mksmoke.c

   Changes by Harald Deischinger to compile with cthugha-L under Linux.
   Changes:
       int -> short
       Random
       short -> tint
       unsigned tint -> utint

       const buff

       removed unused variables dx, dy
*/

/*
//
// Create a smoke translation table for Cthugha.
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

int BUFF_WIDTH;
int BUFF_HEIGHT;

#define Y_CENTER (BUFF_HEIGHT / 2)
#define X_CENTER (BUFF_WIDTH / 2)

#define DEFAULT_SPEED 100
#define DEFAULT_RAND 70

int main(int argc, char** argv) {
    int x, y, map_x, map_y;
    int speed, Randomness;
    uint32_t map;

    if (argc < 3 || argc > 5) {
        char* p = strrchr(argv[0], '\\');
        p = p ? p + 1 : argv[0];
        fprintf(stderr, "Smoke table generator for CTHUGHA.");
        fprintf(stderr, "The generated table creates an effect of smoke rising from the burning");
        fprintf(stderr, "wave forms.");
        fprintf(stderr, " ");
        fprintf(stderr, "Usage:  %s WIDTH HEIGHT [speed [Randomness]]", p);
        fprintf(stderr, "  Speed should be 30..300 (default %d)", DEFAULT_SPEED);
        fprintf(stderr, "  Randomness should be 0..100 (default %d)", DEFAULT_RAND);
        return 1;
    }

    BUFF_WIDTH = atoi(argv[1]);
    BUFF_HEIGHT = atoi(argv[2]);

    speed = (argc > 3) ? atoi(argv[3]) : DEFAULT_SPEED;
    Randomness = (argc > 4) ? atoi(argv[4]) : DEFAULT_RAND;

    speed = min(max(speed, 30), 300);
    Randomness = min(max(Randomness, 0), 100);

    /*    printf ("Generating table %s with speed=%d and Randomness=%d\n",
                fileName, speed, Randomness);
                */

    /* now really generate */
    for (y = 0; y < BUFF_HEIGHT; y++) {

        for (x = 0; x < BUFF_WIDTH; x++) {
            map_x = x - (5 + Random(12 * Randomness / 100)) * speed / 100;
            map_y = y - (5 + Random(12 * Randomness / 100)) * speed / 100;

            if (map_y >= BUFF_HEIGHT || map_y < 0 || map_x >= BUFF_WIDTH || map_x < 0) {
                map_x = 0;
                map_y = 0;
            }

            map = map_y * BUFF_WIDTH + map_x;

            if (fwrite(&map, sizeof(uint32_t), 1, stdout) != 1) {
                fprintf(stderr, "\n*** Error while writing to output\n");
                return 3;
            }
        }
    }

    return 0;
}
