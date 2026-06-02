#include "cthugha.h"
#include "AudioSettings.h"
#include "AudioOptions.h"
#include "AudioTypes.h"
#include "defaults.h"

#include <string.h>

AudioSettings::AudioSettings()
    : audioInputMode(DEFAULT_AUDIO_INPUT_MODE)
    , soundDSPMethod(DEFAULT_SOUND_DSP_METHOD)
    , silent(DEFAULT_SOUND_SILENT_ENABLED) {
    fileName[0] = '\0';
}

void AudioSettings::refreshFromCurrentOptions() {
    audioInputMode = int(::audioInputMode);
    soundDSPMethod = int(::soundDSPMethod);
    silent = int(soundSilent);
    strncpy(fileName, audio_input_file, PATH_MAX);
    fileName[PATH_MAX - 1] = '\0';
}

AudioSettings AudioSettings::fromCurrentOptions() {
    AudioSettings settings;
    settings.refreshFromCurrentOptions();

    CTH_DEBUG("runtime settings: audio-input-mode=%d sound-dsp-method=%d silent=%d file=`%s'\n",
        settings.audioInputMode, settings.soundDSPMethod, settings.silent, settings.fileName);

    return settings;
}
