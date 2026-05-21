#include "cthugha.h"
#include "keys.h"
#include "display.h"

#ifdef CTH_XWIN
#include "xcthugha.h"
#endif

#include <signal.h>

int key_esc = 1; /* disable/enable ESC-key. When enable it
                    sometimes happens that when pressing
                    functions keys or cursor keys cthugha
                    only get the leading ESC and quits. */
int x11_key = CK_NONE;

// to handle keys, that give shifted and normal the same result
static int shiftMap[][2] = {
    { '0', CK_SHIFT(0) },
    { '1', CK_SHIFT(1) },
    { '2', CK_SHIFT(2) },
    { '3', CK_SHIFT(3) },
    { '4', CK_SHIFT(4) },
    { '5', CK_SHIFT(5) },
    { '6', CK_SHIFT(6) },
    { '7', CK_SHIFT(7) },
    { '8', CK_SHIFT(8) },
    { '9', CK_SHIFT(9) },
};
static int nShiftMap = sizeof(shiftMap) / sizeof(int[2]);

int shift(int key, int shift) {
    if (shift) {
        for (int i = 0; i < nShiftMap; i++)
            if (key == shiftMap[i][0]) {
                return shiftMap[i][1];
            }
    }

    return key;
}

KeyAssoc keyAssoc[] = {
    { "F10", CK_FKT(10) },
    { "F11", CK_FKT(11) },
    { "F12", CK_FKT(12) },
    { "F13", CK_FKT(13) },
    { "F14", CK_FKT(14) },
    { "F15", CK_FKT(15) },
    { "F16", CK_FKT(16) },
    { "F17", CK_FKT(17) },
    { "F18", CK_FKT(18) },
    { "F19", CK_FKT(19) },
    { "F20", CK_FKT(20) },
    { "F21", CK_FKT(21) },
    { "F22", CK_FKT(22) },
    { "F23", CK_FKT(23) },
    { "F24", CK_FKT(24) },
    { "F1", CK_FKT(1) },
    { "F2", CK_FKT(2) },
    { "F3", CK_FKT(3) },
    { "F4", CK_FKT(4) },
    { "F5", CK_FKT(5) },
    { "F6", CK_FKT(6) },
    { "F7", CK_FKT(7) },
    { "F8", CK_FKT(8) },
    { "F9", CK_FKT(9) },

    { "Return", CK_ENTER },
    { "Up", CK_UP },
    { "Down", CK_DOWN },
    { "Left", CK_LEFT },
    { "Right", CK_RIGHT },
    { "Prior", CK_PGUP },
    { "Next", CK_PGDN },
    { "End", CK_END },
    { "Home", CK_HOME },
    { "Print", CK_PRINT },
    { "BackSpace", CK_BACK },
    { "Delete", CK_DELETE },

    { "KP_0", '0' },
    { "KP_1", '1' },
    { "KP_2", '2' },
    { "KP_3", '3' },
    { "KP_4", '4' },
    { "KP_5", '5' },
    { "KP_6", '6' },
    { "KP_7", '7' },
    { "KP_8", '8' },
    { "KP_9", '9' },

    { "S-0", CK_SHIFT(0) }, // these are not X11 keys, but are only for the keymap
    { "S-1", CK_SHIFT(1) },
    { "S-2", CK_SHIFT(2) },
    { "S-3", CK_SHIFT(3) },
    { "S-4", CK_SHIFT(4) },
    { "S-5", CK_SHIFT(5) },
    { "S-6", CK_SHIFT(6) },
    { "S-7", CK_SHIFT(7) },
    { "S-8", CK_SHIFT(8) },
    { "S-9", CK_SHIFT(9) },

};
int nKeyAssoc = sizeof(keyAssoc) / sizeof(KeyAssoc);

#ifdef CTH_XWIN

/*
 * Handler for key-board
 */
