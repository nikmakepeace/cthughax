#include "cthugha.h"
#include "AudioRuntime.h"

static const char* audioRuntimeContextName(RuntimeSoundInputContext context) {
    return (context == RSIC_FileChild) ? "file child" : "main process";
}

void audioRuntimeInit(RuntimeSoundInputContext context, int initializeInputControls) {
    CTH_TRACE("audio runtime: init requested context=%s initialize-input-controls=%d\n",
        audioRuntimeContextName(context), initializeInputControls);

    if (soundDevice != NULL) {
        CTH_TRACE("audio runtime: init skipped because a sound device is already installed\n");
        return;
    }

    RuntimeFactory runtimeFactory(Settings::fromCurrentOptions(), Environment::detect());
    SoundDevice::install(runtimeFactory.createLegacySoundDevice(context), initializeInputControls);

    CTH_TRACE("audio runtime: init completed context=%s\n", audioRuntimeContextName(context));
}

void audioRuntimeShutdown() {
    CTH_TRACE("audio runtime: shutdown requested installed=%d\n", soundDevice != NULL);
    delete soundDevice;
    soundDevice = NULL;
    CTH_TRACE("audio runtime: shutdown completed\n");
}
