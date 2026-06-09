#include "cthugha.h"
#include "keys.h"
#include "Interface.h"
#include "InterfaceRuntime.h"
#include "display.h"
#include "OverlaySource.h"
#include "RuntimeCommandSink.h"
#include "keymap.h"

class InterfaceCredits : public Interface {
    static const char* credits[];
    static int nCredits;

public:
    InterfaceCredits()
        : Interface("credits", NULL, NULL) { }

    virtual void doKey(InterfaceRuntime& runtime,
        KeymapRegistry& /* keymaps */, CommandRegistry& /* commands */,
        CommandDispatcher& /* dispatcher */, CommandContext& context,
        int /* key */) {
        (void)runtime;
        RuntimeCommandSink* sink = context.runtimeCommandSink();
        if (sink != NULL)
            sink->apply(RuntimeCommand::requestClose());
    }

    virtual void display(InterfaceRuntime& runtime,
        OverlayRenderContext& overlay) {

        const int currentTime = runtime.milliseconds();
        const double pos = runtime.updateCreditsPosition(currentTime,
            overlay.textRows());

        for (int i = 1; i < overlay.textRows(); i++) {
            int L = (int(pos) + i) % nCredits;
            if (L < 0)
                continue;
            overlay.printText(credits[L] + 1, -pos + int(pos) + i, 'c',
                credits[L][0]);
        }
    }
};

#define N "\000"
#define H "\001"
#define E "\002"

const char* InterfaceCredits::credits[] = { E
    "--------------------------------------------------------------------------------",
    H "C T H U G H A - L  " VERSION,
    E "--------------------------------------------------------------------------------",
    N "                                        ", H "               mail                     ",
    N "         Harald Deischinger             ", N "         Am Edhuegel 45                 ",
    N "         4115  Kleinzell                ", N "         AUSTRIA                        ",
    N "                                        ", H "               email                    ",
    N "         deischi@geocities.com          ", N "                                        ",
    H "                WWW                     ", N "www.geocities.com/CapeCanaveral/Lab/6386",
    N "                                        ",
    E "--------------------------------------------------------------------------------",
    N "                                        ", N "Original Program (CTHUGHA V5.1,V5.3)    ",
    N "coded by - Torps Productions:           ", N "    The Digital Aasvogel Group - 1995   ",
    N "Original Idea & Code:                   ", N " Kevin Burfitt <zaph@torps.apana.org.au>",
    N "                                        ",
    E "--------------------------------------------------------------------------------",
    N "                           ", H "      Cthugha WWW-Page     ",
    E "      ----------------     ", N "http://www.afn.org/~cthugha",
    N "                           ", H "     Cthugha newsgroup     ",
    E "     -----------------     ", N "   alt.graphics.cthugha    ",
    N "                           ",
    E "--------------------------------------------------------------------------------",
    N "                                   ", H "          Want to register?        ",
    N "                                   ", H "          for personal use         ",
    E "          ----------------         ", N "         send me a Postcard        ",
    N " or a CD if you have too much money", N "                                   ",
    H "         for commercial use        ", E "         ------------------        ",
    N "    register for US$50 or 2 CDs    ", N "                                   ",
    H "           for magazines           ", E "           -------------           ",
    N " please send a copy of your review ", N "         or coverdisk to me        ",
    N "                                   ", N "                                   ",
    E "-------------------------------------------------------------------------------",
    N "                               ", H "The first postcard I received !",
    E "-------------------------------", N "   Luigi - Milano, Italy       ",
    N "      special thanks           ", N " ", H "The second (electronic) postcard !",
    E "----------------------------------", N "    Kevin McCarthy - USA          ", N " ",
    H "The next three postcards I received !", E "-------------------------------------",
    N "    Reinhold Platzer - Germany       ", N "    Danke - Thanks - Merci           ", N " ",
    H "The next real postcard ! ", E "------------------------ ", N "Giovanni Iachello - Italy",
    N " ", N " ", H "CDs and Software I received from", E "--------------------------------",
    N "William Weston (2 CDs)", N "Opcode (vision dsp)", N " ", N " ",
    H "Some more respons I got from", E "------------------------------", N "Fizz",
    N "Wolfgang Surholt", N "Jeremiah Cox", N "The Mangement - Lick Observatory",
    N "Christian Lorenz", N "The Prince of Darkness", N "Syv Knight", N "A confused American",
    N "greM/TB", N "Matthew Weigel", N "W. Michael Petullo", N "Someone from Nebraska",
    N "Merens Heinim", N "Antonio Schifano", N "Tobias Rapp", N "Corwin Grey", N "Peter Wasiewicz",
    N "Richard Bailey", N "Stephan Lehmann", N "William Weston", N "'eike' from Germany",
    N "Delgado Friedrichs", N "Russel McKenzie", N "Kealan", N "Gisbert Berger", N " ", N " ",
    N " ", E "++++++++++++++++++++++++++++++++++++++++++",
    H "            Please send me                ", H "            more postcards!               ",
    H "                                          ", H "   and don't forget the CDs when you use  ",
    H "        Cthugha commercially!             ", E "++++++++++++++++++++++++++++++++++++++++++",
    N " ", N " ", N "If I have misspelled your name", N "or have missed you completely,",
    N "please forgive me.", N "Drop me a mail, and I will fix it.", N " ", N " ",
    E "-------------------------------------------------------------------------------", N " ",
    H "These guys are the main authors", E "-------------------------------",
    N "Kevin Burfitt <zaph@torps.apana.org.au>", E "original Author", E "DOS and Windows 95", N " ",
    N "Rus Maxham <rus@interstice.com>", E "Mac Version", N " ",
    N "Mark Vojkovich <mvojkov@@ucsd.edu>", E "DGA support", N " ",
    N "Jeff The Riffer <riffer@afn.org>", E "WWW page and FTP archive", N " ", N " ",
    H "Some more guys who helped", E "-------------------------", N "Christopher L. Platt",
    N "Stanislav V. Voronyi", N "Torsten Martinsen", N "Jarkko Lietolahti", N "Jan Kujawa",
    N "Someone who gave only root as name", N "James S. Blachly <blach@giblets.com>",
    N "Robert Bihlmeyer", N "Antonio Schifano", N "Richard Boulton", N "John Morton", N " ", N " ",
    N " ", N " ", N " ", N " ", N " ", N " " };
int InterfaceCredits::nCredits = sizeof(InterfaceCredits::credits) / sizeof(const char*);

void registerCreditsInterface(InterfaceRuntime& runtime) {
    runtime.registerOwnedInterface(new InterfaceCredits());
}
