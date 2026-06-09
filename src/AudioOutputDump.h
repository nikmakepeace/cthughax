/** @file
 * Optional WAV dump writer for submitted output PCM.
 */

#ifndef CTHUGHA_AUDIO_OUTPUT_DUMP_H
#define CTHUGHA_AUDIO_OUTPUT_DUMP_H

#include "AudioTypes.h"

#include <stdio.h>
#include <string>

class LogSink;

/**
 * Writes submitted output PCM to a WAV file when configured.
 *
 * The dump object is owned by the ingest/session layer and shared with the
 * selected output backend. It has no global path state.
 */
class AudioOutputDump {
    std::string pathValue;
    LogSink& log;
    FILE* out;
    int initialized;
    int enabled;
    int sampleRate;
    int channels;
    int bitsPerSample;
    long long bytesWritten;

    int bitsPerSampleForFormat(const PcmFormat& format) const;
    void initialize(const PcmFormat& format);
    void refreshHeader();

public:
    /**
     * Creates an output dump for a requested path.
     *
     * @param path Output WAV path. Empty disables dumping.
     * @param log_ Sink for dump diagnostics.
     */
    AudioOutputDump(const std::string& path, LogSink& log_);

    /** Closes the dump file. */
    virtual ~AudioOutputDump();

    /**
     * Reconfigures the dump path and closes any previous file.
     *
     * @param path Output WAV path. Empty disables dumping.
     */
    void configure(const std::string& path);

    /** Closes the dump file if one is open. */
    void close();

    /** @return Configured dump path, empty when disabled. */
    const std::string& path() const;

    /**
     * Appends submitted PCM to the configured WAV dump.
     *
     * @param format PCM format represented by data.
     * @param data PCM bytes in the current output sound format.
     * @param bytes Number of bytes available at data.
     */
    virtual void append(const PcmFormat& format, const char* data, int bytes);
};

#endif
