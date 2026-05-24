#include "cthugha.h"
#include "AudioFrame.h"
#include "AudioRuntime.h"

static char2 silentAudioFrameData[1024];
static char2 silentAudioFrameProcessedData[1024];

void audioFrameTick() {
    audioRuntimeTick();
}

void audioFrameChange() {
    if (audioRuntimeProcessor()) {
        audioRuntimeProcessor()->change();
        return;
    }

    if (soundDevice)
        soundDevice->change();
}

char2* audioFrameData() {
    if (audioRuntimeProcessor())
        return audioRuntimeProcessor()->data;

    return soundDevice ? soundDevice->data : silentAudioFrameData;
}

char2* audioFrameProcessedData() {
    if (audioRuntimeProcessor())
        return audioRuntimeProcessor()->dataProc;

    return soundDevice ? soundDevice->dataProc : silentAudioFrameProcessedData;
}

int audioFrameBroadcastBytes() {
    if (audioRuntimeProcessor())
        return audioRuntimeProcessor()->frameRawSize();

    return soundDevice ? soundDevice->frameRawSize() : 0;
}
