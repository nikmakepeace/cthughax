/** @file
 * Startup/session audio settings snapshot used to construct ingest devices.
 */

#ifndef __AUDIO_SETTINGS_H
#define __AUDIO_SETTINGS_H

#include "AudioTypes.h"

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

struct AudioConfig;
class LogSink;

/**
 * Local copy of startup audio settings used during device/source construction.
 *
 * AudioIngest builds this snapshot from AudioConfig, then passes it into the
 * source/output factories. Device negotiation may update source/output-local
 * PcmFormat values, but it no longer mutates the legacy global audio config.
 */
class AudioSettings {
public:
    /** Selected audio input mode. */
    int audioInputMode;

    /** Nonzero when finite input sources should loop at EOF. */
    int inputLoopEnabled;

    /** Requested startup PCM format before source/device negotiation. */
    PcmFormat pcmFormat;

    /** Requested OSS/DSP access method. */
    int soundDSPMethod;

    /** Requested OSS/DSP fragment count. */
    int dspFragments;

    /** Requested OSS/DSP fragment-size exponent. */
    int dspFragmentSize;

    /** Nonzero when OSS/DSP input should reset after each read. */
    int dspSyncEnabled;

    /** Nonzero when audible passthrough should be disabled. */
    int silent;

    /** Startup input file path for file-backed sources. */
    char fileName[PATH_MAX];

    /** Startup OSS/DSP device path. */
    char dspDevicePath[PATH_MAX];

    /** Creates an empty/default audio settings snapshot. */
    AudioSettings();

    /** Copies audio startup values from the supplied application config. */
    void refreshFromConfig(const AudioConfig& config);

    /**
     * Creates a populated audio settings snapshot.
     *
     * @param config Application startup audio config.
     * @param log Sink for settings diagnostics. The referenced object must
     *        outlive this call.
     * @return Audio settings copied from config.
     */
    static AudioSettings fromConfig(const AudioConfig& config, LogSink& log);
};

#endif
