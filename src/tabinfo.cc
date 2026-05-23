#include "cth_buffer.h"
#include "tab_header.h"

#include <stdio.h>

/*
 * This program displays the header-information form tab-files
 */

int main(int argc, char* argv[]) {
    FILE* file;
    tab_header header;

    /* check command-line syntax */
    if (argc < 2) {
        printf("Syntax: tabinfo <filename>\n");
        return 1;
    }

    /* open tab-file */
    if ((file = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Can't open file %s\n", argv[1]);
        return 1;
    }

    /* check if new-style */
    if (fread(&header, sizeof(tab_header), 1, file) != 1) {
        fprintf(stderr, "Can't read header.\n");
        fclose(file);
        return 1;
    }
    /* check ID */
    if (!tab_header_has_id(&header)) {
        fprintf(stderr, "No new-style tab-file.\n");
        fclose(file);
        return 1;
    }
    /* print information */
    printf("file  : %s\n"
           "size-x: %d\n"
           "size-y: %d\n"
           "descr.: %s\n",
        argv[1], header.size_x, header.size_y, header.description);
    /* clean up */
    fclose(file);
    return 0;
}
