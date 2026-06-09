/** @file
 * Runtime audio control adapter.
 */

#include "RuntimeAudioControls.h"

#include "AudioProcessing.h"
#include "Mixer.h"

DefaultRuntimeAudioControls::DefaultRuntimeAudioControls(
    AudioProcessingSelector& audioProcessingSelector_,
    MixerControls* mixerControls_)
    : audioProcessingSelector(audioProcessingSelector_)
    , mixerControls(mixerControls_) { }

void DefaultRuntimeAudioControls::changeSoundProcessingBy(int by) {
    audioProcessingSelector.changeBy(by);
}

void DefaultRuntimeAudioControls::changeSoundProcessingTo(const char* to) {
    audioProcessingSelector.changeTo(to);
}

int DefaultRuntimeAudioControls::changeAudioOptionBy(
    Option& option, int by, RuntimeChangeSet& changes) {
    if (&option != &audioProcessingSelector.option())
        return mixerControls != 0
            && mixerControls->changeOptionBy(option, by)
            ? (changes.uiChanged = 1)
            : 0;

    audioProcessingSelector.changeBy(by);
    changes.audioProcessingChanged = 1;
    return 1;
}

int DefaultRuntimeAudioControls::changeAudioOptionTo(
    Option& option, const char* to, RuntimeChangeSet& changes) {
    if (&option != &audioProcessingSelector.option())
        return mixerControls != 0
            && mixerControls->changeOptionTo(option, to)
            ? (changes.uiChanged = 1)
            : 0;

    audioProcessingSelector.changeTo(to);
    changes.audioProcessingChanged = 1;
    return 1;
}
