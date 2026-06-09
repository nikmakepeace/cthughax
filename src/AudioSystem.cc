/** @file
 * Stateless audio option text helpers.
 */

#include "AudioOptions.h"
#include "Interface.h"
#include "InterfaceRuntime.h"

static const char* audioSampleFormatNames[] = {
    "8bit unsigned",
    "8bit signed",
    "16bit unsigned (le)",
    "16bit signed (le)",
    "16bit unsigned (be)",
    "16bit signed (be)",
};
static const int audioSampleFormatCount
    = sizeof(audioSampleFormatNames) / sizeof(const char*);

const char* audioChannelsText(int channels) {
    switch (channels) {
    case 1:
        return "mono";
    case 2:
        return "stereo";
    default:
        return "unknown";
    }
}

const char* audioSampleFormatText(int sampleFormat) {
    if ((sampleFormat >= 0) && (sampleFormat < audioSampleFormatCount))
        return audioSampleFormatNames[sampleFormat];
    return "unknown";
}

const char* audioOnOffText(int enabled) {
    return enabled ? (char*)" on" : (char*)"off";
}

void registerAudioInterfaces(InterfaceRuntime& runtime) {
    runtime.registerOwnedInterface(
        new Interface("playList", "Sound Play List", "Not yet implemented"));
}
