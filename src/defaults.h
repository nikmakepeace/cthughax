// Non-configuration defaults for option/schema mechanics.

#ifndef CTHUGHA_DEFAULTS_H
#define CTHUGHA_DEFAULTS_H

#include "cthugha.h"
#include "AudioTypes.h"

static const int DEFAULT_OPTION_VALUE = 0; // Unit: raw option value; used by Option::Option before concrete defaults are applied.
static const int DEFAULT_OPTION_INT_NO_MAX = 0; // Unit: OptionInt max sentinel; used by OptionInt when an integer option has no maximum.
static const int DEFAULT_OPTION_INT_MIN = 0; // Unit: raw option value; used by OptionInt when an integer option has no explicit minimum.
static const int OPTION_ON_OFF_MAX_EXCLUSIVE = 2; // Unit: exclusive raw option maximum; used by OptionOnOff for off/on values.

static const int DEFAULT_VERBOSE_MIN_LEVEL = -1; // Unit: log verbosity level; used as the minimum accepted logging verbosity.
static const char* DEFAULT_VERBOSE_COMMAND_LEVEL_TEXT = "4"; // Unit: option text; used when --verbose is provided without an explicit level.

static const int SOUND_CHANNELS_MIN = 1; // Unit: channel count; used by OptionChannels as the minimum accepted channel count.
static const int SOUND_CHANNELS_MAX_EXCLUSIVE = 3; // Unit: exclusive channel count; used by OptionChannels to accept mono or stereo.
static const int SOUND_MINNOISE_MAX_EXCLUSIVE = 256; // Unit: 8-bit RMS amplitude; used by minnoise as the exclusive maximum.
static const int ZOOM_MODE_MAX_EXCLUSIVE = 3; // Unit: zoom mode; used by zoom to accept fit, 1x, or 2x modes.
#define DEFAULT_FLASHLIGHT_ENABLE_INITIAL_ENTRY "non-locked:on" // Unit: Effect choice text; used by --flashlight without an explicit value.
#define DEFAULT_FLASHLIGHT_DISABLE_INITIAL_ENTRY "locked:off" // Unit: Effect choice text; used by --no-flashlight without an explicit value.

#endif
