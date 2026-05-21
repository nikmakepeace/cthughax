#include "cthugha.h"
#include "imath.h"
#include "AutoChanger.h"
#include "SoundAnalyze.h"
#include "Interface.h"
#include "display.h"
#include "options.h"
#include "CthughaBuffer.h"

OptionTime changeQuiet("quiet-change", 150); /* change after quiet-pause (1.5 sec) */

OptionTime changeMsgTime("change-msg-time", 500); /* max. quiet interval (5 sec)
                                                     then text is displayed */

/* Default to roughly the DOS Cthugha 5.3 dwell: 200-949 frames at the old
   320x200 VGA mode's ~70 Hz scan rate, or about 3-14 seconds. */
OptionTime changeWaitMin("min-time", 300); /* min time between change (3 sec) */
OptionTime changeWaitRandom("random-time", 1100); /* extra random wait-time (11 sec) */

OptionInt changeFireLevel("fire-level", 1000);

OptionOnOff lock("lock", 0); /* change automatically */
OptionOnOff change_little("little", 0); /* only change one options */

AutoChanger* autoChanger = NULL;

AutoChanger::AutoChanger()
    : quietSince(0)
    , lastChange(0) {

    if (changeWaitRandom <= 0)
        changeWaitRandom.setValue(1);

    /* set initial wait-time till change */
    waitTime = changeWaitMin + rand() % changeWaitRandom;

    read_ini_usage();

    lastChange = gettime();
    quietSince = gettime();
}

AutoChanger::~AutoChanger() {
    if (options_save) {
        write_ini();
    }
}

void AutoChanger::operator()() {

    int now = gettime();

    /* get time since last sound */
    int quiet_length = now - quietSince;
    if (soundAnalyze.noisy)
        quietSince = now;

    /* Check for long quietness */
    if (int(changeMsgTime)) {
        if (!soundAnalyze.noisy && (quiet_length > int(changeMsgTime))) {
            quietSince = now; // start quiet-length again

            silenceMessage();
        }
    }

    if (int(lock))
        return;

    /* Check for interrupted silence (like the pause btw. 2 tracks on a CD) */
    if (int(changeQuiet))
        if (soundAnalyze.noisy && (quiet_length > int(changeQuiet))) {
            change();
            return;
        }

    /* Check for enough fire to change */
    if (int(changeFireLevel))
        if (soundAnalyze.fireLevel > int(changeFireLevel)) {
            CTH_DEBUG("autochange: fire threshold reached fireLevel=%d threshold=%d\n",
                soundAnalyze.fireLevel, int(changeFireLevel));
            soundAnalyze.fireLevel = 0;
            change();
            return;
        }

    /* nothing special happend, maybe we waited long enough */
    if ((changeWaitMin + changeWaitRandom) > 0)
        if ((now - lastChange) > int(waitTime)) {
            lastChange = now;
            waitTime = int(changeWaitMin) + rand() % max(1, int(changeWaitRandom));
            change();
            return;
        }
}

void AutoChanger::change() {

    if (int(change_little)) {
        CoreOption::changeOne();
    } else {
        CoreOption::changeAll();
    }
}

char* AutoChanger::silenceStrings[MAX_SILENCE_STRINGS] = {
    /*   1234567890123456789012345678901234567890	*/
    "Where is the music?", /* 0 */
    "JOLT !", /* 1 */
    "Turn The Music On", /* 2 */
    "Lets Party!!!", /* 3 */
    "Pink Floyd Rules", /* 4 */
    "Sounds of Silence ?", /* 5 */
    "The Torps", /* 6 */
    "Study Mathematics", /* 7 */
    "Visit Linz", /* 8 */
    "Press ? for help", /* 9 */
    "Number 5 is ALIVE!!", /* 10 */
    "Spooky Mulder.....", /* 11 */
    "Math is Power", /* 12 */
    "Subliminal Ads", /* 13 */
    "Read a book", /* 14 */
    "Get a life...", /* 15 */
    "SMILE!", /* 16 */
    "Cthugha-L", /* 17 */
    "Don't Panic!", /* 18 */
    "@fortune" /* 19 */
};
int AutoChanger::nSilenceStrings = 20;

void AutoChanger::loadSilenceStrings(const char* fname) {

    FILE* file;
    char* s;

    if ((fname == NULL) || (*fname == '\0'))
        return;

    if ((file = fopen(fname, "r")) == NULL) {
        CTH_ERRNO(errno, "Can't open quiet strings file `%s'.", fname);
        return;
    }

    nSilenceStrings = 0;
    do {
        /* read a line */
        silenceStrings[nSilenceStrings] = new char[256];
        s = fgets(silenceStrings[nSilenceStrings], 255, file);
        if (s != NULL)
            nSilenceStrings++;
    } while ((nSilenceStrings < MAX_SILENCE_STRINGS) && (s != NULL));

    /* check if file was empty */
    if (nSilenceStrings == 0) {
        CTH_WARN("silence strings file `%s' was empty.\n", fname);
        nSilenceStrings = 1;
        strcpy(silenceStrings[0], "Where is the music?");
    }
}

void AutoChanger::silenceMessage() {

    int print_nr = rand() % nSilenceStrings;

    if (silenceStrings[print_nr][0] != '@') {
        /* traditional string */
        Interface::current->msg(silenceStrings[print_nr]);
    } else {
        /* print fortune */
        FILE* msgPipe;

        if ((msgPipe = popen("fortune", "r")) == NULL) {
            Interface::current->msg("Can not open fortune pipe");
        } else {
            static char str[4096];
            str[fread(str, 1, 4096, msgPipe)] = '\0';
            Interface::current->msg(str);
            pclose(msgPipe);
        }
    }
}

const char* AutoChanger::status() {
    static char txt[512];

    if (lock) {
        sprintf(txt, "locked ");
    } else {
        int now = gettime();

        sprintf(txt, "change: T:%.2f F:%d S:%.2f ", double(waitTime - (now - lastChange)) / 100.0,
            changeFireLevel - soundAnalyze.fireLevel,
            double(changeQuiet - (now - quietSince)) / 100.0);
    }

    return txt;
}
