#include "cthugha.h"
#include "information.h"
#include "AudioOptions.h"
#include "Mixer.h"
#include "Option.h"
#include "DisplayDevice.h"
#include "AutoChanger.h"
#include "CthughaBuffer.h"
#include "VideoDirector.h"
#include "QotdMessagesProvider.h"
#include "TranslationOptions.h"
#include "configuration_defaults.h"

#include <ctype.h>

void title() {

    printfv(0,
        "--------------------------------------------------------------------------------\n"
        "C T H U G H A - L  " VERSION "\n"
        "An oscilloscope on acid\n"
        "by Harald Deischinger\n"
        "--------------------------------------------------------------------------------\n"
        "email: deischi@geocities.com\n"
        "www:   http://www.geocities.com/CapeCanaveral/Lab/6386\n"
        "mail:  Harald Deischinger\n"
        "       Am Edhuegel 45\n"
        "       4115  Kleinzell\n"
        "       AUSTRIA\n"
        "--------------------------------------------------------------------------------\n"
        "Original Program (CTHUGHA V5.1 and V5.3)\n"
        "  Coded by - Torps Productions: The Digital Aasvogel Group - 1995\n"
        "  Original Idea & Code:          Kevin Burfitt (zaph@torps.apana.org.au)\n"
        "--------------------------------------------------------------------------------\n"
        "Cthugha WWW-Page:            http://www.afn.org/~cthugha\n"
        "Cthugha newsgroup:           alt.graphics.cthugha\n"
        "--------------------------------------------------------------------------------\n",
        "\n");
}

static const char* defValue(const char* v) {
    static char str[512];

    while (isspace(*v))
        v++;

    str[0] = '<';
    strncpy(str + 1, v, 510);
    strcat(str, ">");

    return str;
}

static void PH(const char* txt, const char* def = "") {
    const char* d = (def[0] != '\0') ? (char*)defValue(def) : (char*)"";

    int dl = 79 - strlen(txt);

    char fmt[100];
    snprintf(fmt, sizeof(fmt), "%%s%%%ds\n", dl);

    printfv(0, fmt, txt, d);
}

static void PH(const char* txt, const Option& Opt) { PH(txt, Opt.text()); }

static const char* PHInt(int value) {
    static char s[64];
    snprintf(s, sizeof(s), "%7d", value);
    return s;
}

