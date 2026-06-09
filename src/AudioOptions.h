/** @file
 * Stateless audio option text helpers.
 */

#ifndef __AUDIO_OPTIONS_H
#define __AUDIO_OPTIONS_H

#include "AudioTypes.h"

/**
 * Formats a channel count for display.
 *
 * @param channels PCM channel count.
 * @return Human-readable channel text.
 */
const char* audioChannelsText(int channels);

/**
 * Formats a sample format id for display.
 *
 * @param sampleFormat Sample format id.
 * @return Human-readable sample format text.
 */
const char* audioSampleFormatText(int sampleFormat);

/**
 * Formats a boolean setting as legacy on/off text.
 *
 * @param enabled Nonzero for on.
 * @return " on" or "off".
 */
const char* audioOnOffText(int enabled);

#endif
