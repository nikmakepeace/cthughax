// Audio device configuration and lifecycle wrapper for the current audio runtime.
// Backend selection enters through AudioRuntime/RuntimeFactory; device settings
// are startup configuration plus backend negotiation, not runtime UI controls.

#include "cthugha.h"
#include "AudioSystem.h"
#include "AudioOptions.h"
#include "AudioRuntime.h"
#include "Configuration.h"
#include "Interface.h"
#include "Mixer.h"

#include <string.h>

static const char* audioSampleFormatNames[] = {
    "8bit unsigned",
    "8bit signed",
    "16bit unsigned (le)",
    "16bit signed (le)",
    "16bit unsigned (be)",
    "16bit signed (be)",
};
static const int audioSampleFormatCount
    = sizeof(audioSampleFormatNames) / sizeof(const char*);

static AudioDeviceConfig audioDeviceConfigValue;

AudioDeviceConfig::AudioDeviceConfig()
    : inputMode(AIM_DSPIn)
    , inputLoopEnabled(0)
    , pcmFormat()
    , dspMethod(0)
    , dspFragments(0)
    , dspFragmentSize(0)
    , dspSyncEnabled(0)
    , silentEnabled(0) {
    dspDevicePath[0] = '\0';
}

const AudioDeviceConfig& audioDeviceConfig() {
    return audioDeviceConfigValue;
}

const PcmFormat& audioPcmFormat() {
    return audioDeviceConfigValue.pcmFormat;
}

AudioInputMode audioInputModeValue() {
    return audioDeviceConfigValue.inputMode;
}

int audioInputLoopEnabled() {
    return audioDeviceConfigValue.inputLoopEnabled;
}

void audioSetInputLoopEnabled(int enabled) {
    audioDeviceConfigValue.inputLoopEnabled = enabled ? 1 : 0;
}

int audioSampleRateHz() {
    return audioDeviceConfigValue.pcmFormat.sampleRate;
}

int audioChannels() {
    return audioDeviceConfigValue.pcmFormat.channels;
}

int audioSampleFormat() {
    return audioDeviceConfigValue.pcmFormat.sampleFormat;
}

void audioSetPcmFormat(const PcmFormat& format) {
    audioDeviceConfigValue.pcmFormat = format;
}

void audioSetSampleRateHz(int sampleRateHz) {
    audioDeviceConfigValue.pcmFormat.sampleRate = sampleRateHz;
}

void audioSetChannels(int channels) {
    audioDeviceConfigValue.pcmFormat.channels = channels;
}

void audioSetSampleFormat(int sampleFormat) {
    audioDeviceConfigValue.pcmFormat.sampleFormat = sampleFormat;
}

int audioBytesPerSample() {
    return audioDeviceConfigValue.pcmFormat.bytesPerSample();
}

int audioDspMethod() {
    return audioDeviceConfigValue.dspMethod;
}

int audioDspFragments() {
    return audioDeviceConfigValue.dspFragments;
}

int audioDspFragmentSize() {
    return audioDeviceConfigValue.dspFragmentSize;
}

void audioSetDspFragment(int fragments, int fragmentSize) {
    audioDeviceConfigValue.dspFragments = fragments;
    audioDeviceConfigValue.dspFragmentSize = fragmentSize;
}

void audioSetDspFragments(int fragments) {
    audioDeviceConfigValue.dspFragments = fragments;
}

void audioSetDspFragmentSize(int fragmentSize) {
    audioDeviceConfigValue.dspFragmentSize = fragmentSize;
}

int audioDspSyncEnabled() {
    return audioDeviceConfigValue.dspSyncEnabled;
}

int audioSilentEnabled() {
    return audioDeviceConfigValue.silentEnabled;
}

const char* audioDspDevicePath() {
    return audioDeviceConfigValue.dspDevicePath;
}

const char* audioChannelsText() {
    switch (audioChannels()) {
    case 1:
        return "mono";
    case 2:
        return "stereo";
    default:
        return "unknown";
    }
}

const char* audioSampleFormatText(int sampleFormat) {
    if ((sampleFormat >= 0) && (sampleFormat < audioSampleFormatCount))
        return audioSampleFormatNames[sampleFormat];
    return "unknown";
}

const char* audioSampleFormatText() {
    return audioSampleFormatText(audioSampleFormat());
}

const char* audioOnOffText(int enabled) {
    return enabled ? (char*)" on" : (char*)"off";
}

void configureAudioOptions(const AudioConfig& config) {
    audioDeviceConfigValue.inputMode = config.inputMode;
    audioDeviceConfigValue.inputLoopEnabled = config.inputLoopEnabled ? 1 : 0;
    audioDeviceConfigValue.pcmFormat.sampleRate = config.sampleRateHz;
    audioDeviceConfigValue.pcmFormat.channels = config.channels;
    audioDeviceConfigValue.pcmFormat.sampleFormat = int(config.sampleFormat);
    audioDeviceConfigValue.dspMethod = config.dspMethod;
    audioDeviceConfigValue.dspFragments = config.dspFragments;
    audioDeviceConfigValue.dspFragmentSize = config.dspFragmentSize;
    audioDeviceConfigValue.dspSyncEnabled = config.dspSyncEnabled ? 1 : 0;
    audioDeviceConfigValue.silentEnabled = config.silentEnabled ? 1 : 0;

    strncpy(audioDeviceConfigValue.dspDevicePath, config.dspDevicePath.c_str(),
        PATH_MAX);
    audioDeviceConfigValue.dspDevicePath[PATH_MAX - 1] = '\0';
    strncpy(dev_mixer, config.mixerDevicePath.c_str(), PATH_MAX);
    dev_mixer[PATH_MAX - 1] = '\0';
    for (std::vector<AudioMixerInitialVolumeConfig>::const_iterator it
         = config.mixerInitialVolumes.begin();
         it != config.mixerInitialVolumes.end(); ++it) {
        mixer_initial_volume(it->name.c_str(), it->volume);
    }

    configureAudioOutputOptions(config);
}

int init_sound(const AudioConfig& config, int visualMaxDimension,
    RuntimeCommandSink* runtimeCommands) {
    if (audioRuntimeIsInitialized())
        return 0;

    return audioRuntimeInit(config, 1, visualMaxDimension, runtimeCommands);
}

int exit_sound() {
    audioRuntimeShutdown();
    return 0;
}

Interface interfacePlayList("playList", "Sound Play List", "Not yet implemented");
