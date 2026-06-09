// PCM source selection.
//
// Runtime orchestration should not know which file extensions map to which
// PCM drivers.  This factory is the boundary between startup settings and
// concrete PCM-producing sources.

#ifndef __PCM_SOURCE_FACTORY_H
#define __PCM_SOURCE_FACTORY_H

#include "Audio.h"

class AudioSettings;
class LogSink;
class RandomSource;

enum AudioSourceStrategy {
    ASS_LineIn,
    ASS_Random,
    ASS_WavFile,
    ASS_Mp3File,
    ASS_RawFile,
    ASS_Unknown
};

class PcmSourceFactory {
    LogSink& log;

public:
    /**
     * Creates a PCM source factory with explicit diagnostics.
     *
     * @param log_ Sink for source-selection diagnostics.
     */
    explicit PcmSourceFactory(LogSink& log_);

    /**
     * @param strategy Audio source strategy enum value.
     * @return Stable human-readable strategy name for logs.
     */
    static const char* strategyName(AudioSourceStrategy strategy);

    /**
     * Selects the concrete PCM source family for the current audio settings.
     *
     * @param settings Snapshot of audio input mode and filename options.
     * @return Source strategy to instantiate, or ASS_Unknown if no source matches.
     */
    AudioSourceStrategy selectAudioSourceStrategy(const AudioSettings& settings) const;

    /**
     * Creates the selected PCM source.
     *
     * @param settings Snapshot of audio input mode and filename options.
     * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
     *        before display zoom. Passed to live DSP input so its sample window
     *        tracks the visual buffer scale.
     * @param randomSource Random source passed to synthetic random PCM input.
     * @return Newly allocated source, or NULL if settings do not select a valid source.
     */
    PcmSource* create(const AudioSettings& settings, int visualMaxDimension,
        RandomSource& randomSource) const;

    /**
     * Creates a miniaudio live-capture PCM source when compiled in.
     *
     * @param settings Snapshot of live-input audio options.
     * @return Newly allocated source, or NULL when miniaudio capture is not
     *         compiled in.
     */
    PcmSource* createMiniAudioCapture(const AudioSettings& settings) const;
};

#endif
