// PCM source selection.
//
// Runtime orchestration should not know which file extensions map to which
// PCM drivers.  This factory is the boundary between startup settings and
// concrete PCM-producing sources.

#ifndef __PCM_SOURCE_FACTORY_H
#define __PCM_SOURCE_FACTORY_H

#include "Audio.h"

class Settings;

enum AudioSourceStrategy {
    ASS_LineIn,
    ASS_Network,
    ASS_Random,
    ASS_WavFile,
    ASS_Mp3File,
    ASS_RawFile,
    ASS_Unknown
};

class PcmSourceFactory {
public:
    static const char* strategyName(AudioSourceStrategy strategy);

    AudioSourceStrategy selectAudioSourceStrategy(const Settings& settings) const;
    PcmSource* create(const Settings& settings) const;
};

#endif
