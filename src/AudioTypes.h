// Shared audio sample and input-mode types.

#ifndef __AUDIO_TYPES_H
#define __AUDIO_TYPES_H

enum AudioSampleFormat {
    SF_u8 = 0,
    SF_s8,
    SF_u16_le,
    SF_s16_le,
    SF_u16_be,
    SF_s16_be
};

// One stereo sample in Cthugha's internal signed 8-bit format.
typedef char char2[2];

enum AudioInputMode {
    AIM_DSPIn,
    AIM_Random,
    AIM_File,
    AIM_Max
};

#endif
