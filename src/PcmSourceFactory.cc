#include "config.h"
#include "PcmSourceFactory.h"
#include "ProcessServices.h"
#include "RuntimeFactory.h"

#include <ctype.h>
#include <string.h>

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

PcmSourceFactory::PcmSourceFactory(LogSink& log_)
    : log(log_) {
}

const char* PcmSourceFactory::strategyName(AudioSourceStrategy strategy) {
    switch (strategy) {
    case ASS_LineIn:
        return "line-in";
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

AudioSourceStrategy PcmSourceFactory::selectAudioSourceStrategy(const AudioSettings& settings) const {
    AudioSourceStrategy strategy;

    switch (settings.audioInputMode) {
    case AIM_DSPIn:
        strategy = ASS_LineIn;
        break;
    case AIM_Random:
        strategy = ASS_Random;
        break;
    case AIM_File:
        if (filenameEndsWith(settings.fileName, ".wav"))
            strategy = ASS_WavFile;
        else if (filenameEndsWith(settings.fileName, ".mp3"))
            strategy = ASS_Mp3File;
        else if (settings.fileName[0] != '\0')
            strategy = ASS_RawFile;
        else
            strategy = ASS_Unknown;
        break;
    case AIM_None:
        strategy = ASS_Unknown;
        break;
    default:
        strategy = ASS_Unknown;
        break;
    }

    log.debug("    pcm source strategy: selected strategy=%s audio-input-mode=%d file=`%s'\n",
        strategyName(strategy), settings.audioInputMode, settings.fileName);
    return strategy;
}

PcmSource* PcmSourceFactory::create(const AudioSettings& settings,
    int visualMaxDimension, RandomSource& randomSource) const {
    AudioSourceStrategy strategy = selectAudioSourceStrategy(settings);

    switch (strategy) {
    case ASS_LineIn:
        log.debug("    pcm source strategy: creating DspPcmSource\n");
        return new DspPcmSource(settings, visualMaxDimension, log);

    case ASS_Random:
        log.debug("    pcm source strategy: creating RandomNoisePcmSource\n");
        return new RandomNoisePcmSource(settings.pcmFormat, randomSource, log);

    case ASS_WavFile:
        log.debug("    pcm source strategy: creating WavPcmSource file=`%s'\n",
            settings.fileName);
        return new WavPcmSource(settings.fileName, log);

    case ASS_Mp3File:
#if WITH_MINIMP3 == 1
        log.debug("    pcm source strategy: creating Minimp3PcmSource file=`%s'\n",
            settings.fileName);
        return new Minimp3PcmSource(settings.fileName, log);
#else
        log.debug("    pcm source strategy: no MP3 PCM driver is compiled in file=`%s'\n",
            settings.fileName);
        return NULL;
#endif

    case ASS_RawFile:
        log.debug("    pcm source strategy: creating RawPcmSource file=`%s'\n",
            settings.fileName);
        return new RawPcmSource(settings.fileName, settings.pcmFormat, log);

    default:
        log.debug("    pcm source strategy: no PCM source for strategy=%s\n",
            strategyName(strategy));
        return NULL;
    }
}

PcmSource* PcmSourceFactory::createMiniAudioCapture(
    const AudioSettings& settings) const {
#if WITH_MINIAUDIO == 1
    log.debug("    pcm source strategy: creating MiniAudioCapturePcmSource\n");
    return new MiniAudioCapturePcmSource(settings, log);
#else
    (void)settings;
    log.debug("    pcm source strategy: no miniaudio capture driver is compiled in\n");
    return NULL;
#endif
}
