/** @file
 * Startup and negotiated audio device configuration declarations.
 */

#ifndef __AUDIO_OPTIONS_H
#define __AUDIO_OPTIONS_H

#include "AudioTypes.h"

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

struct AudioConfig;

/** Startup/requested and device-negotiated audio configuration. */
struct AudioDeviceConfig {
    AudioInputMode inputMode;
    int inputLoopEnabled;
    PcmFormat pcmFormat;
    int dspMethod;
    int dspFragments;
    int dspFragmentSize;
    int dspSyncEnabled;
    int silentEnabled;
    char dspDevicePath[PATH_MAX];

    /** Creates an empty audio device configuration. */
    AudioDeviceConfig();
};

/**
 * @return Current startup/requested audio device configuration.
 */
const AudioDeviceConfig& audioDeviceConfig();

/**
 * @return Current PCM format, including any format negotiated by the backend.
 */
const PcmFormat& audioPcmFormat();

/** @return Current audio input mode. */
AudioInputMode audioInputModeValue();

/** @return Nonzero when finite input sources should loop. */
int audioInputLoopEnabled();

/**
 * Sets input-loop behavior for startup configuration or test fixtures.
 *
 * @param enabled Nonzero to loop finite audio input.
 */
void audioSetInputLoopEnabled(int enabled);

/** @return Current sample rate in Hertz. */
int audioSampleRateHz();

/** @return Current channel count. */
int audioChannels();

/** @return Current PCM sample format. */
int audioSampleFormat();

/**
 * Sets the current PCM format after startup configuration or backend negotiation.
 *
 * @param format PCM format to publish.
 */
void audioSetPcmFormat(const PcmFormat& format);

/** @param sampleRateHz Sample rate negotiated by the backend. */
void audioSetSampleRateHz(int sampleRateHz);

/** @param channels Channel count negotiated by the backend. */
void audioSetChannels(int channels);

/** @param sampleFormat Sample format negotiated by the backend. */
void audioSetSampleFormat(int sampleFormat);

/** @return Bytes in one interleaved sample frame for the current PCM format. */
int audioBytesPerSample();

/** @return Current DSP method id. */
int audioDspMethod();

/** @return Current DSP fragment count. */
int audioDspFragments();

/** @return Current DSP fragment size exponent. */
int audioDspFragmentSize();

/**
 * Sets negotiated DSP fragment settings.
 *
 * @param fragments Fragment count.
 * @param fragmentSize Fragment size exponent.
 */
void audioSetDspFragment(int fragments, int fragmentSize);

/** @param fragments Fragment count. */
void audioSetDspFragments(int fragments);

/** @param fragmentSize Fragment size exponent. */
void audioSetDspFragmentSize(int fragmentSize);

/** @return Nonzero when DSP reset-after-read is enabled. */
int audioDspSyncEnabled();

/** @return Nonzero when audio output should be silent. */
int audioSilentEnabled();

/** @return Configured OSS DSP device path. */
const char* audioDspDevicePath();

/** @return Human-readable channel text for the current channel count. */
const char* audioChannelsText();

/**
 * Formats a sample format id for display.
 *
 * @param sampleFormat Sample format id.
 * @return Human-readable sample format text.
 */
const char* audioSampleFormatText(int sampleFormat);

/** @return Human-readable sample format text for the current sample format. */
const char* audioSampleFormatText();

/**
 * Formats a boolean setting as legacy on/off text.
 *
 * @param enabled Nonzero for on.
 * @return " on" or "off".
 */
const char* audioOnOffText(int enabled);

extern char pulse_server[];
extern int pulse_latency_msec;
extern char audio_output_dump[];
extern int audio_null_target_latency_msec;
extern int audio_pulse_target_latency_msec;
extern int audio_dsp_target_latency_msec;

const char* pulse_server_name();
const char* pulse_server_display_name();
void configureAudioOptions(const AudioConfig& config);
void configureAudioOutputOptions(const AudioConfig& config);

#endif
