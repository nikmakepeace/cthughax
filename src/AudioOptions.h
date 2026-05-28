// Runtime audio option declarations.

#ifndef __AUDIO_OPTIONS_H
#define __AUDIO_OPTIONS_H

#include "AudioTypes.h"
#include "Option.h"

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

extern Option& audioInputMode;

extern Option& soundFormat;
extern Option& soundChannels;
extern Option& soundSampleRate;

extern Option& soundDSPMethod;
extern Option& soundDSPFragments;
extern Option& soundDSPFragmentSize;
extern Option& soundDSPSync;

extern Option& soundSilent;

extern int audioInputLoop;
extern char dev_dsp[];
extern char pulse_server[];
extern int pulse_latency_msec;
extern char audio_output_dump[];
extern char audio_input_file[];

const char* pulse_server_name();
const char* pulse_server_display_name();

#endif
