/** @file
 * Shared audio sample, input-mode, and PCM format types.
 */

#ifndef __AUDIO_TYPES_H
#define __AUDIO_TYPES_H

#include <stdint.h>

enum AudioSampleFormat {
    SF_u8 = 0,
    SF_s8,
    SF_u16_le,
    SF_s16_le,
    SF_u16_be,
    SF_s16_be
};

// One stereo sample in Cthugha's internal signed 8-bit format.
typedef int8_t AudioSample;
typedef uint8_t AudioByte;
typedef AudioSample char2[2];

static inline int audioSigned8FromByte(AudioByte value) {
    return (value < 128) ? int(value) : int(value) - 256;
}

static inline AudioSample audioSampleFromSigned8Byte(AudioByte value) {
    return (AudioSample)audioSigned8FromByte(value);
}

static inline AudioSample audioSampleFromUnsigned8Byte(AudioByte value) {
    return (AudioSample)(int(value) - 128);
}

static inline int audioSampleMagnitude(AudioSample sample) {
    return (sample < 0) ? -int(sample) : int(sample);
}

enum AudioInputMode {
    AIM_DSPIn,
    AIM_Random,
    AIM_File,
    AIM_None,
    AIM_Max
};

enum AudioOutputDriverId {
    AudioOutputDriverAuto = 0,
    AudioOutputDriverNull,
    AudioOutputDriverPulse,
    AudioOutputDriverOss,
    AudioOutputDriverMiniAudio,
    AudioOutputDriverMax
};

/**
 * Converts a sample count to bytes.
 *
 * @param samples Number of PCM frames/samples, not bytes.
 * @param bytesPerSample Bytes per interleaved PCM sample frame.
 * @return Byte count required to hold the requested samples.
 */
static inline int pcmBytesForSamples(int samples, int bytesPerSample) {
    return samples * bytesPerSample;
}

/** Interleaved PCM format used by audio sources, buffers, and outputs. */
struct PcmFormat {
    int sampleRate;
    int channels;
    int sampleFormat;

    /** Creates an empty unsigned-8-bit PCM format. */
    PcmFormat()
        : sampleRate(0)
        , channels(0)
        , sampleFormat(SF_u8) { }

    /**
     * @return Bytes in one interleaved sample frame.
     */
    int bytesPerSample() const {
        return (sampleFormat < 2) ? channels : 2 * channels;
    }

    /**
     * @param samples Sample frames to size.
     * @return Bytes needed for samples in this format.
     */
    int bytesForSamples(int samples) const {
        return pcmBytesForSamples(samples, bytesPerSample());
    }
};

#endif
