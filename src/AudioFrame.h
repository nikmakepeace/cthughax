// Current audio-frame facade.
//
// This keeps visual/audio consumers from depending directly on SoundDevice
// while startup is migrated to the new Audio* composition model.

#ifndef __AUDIO_FRAME_H
#define __AUDIO_FRAME_H

#include "SoundDevice.h"

class AudioFrame {
public:
    long long centerByte;
    int samples;
    int rawBytes;
    char2 data[1024];
    char2 processed[1024];

    AudioFrame();

    void clear();
};

void audioFrameTick();
void audioFrameChange();

AudioFrame* audioFrameCurrent();
char2* audioFrameData();
char2* audioFrameProcessedData();
int audioFrameBroadcastBytes();

#endif
