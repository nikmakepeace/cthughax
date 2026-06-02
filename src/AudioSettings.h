// Current audio settings snapshot.

#ifndef __AUDIO_SETTINGS_H
#define __AUDIO_SETTINGS_H

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

class AudioSettings {
public:
    int audioInputMode;
    int soundDSPMethod;
    int silent;
    char fileName[PATH_MAX];

    AudioSettings();

    void refreshFromCurrentOptions();
    static AudioSettings fromCurrentOptions();
};

#endif
