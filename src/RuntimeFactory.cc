#include "cthugha.h"
#include "RuntimeFactory.h"
#include "Sound.h"

#include <string.h>
#include <unistd.h>

Settings::Settings()
    : soundDeviceNumber(SDN_DSPIn)
    , soundDSPMethod(0)
    , silent(0) {
    fileName[0] = '\0';
}

Settings Settings::fromCurrentOptions() {
    Settings settings;

    settings.soundDeviceNumber = int(soundDeviceNr);
    settings.soundDSPMethod = int(::soundDSPMethod);
    settings.silent = int(soundSilent);
    strncpy(settings.fileName, SoundDeviceFile::name, PATH_MAX);
    settings.fileName[PATH_MAX - 1] = '\0';

    CTH_TRACE("sound-device-number=%d sound-dsp-method=%d silent=%d file=`%s'\n", "runtime settings",
        settings.soundDeviceNumber, settings.soundDSPMethod, settings.silent, settings.fileName);

    return settings;
}

Environment::Environment()
    : ossInputAvailable(0)
    , ossOutputAvailable(0)
    , pulseOutputAvailable(0) { }

Environment Environment::detect() {
    Environment environment;

    if (SoundDeviceDSP::dev_dsp[0] != '\0') {
        environment.ossInputAvailable = (access(SoundDeviceDSP::dev_dsp, R_OK) == 0);
        environment.ossOutputAvailable = (access(SoundDeviceDSP::dev_dsp, W_OK) == 0);
    }

#if WITH_PULSE == 1
    environment.pulseOutputAvailable = 1;
#endif

    CTH_TRACE("dev-dsp=`%s' oss-input=%d oss-output=%d pulse-output=%d\n", "runtime environment",
        SoundDeviceDSP::dev_dsp, environment.ossInputAvailable, environment.ossOutputAvailable,
        environment.pulseOutputAvailable);

    return environment;
}

RuntimeFactory::RuntimeFactory(const Settings& settings_, const Environment& environment_)
    : settings(settings_)
    , environment(environment_) {
    CTH_TRACE("created with sound-device-number=%d sound-dsp-method=%d silent=%d oss-input=%d oss-output=%d pulse-output=%d\n", "runtime factory",
        settings.soundDeviceNumber, settings.soundDSPMethod, settings.silent,
        environment.ossInputAvailable, environment.ossOutputAvailable,
        environment.pulseOutputAvailable);
}

AudioSourceStrategy RuntimeFactory::selectAudioSourceStrategy() const {
    return pcmSourceFactory.selectAudioSourceStrategy(settings);
}

AudioInput* RuntimeFactory::createAudioInput() const {
    AudioSourceStrategy sourceStrategy = selectAudioSourceStrategy();

    CTH_TRACE("selecting AudioInput for sound-device-number=%d\n", "runtime factory",
        settings.soundDeviceNumber);

    switch (settings.soundDeviceNumber) {
    case SDN_DSPIn:
        CTH_DEBUG("    audio input strategy: native OSS DSP input from %s source\n",
            PcmSourceFactory::strategyName(sourceStrategy));
        break;

    case SDN_Random:
    case SDN_File:
        CTH_DEBUG("    audio input strategy: native PCM input from %s source\n",
            PcmSourceFactory::strategyName(sourceStrategy));
        break;

    default:
        CTH_DEBUG("    audio input strategy: none, because requested device %d is illegal\n",
            settings.soundDeviceNumber);
        CTH_TRACE("illegal native AudioInput request %d\n", "runtime factory",
            settings.soundDeviceNumber);
        return NULL;
    }

    PcmSource* source = pcmSourceFactory.create(settings);
    if (source != NULL) {
        CTH_TRACE("selected AudioInput with source strategy=%s\n", "runtime factory",
            PcmSourceFactory::strategyName(sourceStrategy));
        return new AudioInput(source);
    }
    if (settings.soundDeviceNumber == SDN_File) {
        CTH_DEBUG("    audio input strategy: legacy file input bridge for %s source, because file playback is not native yet\n",
            PcmSourceFactory::strategyName(sourceStrategy));
    } else {
        CTH_DEBUG("    audio input strategy: no native PCM source for %s source\n",
            PcmSourceFactory::strategyName(sourceStrategy));
    }
    CTH_TRACE("no native AudioInput for source strategy=%s\n", "runtime factory",
        PcmSourceFactory::strategyName(sourceStrategy));
    return NULL;
}

