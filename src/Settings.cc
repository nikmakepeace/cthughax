#include "cthugha.h"
#include "Settings.h"
#include "AudioOptions.h"
#include "AudioTypes.h"

#include <string.h>

Settings::Settings()
    : audioInputMode(AIM_DSPIn)
    , soundDSPMethod(0)
    , silent(0) {
    fileName[0] = '\0';
}

void Settings::refreshFromCurrentOptions() {
    audioInputMode = int(::audioInputMode);
    soundDSPMethod = int(::soundDSPMethod);
    silent = int(soundSilent);
    strncpy(fileName, audio_input_file, PATH_MAX);
    fileName[PATH_MAX - 1] = '\0';
}

Settings Settings::fromCurrentOptions() {
    Settings settings;
    settings.refreshFromCurrentOptions();

    CTH_DEBUG("runtime settings: audio-input-mode=%d sound-dsp-method=%d silent=%d file=`%s'\n",
        settings.audioInputMode, settings.soundDSPMethod, settings.silent, settings.fileName);

    return settings;
}
