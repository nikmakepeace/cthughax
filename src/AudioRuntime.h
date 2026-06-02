// Audio lifecycle owner.

#ifndef __AUDIO_RUNTIME_H
#define __AUDIO_RUNTIME_H

#include "RuntimeFactory.h"

int audioRuntimeInit(int initializeInputControls, int visualMaxDimension);
void audioRuntimeTick();
void audioRuntimeShutdown();
int audioRuntimeIsInitialized();
AudioInputProcessor* audioRuntimeProcessor();
AudioFrame* audioRuntimeCurrentFrame();
int audioRuntimeIsComplete();

#endif
