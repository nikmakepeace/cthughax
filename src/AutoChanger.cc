#include "cthugha.h"
#include "imath.h"
#include "AutoChanger.h"
#include "AudioAnalyzer.h"
#include "Configuration.h"
#include "CthughaBuffer.h"
#include "RuntimeCommandSink.h"
#include "Scene.h"
#include "VideoDirector.h"

OptionTime changeQuiet("quiet-change", 0); /* change after quiet-pause */

/* Default to roughly the DOS Cthugha 5.3 dwell: 200-949 frames at the old
   320x200 VGA mode's ~70 Hz scan rate, or about 3-14 seconds. */
OptionTime changeWaitMin("min-time", 0); /* min time between change */
OptionTime changeWaitRandom("random-time", 1); /* extra random wait-time */

OptionInt changeCumulativeFireLevel("cumulative-fire-level", 0);

OptionOnOff lock("lock", 0); /* change automatically */
OptionOnOff change_little("little", 0); /* only change one options */

AutoChanger* autoChanger = NULL;
static int changeWaitRandomMinimumMs = 1;

void configureAutoChanger(const AutoChangeConfig& config) {
    changeQuiet.setValue(config.quietMs);
    changeWaitMin.setValue(config.waitMinMs);
    changeWaitRandom.setValue(config.waitRandomMs);
    changeWaitRandomMinimumMs = config.waitRandomMinimumMs;
    changeCumulativeFireLevel.setValue(config.cumulativeFireLevel);
    lock.setValue(config.locked);
    change_little.setValue(config.changeLittle);
}

AutoChanger::AutoChanger(RuntimeCommandSink& runtimeCommands_)
    : runtimeCommands(runtimeCommands_)
    , quietSince(0)
    , lastChange(0) {

    if (changeWaitRandom <= 0)
        changeWaitRandom.setValue(changeWaitRandomMinimumMs);

    /* set initial wait-time till change */
    waitTime = changeWaitMin + rand() % changeWaitRandom;

    lastChange = gettime();
    quietSince = gettime();
}

AutoChanger::~AutoChanger() { }

void AutoChanger::operator()() {

    int now = gettime();

    /* get time since last sound */
    int quiet_length = now - quietSince;
    if (audioMetrics.noisy)
        quietSince = now;

    /* Report long quietness to visual policy. */
    if (!audioMetrics.noisy && videoDirector().observeQuiet(quiet_length))
        quietSince = now; // start quiet-length again

    if (int(lock))
        return;

    /* Check for interrupted silence (like the pause btw. 2 tracks on a CD) */
    if (int(changeQuiet))
        if (audioMetrics.noisy && (quiet_length > int(changeQuiet))) {
            change();
            return;
        }

    /* Check for enough fire to change */
    if (int(changeCumulativeFireLevel))
        if (acousticContext.cumulativeFireLevel() > int(changeCumulativeFireLevel)) {
            CTH_DEBUG("autochange: cumulativeFireLevel threshold reached level=%d threshold=%d\n",
                acousticContext.cumulativeFireLevel(), int(changeCumulativeFireLevel));
            acousticContext.resetCumulativeFireLevel();
            change();
            return;
        }

    /* nothing special happend, maybe we waited long enough */
    if ((changeWaitMin + changeWaitRandom) > 0)
        if ((now - lastChange) > int(waitTime)) {
            lastChange = now;
            waitTime = int(changeWaitMin)
                + rand() % max(changeWaitRandomMinimumMs, int(changeWaitRandom));
            change();
            return;
        }
}

void AutoChanger::change() {

    if (int(change_little)) {
        runtimeCommands.apply(RuntimeCommand::changeOne());
    } else {
        runtimeCommands.apply(RuntimeCommand::changeAll());
    }
}

const char* AutoChanger::status() {
    static char txt[512];

    if (lock) {
        snprintf(txt, sizeof(txt), "locked ");
    } else {
        int now = gettime();

        snprintf(txt, sizeof(txt), "change: T:%.2f F:%d S:%.2f ", double(waitTime - (now - lastChange)) / 1000.0,
            changeCumulativeFireLevel - acousticContext.cumulativeFireLevel(),
            double(changeQuiet - (now - quietSince)) / 1000.0);
    }

    return txt;
}
