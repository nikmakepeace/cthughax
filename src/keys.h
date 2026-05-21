/*
    CTHUGHA-L 								keys.h
*/

#ifndef __KEYS_H__
#define __KEYS_H__

extern int key_esc; /* disable/enable ESC-key */

int translate_key(int key);
int getkey(); /* get a key and return a code */

void keys_x11(char* input, int state = 0);

struct KeyAssoc { // keyboard association table
    const char* name;
    int keyValue;
};
extern KeyAssoc keyAssoc[];
extern int nKeyAssoc;

#define CK_BASE 65536
#define CK_FKT(x) (CK_BASE + (x))
#define CK_ENTER (CK_BASE + 25)

/* cursor movement */
#define CK_UP (CK_BASE + 26)
#define CK_DOWN (CK_BASE + 27)
#define CK_LEFT (CK_BASE + 28)
#define CK_RIGHT (CK_BASE + 29)
#define CK_PGUP (CK_BASE + 30)
#define CK_PGDN (CK_BASE + 31)
#define CK_HOME (CK_BASE + 32)
#define CK_END (CK_BASE + 33)

#define CK_ESC (CK_BASE + 34)
#define CK_NONE (CK_BASE + 35)

#define CK_PRINT (CK_BASE + 36)

#define CK_BACK (CK_BASE + 37)

#define CK_DELETE (CK_BASE + 38)

/* shifted number */
#define CK_SHIFT(x) (CK_BASE + 39 + x)

#define CK_OTHER (CK_BASE + 99)

#endif