void keys_x11(char* input, int state) {

    if (input[1] == '\0')
        switch (input[0]) {
        case 0:
        case -1:
            x11_key = CK_NONE;
            break;
        case 27:
            x11_key = (key_esc ? CK_ESC : CK_NONE);
            break;
        case 10:
        case 13:
            x11_key = CK_ENTER;
            break;
        case 8:
            x11_key = CK_BACK;
            break;
        default:
            x11_key = input[0];
        }
    else {
        int i;
        for (i = 0; i < nKeyAssoc; i++)
            if (strcasecmp(input, keyAssoc[i].name) == 0) {
                x11_key = keyAssoc[i].keyValue;
                return;
            }
        x11_key = CK_NONE;
    }

    x11_key = shift(x11_key, state & ShiftMask);
}

int getkey_x11() {
    int key;
    key = x11_key;
    x11_key = CK_NONE;
    return key;
}

#endif /* CTH_XWIN */

#if HAVE_NCURSES == 1

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#if HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#else
#if HAVE_CURSES_H
#include <curses.h>
#else
#if HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#endif
#endif
#endif
#endif

int translate_key(int key) {

    switch (key) {
    case 0:
    case -1:
        return CK_NONE;
    case 27:
        return (key_esc ? CK_ESC : CK_NONE);
    case KEY_F(1):
        return CK_FKT(1);
    case KEY_F(2):
        return CK_FKT(2);
    case KEY_F(3):
        return CK_FKT(3);
    case KEY_F(4):
        return CK_FKT(4);
    case KEY_F(5):
        return CK_FKT(5);
    case KEY_F(6):
        return CK_FKT(6);
    case KEY_F(7):
        return CK_FKT(7);
    case KEY_F(8):
        return CK_FKT(8);
    case KEY_F(9):
        return CK_FKT(9);
    case KEY_F(10):
        return CK_FKT(10);
    case KEY_F(11):
        return CK_FKT(11);
    case KEY_F(12):
        return CK_FKT(12);
    case KEY_F(13):
        return CK_FKT(13);
    case KEY_F(14):
        return CK_FKT(14);
    case KEY_F(15):
        return CK_FKT(15);
    case KEY_F(16):
        return CK_FKT(16);
    case KEY_F(17):
        return CK_FKT(17);
    case KEY_F(18):
        return CK_FKT(18);
    case KEY_F(19):
        return CK_FKT(19);
    case KEY_F(20):
        return CK_FKT(20);

    case KEY_UP:
        return CK_UP;
    case KEY_DOWN:
        return CK_DOWN;
    case KEY_PPAGE:
        return CK_PGUP;
    case KEY_NPAGE:
        return CK_PGDN;
    case KEY_HOME:
        return CK_HOME;
    case KEY_END:
        return CK_END;
    case KEY_RIGHT:
        return CK_RIGHT;
    case KEY_LEFT:
        return CK_LEFT;

    case KEY_PRINT:
        return CK_PRINT;

    case KEY_DC:
        return CK_DELETE;

    case 8:
    case KEY_BACKSPACE:
        return CK_BACK;

    case 10:
    case 13:
        return CK_ENTER;

    default:
        return key;
    }
}

/* Now I'm using ncurses to read from the keyboard
   In version 0.1 I used vga_getkey, but getch is much better.
   There is a 1 sec. delay when only ESC is pressed.
   */

int getkey_ncurs() {
    static int next_key = CK_NONE;
    int key;

    key = next_key;
    next_key = getch();

    if ((key == 27) && (next_key > 0)) {
        /* seems like an unrecognized special key. skip everything still
         waiting */
        while ((next_key = getch()) > 0)
            ;

        return CK_OTHER;
    }

    if (key == KEY_SUSPEND) { /* suspend (^Z) */
        raise(SIGTSTP);
        return CK_NONE;
    }

    return shift(translate_key(key), 0);
}

#endif

int getkey() {

#ifdef CTH_XWIN
    // first get the X key
    int k = getkey_x11();
    if (k != CK_NONE)
        return k;
#endif

#ifdef CTH_GL
    extern int GLkey;
    if (GLkey != CK_NONE) {
        int k = GLkey;
        GLkey = CK_NONE;
        return k;
    }
#endif

#if HAVE_NCURSES == 1
    // OK, now check ncurses, if it is in use
    if (ncurses_use)
        return getkey_ncurs();
    else
        return CK_NONE;
#endif
}
