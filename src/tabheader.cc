#define CONST_BUFF
#include "cth_buffer.h"
#include "tab_header.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * This program writes a header to old-style tab-files.
 * The output is given to stdoutput.
 *
 + usage:
 *   tabheader [<tab-file>] [size-x  size-y]  [description]
 * if no parameter is given, stdin is used for input.
 */

int main(int argc, char* argv[]) {
    FILE* file;
    tab_header header;
    char c[5000];
    int i;

    if (argc < 2) { /* no file-name given, use stdinput */
        file = stdin;
    } else
        /* open tab-file */
        if ((file = fopen(argv[1], "r")) == NULL) {
            fprintf(stderr, "Can not open %s\n", argv[1]);
            return 1;
        }

    /* check if already new-style */
    if (fread(&header, sizeof(tab_header), 1, file) != 1) {
        fprintf(stderr, "Can't read header.\n");
        fclose(file);
        return 1;
    }
    if (tab_header_has_id(&header)) {
        fprintf(stderr, "This file contains a tab-id. Replacing old.\n");
    } else {
        rewind(file);
    }

    /* write new header */
    tab_header_set_id(&header);
    header.size_x = BUFF_WIDTH;
    header.size_y = BUFF_HEIGHT;
    header.description[0] = '\0'; /* init descr. */
    if (argc >= 4) { /* size-information present */
        header.size_x = atoi(argv[2]);
        header.size_y = atoi(argv[3]);
        if (argc >= 5) { /* and description present */
            strncpy(header.description, argv[4], 40);
        }
    }
    if (argc == 3) { /* only description */
        strncpy(header.description, argv[2], 40);
    }

    fwrite(&header, sizeof(header), 1, stdout);

    /* copy data */
    while (!feof(file)) {
        i = fread(c, 1, 5000, file);
        fwrite(c, 1, i, stdout);
    }

    /* close (if not stdin) and exit */
    if (argc > 1)
        fclose(file);
    return 0;
}
