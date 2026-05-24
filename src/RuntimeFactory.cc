#include "cthugha.h"
#include "RuntimeFactory.h"

#include <unistd.h>

Settings::Settings()
    : soundDeviceNumber(SDN_DSPIn)
    , silent(0) { }

Settings Settings::fromCurrentOptions() {
    Settings settings;

    settings.soundDeviceNumber = int(soundDeviceNr);
    settings.silent = int(soundSilent);

    CTH_TRACE("runtime settings: sound-device-number=%d silent=%d\n",
        settings.soundDeviceNumber, settings.silent);

    return settings;
}

Environment::Environment()
    : ossInputAvailable(0)
    , ossOutputAvailable(0) { }

Environment Environment::detect() {
    Environment environment;

    if (SoundDeviceDSP::dev_dsp[0] != '\0') {
        environment.ossInputAvailable = (access(SoundDeviceDSP::dev_dsp, R_OK) == 0);
        environment.ossOutputAvailable = (access(SoundDeviceDSP::dev_dsp, W_OK) == 0);
    }

    CTH_TRACE("runtime environment: dev-dsp=`%s' oss-input=%d oss-output=%d\n",
        SoundDeviceDSP::dev_dsp, environment.ossInputAvailable, environment.ossOutputAvailable);

    return environment;
}

RuntimeFactory::RuntimeFactory(const Settings& settings_, const Environment& environment_)
    : settings(settings_)
    , environment(environment_) {
    CTH_TRACE("runtime factory: created with sound-device-number=%d silent=%d oss-input=%d oss-output=%d\n",
        settings.soundDeviceNumber, settings.silent,
        environment.ossInputAvailable, environment.ossOutputAvailable);
}

AudioInput* RuntimeFactory::createAudioInput() const {
    CTH_TRACE("runtime factory: selecting AudioInput for sound-device-number=%d\n",
        settings.soundDeviceNumber);

    switch (settings.soundDeviceNumber) {
    case SDN_Random:
        CTH_DEBUG("    audio input strategy: random input from new runtime factory\n");
        CTH_TRACE("runtime factory: selected AudioRandomInput\n");
        return new AudioRandomInput();

    default:
        CTH_DEBUG("    audio input strategy: random input placeholder; requested device %d is not migrated yet\n",
            settings.soundDeviceNumber);
        CTH_TRACE("runtime factory: selected AudioRandomInput placeholder for requested device %d\n",
            settings.soundDeviceNumber);
        return new AudioRandomInput();
    }
}

AudioProcessor* RuntimeFactory::createAudioProcessor() const {
    CTH_TRACE("runtime factory: creating AudioProcessor\n");
    return new AudioProcessor(createAudioInput());
}

SoundDevice* RuntimeFactory::createLegacySoundDevice(RuntimeSoundInputContext context) const {
    const char* contextName = (context == RSIC_FileChild) ? "file child" : "main process";
    int effectiveSilent = settings.silent;

    CTH_TRACE("runtime factory: selecting legacy SoundDevice context=%s sound-device-number=%d silent=%d\n",
        contextName, settings.soundDeviceNumber, effectiveSilent);

    // Preserve the old file-input decision exactly while moving it into the
    // composition layer: file playback forks only when passthrough is enabled.
    if ((context == RSIC_MainProcess) && (settings.soundDeviceNumber == SDN_File) && !effectiveSilent
        && !SoundDeviceFile::hasSoundOutputDevice()) {
        CTH_WARN("  No usable audio passthrough device; playing file silently.\n");
        CTH_DEBUG("    sound input strategy: direct file input in %s, because audio passthrough is unavailable\n",
            contextName);
        soundSilent.setValue(1);
        effectiveSilent = 1;
        CTH_TRACE("runtime factory: forced silent file playback because no passthrough output is available\n");
    }

    switch (settings.soundDeviceNumber) {
    case SDN_DSPIn:
        CTH_DEBUG("    sound input strategy: OSS DSP input in %s, because sound-device-number=%d\n",
            contextName, settings.soundDeviceNumber);
        CTH_TRACE("runtime factory: selected SoundDeviceDSPIn\n");
        return new SoundDeviceDSPIn();

    case SDN_Net:
        CTH_DEBUG("    sound input strategy: network input in %s, because sound-device-number=%d\n",
            contextName, settings.soundDeviceNumber);
        CTH_TRACE("runtime factory: selected SoundDeviceNet\n");
        return new SoundDeviceNet();

    case SDN_Random:
        CTH_DEBUG("    sound input strategy: random input in %s, because sound-device-number=%d\n",
            contextName, settings.soundDeviceNumber);
        CTH_TRACE("runtime factory: selected SoundDeviceRandom\n");
        return new SoundDeviceRandom();

    case SDN_File:
        if ((context == RSIC_MainProcess) && !effectiveSilent) {
            CTH_DEBUG("    sound input strategy: forked file input in %s, because audio passthrough is enabled\n",
                contextName);
            CTH_TRACE("runtime factory: selected SoundDeviceFork\n");
            return new SoundDeviceFork();
        }

        CTH_DEBUG("    sound input strategy: direct file input in %s, because %s\n",
            contextName,
            (context == RSIC_FileChild) ? "this process owns the file reader"
                                        : "playback is silent");
        CTH_TRACE("runtime factory: selected SoundDeviceFile\n");
        return new SoundDeviceFile();

    default:
        CTH_TRACE("runtime factory: illegal legacy sound-device-number=%d\n",
            settings.soundDeviceNumber);
        CTH_ERROR("Illegal SoundDeviceNr %d.\n", settings.soundDeviceNumber);
        exit(0);
    }
}
