#include "cthugha.h"
#include "RuntimeFactory.h"

#include <ctype.h>
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

    CTH_TRACE("runtime settings: sound-device-number=%d sound-dsp-method=%d silent=%d file=`%s'\n",
        settings.soundDeviceNumber, settings.soundDSPMethod, settings.silent, settings.fileName);

    return settings;
}

static const char* audioSourceStrategyName(AudioSourceStrategy strategy) {
    switch (strategy) {
    case ASS_LineIn:
        return "line-in";
    case ASS_Network:
        return "network";
    case ASS_Random:
        return "random";
    case ASS_WavFile:
        return "wav-file";
    case ASS_Mp3File:
        return "mp3-file";
    case ASS_RawFile:
        return "raw-file";
    default:
        return "unknown";
    }
}

static int filenameEndsWith(const char* name, const char* suffix) {
    int nameLen = strlen(name);
    int suffixLen = strlen(suffix);

    if (suffixLen > nameLen)
        return 0;

    name += nameLen - suffixLen;
    for (int i = 0; i < suffixLen; i++) {
        if (tolower(name[i]) != tolower(suffix[i]))
            return 0;
    }

    return 1;
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

    CTH_TRACE("runtime environment: dev-dsp=`%s' oss-input=%d oss-output=%d pulse-output=%d\n",
        SoundDeviceDSP::dev_dsp, environment.ossInputAvailable, environment.ossOutputAvailable,
        environment.pulseOutputAvailable);

    return environment;
}

RuntimeFactory::RuntimeFactory(const Settings& settings_, const Environment& environment_)
    : settings(settings_)
    , environment(environment_) {
    CTH_TRACE("runtime factory: created with sound-device-number=%d sound-dsp-method=%d silent=%d oss-input=%d oss-output=%d pulse-output=%d\n",
        settings.soundDeviceNumber, settings.soundDSPMethod, settings.silent,
        environment.ossInputAvailable, environment.ossOutputAvailable,
        environment.pulseOutputAvailable);
}

AudioSourceStrategy RuntimeFactory::selectAudioSourceStrategy() const {
    AudioSourceStrategy strategy;

    switch (settings.soundDeviceNumber) {
    case SDN_DSPIn:
        strategy = ASS_LineIn;
        break;
    case SDN_Net:
        strategy = ASS_Network;
        break;
    case SDN_Random:
        strategy = ASS_Random;
        break;
    case SDN_File:
        if (filenameEndsWith(settings.fileName, ".wav"))
            strategy = ASS_WavFile;
        else if (filenameEndsWith(settings.fileName, ".mp3"))
            strategy = ASS_Mp3File;
        else if (settings.fileName[0] != '\0')
            strategy = ASS_RawFile;
        else
            strategy = ASS_Unknown;
        break;
    default:
        strategy = ASS_Unknown;
        break;
    }

    CTH_TRACE("runtime factory: selected audio source strategy=%s sound-device-number=%d file=`%s'\n",
        audioSourceStrategyName(strategy), settings.soundDeviceNumber, settings.fileName);
    return strategy;
}

AudioInput* RuntimeFactory::createAudioInput() const {
    AudioSourceStrategy sourceStrategy = selectAudioSourceStrategy();

    CTH_TRACE("runtime factory: selecting AudioInput for sound-device-number=%d\n",
        settings.soundDeviceNumber);

    switch (settings.soundDeviceNumber) {
    case SDN_DSPIn:
        CTH_DEBUG("    audio input strategy: native OSS DSP input from %s source\n",
            audioSourceStrategyName(sourceStrategy));
        CTH_TRACE("runtime factory: selected AudioDSPInput\n");
        return new AudioDSPInput();

    case SDN_Net:
        CTH_DEBUG("    audio input strategy: native network input from %s source\n",
            audioSourceStrategyName(sourceStrategy));
        CTH_TRACE("runtime factory: selected AudioNetInput\n");
        return new AudioNetInput();

    case SDN_Random:
        CTH_DEBUG("    audio input strategy: random input from %s source\n",
            audioSourceStrategyName(sourceStrategy));
        CTH_TRACE("runtime factory: selected AudioRandomInput\n");
        return new AudioRandomInput();

    case SDN_File:
        if (sourceStrategy == ASS_WavFile) {
            CTH_DEBUG("    audio input strategy: native WAV file input\n");
            CTH_TRACE("runtime factory: selected AudioPcmInput with WavAudioSource\n");
            return new AudioPcmInput(new WavAudioSource(settings.fileName));
        }
        CTH_DEBUG("    audio input strategy: legacy file input bridge for %s source, because file playback is not native yet\n",
            audioSourceStrategyName(sourceStrategy));
        CTH_TRACE("runtime factory: no native AudioInput for file source strategy=%s yet\n",
            audioSourceStrategyName(sourceStrategy));
        return NULL;

    default:
        CTH_DEBUG("    audio input strategy: none, because requested device %d is illegal\n",
            settings.soundDeviceNumber);
        CTH_TRACE("runtime factory: illegal native AudioInput request %d\n",
            settings.soundDeviceNumber);
        return NULL;
    }
}

AudioOutput* RuntimeFactory::createAudioOutput() const {
    CTH_TRACE("runtime factory: selecting AudioOutput silent=%d pulse-output=%d oss-output=%d\n",
        settings.silent, environment.pulseOutputAvailable, environment.ossOutputAvailable);

    if (settings.silent) {
        CTH_DEBUG("    audio output strategy: null output, because playback is silent\n");
        CTH_TRACE("runtime factory: selected AudioNullOutput\n");
        return new AudioNullOutput();
    }

    if (environment.pulseOutputAvailable) {
        CTH_DEBUG("    audio output strategy: trying Pulse output\n");
        AudioPulseOutput* pulse = new AudioPulseOutput();
        if (pulse->isOpen()) {
            CTH_TRACE("runtime factory: selected AudioPulseOutput\n");
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
            CTH_TRACE("runtime factory: selected AudioDSPOutput method=%d\n",
                settings.soundDSPMethod);
            return dsp;
        }
        delete dsp;
    } else {
        CTH_DEBUG("    audio output strategy: skipping OSS DSP output because it is unavailable\n");
    }

    CTH_DEBUG("    audio output strategy: null output, because no real output opened\n");
    CTH_TRACE("runtime factory: selected AudioNullOutput fallback\n");
    return new AudioNullOutput();
}

AudioProcessor* RuntimeFactory::createAudioProcessor() const {
    CTH_TRACE("runtime factory: creating AudioProcessor\n");
    AudioInput* input = createAudioInput();
    if (input == NULL)
        return NULL;

    if (input->hasError()) {
        CTH_TRACE("runtime factory: native AudioInput construction failed\n");
        delete input;
        return NULL;
    }

    return new AudioProcessor(input);
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
