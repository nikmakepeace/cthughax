/** @file
 * Owned audio-processing selection state and option adapter.
 */

#include "AudioProcessing.h"

#include "Configuration.h"
#include "ProcessServices.h"

#include <ctype.h>
#include <stdlib.h>

namespace {

struct AudioProcessingModeEntry {
    const char* name;
    AudioProcessingMode mode;
};

static const AudioProcessingModeEntry kAudioProcessingModes[] = {
    { "none", AudioProcessingModeNone },
    { "Filter1", AudioProcessingModeFilter1 },
    { "Filter2", AudioProcessingModeFilter2 },
    { "FFT", AudioProcessingModeFft },
};

static int wrapIndex(int value, int count) {
    if (count <= 0)
        return 0;

    int wrapped = value % count;
    if (wrapped < 0)
        wrapped += count;
    return wrapped;
}

static int sameModeName(const char* expected, const char* candidate) {
    if (expected == 0 || candidate == 0)
        return 0;

    while ((*candidate != '\0') && (*candidate != ' ') && (*expected != '\0')) {
        if (toupper(*candidate) != toupper(*expected))
            return 0;
        candidate++;
        expected++;
    }

    return (*expected == '\0') && ((*candidate == '\0') || (*candidate == ' '));
}

} // namespace

AudioProcessingState::AudioProcessingState(RandomSource& randomSource_)
    : randomSource(randomSource_)
    , selectedMode(0)
    , initialEntry() { }

int AudioProcessingState::entryCount() const {
    return int(sizeof(kAudioProcessingModes) / sizeof(kAudioProcessingModes[0]));
}

int AudioProcessingState::optNr(const char* name) const {
    int n = entryCount();

    if (n == 0)
        return 0;

    if ((name == 0) || (name[0] == '\0'))
        return randomSource.uniformInt(n);

    for (int i = 0; i < n; i++) {
        if (sameModeName(kAudioProcessingModes[i].name, name))
            return i;
    }

    char* pos;
    int parsed = strtol(name, &pos, 0);
    if (pos == name)
        return randomSource.uniformInt(n);

    return wrapIndex(parsed, n);
}

void AudioProcessingState::setInitialEntry(const char* entry) {
    initialEntry = (entry != 0) ? entry : "";
}

void AudioProcessingState::changeToInitial() {
    changeTo(initialEntry.c_str());
}

void AudioProcessingState::changeBy(int by) {
    selectedMode = wrapIndex(selectedMode + by, entryCount());
}

void AudioProcessingState::changeTo(const char* to) {
    selectedMode = optNr(to);
    if ((selectedMode < 0) || (selectedMode >= entryCount()))
        selectedMode = 0;
}

const char* AudioProcessingState::text() const {
    if ((selectedMode < 0) || (selectedMode >= entryCount()))
        return "unknown";

    return kAudioProcessingModes[selectedMode].name;
}

AudioProcessingMode AudioProcessingState::mode() const {
    if ((selectedMode < 0) || (selectedMode >= entryCount()))
        return AudioProcessingModeNone;

    return kAudioProcessingModes[selectedMode].mode;
}

AudioProcessingOption::AudioProcessingOption(AudioProcessingSelector& selector_)
    : Option("sound-processing")
    , selector(selector_) { }

void AudioProcessingOption::change(int by) {
    selector.changeBy(by);
}

void AudioProcessingOption::change(const char* to) {
    selector.changeTo(to);
}

const char* AudioProcessingOption::text() const {
    return selector.text();
}

AudioProcessingSelector::AudioProcessingSelector(AudioProcessingState& state_,
    AudioProcessor& processor_, LogSink& log_)
    : state(state_)
    , processor(processor_)
    , log(log_)
    , optionValue(*this) { }

void AudioProcessingSelector::configureStartup(const SceneConfig& config) {
    state.setInitialEntry(config.audioProcessing.c_str());
    state.changeToInitial();
}

void AudioProcessingSelector::changeBy(int by) {
    state.changeBy(by);
}

void AudioProcessingSelector::changeTo(const char* to) {
    state.changeTo(to);
}

const char* AudioProcessingSelector::text() const {
    return state.text();
}

AudioProcessingMode AudioProcessingSelector::mode() const {
    return state.mode();
}

AudioProcessingOption& AudioProcessingSelector::option() {
    return optionValue;
}

const AudioProcessingOption& AudioProcessingSelector::option() const {
    return optionValue;
}
