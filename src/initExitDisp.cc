#include "cthugha.h"
#include "information.h" /* title, credits, ... */
#include "display.h"
#include "Sound.h"
#include "translate.h"
#include "options.h"
#include "keys.h"
#include "imath.h"
#include "waves.h"
#include "Option.h"
#include "SoundAnalyze.h"
#include "AutoChanger.h"
#include "CthughaBuffer.h"
#include "SoundServer.h"
#include "CthughaDisplay.h"
#include "CDPlayer.h"
#include "DisplayDevice.h"
#include "Interface.h"
#include "keymap.h"

#include <unistd.h>
#include <signal.h>

/*
 * handle ^Z and continue
 */
void sig_tty_cont(int);
void sig_tty_stop(int) {
    CTH_INFO("Stopping...\n");

    signal(SIGCONT, sig_tty_cont); /* set, how to continue */

    cthugha_pause = 1; /* in interface we will really stop */
}
void sig_tty_cont(int) {
    CTH_INFO("Continuing...\n");

    init_sound(); /* and sound */

    signal(SIGTSTP, sig_tty_stop); /* set, how to stop again */

    raise(SIGCONT); /* default action */
}

//
// deleter of global objects
//
void deleter() {
    delete autoChanger; // this also saves options
    delete cthughaDisplay;
    delete soundServer;
    delete soundDevice;
    delete cdPlayer;
}

/*
 *
 */
int main(int argc, char* argv[]) {

    srand(time(0)); /* initialize random generator */
    seteuid(getuid()); // give up root privileges

    if (get_pre_params(argc, argv)) // handle some special arguments (verbose, ...)
        return 1;

    if (cth_init(&argc, argv)) /* special initialization */
        return 1;

    if (get_params(argc, argv)) /* parse cmd-line and read ini-files*/
        return 1;

    title(); /* Display titlemessage */

    init_imath();

    atexit(deleter);

    if (ncurses_use) {
        init_ncurses();
        atexit(exit_ncurses);
    }

    CTH_INFO("Initializing the sound device...\n");
    SoundDevice::newSD();

    CTH_INFO("Initializing the sound server...\n");
    soundServer = new SoundServer;

    CTH_INFO("Initializing CD player...\n");
    cdPlayer = new CDPlayer;

    CTH_INFO("Initializing Mixer device...\n");
    if (init_mixer())
        exit(0);

    CTH_INFO("Initializing cthugha Buffer...\n");
    CthughaBuffer::initAll();

    CTH_INFO("Initializing display...\n");
    newDisplayDevice();
    newCthughaDisplay();

    CTH_INFO("Setting initial core options...\n");
    CoreOption::changeToInitial();

    CTH_INFO("Initializing interface...\n");
    Interface::set("main");

    CTH_INFO("Initializing keymaps...\n");
    Keymap::init();

    CTH_INFO("Initializing the automatic changing...\n");
    autoChanger = new AutoChanger;

    signal(SIGTSTP, sig_tty_stop); /* react to ^Z */

    displayDevice->mainLoop();

    CTH_INFO("Exiting cthugha...\n");

    return 0;
}

// Ad hoc profiling of the main loop. 
// This is not very exact, but it gives a good impression of where the time is spent.
// To enable it, define PROF in the Makefile and recompile.
// 
// The timing is accumulated and output is printed every 25 frames
// It shows the time spent in each module (sound reading, sound analyzing, display, ...) 
// and the total time for 25 frames.


//#define PROF
#undef PROF

#ifdef PROF
#define P(a) a
#else
#define P(a)
#endif

void run(int doDisplay) {
#ifdef PROF
    double T[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif

    P(T[0] = getTime();)
    cthughaDisplay->nextFrame();

    P(T[1] = getTime();) (*soundDevice)();

    P(T[2] = getTime();)
    soundAnalyze();

    P(T[3] = getTime();) (*autoChanger)();

    P(T[4] = getTime();) (*soundServer)();

    P(T[5] = getTime();)
    CthughaBuffer::run();

    P(T[6] = getTime();)
    if (doDisplay)
        (*cthughaDisplay)();

    P(T[7] = getTime();) (*cdPlayer)();

    P(T[8] = getTime();)
    // this is here to be sure not to interrupt a graphics operation.
    if (cthugha_pause) {
        cthugha_pause = 0;

        exit_sound(); /* and sound */

        raise(SIGTSTP); /* default action */
    }

    P(T[9] = getTime();)

#ifdef PROF
    static double Ts[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < 8; i++)
        Ts[i] = Ts[i] + T[i + 1] - T[i];

    static int n = 24;
    n++;
    if (n == 25) {
        static double To = 0;
        double to = getTime();

        printf("%6.4f: ", to - To);
        To = to;

        for (int i = 0; i < 8; i++) {
            printf("%6.4f ", Ts[i]);
            Ts[i] = 0;
        }
        printf("\n");
        n = 0;
    }
#endif
}
