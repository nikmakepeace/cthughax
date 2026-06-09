/** @file
 * Startup/session audio settings snapshot implementation.
 */

#include "AudioSettings.h"
#include "Configuration.h"
#include "ProcessServices.h"

#include <string.h>

AudioSettings::AudioSettings()
    : audioInputMode(0)
    , inputLoopEnabled(0)
    , pcmFormat()
    , soundDSPMethod(0)
    , dspFragments(0)
    , dspFragmentSize(0)
    , dspSyncEnabled(0)
    , silent(0) {
    fileName[0] = '\0';
    dspDevicePath[0] = '\0';
    miniAudioCaptureDeviceName[0] = '\0';
}

void AudioSettings::refreshFromConfig(const AudioConfig& config) {
    audioInputMode = int(config.inputMode);
    inputLoopEnabled = config.inputLoopEnabled ? 1 : 0;
    pcmFormat.sampleRate = config.sampleRateHz;
    pcmFormat.channels = config.channels;
    pcmFormat.sampleFormat = int(config.sampleFormat);
    soundDSPMethod = config.dspMethod;
    dspFragments = config.dspFragments;
    dspFragmentSize = config.dspFragmentSize;
    dspSyncEnabled = config.dspSyncEnabled ? 1 : 0;
    silent = config.silentEnabled ? 1 : 0;
    strncpy(fileName, config.inputFile.c_str(), PATH_MAX);
    fileName[PATH_MAX - 1] = '\0';
    strncpy(dspDevicePath, config.dspDevicePath.c_str(), PATH_MAX);
    dspDevicePath[PATH_MAX - 1] = '\0';
    strncpy(miniAudioCaptureDeviceName,
        config.miniAudioCaptureDeviceName.c_str(), PATH_MAX);
    miniAudioCaptureDeviceName[PATH_MAX - 1] = '\0';
}

AudioSettings AudioSettings::fromConfig(const AudioConfig& config,
    LogSink& log) {
    AudioSettings settings;
    settings.refreshFromConfig(config);

    log.debug("runtime settings: audio-input-mode=%d rate=%d channels=%d format=%d sound-dsp-method=%d silent=%d file=`%s' miniaudio-capture-device=`%s'\n",
        settings.audioInputMode, settings.pcmFormat.sampleRate,
        settings.pcmFormat.channels, settings.pcmFormat.sampleFormat,
        settings.soundDSPMethod, settings.silent, settings.fileName,
        settings.miniAudioCaptureDeviceName);

    return settings;
}
