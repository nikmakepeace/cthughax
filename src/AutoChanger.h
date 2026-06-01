// -*- c++ -*-

#ifndef __AUTO_CHANGER_H
#define __AUTO_CHANGER_H

#include "Option.h"

//
// automatically change the display, based on timeouts and noise
// does also silence messages
//

#define MAX_SILENCE_STRINGS 256

extern OptionTime changeQuiet; /* change after quiet-pause (1.5 sec) */
extern OptionTime changeMsgTime; /* max. quiet interval (5 sec) then text is displayed */
extern OptionTime changeWaitMin; /* min time between change (5 sec) */
extern OptionTime changeWaitRandom; /* extra random wait-time (10 sec) */
extern OptionInt changeCumulativeFireLevel;
extern OptionOnOff lock; /* change automatically */
extern OptionOnOff change_little; /* only change one options */

class SceneCommands;

class AutoChanger {
    SceneCommands& sceneCommands;

    int quietSince;
    int waitTime;
    int lastChange;

    static const char* silenceStrings[MAX_SILENCE_STRINGS];
    static int nSilenceStrings;

public:
    AutoChanger(SceneCommands& sceneCommands_);
    ~AutoChanger();
    static void loadSilenceStrings(const char* fname);

    void operator()();

    void silenceMessage();
    void change();

    const char* status(); // print status information
};
extern AutoChanger* autoChanger;

#endif
