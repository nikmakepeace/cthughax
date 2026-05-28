// OSS mixer bridge used by audio input setup and option parsing.

#ifndef __MIXER_H
#define __MIXER_H

int mixer_initial_volume(char* name, int volume);
int init_mixer();

extern char dev_mixer[];

#endif
