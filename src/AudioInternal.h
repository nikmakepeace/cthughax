// Private helpers shared by the audio implementation units.

#ifndef CTHUGHA_AUDIO_INTERNAL_H
#define CTHUGHA_AUDIO_INTERNAL_H

struct PcmFormat;

/**
 * Converts the largest logical visual dimension into the audio sample window.
 *
 * @param visualMaxDimension Maximum logical visual-buffer dimension, in pixels,
 *        before display zoom.
 * @return Power-of-two sample window used by OSS/DSP and input processors.
 */
int audioSampleWindowForVisualMaxDimension(int visualMaxDimension);

#endif
