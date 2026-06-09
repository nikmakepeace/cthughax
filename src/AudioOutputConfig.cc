/** @file
 * Startup/session audio output configuration implementation.
 */

#include "AudioOutputConfig.h"
#include "Configuration.h"
#include "configuration_defaults.h"

AudioOutputConfig::AudioOutputConfig()
    : outputDriver(AUDIO_CONFIG_DEFAULT_OUTPUT_DRIVER)
    , pulseServer(AUDIO_CONFIG_DEFAULT_PULSE_SERVER_TEXT)
    , pulseLatencyMs(AUDIO_CONFIG_DEFAULT_PULSE_LATENCY_MS)
    , outputDumpPath(AUDIO_CONFIG_DEFAULT_OUTPUT_DUMP_PATH)
    , miniAudioPlaybackDeviceName(
          AUDIO_CONFIG_DEFAULT_MINIAUDIO_PLAYBACK_DEVICE_NAME)
    , nullOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_NULL_TARGET_LATENCY_MS)
    , pulseOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_PULSE_TARGET_LATENCY_MS)
    , miniAudioOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_MINIAUDIO_TARGET_LATENCY_MS)
    , dspOutputTargetLatencyMs(AUDIO_CONFIG_DEFAULT_DSP_TARGET_LATENCY_MS) { }

void AudioOutputConfig::refreshFromConfig(const AudioConfig& config) {
    outputDriver = config.outputDriver;
    pulseServer = config.pulseServer;
    pulseLatencyMs = config.pulseLatencyMs;
    outputDumpPath = config.outputDumpPath;
    miniAudioPlaybackDeviceName = config.miniAudioPlaybackDeviceName;
    nullOutputTargetLatencyMs = config.nullOutputTargetLatencyMs;
    pulseOutputTargetLatencyMs = config.pulseOutputTargetLatencyMs;
    miniAudioOutputTargetLatencyMs = config.miniAudioOutputTargetLatencyMs;
    dspOutputTargetLatencyMs = config.dspOutputTargetLatencyMs;
}

AudioOutputConfig AudioOutputConfig::fromConfig(const AudioConfig& config) {
    AudioOutputConfig outputConfig;
    outputConfig.refreshFromConfig(config);
    return outputConfig;
}

const char* AudioOutputConfig::pulseServerName() const {
    return pulseServer.empty() ? NULL : pulseServer.c_str();
}

const char* AudioOutputConfig::pulseServerDisplayName() const {
    return pulseServerName() ? pulseServerName() : "default";
}
