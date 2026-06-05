/** @file
 * Runtime audio control adapter.
 */

#include "RuntimeAudioControls.h"

#include "AudioProcessor.h"

void DefaultRuntimeAudioControls::changeSoundProcessingBy(int by) {
    audioProcessing.change(by);
}

void DefaultRuntimeAudioControls::changeSoundProcessingTo(const char* to) {
    audioProcessing.change(to);
}

int DefaultRuntimeAudioControls::changeAudioOptionBy(
    Option& option, int by, RuntimeChangeSet& changes) {
    if (&option != &audioProcessing)
        return 0;

    option.change(by);
    changes.audioProcessingChanged = 1;
    return 1;
}

int DefaultRuntimeAudioControls::changeAudioOptionTo(
    Option& option, const char* to, RuntimeChangeSet& changes) {
    if (&option != &audioProcessing)
        return 0;

    option.change(to);
    changes.audioProcessingChanged = 1;
    return 1;
}