AudioOutput* RuntimeFactory::createAudioOutput() const {
    CTH_TRACE("selecting AudioOutput silent=%d pulse-output=%d oss-output=%d\n", "runtime factory",
        settings.silent, environment.pulseOutputAvailable, environment.ossOutputAvailable);

    if (settings.silent) {
        CTH_DEBUG("    audio output strategy: null output, because playback is silent\n");
        CTH_TRACE("selected AudioNullOutput\n", "runtime factory");
        return new AudioNullOutput();
    }

    if (environment.pulseOutputAvailable) {
        CTH_DEBUG("    audio output strategy: trying Pulse output on server `%s'\n",
            pulse_server_display_name());
        AudioPulseOutput* pulse = new AudioPulseOutput();
        if (pulse->isOpen()) {
            CTH_DEBUG("    audio output strategy: selected Pulse output\n");
            CTH_TRACE("selected AudioPulseOutput server=`%s'\n", "runtime factory",
                pulse_server_display_name());
            return pulse;
        }
        delete pulse;
    } else {
        CTH_DEBUG("    audio output strategy: skipping Pulse output because support is unavailable\n");
    }

    if (environment.ossOutputAvailable) {
        CTH_DEBUG("    audio output strategy: trying OSS DSP output with method %d\n",
            settings.soundDSPMethod);
        AudioDSPOutput* dsp = new AudioDSPOutput(settings.soundDSPMethod);
        if (dsp->isOpen()) {
            CTH_TRACE("selected AudioDSPOutput method=%d\n", "runtime factory",
                settings.soundDSPMethod);
            return dsp;
        }
        delete dsp;
    } else {
        CTH_DEBUG("    audio output strategy: skipping OSS DSP output because it is unavailable\n");
    }

    CTH_DEBUG("    audio output strategy: null output, because no real output opened\n");
    CTH_TRACE("selected AudioNullOutput fallback\n", "runtime factory");
    return new AudioNullOutput();
}

AudioInputProcessor* RuntimeFactory::createAudioProcessor() const {
    CTH_TRACE("creating AudioInputProcessor\n", "runtime factory");
    AudioInput* input = createAudioInput();
    if (input == NULL)
        return NULL;

    if (input->hasError()) {
        CTH_TRACE("native AudioInput construction failed\n", "runtime factory");
        delete input;
        return NULL;
    }

    return new AudioInputProcessor(input);
}

SoundDevice* RuntimeFactory::createLegacySoundDevice(RuntimeSoundInputContext context) const {
    const char* contextName = (context == RSIC_FileChild) ? "file child" : "main process";
    int effectiveSilent = settings.silent;

    CTH_TRACE("selecting legacy SoundDevice context=%s sound-device-number=%d silent=%d\n", "runtime factory",
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
        CTH_TRACE("forced silent file playback because no passthrough output is available\n", "runtime factory");
    }

    switch (settings.soundDeviceNumber) {
    case SDN_DSPIn:
        CTH_DEBUG("    sound input strategy: OSS DSP input in %s, because sound-device-number=%d\n",
            contextName, settings.soundDeviceNumber);
        CTH_TRACE("selected SoundDeviceDSPIn\n", "runtime factory");
        return new SoundDeviceDSPIn();

    case SDN_Random:
        CTH_DEBUG("    sound input strategy: random input in %s, because sound-device-number=%d\n",
            contextName, settings.soundDeviceNumber);
        CTH_TRACE("selected SoundDeviceRandom\n", "runtime factory");
        return new SoundDeviceRandom();

    case SDN_File:
        if ((context == RSIC_MainProcess) && !effectiveSilent) {
            CTH_DEBUG("    sound input strategy: forked file input in %s, because audio passthrough is enabled\n",
                contextName);
            CTH_TRACE("selected SoundDeviceFork\n", "runtime factory");
            return new SoundDeviceFork();
        }

        CTH_DEBUG("    sound input strategy: direct file input in %s, because %s\n",
            contextName,
            (context == RSIC_FileChild) ? "this process owns the file reader"
                                        : "playback is silent");
        CTH_TRACE("selected SoundDeviceFile\n", "runtime factory");
        return new SoundDeviceFile();

    default:
        CTH_TRACE("illegal legacy sound-device-number=%d\n", "runtime factory",
            settings.soundDeviceNumber);
        CTH_ERROR("Illegal SoundDeviceNr %d.\n", settings.soundDeviceNumber);
        exit(0);
    }
}
