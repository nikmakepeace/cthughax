// Startup-time composition helpers.

#ifndef __RUNTIME_FACTORY_H
#define __RUNTIME_FACTORY_H

#include "Audio.h"

enum RuntimeSoundInputContext {
    RSIC_MainProcess,
    RSIC_FileChild
};

class Settings {
public:
    int soundDeviceNumber;
    int silent;

    Settings();

    static Settings fromCurrentOptions();
};

class Environment {
public:
    int ossInputAvailable;
    int ossOutputAvailable;

    Environment();

    static Environment detect();
};

class RuntimeFactory {
    Settings settings;
    Environment environment;

public:
    RuntimeFactory(const Settings& settings, const Environment& environment);

    AudioInput* createAudioInput() const;
    AudioProcessor* createAudioProcessor() const;

    SoundDevice* createLegacySoundDevice(RuntimeSoundInputContext context) const;
};

#endif
