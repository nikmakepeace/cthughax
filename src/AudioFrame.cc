#include "cthugha.h"
#include "AudioFrame.h"

void audioFrameTick() {
    if (soundDevice)
        (*soundDevice)();
}

void audioFrameChange() {
    if (soundDevice)
        soundDevice->change();
}

char2* audioFrameData() {
    return soundDevice ? soundDevice->data : NULL;
}

char2* audioFrameProcessedData() {
    return soundDevice ? soundDevice->dataProc : NULL;
}

int audioFrameBroadcastBytes() {
    return soundDevice ? soundDevice->frameRawSize() : 0;
}
