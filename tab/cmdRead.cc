//
// just read an .tab file
//
// do necessary stretching
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../src/cthugha.h"
#include "../src/cth_buffer.h"
#include "../src/imath.h"

#include "../src/tab_header.h"

int BUFF_WIDTH = 0;
int BUFF_HEIGHT = 0;

int readLine(uint32_t* line, int n, int big, FILE* in) {

    union data {
        uint32_t* l;
        uint16_t* s;
    } D;

    int size = big ? sizeof(uint32_t) : sizeof(uint16_t);

    uint32_t tmp[n];
    D.l = tmp;

    /* read data */
    if (fread(tmp, size, n, in) < (size_t)n) {
        fprintf(stderr, "cmdRead: Reading failed.\n");
        return 1;
    }
    for (int i = 0; i < n; i++) {
        line[i] = (big) ? D.l[i] : D.s[i];
    }

    return 0;
}

int main(int argc, char* argv[]) {
    int stretch = 0;
    FILE* in = stdin;
    FILE* out = stdout;
    int pipe = 0;

    if (argc < 3) {
        fprintf(stderr,
            "usage: cmdRead WIDTH HEIGHT [-s] [in] [out]\n"
            "       -s  allow stretching\n"
            "       in  .tab file (can be compressed)\n"
            "       out converted data\n");
        return 1;
    }

    BUFF_WIDTH = atoi(argv[1]);
    BUFF_HEIGHT = atoi(argv[2]);
    int bufferSize = BUFF_WIDTH * BUFF_HEIGHT;

    if (argc > 3) {
        if (strcmp(argv[3], "-s") == 0) {
            stretch = 1;

            argv++;
            argc--;
        }
    }
    if (argc > 3) {
        if (strcmp(argv[3], "-") == 0)
            in = stdin;
        else {
            if (strstr(argv[3], ".gz") != NULL) {
                char cmd[PATH_MAX];
                sprintf(cmd, "gzip -cd \"%s\"", argv[3]);
                in = popen(cmd, "r");
                pipe = 1;
            } else
                in = fopen(argv[3], "r");
        }
    }
    if (argc > 4) {
        out = (strcmp(argv[3], "-") == 0) ? stdout : fopen(argv[4], "r");
    }

    if (in == NULL) {
        fprintf(stderr, "cmdRead: can not open input file '%s'.\n", argv[3]);
        return 1;
    }
    if (out == NULL) {
        fprintf(stderr, "cmdRead: can not open output file.\n");
        return 1;
    }

    //
    // try to read a header
    //
    tab_header header;
    if (fread(&header, sizeof(tab_header), 1, in) != 1) {
        fprintf(stderr, "cmdRead: Can't read header.\n");
        return 1;
    }

    /* check header ID */
    if (!tab_header_has_id(&header)) {
        fprintf(stderr, "cmdRead: Old style .tab file without header.\n");

        rewind(in); // back to start of file

        /* fill in header */
        header.size_x = BUFF_WIDTH;
        header.size_y = BUFF_HEIGHT;
    }

    int big = (header.size_x * header.size_y) > 65535;

    //
    // check if stretching is necessary
    //
    if ((header.size_x == BUFF_WIDTH) && (header.size_y == BUFF_HEIGHT)) {
        // size matches

        uint32_t line[BUFF_WIDTH];

        for (int i = 0; i < BUFF_HEIGHT; i++) {
            if (readLine(line, BUFF_WIDTH, big, in))
                return 1;
            if (fwrite(line, sizeof(uint32_t), BUFF_WIDTH, out) != (size_t)BUFF_WIDTH) {
                fprintf(stderr, "cmdRead: writing failed.\n");
                return 1;
            }
        }

        int dummy;
        if (fread(&dummy, sizeof(int), 1, in) > 0) {
            fprintf(stderr, "cmdRead: too much data.\n");
            return 0;
        }

    } else if (stretch) {
        // size differs, but stretching is allowed

        uint32_t buff[header.size_x * header.size_y];
        if (readLine(buff, header.size_x * header.size_y, big, in))
            return 1;

        double xs = (double)header.size_x / BUFF_WIDTH;
        double ys = (double)header.size_y / BUFF_HEIGHT;

        for (int j = 0; j < BUFF_HEIGHT; j++) {

            int y = int((double)j * ys);
            if (y >= header.size_y)
                y = header.size_y - 1;

            uint32_t line[BUFF_WIDTH];
            for (int i = 0; i < BUFF_WIDTH; i++) {

                int x = int((double)i * xs);
                if (x >= header.size_x)
                    x = header.size_x - 1;

                uint32_t tp = buff[x + y * header.size_x];
                int ox = tp % header.size_x;
                int oy = tp / header.size_x;
                int dx = int((double)(ox - x) / xs);
                int dy = int((double)(oy - y) / ys);

                line[i] = abs(i + dx + (j + dy) * BUFF_WIDTH) % bufferSize;
            }
            if (fwrite(line, sizeof(uint32_t), BUFF_WIDTH, out) != (size_t)BUFF_WIDTH) {
                fprintf(stderr, "cmdRead: writing failed.\n");
                return 1;
            }
        }

    } else {
        fprintf(stderr, "cmdRead: Wrong size. stretching not allowed.\n");
        return 1;
    }

    if (pipe)
        pclose(in);

    return 0;
}
