// Audio lifecycle owner.

#ifndef __AUDIO_RUNTIME_H
#define __AUDIO_RUNTIME_H

#include "RuntimeFactory.h"

void audioRuntimeInit(RuntimeSoundInputContext context, int initializeInputControls);
void audioRuntimeTick();
void audioRuntimeShutdown();
int audioRuntimeIsInitialized();
AudioProcessor* audioRuntimeProcessor();

#endif
