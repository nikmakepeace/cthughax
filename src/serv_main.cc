#include "cthugha.h"
#include "Sound.h"
#include "AudioFrame.h"
#include "Interface.h"
#include "SoundServer.h"
#include "CDPlayer.h"
#include "options.h"
#include "imath.h"
#include "information.h"
#include "DisplayDevice.h"

#include <unistd.h>

#if HAVE_NCURSES_H
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

/*
 * Get a "line" of sound from the soundcard
 */
int serv_sound_read() {

    if (soundServer->nClients == 0) { /* if no client connected, */
        exit_sound(); /* we don't need the sound device */
        return 0;
    }

    init_sound(); /* make sure we have the sound dev */

    audioFrameTick();

    return 0;
}

void deleter() {
    delete soundServer;
    delete soundDevice;
    delete cdPlayer;
}

int main(int argc, char* argv[]) {
    srand(time(0)); /* initialize random generator */

    if (get_pre_params(argc, argv)) // handle some special arguments (verbose, ...)
        return 1;

    if (get_params(argc, argv)) /* parse cmd-line and read ini-files*/
        return 1;

    title(); /* Display titlemessage */

    init_imath();

    atexit(deleter);

    ncurses_use = 1;
    DisplayDevice::text_on_term = 1;
    init_ncurses();
    atexit(exit_ncurses);

    CTH_INFO("Initializing the sound device...\n");
    SoundDevice::newSD();

    CTH_INFO("Initializing the sound server...\n");
    soundServer = new SoundServer;

    CTH_INFO("Initializing CD player...\n");
    cdPlayer = new CDPlayer;

    CTH_INFO("Initializing keymaps...\n");
    Keymap::init();

    Interface::set("server");
    displayDevice = new DisplayDevice();
    do {
        serv_sound_read();
        (*soundServer)();
        Interface::current->run();

        displayDevice->prePrint();

        CTH_INFO("Displaying current interface...\n");
        Interface::current->display(); // print the text of the current interface

        errors.display(); // and the error messages

        displayDevice->postPrint();

        (*cdPlayer)();

        if (int(srv_wait_time))
            usleep(int(srv_wait_time) * 1000);
    } while (cthugha_close == 0);

    return 0;
}
