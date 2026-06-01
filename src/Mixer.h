// OSS mixer option declarations.

#ifndef __MIXER_H
#define __MIXER_H

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

extern char dev_mixer[];

int mixer_initial_volume(const char* name, int volume);
int init_mixer();

#endif