void usage() {
    char qotdPrefetchTimeoutDefault[32];
    snprintf(qotdPrefetchTimeoutDefault, sizeof(qotdPrefetchTimeoutDefault),
        "%d", MESSAGES_CONFIG_DEFAULT_QOTD_PREFETCH_TIMEOUT_MS);

    PH("Cthugha command line options:");
    PH("-----------------------------");
    PH("Selecting sound device (DSP device by default):");
    PH(" -x, --no-sound      Use no sound input");
    PH(" --random-noise      Use generated random-noise input");
    PH(" --play FILE         Play FILE for sound input");
    PH(" --silent            Play silently", audioOnOffText(audioSilentEnabled()));
    PH(" --loop, --no-loop   Play sound file over and over again or only once");

    PH("");
    PH("General sound device options:");
    PH(" -v, --rate N        Set sample rate to N", PHInt(audioSampleRateHz()));
    PH(" -1, -2, --stereo, --no-stereo Set number of sound channels", audioChannelsText());
    PH(" --snd-format FMT    Set sound format to FMT", audioSampleFormatText());
    PH(" --pulse-server SERVER  Set PulseAudio server", pulse_server);
    PH(" --pulse-latency-ms N  Set PulseAudio target latency");
    PH(" --audio-output-dump FILE  Dump submitted output PCM to WAV");
    PH("");

#if WITH_DSP == 1
    PH("Advanced sound device options:");
    PH(" --snd-method M      Use method M for sound reading", PHInt(audioDspMethod()));
    PH(" --snd-sync          Reset soundcard after reading each block", audioOnOffText(audioDspSyncEnabled()));
    PH(" --snd-fragments     Set number of sound fragments", PHInt(audioDspFragments()));
    PH(" --dev-dsp DEV       Set the DSP device to DEV", audioDspDevicePath());
#endif

    PH("");

#if WITH_MIXER == 1
    PH("Mixer options:");
    PH(" --dev-mixer DEV     Set the mixer device to DEV", dev_mixer);
    PH(" --mixer DEV:VOL     Set mixer device DEV to volume VOL");
    PH(" -L, --line VOL      Use Line In as input with volume VOL");
    PH(" -M, --mic VOL       Use Mic as input with volume VOL");
    PH("");
#endif

    PH("Automatic Changer options:");
    PH(" -l, --lock          Start in Locked mode", lock.text());
    PH(" --little            Only change one option at a time", change_little.text());
    PH(" -T, --min-time N    Minimum time before changing", changeWaitMin.text());
    PH(" -R, --random-time N Extra random time before changing", changeWaitRandom.text());
    PH(" -Q, --quiet-time N  Change after short silence", changeQuiet.text());
    PH(" --msg-time N        Time before quiet message are displayed", changeMsgTime.text());
    PH(" --quiet-message-duration-ms N  Quiet message display duration");
    PH(" -q, --quiet-file FILE  Load alternate quiet messages from FILE");
    PH(" --qotd              Enable Quote of the Day quiet messages");
    PH(" --no-qotd           Disable Quote of the Day quiet messages");
    PH(" --qotd-server SERVER  Quote of the Day server", QotdMessagesProvider::defaultServer());
    PH(" --qotd-port PORT    Default Quote of the Day port", MESSAGES_CONFIG_DEFAULT_QOTD_PORT_TEXT);
    PH(" --qotd-prefetch-timeout-ms N  QOTD fetch timeout", qotdPrefetchTimeoutDefault);
    PH(" --min-noise N       Set level for quiet sound");
    PH(" --cumulative-fire-level N      Set cumulative fire threshold to N", changeCumulativeFireLevel.text());
    PH("");

    PH("General Effect Controls (\"Buffer\" options):");
    PH(" -S, --buff-size SIZE    Set buffer size. Perdefined sizes:");
    for (int i = 0; i < nBufferSizes; i++) {
        printfv(0, "                   ");
        for (int j = 0; (j < 4) && (i < nBufferSizes); i++, j++)
            printfv(0, "  %d: %dx%d", i, bufferSizes[i].x, bufferSizes[i].y);
        printfv(0, "\n");
    }
    PH("                     or a special size in the form: WIDTHxHEIGHT.");
    PH(" --no-trans          Disable translation tables");
    PH(" --images            Enable indexed image files (default)");
    PH(" --no-images         Disable indexed image files");
    PH(" --palette-smoothing F  Chance palette changes smooth, 0..1", "1");
    PH(" --no-palette-smoothing Disable palette smoothing");
    PH(" --no-object         Disable 3-D objects");
    PH(" -s, --no-flashlight Diable usage of flashlights (changing palette on beats)");
    PH("");

    PH("Initial Values for Effect Controls:");
    PH(" -d, --display N     Start with display N (how buffer is mapped to screen)");
    PH(" -f, --flame N       Start with flame N");
    PH(" -w, --wave N        Start with wave N (how sound is drawn)");
    PH(" --wave-scale N      Start with wave scale value N");
    PH(" -o, --object N      Start with 3D object N (used by some waves)");
    PH(" -t, --translation N Start with translation N");
    PH(" --border N          Start with buffer border N");
    PH(" -p, --palette N     Start with palette N");
    PH(" --palette-set SETS  Enable palettes matching one of the named sets");
    PH(" -m, --sound-processing N  Start with sound processing N");
    PH(" -a, --table N       Start with table N (how palette is used in wave)");
    PH(" --image N           Start with image N");
    PH("Display options:");
    PH(" -D, --disp-mode MODE    Set graphics mode (window size)");
    for (int i = 0; i < nScreenSizes; i++) {
        printfv(0, "                   ");
        for (int j = 0; (j < 4) && (i < nScreenSizes); i++, j++)
            printfv(0, "  %d: %dx%d", i, screenSizes[i].x, screenSizes[i].y);
        printfv(0, "\n");
    }
    PH("                     or a special resolution in the form: WIDTHxHEIGHT.");
    PH(" --zoom N            Set Zoom factor to N (0 = fit window/screen)");
    PH(" --max-fps N         Set maximal frames per seond to N (0 = no limit)");
    PH(" --show-fps          Show a live FPS counter");
    PH("X11 options:");
    PH(" --root              Display on root window");
    PH(" --install           Install a private colormap");
    PH(" --mit-shm           Use MIT-SHM extension");
    PH(" --full-screen       Fill the whole screen");
    PH(" --panel             Show control panel");
    PH(" --position NN       Set position of window (e.g. --position +10+30)");
    PH(" --no-decorate       Do not decorate window (set override_redirect flag)");
    PH(" --font F            Font to use for Cthugha");
    PH("");
    PH("General options:");
    PH(" -E, --path LIBDIR   An extra searchpath for map, image and tab-files");
    PH("                     cthugha searches in LIBDIR/map, LIBDIR/img and LIBDIR/tab");
    PH(" --ini-file FILE     Read settings from FILE instead of default ini files");
    PH(" --keymap KEYMAP     Load keyboard definitions from KEYMAP file");
    PH(" --no-esc            Disable quit on ESC");
    PH(" --save              Save settings on exit");
    PH(" --verbose[=LVL]     Print some extra information");
    PH("                     --verbose without LVL sets a level of 4");
    PH(" --no-verbose        Print no extra information");
    PH(" -?, --help          This Text");
    PH("");
    PH("Most switches can be used in a no-form to disable the feature.");
}
