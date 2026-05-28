#include "cthugha.h"
#include "waves.h"

// TODO: This should be change to OptionList

#define MAX_TABLES 10 /* Max. nr. of tables */

pal_table tables[MAX_TABLES]; /* Palette-usage-tables */
int nr_tables = 0;

int init_tables() {
    int i;

    /* Built in tables */
    for (i = 0; i < 256; i++) {
        tables[0][i] = abs(128 - i) * 2;
        tables[1][i] = 255 - abs(128 - i) * 2;
        tables[2][i] = i;
        tables[3][i] = 255 - i;
        tables[4][i] = abs(128 - i) + 127;
        tables[5][i] = 255 - abs(128 - i) + 127;
        tables[6][i] = abs(i - 128) + 127;
        tables[7][i] = (abs(128 - i) < 64) ? 255 : abs(128 - i) * 4;
        tables[8][i] = abs(128 - i);
        tables[9][i] = 255 - abs(128 - i);
    }

    /* Tables from File */
    /* TODO */
    return 0;
}
