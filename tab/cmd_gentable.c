/*
 * cmd_gentable.c
 *
 * Generate a translation table -> write table to stdout.
 * based on mkgentable.
 */
/*
  Changes by Harald Deischinger to compile with cthugha-L under Linux.
  Changes:
      int -> short
      removed headerfiles dos.h, io.h, conio.h
      cosl -> cos
      sin -> sin

      short -> tint
      unsigned tint -> utint

      const buff

      parameter for nr_spirals
      some changes for parameters
*/
/*
//
// Cthugha - Audio Seeded Image Processing
//
// Zaph, Digital Aasvogel Group, Torps Productions 1993-1994
//


// This is an example program of how to generate rotating tables.
//
// Feel free to change the code, make your own, and send me the
// resulting .TAB file for inclusion in the next version!!
//
// For more info, check CTHUGHA.DOC (and hope I got around to writing
// that section!
//
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "../src/cthugha.h"
#include "../src/cth_buffer.h"
#include "../src/imath.h"

int BUFF_WIDTH;
int BUFF_HEIGHT;

unsigned int map;

#define M_PI 3.14159265358979323846
#define RADEG (180.0 / M_PI)

char maptabfile[255];

#define MAX_NR_SPIRALS 64
int centersX[MAX_NR_SPIRALS];
int centersY[MAX_NR_SPIRALS];
int dir[MAX_NR_SPIRALS];

int main(int argc, char** argv) {
    int x, y, map_x, map_y, i;
    double dist;
    double polar_r, polar_a;
    double delta_r = 2.0, delta_a = 0.1;
    double temp_y, temp_x;
    double cent_y, cent_x;
    double mapped_x, mapped_y, total_weight;
    long l;
    int nr_spirals = 1;
    int yinyang = 0;
    double yywidth = 10.0;

    time(&l);
    srand(l);

    argc--;
    argv++;
    if (argc > 1) {
        BUFF_WIDTH = atoi(argv[0]);
        BUFF_HEIGHT = atoi(argv[1]);
        argc -= 2;
        argv += 2;
    } else {

        fprintf(stderr,
            "Gentable usage:\n"
            "\tgentable tabname.tab WIDTH HEIGHT [-]<#spirals> <yywidth> <(float)delta_r> "
            "<(float)delta_a>\n"
            "\tIf the first parameter starts with a '-' then the direction of rotation\n"
            "\tchanges with the radius.\n"
            "\t#spirals: number of spirals (0 for one centered spiral) (def: %d)\n"
            "\tyywidth:  Width of section of constant direction (if changing dir.)\n"
            "\tdelta_r:  change of radius (0 -> simple rotation) (def: %f)\n"
            "\tdelta_a:  change of angle (def: %f)\n",
            nr_spirals, delta_r, delta_a);
        exit(1);
    }

    /*
        printf("%s %s\n", *(argv+1), *(argv+2));
    */

    if (argc) {
        yinyang = (*argv[0] == '-') ? 1 : 0;

        nr_spirals = atoi((*argv) + yinyang);
        if (nr_spirals == 0) {
            centersX[0] = BUFF_WIDTH / 2;
            centersY[0] = BUFF_HEIGHT / 2;
            dir[0] = 1;
            nr_spirals = 1;
        } else {
            nr_spirals = max(min(nr_spirals, MAX_NR_SPIRALS), 1);
            for (i = 0; i < nr_spirals; i++) {
                centersX[i] = rand() % BUFF_WIDTH;
                centersY[i] = rand() % BUFF_HEIGHT;
                dir[i] = rand() % 5 - 2;
            }
        }
    }
    if (yinyang) {
        argc--;
        argv++;
        if (argc) {
            yywidth = atof(*argv);
        }
    }

    argc--;
    argv++;
    if (argc) {
        delta_r = atof(*argv);
    }
    argc--;
    argv++;
    if (argc) {
        delta_a = atof(*argv);
    }

    /*
      if( yinyang)
      printf("Writing mapping table: %s\n"
      "  #spirals: %d  yywidth: %f  delta_a: %f  delta_r: %f\n" ,
      maptabfile, nr_spirals, yywidth, delta_a, delta_r);
      else
      printf("Writing mapping table: %s\n"
      "  #spirals: %d  delta_a: %f  delta_r: %f\n" ,
      maptabfile, nr_spirals, delta_a, delta_r);
      */

    for (y = 0; y < BUFF_HEIGHT; y++) {

        for (x = 0; x < BUFF_WIDTH; x++) {
            mapped_x = 0.0;
            mapped_y = 0.0;
            total_weight = 0.0;

            for (i = 0; i < nr_spirals; i++) {
                double weight;
                double source_x;
                double source_y;

                cent_y = centersY[i];
                cent_x = centersX[i];
                temp_x = fabs(x - cent_x);
                temp_y = fabs(y - cent_y);

                polar_r = sqrt(temp_x * temp_x + temp_y * temp_y);
                polar_a = atan2((double)(x - cent_x), (double)(y - cent_y));

                polar_r += (delta_r + (rand() % 10) * 0.01) * (double)dir[i];

                if (polar_r < 0)
                    polar_r = 0.0;

                if (yinyang) {

                    polar_a -= delta_a * 3.0 * (float)(5 - (int)(polar_r / 11) % 11) / 5.0;

                    if (((int)(polar_r / yywidth) % 2)) {
                        polar_a += delta_a;
                    } else {
                        polar_a -= delta_a;
                    }
                } else {

                    polar_a += (delta_a + (rand() % 10) * 0.01) * (double)dir[i];
                }

                source_y = polar_r * cos(polar_a) + cent_y;
                source_x = polar_r * sin(polar_a) + cent_x;

                dist = sqrt(((double)x - cent_x) * ((double)x - cent_x)
                    + ((double)y - cent_y) * ((double)y - cent_y));
                weight = 1.0 / (dist * dist + 1.0);

                mapped_x += weight * source_x;
                mapped_y += weight * source_y;
                total_weight += weight;
            }

            map_x = (int)(mapped_x / total_weight);
            map_y = (int)(mapped_y / total_weight);

            if ((map_y >= BUFF_HEIGHT) || (map_y < 0) || (map_x >= BUFF_WIDTH) || (map_x < 0)) {
                map_x = 0;
                map_y = 0;
            }

            map_x = max(map_x, 0);
            map_y = max(map_y, 0);

            map = map_y * BUFF_WIDTH + map_x;
            fwrite(&map, sizeof(int), 1, stdout);
        }
    }
    return 0;
}
