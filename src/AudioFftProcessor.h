/** @file
 * FFT processing port for visualization audio frames.
 */

#ifndef CTHUGHA_AUDIO_FFT_PROCESSOR_H
#define CTHUGHA_AUDIO_FFT_PROCESSOR_H

#include "AudioTypes.h"

#include <stdint.h>

/**
 * Applies FFT-style processing to Cthugha's 1024-sample stereo frame format.
 */
class AudioFftProcessor {
public:
    /** Releases the FFT processor interface. */
    virtual ~AudioFftProcessor();

    /**
     * Transforms raw samples into processed visualization samples.
     *
     * Implementations preserve the current contract: left samples are treated
     * as the real part, right samples as the imaginary part, and real/imaginary
     * FFT output is written back to processedWaveData.
     *
     * @param raw Source samples in signed 8-bit stereo Cthugha format.
     * @param processedWaveData Destination buffer for 1024 processed samples.
     */
    virtual void transform(const char2* raw, char2* processedWaveData) const = 0;
};

/**
 * Fixed-point radix-2 FFT implementation for visualization audio.
 *
 * The input is scaled to Q7 before stage scaling, so the ten per-stage right
 * shifts preserve the historical implementation's effective output scale,
 * including its final p==0 doubling pass.
 */
class FixedPointAudioFftProcessor : public AudioFftProcessor {
    int16_t cosTable[512];
    int16_t sinTable[512];
    uint16_t bitReversal[1024];

public:
    /** Builds Q15 twiddle-factor and bit-reversal lookup tables. */
    FixedPointAudioFftProcessor();

    /**
     * Applies a branch-light fixed-point FFT transform.
     *
     * @param raw Source samples in signed 8-bit stereo Cthugha format.
     * @param processedWaveData Destination buffer for 1024 processed samples.
     */
    virtual void transform(const char2* raw, char2* processedWaveData) const;
};

#endif
