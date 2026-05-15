#include "cthugha.h"
#include "DisplayDevice.h"

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

int ncurses_use = 1;

/*
 * Initialization of ncurses
 */
int init_ncurses() {

    if (!initscr()) {
        CTH_ERROR("Can not initialize ncurses.\n");
        return 1;
    }

    timeout(0); /* no timeout on getch(); */
    keypad(stdscr, TRUE); /* allow function keys */
    meta(stdscr, TRUE); /* 8-bit clean */
    noecho(); /* don't print out keys */
    cbreak(); /* don't wait for cr */

    text_size.x = COLS;
    text_size.y = LINES;

    return 0;
}

/*
 * Cleanup of ncurses (makes cursor visible again)
 */
void exit_ncurses() {
    echo();
    endwin();
}
