#include "cthugha.h"
#include "PcmSourceFactory.h"
#include "RuntimeFactory.h"

#include <ctype.h>
#include <string.h>

#ifndef WITH_MINIMP3
#define WITH_MINIMP3 1
#endif

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

const char* PcmSourceFactory::strategyName(AudioSourceStrategy strategy) {
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

AudioSourceStrategy PcmSourceFactory::selectAudioSourceStrategy(const Settings& settings) const {
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

    CTH_TRACE("selected strategy=%s sound-device-number=%d file=`%s'\n", "pcm source factory",
        strategyName(strategy), settings.soundDeviceNumber, settings.fileName);
    return strategy;
}

PcmSource* PcmSourceFactory::create(const Settings& settings) const {
    AudioSourceStrategy strategy = selectAudioSourceStrategy(settings);

    switch (strategy) {
    case ASS_Random:
        CTH_TRACE("creating RandomNoisePcmDriver\n", "pcm source factory");
        return new PcmSource(new RandomNoisePcmDriver());

    case ASS_WavFile:
        CTH_TRACE("creating WavPcmDriver file=`%s'\n", "pcm source factory",
            settings.fileName);
        return new PcmSource(new WavPcmDriver(settings.fileName));

    case ASS_Mp3File:
#if WITH_MINIMP3 == 1
        CTH_TRACE("creating Minimp3PcmDriver file=`%s'\n", "pcm source factory",
            settings.fileName);
        return new PcmSource(new Minimp3PcmDriver(settings.fileName));
#else
        CTH_TRACE("no MP3 PCM driver is compiled in file=`%s'\n", "pcm source factory",
            settings.fileName);
        return NULL;
#endif

    default:
        CTH_TRACE("no PCM source for strategy=%s\n", "pcm source factory",
            strategyName(strategy));
        return NULL;
    }
}
