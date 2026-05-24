#include "cthugha.h"
#include "AudioRuntime.h"

void audioRuntimeInit(RuntimeSoundInputContext context, int initializeInputControls) {
    if (soundDevice != NULL)
        return;

    RuntimeFactory runtimeFactory(Settings::fromCurrentOptions(), Environment::detect());
    SoundDevice::install(runtimeFactory.createLegacySoundDevice(context), initializeInputControls);
}

void audioRuntimeShutdown() {
    delete soundDevice;
    soundDevice = NULL;
}
