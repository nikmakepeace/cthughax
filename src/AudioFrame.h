// Current audio-frame facade.
//
// This keeps visual/audio consumers on the frame facade instead of runtime
// implementation details.

#ifndef __AUDIO_FRAME_H
#define __AUDIO_FRAME_H

#include "AudioTypes.h"

class AudioFrame {
public:
    long long centerSample;
    int samples;
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
