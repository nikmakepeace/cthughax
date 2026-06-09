/** @file
 * Startup/session audio output configuration.
 */

#ifndef CTHUGHA_AUDIO_OUTPUT_CONFIG_H
#define CTHUGHA_AUDIO_OUTPUT_CONFIG_H

#include <string>

struct AudioConfig;

/** Output-related startup options copied from AudioConfig. */
struct AudioOutputConfig {
    std::string pulseServer;
    int pulseLatencyMs;
    std::string outputDumpPath;
    int nullOutputTargetLatencyMs;
    int pulseOutputTargetLatencyMs;
    int dspOutputTargetLatencyMs;

    /** Creates output config with default startup values. */
    AudioOutputConfig();

    /**
     * Copies output-related values from application audio config.
     *
     * @param config Application startup audio config.
     */
    void refreshFromConfig(const AudioConfig& config);

    /**
     * Creates a populated output config snapshot.
     *
     * @param config Application startup audio config.
     * @return Output config copied from config.
     */
    static AudioOutputConfig fromConfig(const AudioConfig& config);

    /** @return Pulse server name pointer, or NULL for the default server. */
    const char* pulseServerName() const;

    /** @return Human-readable Pulse server name for diagnostics. */
    const char* pulseServerDisplayName() const;
};

#endif
