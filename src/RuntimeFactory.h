// Startup-time composition helpers.

#ifndef __RUNTIME_FACTORY_H
#define __RUNTIME_FACTORY_H

#include "Audio.h"

#include <limits.h>

enum RuntimeSoundInputContext {
    RSIC_MainProcess,
    RSIC_FileChild
};

enum AudioSourceStrategy {
    ASS_LineIn,
    ASS_Network,
    ASS_Random,
    ASS_WavFile,
    ASS_Mp3File,
    ASS_RawFile,
    ASS_Unknown
};

class Settings {
public:
    int soundDeviceNumber;
    int soundDSPMethod;
    int silent;
    char fileName[PATH_MAX];

    Settings();

    static Settings fromCurrentOptions();
};

class Environment {
public:
    int ossInputAvailable;
    int ossOutputAvailable;
    int pulseOutputAvailable;

    Environment();

    static Environment detect();
};

class RuntimeFactory {
    Settings settings;
    Environment environment;

public:
    RuntimeFactory(const Settings& settings, const Environment& environment);

    AudioInput* createAudioInput() const;
    AudioOutput* createAudioOutput() const;
    AudioProcessor* createAudioProcessor() const;
    AudioSourceStrategy selectAudioSourceStrategy() const;

    SoundDevice* createLegacySoundDevice(RuntimeSoundInputContext context) const;
};

#endif
