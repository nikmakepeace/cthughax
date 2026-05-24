// Current audio-frame facade.
//
// This keeps visual/audio consumers from depending directly on SoundDevice
// while startup is migrated to the new Audio* composition model.

#ifndef __AUDIO_FRAME_H
#define __AUDIO_FRAME_H

#include "SoundDevice.h"

void audioFrameTick();
void audioFrameChange();

char2* audioFrameData();
char2* audioFrameProcessedData();
int audioFrameBroadcastBytes();

#endif
