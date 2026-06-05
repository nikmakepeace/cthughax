/** @file
 * Unit coverage for the default runtime audio control adapter.
 */

#include "RuntimeAudioControls.h"
#include "AudioProcessor.h"

#include <assert.h>
#include <string>

Option::~Option() { }

static EffectChoiceList audioProcessingEntries;
static int audioProcessingByCalls = 0;
static int audioProcessingLastBy = 0;
static int audioProcessingToCalls = 0;
static std::string audioProcessingLastTo;

AudioProcessingOption audioProcessing(
    "sound-processing", audioProcessingEntries);

AudioProcessingOption::AudioProcessingOption(
    const char* name, EffectChoiceList& entries_)
    : Option(name)
    , entries(entries_)
    , initialEntry() { }

int AudioProcessingOption::entryCount() const {
    return 1;
}

int AudioProcessingOption::optNr(const char*) const {
    return 0;
}

void AudioProcessingOption::setInitialEntry(const char* entry) {
    initialEntry = (entry != 0) ? entry : "";
}

void AudioProcessingOption::changeToInitial() {
    change(initialEntry.c_str());
}

void AudioProcessingOption::change(int by) {
    audioProcessingByCalls++;
    audioProcessingLastBy = by;
    value += by;
}

void AudioProcessingOption::change(const char* to) {
    audioProcessingToCalls++;
    audioProcessingLastTo = (to != 0) ? to : "";
    value = 0;
}

const char* AudioProcessingOption::text() const {
    return "test-processing";
}

class RecordingOption : public Option {
public:
    int byCalls;
    int toCalls;

    RecordingOption()
        : Option("recording")
        , byCalls(0)
        , toCalls(0) { }

    virtual void change(int) {
        byCalls++;
    }

    virtual void change(const char*) {
        toCalls++;
    }

    virtual const char* text() const {
        return "recording";
    }
};

static void resetAudioProcessingCalls() {
    audioProcessingByCalls = 0;
    audioProcessingLastBy = 0;
    audioProcessingToCalls = 0;
    audioProcessingLastTo = "";
}

static void testDirectCommandsChangeOnlyAudioProcessing() {
    DefaultRuntimeAudioControls controls;
    resetAudioProcessingCalls();

    controls.changeSoundProcessingBy(3);
    assert(audioProcessingByCalls == 1);
    assert(audioProcessingLastBy == 3);

    controls.changeSoundProcessingTo("fft");
    assert(audioProcessingToCalls == 1);
    assert(audioProcessingLastTo == "fft");
}

static void testGenericOptionRoutingClaimsOnlyAudioProcessing() {
    DefaultRuntimeAudioControls controls;
    resetAudioProcessingCalls();

    RuntimeChangeSet byChanges;
    int handled = controls.changeAudioOptionBy(audioProcessing, 2, byChanges);
    assert(handled == 1);
    assert(byChanges.audioProcessingChanged == 1);
    assert(audioProcessingByCalls == 1);
    assert(audioProcessingLastBy == 2);

    RuntimeChangeSet toChanges;
    handled = controls.changeAudioOptionTo(audioProcessing, "filter1", toChanges);
    assert(handled == 1);
    assert(toChanges.audioProcessingChanged == 1);
    assert(audioProcessingToCalls == 1);
    assert(audioProcessingLastTo == "filter1");

    RecordingOption unrelated;
    RuntimeChangeSet unrelatedChanges;
    handled = controls.changeAudioOptionBy(unrelated, 9, unrelatedChanges);
    assert(handled == 0);
    assert(!unrelatedChanges.any());
    assert(unrelated.byCalls == 0);

    handled = controls.changeAudioOptionTo(unrelated, "anything", unrelatedChanges);
    assert(handled == 0);
    assert(!unrelatedChanges.any());
    assert(unrelated.toCalls == 0);
}

int main() {
    testDirectCommandsChangeOnlyAudioProcessing();
    testGenericOptionRoutingClaimsOnlyAudioProcessing();
    return 0;
}
