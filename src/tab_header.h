#ifndef __TAB_HEADER_H
#define __TAB_HEADER_H

#include <stdint.h>
#include <string.h>

/*
 * This is the header of a translate table
 */

typedef struct { /* header for tab-files */
    char id[4]; /* "HDKB" to identify file */
    char description[40]; /* asciiz */
    int16_t size_x;
    int16_t size_y;
} tab_header;

static inline int tab_header_has_id(const tab_header* header) {
    return memcmp(header->id, "HDKB", 4) == 0;
}

static inline void tab_header_set_id(tab_header* header) {
    memcpy(header->id, "HDKB", 4);
}

#endif
